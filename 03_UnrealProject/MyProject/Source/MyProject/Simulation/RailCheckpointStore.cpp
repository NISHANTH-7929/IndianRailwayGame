#include "RailCheckpointStore.h"
namespace Rail {
namespace {
Result ValidateWorld(const SnapshotRecord& record){OperationsWorld world;return WorldSnapshotCodec::Restore(record,world);}
}
Result CommitCheckpoint(const OperationsWorld& world,const CheckpointIO& io,CheckpointCommit& committed){
    return CommitSnapshot(io,[&](uint64_t generation,SnapshotRecord& next){return WorldSnapshotCodec::Capture(world,generation,next);},[&](const SnapshotRecord& record){return record.ContentId()==world.ContentId()?ValidateWorld(record):Result::Fail("Incompatible content retained");},committed,[&](const SnapshotRecord& record){return record.ContentId()==world.ContentId();});
}
Result CommitSnapshot(const CheckpointIO& io,const CaptureSnapshot& capture,const ValidateSnapshotPayload& validate,
                      CheckpointCommit& committed,const std::function<bool(const SnapshotRecord&)>& compatible){
    if(!io.read||!io.write||!capture||!validate)return Result::Fail("Missing checkpoint filesystem adapter or codec");
    const std::vector<std::vector<uint8_t>> slots{io.read(0),io.read(1)};
    RecoverySelection selected;const auto selection=SelectSnapshot(slots,validate,selected);
    // Existing incompatible/corrupt records must not be silently overwritten if
    // no safe predecessor exists. Explicit reset/export is a separate operation.
    if(!selection.ok&&(!slots[0].empty()||!slots[1].empty()))return Result::Fail("No valid predecessor; existing records retained");
    const unsigned target=selected.slot<0?0:1-static_cast<unsigned>(selected.slot);
    if(!slots[target].empty()){
        SnapshotRecord existing;const auto compatibility=SnapshotRecord::Read(slots[target],existing);
        if(compatibility.ok&&compatible&&!compatible(existing))return Result::Fail("Incompatible application checkpoint retained");
        if(!compatibility.ok&&(compatibility.error.find("Unsupported snapshot schema")!=std::string::npos||
            compatibility.error.find("Required fixture content")!=std::string::npos))
            return Result::Fail("Incompatible checkpoint retained; archive that record explicitly before saving");
    }
    const uint64_t generation=selected.slot<0?1:selected.record.Generation()+1;
    SnapshotRecord next;auto result=capture(generation,next);if(!result.ok)return result;
    result=io.write(target,next.Bytes());if(!result.ok)return result;
    const auto bytes=io.read(target);
    if(bytes!=next.Bytes())return Result::Fail("Checkpoint readback differs; previous generation retained");
    SnapshotRecord verified;result=SnapshotRecord::Read(bytes,verified);if(result.ok)result=validate(verified);
    if(!result.ok)return result;
    committed={target,generation};return {};
}
Result RecoverCheckpoint(const CheckpointIO& io,RecoverySession& session,RecoverySelection& selected){
    session.Pause();if(!io.read)return Result::Fail("Missing checkpoint reader");
    auto result=SelectSnapshot({io.read(0),io.read(1)},ValidateWorld,selected);
    if(result.ok)result=session.Restore(selected.record);return result;
}
TestReport RunCheckpointTests(){
    TestReport report;auto recordCheck=[&](const char* name,bool passed){report.cases.push_back({name,passed,0,0,"Injected file-adapter test; Android persistence validation is separate"});};
    std::vector<uint8_t> slots[2];bool interrupt=false;unsigned writes=0;
    CheckpointIO io{[&](unsigned slot){return slots[slot];},[&](unsigned slot,const std::vector<uint8_t>& bytes){
        ++writes;slots[slot]=bytes;if(interrupt){slots[slot].resize(bytes.size()/2);return Result::Fail("Injected interrupted write");}return Result{};}};
    OperationsWorld world;world.InitializeAIFixture();CheckpointCommit first,second;
    recordCheck("S6-store-first-commit",CommitCheckpoint(world,io,first).ok&&first.generation==1);
    const auto backup=slots[first.slot];world.Update(1.0/30.0);interrupt=true;
    recordCheck("S6-store-interrupted-retains-backup",!CommitCheckpoint(world,io,second).ok&&slots[first.slot]==backup);
    RecoverySession session;RecoverySelection selected;
    recordCheck("S6-store-interrupted-fallback",RecoverCheckpoint(io,session,selected).ok&&selected.record.Generation()==1&&session.Paused());
    interrupt=false;recordCheck("S6-store-next-commit",CommitCheckpoint(world,io,second).ok&&second.generation==2&&slots[first.slot]==backup);
    slots[second.slot].back()^=1;
    recordCheck("S6-store-corrupt-newest-fallback",RecoverCheckpoint(io,session,selected).ok&&selected.record.Generation()==1);
    slots[first.slot].back()^=1;const auto writeCount=writes;
    recordCheck("S6-store-preserves-incompatible-records",!CommitCheckpoint(world,io,second).ok&&writes==writeCount);
    recordCheck("S6-store-failed-recovery-pauses",!RecoverCheckpoint(io,session,selected).ok&&session.Paused());
    // A compatible fallback must not authorize overwriting an incompatible
    // record that may be recoverable by another content/schema version.
    slots[0]=backup;slots[1]=backup;
    auto reseal=[](std::vector<uint8_t>& bytes){uint32_t crc=0xffffffffu;for(size_t i=0;i<bytes.size()-4;++i){crc^=bytes[i];for(unsigned b=0;b<8;++b)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}crc=~crc;for(unsigned i=0;i<4;++i)bytes[bytes.size()-4+i]=static_cast<uint8_t>(crc>>(8*i));};
    slots[1][8]=2;reseal(slots[1]);const auto future=slots[1];const auto priorWrites=writes;
    recordCheck("S6-store-retains-future-with-valid-fallback",RecoverCheckpoint(io,session,selected).ok&&!CommitCheckpoint(world,io,second).ok&&writes==priorWrites&&slots[1]==future&&slots[0]==backup);
    slots[1]=backup;slots[1][28]='X';reseal(slots[1]);const auto missing=slots[1];
    recordCheck("S6-store-retains-missing-content-with-valid-fallback",RecoverCheckpoint(io,session,selected).ok&&!CommitCheckpoint(world,io,second).ok&&writes==priorWrites&&slots[1]==missing&&slots[0]==backup);
    return report;
}
}
