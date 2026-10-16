#pragma once
#include "RailCheckpointStore.h"
#include <array>
namespace Rail {
enum class ServicePhase { Selection, Briefing, Running, Result };
enum class StationPhase { Approaching, Arrival, Berthing, Dwelling, ReadyToDepart, Departed, Completed };
enum class DrivingMode { Assisted, Standard, ExpertReserved };
enum class ObjectiveKind { Departure, Authority, Speed, Station, Accuracy, Dwell, DeparturePermission, Itinerary, Traffic, Count };
struct ObjectiveDefinition { ObjectiveKind kind; std::string title; };
struct ServiceDefinition {
    std::string id="S7-Shantipur-01",name="Shantipur shuttle",formation="Powered locomotive proxy + one coach",station="Shantipur";
    std::string version="IR-S7-service-1";
    std::string contentId=FormationContent::LegacyId;
    bool requireTrafficHold=true,terminalStop=false;
    TrainId player{2},traffic{1};RouteId playerRoute{2},trafficRoute{1};EdgeId platformEdge{2};
    double startHead=100,platformFrom=95,platformTo=145,berthHead=140,berthTolerance=2,finishHead=152;
    double speedLimit=3,assistedCruise=2,arrivalWindowSeconds=75,departureWindowSeconds=95,expectedDurationSeconds=240;
    uint64_t dwellTicks=300,routeRetryTicks=30,reputationReward=10;
    double completionPoints=50,safetyPoints=20,accuracyPoints=15,speedPoints=10,punctualityPoints=5;
    std::string unlock="Shantipur service mastery badge";
    std::array<ObjectiveDefinition,9> objectives{{
        {ObjectiveKind::Departure,"Leave the starting position"},{ObjectiveKind::Authority,"Stay inside movement authority"},
        {ObjectiveKind::Speed,"Keep at or below 10.8 km/h"},{ObjectiveKind::Station,"Serve Shantipur platform"},
        {ObjectiveKind::Accuracy,"Stop within 2 m of the marker"},{ObjectiveKind::Dwell,"Wait for the full station dwell"},
        {ObjectiveKind::DeparturePermission,"Depart only when ready"},{ObjectiveKind::Itinerary,"Stop at the finish marker"},
        {ObjectiveKind::Traffic,"Wait for the express to clear"}}};
    std::array<std::string,4> tutorial{{"Power moves the train; brake slows it. Speed is shown in km/h.",
        "The express has priority. Stop before the junction and wait for the route to clear.",
        "Berth at the platform marker, hold still for 10 seconds, then press Depart.",
        "Stop at Finish. Review the grade and save the reputation award. Assisted helps with braking."}};
    Result Validate() const;
    std::string Json() const;
};
struct ServiceProfile {uint64_t nextRun=1,lastSettledRun=0,awards=0,reputation=0;bool unlocked=false;};
struct ServiceState {
    ServicePhase phase=ServicePhase::Selection;StationPhase station=StationPhase::Approaching;DrivingMode mode=DrivingMode::Assisted;
    uint64_t run=0,ticks=0,trafficTicks=0,overspeedTicks=0,harshTicks=0,dwellTicks=0,arrivalTick=0,departTick=0,lastRequestTick=0,cue=0;
    bool formationConfirmed=false,trafficSeen=false,trafficCleared=false,departureAuthorized=false,success=false,settled=false;
    std::array<unsigned,9> objectives{}; // 0 pending, 1 met, 2 failed; persisted logical results.
    double requestedPower=0,requestedBrake=1,berthError=0,score=0,playerDelaySeconds=0,accumulator=0;
    std::string reason,grade="Not graded";
};
class ServiceRuntime {
public:
    explicit ServiceRuntime(ServiceDefinition definition={}):definition_(std::move(definition)){}
    const ServiceDefinition& Definition() const {return definition_;}
    const ServiceState& Current() const {return state_;}
    const ServiceProfile& Profile() const {return profile_;}
    Result Select();Result ConfirmFormation();Result SetMode(DrivingMode mode);
    Result Start(OperationsWorld& world);
    Result Input(OperationsWorld& world,CommandKind kind,double value);
    Result Depart(const OperationsWorld& world);
    Result Update(OperationsWorld& world,double seconds);
    Result Replay();
    std::string HoldReason(const OperationsWorld& world) const;
    std::string Json() const;
    Result Capture(const OperationsWorld& world,uint64_t generation,SnapshotRecord& out) const;
    std::array<double,5> ScoreComponents() const;
    static Result Restore(const SnapshotRecord& record,const ServiceDefinition& definition,OperationsWorld& world,ServiceRuntime& out);
    Result Save(const OperationsWorld& world,const CheckpointIO& io,CheckpointCommit& committed) const;
    Result Recover(const CheckpointIO& io,OperationsWorld& world,RecoverySelection& selected);
    Result Settle(const OperationsWorld& world,const CheckpointIO& io,CheckpointCommit& committed);
    static bool Compatible(const SnapshotRecord& record,const ServiceDefinition& definition);
private:
    ServiceDefinition definition_;ServiceState state_;ServiceProfile profile_;
    Result Fixed(OperationsWorld& world);
    void Evaluate(const OperationsWorld& world);
    Result Validate(const OperationsWorld& world) const;
    void Fail(const std::string& reason);
    void ResultFor(bool success,const std::string& reason);
    void Assistance(OperationsWorld& world);
    friend TestReport RunServiceTests();
};
const char* ServicePhaseName(ServicePhase phase);
const char* StationPhaseName(StationPhase phase);
TestReport RunServiceTests();
TestReport RunLhbTests();
}
