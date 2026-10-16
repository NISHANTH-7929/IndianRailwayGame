#pragma once
#include "RailWorldSnapshot.h"
namespace Rail {
// Platform adapter writes one inactive slot, flushes/closes it and reads it back.
// No rename or timestamp assumptions; the other complete valid slot is retained.
struct CheckpointIO {
    std::function<std::vector<uint8_t>(unsigned)> read;
    std::function<Result(unsigned,const std::vector<uint8_t>&)> write;
};
struct CheckpointCommit {unsigned slot=0;uint64_t generation=0;};
using CaptureSnapshot = std::function<Result(uint64_t,SnapshotRecord&)>;
// S7 uses the same alternating-slot transaction with a complete application record.
Result CommitSnapshot(const CheckpointIO& io,const CaptureSnapshot& capture,
                      const ValidateSnapshotPayload& validate,CheckpointCommit& committed,
                      const std::function<bool(const SnapshotRecord&)>& compatible = {});
Result CommitCheckpoint(const OperationsWorld& world,const CheckpointIO& io,CheckpointCommit& committed);
Result RecoverCheckpoint(const CheckpointIO& io,RecoverySession& session,RecoverySelection& selected);
TestReport RunCheckpointTests();
}
