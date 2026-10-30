#include "RailAIDriver.h"
#include <cmath>
#include <sstream>

namespace Rail {
const char* AIPhaseName(AIPhase phase){
    switch(phase){
    case AIPhase::Holding:return "Holding";case AIPhase::RequestingRoute:return "RequestingRoute";
    case AIPhase::WaitingForRoute:return "WaitingForRoute";case AIPhase::RouteGranted:return "RouteGranted";
    case AIPhase::Driving:return "Driving";case AIPhase::RestrictiveApproach:return "RestrictiveApproach";
    case AIPhase::ApproachingAuthorityLimit:return "ApproachingAuthorityLimit";case AIPhase::Stopped:return "Stopped";
    case AIPhase::Completed:return "Completed";default:return "FailSafe";
    }
}
AIDriver::AIDriver(TrainId train,RouteId route,uint64_t binding):train_(train),route_(route),binding_(binding){}
AIDecision AIDriver::Decide(const AIObservation& o){
    AIDecision d;d.train=train_;d.route=route_;d.binding=binding_;d.observedTick=o.tick;d.sequence=++sequence_;
    auto safe=[&](AIPhase phase,const char* reason){phase_=phase;reason_=reason;d.controls={{CommandKind::Throttle,0},{CommandKind::Brake,1}};};
    if(int(phase_)<int(AIPhase::Holding)||int(phase_)>=int(AIPhase::FailSafe)||!binding_||!train_.value||!route_.value||!o.valid||o.train!=train_||o.route!=route_||o.binding!=binding_||
       (observed_&&o.tick<=lastTick_)||!std::isfinite(o.head)||!std::isfinite(o.speed)||!std::isfinite(o.length)||o.length<=0||
       !o.authority.valid||o.authority.train!=train_||!std::isfinite(o.authority.minimumRear)||!std::isfinite(o.authority.maximumHead)||
       o.head>o.authority.maximumHead+1e-8||o.head-o.length<o.authority.minimumRear-1e-8){
        safe(AIPhase::FailSafe,"Invalid or stale authoritative observation; brake latched");return d;
    }
    observed_=true;lastTick_=o.tick;
    if(!o.authority.routeGranted){
        if(hadRoute_){safe(AIPhase::Completed,"S4 released route; holding in remaining permission");return d;}
        safe(AIPhase::WaitingForRoute,"Waiting for S4 route permission");
        if(o.tick>=nextRetry_){d.requestRoute=true;nextRetry_=o.tick+90;phase_=AIPhase::RequestingRoute;reason_="Request intent submitted to S4";}
        return d;
    }
    if(o.authority.owner.route!=route_||o.authority.owner.train!=train_||!o.authority.owner.generation||
       o.routeState.owner!=o.authority.owner||!o.resourcesOwned){
        safe(AIPhase::FailSafe,"Contradictory route resources; brake latched");return d;
    }
    if(o.signalAhead&&(o.signal.aspect!=SignalAspect::Proceed||o.signal.route!=route_||o.signal.train!=train_)){
        safe(AIPhase::RestrictiveApproach,"Governing signal Stop; braking");return d;
    }
    const bool first=!hadRoute_;hadRoute_=true;
    const double remaining=o.authority.maximumHead-o.head;
    if(o.speed<-.02){safe(AIPhase::FailSafe,"Reverse movement not enabled for forward route intent");return d;}
    const bool endpoint=remaining<=o.speed*o.speed/(2*.45)+2.5*std::abs(o.speed)+3;
    if(train_.value>=3&&train_.value<=5&&route_.value==train_.value+1)stopping_|=endpoint;
    else stopping_=endpoint; // Retain legacy controllers' checkpoint-free behaviour.
    if(stopping_){safe(std::abs(o.speed)<=.02?AIPhase::Stopped:AIPhase::ApproachingAuthorityLimit,"Conservative endpoint braking; S4 retains release ownership");return d;}
    phase_=first?AIPhase::RouteGranted:AIPhase::Driving;reason_="Driving inside matching S4 route authority";
    if(o.speed>TargetSpeed()+.2)d.controls={{CommandKind::Throttle,0},{CommandKind::Brake,.25}};
    else d.controls={{CommandKind::Brake,0},{CommandKind::Throttle,o.speed<TargetSpeed()-.2?1.0:0.0}};
    return d;
}
std::string AIDriver::Json() const {
    std::ostringstream o;o<<"{\"train\":"<<train_.value<<",\"route\":"<<route_.value<<",\"binding\":"<<binding_
      <<",\"state\":\""<<AIPhaseName(phase_)<<"\",\"sequence\":"<<sequence_<<",\"nextRetryTick\":"<<nextRetry_
      <<",\"lastTick\":"<<lastTick_<<",\"targetSpeedMps\":"<<TargetSpeed()<<",\"reason\":\""<<reason_<<"\"}";return o.str();
}
}
