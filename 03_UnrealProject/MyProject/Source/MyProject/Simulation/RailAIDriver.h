#pragma once
#include "RailOperationsTypes.h"

namespace Rail {
enum class AIPhase { Holding, RequestingRoute, WaitingForRoute, RouteGranted, Driving,
    RestrictiveApproach, ApproachingAuthorityLimit, Stopped, Completed, FailSafe };
const char* AIPhaseName(AIPhase phase);
// Value-only observation: no world, Actor, mutable simulation, or presentation reference.
struct AIObservation {
    uint64_t tick=0,binding=0;
    TrainId train; RouteId route;
    bool valid=false,resourcesOwned=false,signalAhead=true;
    double head=0,speed=0,length=0;
    MovementAuthority authority; SignalState signal; RouteState routeState;
    RequestResult lastRequest;
};
struct AIControl { CommandKind kind; double value; };
struct AIDecision {
    TrainId train; RouteId route; uint64_t observedTick=0,binding=0,sequence=0;
    bool requestRoute=false;
    std::vector<AIControl> controls;
};
// Intent only. The owner validates and dispatches decisions through S3/S4 queues.
class AIDriver {
public:
    AIDriver()=default;
    AIDriver(TrainId train,RouteId route,uint64_t binding);
    AIDecision Decide(const AIObservation& observation);
    AIPhase Phase() const { return phase_; }
    TrainId Train() const { return train_; }
    RouteId Route() const { return route_; }
    uint64_t Binding() const { return binding_; }
    uint64_t Sequence() const { return sequence_; }
    // Conservative 36 km/h opposing service stays below every curve limit in
    // the representative corridor; the shared throat shunt remains 10.8 km/h.
    double TargetSpeed() const { return train_.value>=3&&train_.value<=5&&route_.value==train_.value+1?10.0:3.0; }
    const std::string& Reason() const { return reason_; }
    std::string Json() const;
private:
    friend class WorldSnapshotCodec;
    TrainId train_; RouteId route_; uint64_t binding_=0,sequence_=0,nextRetry_=0,lastTick_=0;
    AIPhase phase_=AIPhase::Holding;
    bool observed_=false,hadRoute_=false,stopping_=false;
    std::string reason_="Awaiting authoritative observation";
};
TestReport RunAITests();
}
