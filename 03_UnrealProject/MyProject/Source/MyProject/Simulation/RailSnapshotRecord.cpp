#include "RailSnapshotRecord.h"
#include "RailFormationContent.h"
#include "RailScenarioContent.h"
#include <algorithm>
#include <limits>

namespace Rail {
namespace {
constexpr uint8_t Magic[8]={'I','R','S','N','A','P','0','6'};
void Put(std::vector<uint8_t>& bytes,uint64_t value,unsigned count) {
    for(unsigned i=0;i<count;++i)bytes.push_back(static_cast<uint8_t>(value>>(8*i)));
}
uint64_t Get(const std::vector<uint8_t>& bytes,size_t offset,unsigned count) {
    uint64_t value=0;for(unsigned i=0;i<count;++i)value|=uint64_t(bytes[offset+i])<<(8*i);return value;
}
uint32_t Checksum(const std::vector<uint8_t>& bytes,size_t count) {
    uint32_t crc=0xffffffffu;
    for(size_t i=0;i<count;++i){crc^=bytes[i];for(unsigned bit=0;bit<8;++bit)crc=(crc>>1)^((crc&1)?0xedb88320u:0u);}
    return ~crc;
}
// Fixed header: magic[8], schema u32, generation u64, content length u32,
// payload length u32, then exact content bytes + payload + CRC32 u32.
constexpr size_t HeaderBytes=28;
}
Result SnapshotRecord::Create(uint64_t generation,const std::vector<uint8_t>& payload,SnapshotRecord& out) {
    return Create(generation,payload,ContentManifest::ContentId,out);
}
Result SnapshotRecord::Create(uint64_t generation,const std::vector<uint8_t>& payload,const std::string& content,SnapshotRecord& out) {
    if(!ScenarioContent::Supports(content))return Result::Fail("Required fixture content is unavailable; record retained");
    if(!generation||generation==std::numeric_limits<uint64_t>::max())return Result::Fail("Invalid/exhausted snapshot generation");
    if(payload.empty()||payload.size()>MaximumBytes-HeaderBytes-content.size()-4)return Result::Fail("Empty/oversized logical payload");
    SnapshotRecord candidate;candidate.contentId_=content;candidate.generation_=generation;candidate.payload_=payload;
    auto& bytes=candidate.bytes_;bytes.insert(bytes.end(),std::begin(Magic),std::end(Magic));
    Put(bytes,Schema,4);Put(bytes,generation,8);Put(bytes,content.size(),4);Put(bytes,payload.size(),4);
    bytes.insert(bytes.end(),content.begin(),content.end());bytes.insert(bytes.end(),payload.begin(),payload.end());
    Put(bytes,Checksum(bytes,bytes.size()),4);out=std::move(candidate);return {};
}
Result SnapshotRecord::Read(const std::vector<uint8_t>& bytes,SnapshotRecord& out) {
    if(bytes.size()<HeaderBytes+4||bytes.size()>MaximumBytes)return Result::Fail("Truncated/oversized snapshot");
    if(!std::equal(std::begin(Magic),std::end(Magic),bytes.begin()))return Result::Fail("Snapshot magic mismatch");
    if(Get(bytes,bytes.size()-4,4)!=Checksum(bytes,bytes.size()-4))return Result::Fail("Snapshot checksum mismatch");
    if(Get(bytes,8,4)!=Schema)return Result::Fail("Unsupported snapshot schema; record retained");
    const auto contentLength=Get(bytes,20,4),payloadLength=Get(bytes,24,4);
    if(contentLength>256||!payloadLength||contentLength+payloadLength!=bytes.size()-HeaderBytes-4)return Result::Fail("Invalid snapshot lengths");
    const std::string content(bytes.begin()+HeaderBytes,bytes.begin()+HeaderBytes+static_cast<size_t>(contentLength));
    if(!ScenarioContent::Supports(content))return Result::Fail("Required fixture content is unavailable; record retained");
    const auto generation=Get(bytes,12,8);
    if(!generation||generation==std::numeric_limits<uint64_t>::max())return Result::Fail("Invalid/exhausted snapshot generation");
    SnapshotRecord candidate;candidate.contentId_=content;candidate.bytes_=bytes;candidate.generation_=generation;
    candidate.payload_.assign(bytes.begin()+HeaderBytes+static_cast<size_t>(contentLength),bytes.end()-4);
    out=std::move(candidate);return {};
}
Result SelectSnapshot(const std::vector<std::vector<uint8_t>>& slots,
    const ValidateSnapshotPayload& validate,RecoverySelection& out) {
    RecoverySelection candidate;
    if(!validate)return Result::Fail("Semantic recovery validator is required");
    if(slots.size()>2)return Result::Fail("Only two bounded checkpoint slots are supported");
    bool conflictingGeneration=false;
    for(size_t i=0;i<slots.size();++i){
        SnapshotRecord record;auto result=SnapshotRecord::Read(slots[i],record);
        if(result.ok)result=validate(record);
        if(!result.ok){candidate.diagnostics.push_back("Slot "+std::to_string(i)+": "+result.error);continue;}
        if(candidate.slot>=0&&record.Generation()==candidate.record.Generation()&&record.Bytes()!=candidate.record.Bytes()){
            conflictingGeneration=true;candidate.diagnostics.push_back("Conflicting records have the same generation");continue;
        }
        if(candidate.slot<0||record.Generation()>candidate.record.Generation()){
            candidate.slot=static_cast<int>(i);candidate.record=std::move(record);
        }
    }
    if(conflictingGeneration){candidate.slot=-1;out=std::move(candidate);return Result::Fail("Ambiguous checkpoint generation; recovery paused");}
    out=std::move(candidate);
    return out.slot>=0?Result{}:Result::Fail("No complete semantically valid checkpoint; recovery paused");
}
TestReport RunSnapshotRecordTests() {
    TestReport report;auto recordCheck=[&](const char* name,bool passed){report.cases.push_back({name,passed,0,0,"Record envelope test; full world recovery remains a separate integration requirement"});};
    SnapshotRecord first,second,read;
    recordCheck("S6-record-create",SnapshotRecord::Create(1,{1,2,3},first).ok&&SnapshotRecord::Create(2,{4,5,6},second).ok);
    recordCheck("S6-record-roundtrip",SnapshotRecord::Read(first.Bytes(),read).ok&&read.Bytes()==first.Bytes()&&read.Payload()==first.Payload());
    auto valid=[](const SnapshotRecord&){return Result{};};RecoverySelection selected;
    recordCheck("S6-record-newest",SelectSnapshot({first.Bytes(),second.Bytes()},valid,selected).ok&&selected.slot==1);
    auto corrupted=second.Bytes();corrupted[corrupted.size()-5]^=1;
    recordCheck("S6-record-corrupt-fallback",SelectSnapshot({first.Bytes(),corrupted},valid,selected).ok&&selected.slot==0);
    auto truncated=second.Bytes();truncated.resize(truncated.size()-3);
    recordCheck("S6-record-truncated-fallback",SelectSnapshot({first.Bytes(),truncated},valid,selected).ok&&selected.slot==0);
    auto semantic=[](const SnapshotRecord& record){return record.Generation()==2?Result::Fail("Unsafe ownership"):Result{};};
    recordCheck("S6-record-semantic-fallback",SelectSnapshot({first.Bytes(),second.Bytes()},semantic,selected).ok&&selected.slot==0);
    recordCheck("S6-record-validator-required",!SelectSnapshot({first.Bytes()}, {},selected).ok);
    const auto unchanged=read.Bytes();
    recordCheck("S6-record-failed-read-transactional",!SnapshotRecord::Read(corrupted,read).ok&&read.Bytes()==unchanged);
    auto reseal=[](std::vector<uint8_t>& bytes){bytes.resize(bytes.size()-4);Put(bytes,Checksum(bytes,bytes.size()),4);};
    auto future=second.Bytes();future[8]=2;reseal(future);
    recordCheck("S6-record-future-schema",SelectSnapshot({first.Bytes(),future},valid,selected).ok&&selected.slot==0);
    auto missing=second.Bytes();missing[HeaderBytes]='X';reseal(missing);
    recordCheck("S6-record-content-mismatch",SelectSnapshot({first.Bytes(),missing},valid,selected).ok&&selected.slot==0);
    recordCheck("S6-record-no-valid-checkpoint",!SelectSnapshot({corrupted,truncated},valid,selected).ok&&selected.slot==-1);
    SnapshotRecord duplicate;SnapshotRecord::Create(1,{9},duplicate);
    recordCheck("S6-record-ambiguous-generation",!SelectSnapshot({first.Bytes(),duplicate.Bytes()},valid,selected).ok);
    recordCheck("S6-record-zero-generation",!SnapshotRecord::Create(0,{1},read).ok);
    recordCheck("S6-record-empty-payload",!SnapshotRecord::Create(1,{},read).ok);
    recordCheck("S6-record-bounded-size",!SnapshotRecord::Create(1,std::vector<uint8_t>(SnapshotRecord::MaximumBytes),read).ok);
    return report;
}
}
