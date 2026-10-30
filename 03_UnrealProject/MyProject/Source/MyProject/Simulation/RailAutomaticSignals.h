#pragma once
#include "RailOperations.h"

namespace Rail {
// Indian Railways multiple-aspect colour-light signalling (MACLS) for the representative corridor.
// Aspects (G&SR): Stop = red; Caution = one yellow, be prepared to stop at the next stop signal;
// Attention = double yellow, be prepared to pass the next signal at restricted speed; Proceed = green.
// Automatic block (GR 9.01): a stop signal may assume an 'off' aspect only when the line is clear up
// to the next stop signal and an adequate distance (120 m on double line) beyond it.
// Sources: irfca.org/docs/rulebook/gr-automatic-block.html, irfca.org/faq/faq-signal2.html.
//
// Signals are fixed lineside infrastructure derived from the route definitions: a manual stop
// signal (station starter/home) at every route origin, and automatic stop signals ('A' marker)
// at a nominal spacing along each route. Aspects are a pure function of the authoritative
// operations snapshot, so they never need saving and can never disagree with movement authority:
// a manual signal is Stop until its route is granted, and the end of a route reads as Stop.
enum class LinesideAspect { Stop, Caution, Attention, Proceed };
const char* LinesideAspectName(LinesideAspect aspect);
enum class SignalRole { Manual, Automatic };
constexpr double AutomaticSignalSpacingMetres=1000; // representative suburban automatic section length
constexpr double AdequateDistanceMetres=120;          // GR 9.01(2), double line
struct SignalSite {
    uint32_t index=0; SignalRole role=SignalRole::Automatic;
    TrackPosition position;                  // edge-local distance, direction of travel
    std::vector<RouteId> routes;             // every route whose path passes this physical signal
    std::map<RouteId,double> station;        // metres from the start of each route's path
    std::map<RouteId,int> next;              // next site along each route; -1 at the end of the route
    std::map<RouteId,double> sectionEnd;     // next signal + adequate distance, capped at the route end
    std::map<RouteId,std::vector<BlockId>> sectionBlocks; // blocks touching that section (static)
};
struct RoutePiece { DirectedEdge edge; double begin=0,length=0; };
struct SignalPlan {
    std::string contentId;
    std::vector<SignalSite> sites;
    std::map<RouteId,std::vector<RoutePiece>> paths;  // route path stationing
    std::map<RouteId,std::map<uint64_t,std::vector<size_t>>> pieceByEdge;
    std::map<RouteId,double> endStation;     // route endpoint along its path
    std::map<RouteId,int> beyondEnd;         // site standing at the route endpoint (next route's origin), or -1
    Result Validate() const;
};
SignalPlan BuildSignalPlan(const OperationsDefinitions& definitions,const Graph& graph,const std::string& contentId);
std::vector<LinesideAspect> EvaluateSignals(const SignalPlan& plan,const OperationsWorld& world);
struct SignalAhead { bool found=false; uint32_t site=0; double distance=0; LinesideAspect aspect=LinesideAspect::Stop; SignalRole role=SignalRole::Manual; };
// Nearest signal in front of a train's head, located along its committed path history.
SignalAhead NextSignal(const SignalPlan& plan,const std::vector<LinesideAspect>& aspects,const OperationsWorld& world,TrainId train);
// Station of a track position along a train's committed path (false when absent or ambiguous).
bool PathStation(const PathHistory& history,TrackPosition position,double& station);
// Built once per content and graph; presentation and HUD share the same plan.
const SignalPlan& SignalPlanFor(const OperationsWorld& world);
TestReport RunSignalTests();
}
