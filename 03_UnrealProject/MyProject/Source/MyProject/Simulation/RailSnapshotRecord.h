#pragma once
#include "RailContent.h"

namespace Rail {
// Immutable serialized value. Payload is an explicit logical-state encoding,
// never a memory image of an OperationsWorld or an Unreal object.
class SnapshotRecord {
public:
    static constexpr uint32_t Schema = 1;
    static constexpr size_t MaximumBytes = 1024 * 1024;
    const std::vector<uint8_t>& Bytes() const { return bytes_; }
    uint64_t Generation() const { return generation_; }
    const std::string& ContentId() const { return contentId_; }
    const std::vector<uint8_t>& Payload() const { return payload_; }
    static Result Create(uint64_t generation,const std::vector<uint8_t>& payload,SnapshotRecord& out);
    static Result Create(uint64_t generation,const std::vector<uint8_t>& payload,const std::string& contentId,SnapshotRecord& out);
    static Result Read(const std::vector<uint8_t>& bytes,SnapshotRecord& out);
private:
    std::vector<uint8_t> bytes_,payload_;
    uint64_t generation_=0;
    std::string contentId_=ContentManifest::ContentId;
};
// Slot timestamps are deliberately irrelevant. A callback must prove complete
// semantic recovery before a structurally valid record can win selection.
struct RecoverySelection {
    int slot=-1;
    SnapshotRecord record;
    std::vector<std::string> diagnostics;
};
using ValidateSnapshotPayload = std::function<Result(const SnapshotRecord&)>;
Result SelectSnapshot(const std::vector<std::vector<uint8_t>>& slots,
                      const ValidateSnapshotPayload& validate,RecoverySelection& out);
TestReport RunSnapshotRecordTests();
}
