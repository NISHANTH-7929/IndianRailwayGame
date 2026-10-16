#pragma once
#include "RailOperations.h"
#include "RailSnapshotRecord.h"

namespace Rail {
class SnapshotArchive;
// Only this codec has privileged serialization access. It never retains live
// state references: capture copies, restore validates an isolated candidate.
class WorldSnapshotCodec {
public:
    static Result Capture(const OperationsWorld& world,uint64_t generation,SnapshotRecord& out);
    static Result Restore(const SnapshotRecord& record,OperationsWorld& out);
    static TestReport RejectionTests();
private:
    static void Transfer(SnapshotArchive& archive,OperationsWorld& world);
    static Result ValidateAndRebuild(OperationsWorld& world);
};
// Continuity pause belongs outside the railway. It preserves queued inputs,
// physical actuators and AI retry timing, unlike the legacy input-cancel pause.
class RecoverySession {
public:
    const OperationsWorld& World() const {return world_;}
    OperationsWorld& Commands() {return world_;}
    bool Paused() const {return paused_;}
    void Pause() {paused_=true;}
    void Resume() {paused_=false;skipDelta_=true;}
    Result Restore(const SnapshotRecord& record) {
        auto result=WorldSnapshotCodec::Restore(record,world_);
        paused_=true;skipDelta_=true;return result;
    }
    Result Update(double seconds) {
        if(paused_)return {};
        if(skipDelta_){skipDelta_=false;return {};}
        return world_.Update(seconds);
    }
private:
    OperationsWorld world_;
    bool paused_=true,skipDelta_=true;
};
TestReport RunWorldSnapshotTests();
}
