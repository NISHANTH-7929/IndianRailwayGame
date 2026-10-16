#pragma once
#include "RailHandling.h"
#include <set>

namespace Rail {
using BlockId = Id<struct BlockTag>;
using RouteId = Id<struct RouteTag>;
using PointId = Id<struct PointTag>;
using SignalId = Id<struct SignalTag>;
struct TrackRange { EdgeId edge; double from=0,to=0; };
struct BlockDefinition {
    BlockId id; std::vector<TrackRange> coverage; std::set<BlockId> conflicts;
    TrainId exclusiveTo; // Bounded fixture holding area; zero means shared infrastructure.
    // Selected versioned holding areas may be traversed by another train only
    // under an explicit reservation. Default preserves legacy exclusive areas.
    bool transitAllowed=false;
};
struct PointDefinition {
    PointId id; std::map<int,std::vector<TransitionId>> positions; std::set<BlockId> protectedBlocks;
};
struct RouteDefinition {
    RouteId id; TrainId train; TrackPosition origin,endpoint;
    std::vector<DirectedEdge> path; std::set<BlockId> blocks; std::map<PointId,int> points;
    BlockId originHolding,destinationHolding; SignalId signal;
};
struct SignalDefinition { SignalId id; TrackPosition boundary; RouteId route; };
struct OperationsDefinitions {
    std::map<BlockId,BlockDefinition> blocks;
    std::map<PointId,PointDefinition> points;
    std::map<RouteId,RouteDefinition> routes;
    std::map<SignalId,SignalDefinition> signals;
    Result Validate(const Graph& graph) const;
};
struct OwnerToken {
    RouteId route; TrainId train; uint64_t generation=0;
    bool Active() const { return route.value!=0; }
    bool operator==(const OwnerToken& b) const { return route==b.route&&train==b.train&&generation==b.generation; }
    bool operator!=(const OwnerToken& b) const { return !(*this==b); }
};
struct BlockState { std::set<TrainId> occupants; OwnerToken reservation; };
struct PointState { int position=0,requested=0; OwnerToken lock; };
struct RouteState { OwnerToken owner; bool entered=false; std::string releaseReason; };
struct MovementAuthority {
    TrainId train; bool valid=false,routeGranted=false; OwnerToken owner;
    double minimumRear=0,maximumHead=0; TrackPosition endpoint;
    std::string restriction;
};
enum class SignalAspect { Stop,Proceed };
struct SignalState { SignalAspect aspect=SignalAspect::Stop; TrainId train; RouteId route; uint64_t revision=0; };
enum class OperationsRequestKind { Route,Cancel,Point };
struct OperationsRequest {
    OperationsRequestKind kind=OperationsRequestKind::Route;
    TrainId train; RouteId route; PointId point; int position=0;
    uint64_t sequence=0,applyTick=0,expectedGeneration=0;
};
struct RequestResult { OperationsRequest request; bool granted=false; std::string reason; uint64_t tick=0; };
struct OperationsSnapshot {
    uint64_t tick=0; bool valid=false;
    std::map<BlockId,BlockState> blocks; std::map<PointId,PointState> points;
    std::map<RouteId,RouteState> routes; std::map<TrainId,MovementAuthority> authorities;
    std::map<SignalId,SignalState> signals; std::map<TrainId,RequestResult> lastRequest;
    std::deque<std::string> events;
};
using Occupancy = std::map<BlockId,std::set<TrainId>>;
// Intersects every retained occurrence, including the conservative coupler gaps.
Result FootprintBlocks(const OperationsDefinitions& definitions,const Graph& graph,
                       const PathHistory& history,double rear,double head,std::set<BlockId>& out);
OperationsDefinitions MakeOperationsDefinitions();

}

