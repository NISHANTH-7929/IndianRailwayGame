#include "RailOperations.h"
#include <cmath>
#include <chrono>
#include <sstream>

namespace Rail {
Result OperationsWorld::InitializeAIFixture(){
    auto r=InitializeFixture();
    // Player scenario begins with a normal queued A route request, never a privileged grant.
    if(r.ok)r=RequestRoute(TrainId(1),RouteId(1));
    if(r.ok)r=EnableAI(TrainId(2),RouteId(2));return r;
}
Result OperationsWorld::EnableAI(TrainId train,RouteId route){
    const auto d=definitions_.routes.find(route);
    if(paused_||suspended_||!state_.snapshot.valid||!state_.trains.count(train)||d==definitions_.routes.end()||d->second.train!=train||
       state_.aiBindings.count(train)||std::abs(Handling(train)->speedMps)>.02||Handling(train)->command.direction!=1||
       state_.snapshot.authorities.at(train).routeGranted)return Result::Fail("Invalid AI binding");
    for(const auto& pending:state_.requests)if(pending.train==train)return Result::Fail("Pending player request prevents AI binding");
    state_.trains.at(train).handling.CancelInput();
    state_.aiBindings.emplace(train,State::AIBinding{route,1,0});
    state_.drivers.emplace(train,AIDriver(train,route,1));return {};
}
const AIDriver* OperationsWorld::AI(TrainId train) const {
    auto i=state_.drivers.find(train);return i==state_.drivers.end()?nullptr:&i->second;
}
AIObservation OperationsWorld::ObserveAI(TrainId train) const {
    AIObservation o;o.tick=state_.snapshot.tick;o.train=train;
    auto binding=state_.aiBindings.find(train);if(binding==state_.aiBindings.end())return o;
    o.binding=binding->second.generation;o.route=binding->second.route;
    const auto* spatial=Spatial(train);const auto* handling=Handling(train);
    auto a=state_.snapshot.authorities.find(train);auto r=state_.snapshot.routes.find(o.route);auto d=definitions_.routes.find(o.route);
    if(!spatial||!handling||a==state_.snapshot.authorities.end()||r==state_.snapshot.routes.end()||d==definitions_.routes.end())return o;
    auto sig=state_.snapshot.signals.find(d->second.signal);if(sig==state_.snapshot.signals.end())return o;
    o.head=spatial->Current().headStationMetres;o.length=spatial->Current().properties.lengthMetres;o.speed=handling->speedMps;
    o.authority=a->second;o.signal=sig->second;o.routeState=r->second;
    auto last=state_.snapshot.lastRequest.find(train);if(last!=state_.snapshot.lastRequest.end())o.lastRequest=last->second;
    bool originFound=false;
    for(const auto& occurrence:spatial->History().occurrences)if(occurrence.directed.edge==d->second.origin.edge){
        if(originFound)return o;originFound=true;const auto& P=d->second.origin;
        o.signalAhead=o.head<=occurrence.beginMetres+(P.direction==1?P.distanceMetres:occurrence.endMetres-occurrence.beginMetres-P.distanceMetres)+1e-8;
    }
    o.resourcesOwned=o.authority.routeGranted;
    for(auto block:d->second.blocks){auto b=state_.snapshot.blocks.find(block);
        if(b==state_.snapshot.blocks.end()||b->second.reservation!=o.authority.owner)o.resourcesOwned=false;
        else for(auto occupant:b->second.occupants)if(occupant!=train)o.resourcesOwned=false;
    }
    for(auto point:d->second.points){auto p=state_.snapshot.points.find(point.first);
        if(p==state_.snapshot.points.end()||p->second.lock!=o.authority.owner||p->second.position!=point.second)o.resourcesOwned=false;
    }
    o.valid=state_.snapshot.valid&&originFound&&spatial->Current().tick==o.tick&&sig->second.revision==o.tick;return o;
}
Result OperationsWorld::DispatchAI(const AIDecision& d){
    auto b=state_.aiBindings.find(d.train);
    if(b==state_.aiBindings.end()||d.route!=b->second.route||d.binding!=b->second.generation||d.observedTick!=state_.snapshot.tick||
       d.sequence!=b->second.acceptedSequence+1)return Result::Fail("Stale or mismatched AI intent");
    if(d.controls.size()!=2)return Result::Fail("Invalid AI control batch");
    bool throttle=false,brake=false;
    for(auto c:d.controls){if(!std::isfinite(c.value)||c.value<0||c.value>1)return Result::Fail("Invalid AI control value");
        if(c.kind==CommandKind::Throttle&&!throttle)throttle=true;
        else if(c.kind==CommandKind::Brake&&!brake)brake=true;
        else return Result::Fail("Unsupported or duplicate AI control");
    }
    // Executed only inside the candidate step. Any queue failure rolls the whole step back.
    for(auto c:d.controls){auto r=SubmitControl(d.train,c.kind,c.value);if(!r.ok)return r;}
    if(d.requestRoute){auto r=SubmitRequest({OperationsRequestKind::Route,d.train,d.route,{},0,state_.sequences[d.train]+1,state_.snapshot.tick+2,0});if(!r.ok)return r;}
    std::ostringstream trace;trace<<"{\"tick\":"<<d.observedTick<<",\"applyTick\":"<<d.observedTick+1<<",\"train\":"<<d.train.value
      <<",\"requestApplyTick\":"<<(d.requestRoute?d.observedTick+2:0)<<",\"sequence\":"<<d.sequence<<",\"requestRoute\":"<<(d.requestRoute?"true":"false")<<",\"controls\":[";
    bool comma=false;for(auto c:d.controls){if(comma)trace<<',';comma=true;trace<<"{\"kind\":"<<int(c.kind)<<",\"value\":"<<c.value<<'}';}
    trace<<"]}";const auto record=trace.str();for(unsigned char byte:record){state_.traceHash^=byte;state_.traceHash*=1099511628211ull;}
    state_.aiTrace.push_back(record);if(state_.aiTrace.size()>32)state_.aiTrace.pop_front();
    b->second.acceptedSequence=d.sequence;return {};
}
Result OperationsWorld::UpdateAI(){
    for(const auto& binding:state_.aiBindings){
        auto driver=state_.drivers.find(binding.first);
        if(driver==state_.drivers.end()||driver->second.Train()!=binding.first||driver->second.Binding()!=binding.second.generation||driver->second.Route()!=binding.second.route){
            auto r=SubmitControl(binding.first,CommandKind::Throttle,0);if(r.ok)r=SubmitControl(binding.first,CommandKind::Brake,1);if(!r.ok)return r;continue;
        }
        if(state_.snapshot.tick%3!=0)continue; // 10 Hz decisions on the shared 30 Hz clock.
        const auto start=std::chrono::steady_clock::now();
        const auto before=driver->second.Phase();auto decision=driver->second.Decide(ObserveAI(binding.first));
        auto r=DispatchAI(decision);if(!r.ok)return r;
        metrics_.aiMilliseconds+=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();++metrics_.decisions;
        if(before!=driver->second.Phase())Event("AI T"+std::to_string(binding.first.value)+" "+AIPhaseName(driver->second.Phase()));
    }return {};
}
}


