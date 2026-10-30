#pragma once
#include "RailAutomaticSignals.h"

namespace Rail {
// Rail discontinuities that wheels strike, for the train's acoustics. The corridor is long-welded
// rail, continuous except at glued insulated rail joints, which bound the track circuits at each
// stop signal, and at points and crossings, where wheels cross the switch and the crossing-nose gap.
// Positions come from the same signal plan and track graph as the railway (no extra state). The
// turnout lead is provisional (BG 1 in 12) until the surveyed layout replaces the representative one.
enum class JointKind { InsulatedJoint, Switch, CrossingNose };
constexpr double TurnoutLeadMetres=28; // toe of switch to nose of crossing, BG 1 in 12 (provisional)
struct TrackJoint { uint64_t key=0; JointKind kind=JointKind::InsulatedJoint; double station=0; };
// Joints on a train's committed path with path stations in [from,to], ordered along the path.
// Keys identify the physical joint (non-zero, stable across calls and trains).
std::vector<TrackJoint> JointsBetween(const OperationsWorld& world,TrainId train,double from,double to);
TestReport RunTrackJointTests();
}
