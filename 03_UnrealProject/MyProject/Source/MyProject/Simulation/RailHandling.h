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
    Result Validate() const;
};
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
}
