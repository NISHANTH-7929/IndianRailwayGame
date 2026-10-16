#pragma once
#include "RailOperations.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace Rail {
// Derived diagnostics only: never grants authority, clears occupancy, or enters saves.
struct DispatchBlocker { TrainId train; std::string resource; };
struct DispatchHold {
    TrainId train; RouteId route; std::vector<DispatchBlocker> blockers;
    std::string reason;
    bool cyclic=false,prolonged=false;
    uint64_t unchangedTicks=0;
    std::string Summary() const {
        std::ostringstream s;s<<"T"<<train.value<<" R"<<route.value<<": "<<reason;
        for(const auto& b:blockers)s<<"; "<<b.resource<<" held by T"<<b.train.value;
        if(cyclic)s<<"; cyclic resource wait";
        if(prolonged)s<<"; no observed progress for "<<unchangedTicks<<" ticks (dispatcher attention required)";
        return s.str();
    }
};
inline std::vector<DispatchHold> InspectDispatch(const OperationsDefinitions& definitions,const OperationsSnapshot& snapshot) {
    std::vector<DispatchHold> out;
    for(const auto& request:snapshot.lastRequest) {
        const auto& q=request.second;
        if(q.granted||q.request.kind!=OperationsRequestKind::Route)continue;
        const auto authority=snapshot.authorities.find(request.first);
        if(authority!=snapshot.authorities.end()&&authority->second.routeGranted)continue;
        DispatchHold h;h.train=request.first;h.route=q.request.route;h.reason=q.reason;
        const auto route=definitions.routes.find(h.route);
        if(route==definitions.routes.end()){out.push_back(h);continue;}
        auto add=[&](TrainId train,const std::string& resource){
            if(!train.value)return;
            for(const auto& old:h.blockers)if(old.train==train&&old.resource==resource)return;
            h.blockers.push_back({train,resource});
        };
        auto block=[&](BlockId id,const char* role,bool includeSelf){
            const auto b=snapshot.blocks.find(id);if(b==snapshot.blocks.end())return;
            const auto name=std::string(role)+" B"+std::to_string(id.value);
            if(b->second.reservation.Active())add(b->second.reservation.train,name);
            for(auto train:b->second.occupants)if(includeSelf||train!=h.train)add(train,name);
        };
        for(auto id:route->second.blocks){
            block(id,"block",false);
            const auto def=definitions.blocks.find(id);
            if(def!=definitions.blocks.end())for(auto conflict:def->second.conflicts)block(conflict,"fouling",true);
        }
        for(auto p:route->second.points){
            const auto state=snapshot.points.find(p.first);
            if(state!=snapshot.points.end()&&state->second.lock.Active())add(state->second.lock.train,"point P"+std::to_string(p.first.value));
            const auto def=definitions.points.find(p.first);
            if(def!=definitions.points.end())for(auto id:def->second.protectedBlocks)block(id,"point protection",true);
        }
        out.push_back(std::move(h));
    }
    // Bounded reachability. A cycle is a dependency warning, not proof that an
    // occupied resource may be released or that a moving blocker cannot clear.
    if(out.size()<=64)for(auto& h:out){
        std::set<TrainId> seen;std::vector<TrainId> todo;
        for(const auto& b:h.blockers)todo.push_back(b.train);
        while(!todo.empty()){
            const auto t=todo.back();todo.pop_back();
            if(t==h.train){h.cyclic=true;break;}
            if(!seen.insert(t).second)continue;
            for(const auto& other:out)if(other.train==t)for(const auto& b:other.blockers)todo.push_back(b.train);
        }
    }
    return out;
}
class DispatchWatchdog {
public:
    // Default envelope: 30 simulation seconds without 0.5 m movement or changed
    // blockers. Pause does not age it. Recovery/backward clock resets history.
    explicit DispatchWatchdog(uint64_t thresholdTicks=900):threshold_(std::max<uint64_t>(1,thresholdTicks)){}
    std::vector<DispatchHold> Observe(const OperationsWorld& world) {
        auto holds=InspectDispatch(world.DefinitionsView(),world.Current());
        if(world.Current().tick<lastTick_)history_.clear();lastTick_=world.Current().tick;
        std::set<TrainId> present;
        for(auto& h:holds){
            present.insert(h.train);std::string signature=std::to_string(h.route.value)+":"+h.reason;
            std::map<TrainId,double> positions;
            auto position=[&](TrainId id){const auto* s=world.Spatial(id);if(s)positions[id]=s->Current().headStationMetres;};
            position(h.train);for(const auto& b:h.blockers){signature+=b.resource+":"+std::to_string(b.train.value);position(b.train);}
            auto found=history_.find(h.train);bool changed=found==history_.end();
            if(!changed){changed=found->second.signature!=signature||found->second.positions.size()!=positions.size();
                for(const auto& p:positions){auto old=found->second.positions.find(p.first);if(old==found->second.positions.end()||std::abs(old->second-p.second)>=.5)changed=true;}}
            if(changed)history_[h.train]={signature,positions,lastTick_};
            h.unchangedTicks=lastTick_-history_.at(h.train).since;h.prolonged=h.unchangedTicks>=threshold_;
        }
        for(auto i=history_.begin();i!=history_.end();)if(!present.count(i->first))i=history_.erase(i);else ++i;
        if(history_.size()>64)history_.clear();
        return holds;
    }
    void Reset(){history_.clear();lastTick_=0;}
private:
    struct History{std::string signature;std::map<TrainId,double> positions;uint64_t since;};
    std::map<TrainId,History> history_;uint64_t threshold_,lastTick_=0;
};
TestReport RunDispatchTests();
}
