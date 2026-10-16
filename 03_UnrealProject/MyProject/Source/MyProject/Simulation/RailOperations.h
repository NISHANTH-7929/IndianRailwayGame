#pragma once
#include "RailAIDriver.h"
#include "RailFormationContent.h"
namespace Rail {
// Pure multi-train owner. Unreal owns one instance; views only receive const snapshots/copies.
class OperationsWorld {
public:
    Result InitializeFixture();
    Result InitializeContent(const std::string& contentId);
    const std::string& ContentId() const { return contentId_; }
    Result InitializeAIFixture();
    Result EnableAI(TrainId train,RouteId route);
    const AIDriver* AI(TrainId train) const;
    AIObservation ObserveAI(TrainId train) const;
    struct UpdateMetrics { double aiMilliseconds=0,stepMilliseconds=0; unsigned decisions=0,steps=0; };
    const UpdateMetrics& Metrics() const { return metrics_; }
    Result Update(double renderDeltaSeconds);
    Result Submit(const OperationsRequest& request);
    Result RequestRoute(TrainId train,RouteId route);
    Result RequestPoint(TrainId train,PointId point,int position);
    Result CancelRoute(TrainId train,RouteId route,uint64_t generation);
    Result Command(TrainId train,CommandKind kind,double value);
    void SetSuspended(bool suspended);
    void CancelInput();
    bool Suspended() const { return suspended_; }
    bool Faulted() const { return paused_; }
    const std::string& Error() const { return error_; }
    const OperationsSnapshot& Current() const { return state_.snapshot; }
    const OperationsDefinitions& DefinitionsView() const { return definitions_; }
    const Simulation* Spatial(TrainId train) const;
    const LongitudinalState* Handling(TrainId train) const;
    Result ValidateInvariants() const;
    std::string Json() const;
private:
    friend class WorldSnapshotCodec;
    struct TrainRuntime { Simulation spatial; HandlingModel handling; uint64_t commandSequence=0; };
    struct State {
        std::map<TrainId,TrainRuntime> trains; OperationsSnapshot snapshot;
        std::deque<OperationsRequest> requests;
        std::map<TrainId,uint64_t> sequences; uint64_t nextGeneration=1;
        struct AIBinding { RouteId route; uint64_t generation=1,acceptedSequence=0; };
        std::map<TrainId,AIBinding> aiBindings;
        std::map<TrainId,AIDriver> drivers;
        std::deque<std::string> aiTrace;
        uint64_t traceHash=14695981039346656037ull;
    } state_;
    OperationsDefinitions definitions_;
    std::string contentId_=FormationContent::LegacyId;
    double accumulator_=0; bool paused_=true,suspended_=false,skipResumeDelta_=false;
    std::string error_;
    UpdateMetrics metrics_; // Wall-clock diagnostics only; excluded from authoritative state/JSON.
    Result Step();
    Result SubmitRequest(const OperationsRequest& request);
    Result SubmitControl(TrainId train,CommandKind kind,double value);
    Result DispatchAI(const AIDecision& decision);
    Result UpdateAI();
    Result Evaluate(const OperationsRequest& request);
    Result Grant(const OperationsRequest& request);
    void Release(RouteId route,const std::string& reason);
    Result RecomputeOccupancy();
    Result DeriveAuthorities();
    void DeriveSignals();
    void Event(const std::string& event);
    friend TestReport RunOperationsTests();
    friend TestReport RunAITests();
    friend TestReport RunDispatchTests();
};
TestReport RunOperationsTests();
}

