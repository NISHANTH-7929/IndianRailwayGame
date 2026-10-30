#pragma once
#include "RailKernel.h"
#include <deque>

namespace Rail {
// Provisional engineering data. One record per S2 vehicle definition, never a class-name switch.
struct HandlingDefinition {
    DefinitionId id;
    uint32_t version = 1;
    double maxTractionN = 0, maxPowerWatts = 0, tractionCutoffMps = 16;
    double adhesiveMassFraction = 1, adhesionCoefficient = .25;
    double rollingNPerKg = .015, aeroNPerMpsSquared = 15;
    double serviceBrakeNPerKg = .8, emergencyBrakeNPerKg = 1.2;
    double tractionResponseSeconds = .5, brakeApplySeconds = 1.5, brakeReleaseSeconds = 2;
    double emergencyApplySeconds = .5;
    // S10: graduated twin-pipe air brake (A9 handle -> brake pipe -> distributor -> cylinder) and
    // regenerative braking from the master controller pulled back past zero. Off/zero keeps the
    // earlier lumped brake, so fixtures and their evidence are unchanged.
    bool gradedAirBrake = false; double maxRegenN = 0;
    Result Validate() const;
};
// Indian Railways twin-pipe graduated-release air brake on LHB stock (kg/cm2). Sources:
// IRIMEE/RDSO LHB air brake handbook (BP 5.0; application 3-5 s; release 15-20 s; LHB BC limited
// to 3.0 by the pressure converter) and the CRS question bank (full service reduction 1.0-1.5).
// The minimum reduction and distributor ratio are provisional engineering values.
struct AirBrake {
    static constexpr double ChargedPipe=5.0,FeedPipe=6.0,MainReservoir=8.5;
    static constexpr double MinimumReduction=.4,FullServiceReduction=1.5,RunningZone=.08;
    static constexpr double DistributorRatio=2.5,MaxCylinder=3.0,TractionInterlockPipe=4.7;
    // A9 handle (0..1): Release/Running below RunningZone, then minimum to full service reduction.
    static double BrakePipe(double handle,bool emergency){
        if(emergency)return 0;if(handle<RunningZone)return ChargedPipe;
        return ChargedPipe-(MinimumReduction+(FullServiceReduction-MinimumReduction)*(handle-RunningZone)/(1-RunningZone));
    }
    // Distributor valve: cylinder follows pipe reduction (graduated application and release).
    static double CylinderFraction(double brakePipe){
        const double c=DistributorRatio*(ChargedPipe-brakePipe)/MaxCylinder;return c<0?0:c>1?1:c;
    }
};
// Regenerative (electric) braking effort available at a speed: constant up to the power limit,
// fading out below ~10 km/h as the traction motors lose regeneration voltage.
double RegenerativeEffortN(const HandlingDefinition& d,double speedMps,double adhesionN);
using HandlingDefinitions = std::map<DefinitionId,HandlingDefinition>;
enum class CommandKind { Throttle, Brake, Direction, Emergency };
struct TrainCommand {
    TrainId train;
    uint64_t source = 1, sequence = 0, applyTick = 0;
    CommandKind kind = CommandKind::Throttle;
    double value = 0;
};
struct ControlState { double throttle = 0, brake = 0; int direction = 1; bool emergency = false; };
struct LongitudinalState {
    ControlState command;
    double speedMps = 0, accelerationMps2 = 0, appliedThrottle = 0, appliedBrake = 0;
    double tractionN = 0, brakingN = 0, resistanceN = 0, gradeN = 0, netN = 0, massKg = 0;
    uint64_t lastAppliedSequence = 0, rejectedCommands = 0;
    bool held = true;
    std::string lastRejection;
};
// Pure train-level model. It uses, but does not own or replace, the S2 spatial simulation.
class HandlingModel {
public:
    Result Initialize(const Simulation& spatial, Definitions vehicles, Formation formation,
                      HandlingDefinitions definitions, uint64_t source = 1);
    Result Submit(const Simulation& spatial,const TrainCommand& command);
    Result Update(Simulation& spatial,double renderDeltaSeconds);
    // Called once by an enclosing shared world clock; does not run an accumulator.
    Result AdvanceFixedStep(Simulation& spatial,double seconds);
    void SetSuspended(bool suspended);
    void CancelInput();
    bool Suspended() const { return suspended_; }
    const LongitudinalState& Current() const { return state_; }
    uint64_t Source() const { return source_; }
    size_t PendingCommands() const { return queue_.size(); }
    std::string Json(const Snapshot& spatial) const;
private:
    friend class WorldSnapshotCodec;
    Result Step(Simulation& spatial,double seconds);
    Definitions vehicles_;
    Formation formation_;
    HandlingDefinitions definitions_;
    LongitudinalState state_;
    TrainId train_;
    uint64_t source_ = 1, lastSubmittedSequence_ = 0;
    std::deque<TrainCommand> queue_;
    bool initialized_ = false, suspended_ = false, skipResumeDelta_ = false, cancelPending_ = false;
};
HandlingDefinitions MakeHandlingDefinitions();
TestReport RunHandlingTests();
TestReport RunAirBrakeTests();
}
