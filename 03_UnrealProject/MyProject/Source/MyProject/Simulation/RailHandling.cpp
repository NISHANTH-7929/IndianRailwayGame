#include "RailHandling.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace Rail {
namespace {
constexpr double Gravity = 9.80665;
double Sign(double x) { return x>0?1.0:(x<0?-1.0:0.0); }
double Approach(double value,double target,double maximumChange) {
    return value+std::clamp(target-value,-maximumChange,maximumChange);
}
struct Forces {
    double traction=0,brake=0,resistance=0,grade=0;
    double Net() const { return traction+brake+resistance+grade; }
    Forces Blend(const Forces& b,double weight) const {
        return {traction*weight+b.traction*(1-weight),brake*weight+b.brake*(1-weight),
                resistance*weight+b.resistance*(1-weight),grade*weight+b.grade*(1-weight)};
    }
};
}
Result HandlingDefinition::Validate() const {
    const double values[]={maxTractionN,maxPowerWatts,tractionCutoffMps,adhesiveMassFraction,
        adhesionCoefficient,rollingNPerKg,aeroNPerMpsSquared,serviceBrakeNPerKg,emergencyBrakeNPerKg,
        tractionResponseSeconds,brakeApplySeconds,brakeReleaseSeconds,emergencyApplySeconds};
    for(double v:values)if(!std::isfinite(v)||v<0)return Result::Fail("Nonfinite/negative handling parameter");
    if(!id.value||!version||tractionCutoffMps<=0||adhesiveMassFraction>1||adhesionCoefficient>1||
       serviceBrakeNPerKg<=0||emergencyBrakeNPerKg<serviceBrakeNPerKg||
       tractionResponseSeconds<=0||brakeApplySeconds<=0||brakeReleaseSeconds<=0||emergencyApplySeconds<=0||
       (maxTractionN>0 && (maxPowerWatts<=0||adhesiveMassFraction<=0)))
        return Result::Fail("Invalid handling definition");
    return {};
}
Result HandlingModel::Initialize(const Simulation& spatial,Definitions vehicles,Formation formation,
                                HandlingDefinitions definitions,uint64_t source) {
    if(spatial.Paused()||!source)return Result::Fail("Handling requires an initialized S2 simulation and source");
    FormationProperties properties;auto valid=formation.Derive(vehicles,properties);if(!valid.ok)return valid;
    const auto& snapshot=spatial.Current();
    if(snapshot.vehicles.size()!=formation.members.size()||snapshot.properties.massKg!=properties.massKg||
       snapshot.properties.lengthMetres!=properties.lengthMetres||snapshot.commandedSpeedMps!=0)
        return Result::Fail("Handling/S2 formation mismatch or nonstationary initialization");
    for(size_t i=0;i<formation.members.size();++i){
        const auto& m=formation.members[i];const auto h=definitions.find(m.definition);
        if(h==definitions.end()||h->first!=h->second.id||!h->second.Validate().ok||snapshot.vehicles[i].id!=m.id||
           snapshot.vehicles[i].definition!=m.definition||(!vehicles.at(m.definition).powered&&h->second.maxTractionN>0))
            return Result::Fail("Missing/invalid handling definition or member identity");
    }
    vehicles_=std::move(vehicles);formation_=std::move(formation);definitions_=std::move(definitions);
    state_=LongitudinalState{};state_.massKg=properties.massKg;train_=snapshot.train;source_=source;
    queue_.clear();lastSubmittedSequence_=0;initialized_=true;suspended_=false;skipResumeDelta_=false;cancelPending_=false;
    return {};
}
Result HandlingModel::Submit(const Simulation& spatial,const TrainCommand& c) {
    auto reject=[&](const char* reason){++state_.rejectedCommands;state_.lastRejection=reason;return Result::Fail(reason);};
    if(!initialized_||spatial.Current().train!=train_||c.train!=train_||c.source!=source_)return reject("Wrong train or command source");
    if(suspended_||spatial.Paused())return reject("Simulation is paused");
    if(!c.sequence||c.sequence<=lastSubmittedSequence_||c.applyTick<=spatial.Current().tick||
       c.applyTick-spatial.Current().tick>18000||queue_.size()>=256||(!queue_.empty()&&c.applyTick<queue_.back().applyTick))
        return reject("Invalid sequence/tick or full command queue");
    if(!std::isfinite(c.value))return reject("Nonfinite command");
    switch(c.kind){
        case CommandKind::Throttle:case CommandKind::Brake:
            if(c.value<0||c.value>1)return reject("Command outside zero to one");break;
        case CommandKind::Direction:
            if(c.value!=-1&&c.value!=0&&c.value!=1)return reject("Invalid reverser direction");break;
        case CommandKind::Emergency:
            if(c.value!=0&&c.value!=1)return reject("Invalid emergency command");break;
        default:return reject("Unknown command kind");
    }
    queue_.push_back(c);lastSubmittedSequence_=c.sequence;return {};
}
void HandlingModel::CancelInput(){queue_.clear();cancelPending_=true;}
void HandlingModel::SetSuspended(bool suspended){
    if(suspended&&!suspended_)CancelInput();
    if(!suspended&&suspended_)skipResumeDelta_=true;
    suspended_=suspended;
}
Result HandlingModel::Update(Simulation& spatial,double dt){
    if(!initialized_)return Result::Fail("Handling not initialized");
    if(!std::isfinite(dt)||dt<0)return Result::Fail("Invalid handling delta");
    if(suspended_)return {};
    if(skipResumeDelta_){dt=0;skipResumeDelta_=false;}
    return spatial.UpdateSteps(dt,[this](Simulation& s,double seconds){return Step(s,seconds);});
}
Result HandlingModel::Step(Simulation& spatial,double dt){
    LongitudinalState next=state_;
    if(cancelPending_){next.command.throttle=0;next.command.brake=1;cancelPending_=false;}
    const uint64_t tick=spatial.Current().tick+1;
    bool emergencyPressed=false;
    while(!queue_.empty()&&queue_.front().applyTick<=tick){
        const auto c=queue_.front();queue_.pop_front();next.lastAppliedSequence=c.sequence;
        auto reject=[&](const char* message){++next.rejectedCommands;next.lastRejection=message;};
        switch(c.kind){
            case CommandKind::Throttle:next.command.throttle=c.value;break;
            case CommandKind::Brake:next.command.brake=c.value;break;
            case CommandKind::Direction:
                if(c.value==0)next.command.direction=0;
                else if(std::abs(next.speedMps)>.02||next.command.throttle>0||next.appliedThrottle>.001)
                    reject("Reverser change requires stopped train and zero throttle");
                else next.command.direction=int(c.value);
                break;
            case CommandKind::Emergency:
                if(c.value==1){emergencyPressed=true;next.command.emergency=true;}
                else if(std::abs(next.speedMps)>.02||next.command.throttle>0||emergencyPressed)
                    reject("Emergency reset requires stopped train and zero throttle");
                else next.command.emergency=false;
                break;
        }
    }
    if(emergencyPressed)next.command.emergency=true;
    double tractionSeconds=0,applySeconds=0,releaseSeconds=0,emergencySeconds=0;
    for(const auto& member:formation_.members){const auto& d=definitions_.at(member.definition);
        tractionSeconds=std::max(tractionSeconds,d.tractionResponseSeconds);
        applySeconds=std::max(applySeconds,d.brakeApplySeconds);releaseSeconds=std::max(releaseSeconds,d.brakeReleaseSeconds);
        emergencySeconds=std::max(emergencySeconds,d.emergencyApplySeconds);
    }
    const double brakeTarget=next.command.emergency?1:next.command.brake;
    const double brakeSeconds=brakeTarget>next.appliedBrake?(next.command.emergency?emergencySeconds:applySeconds):releaseSeconds;
    next.appliedBrake=Approach(next.appliedBrake,brakeTarget,dt/brakeSeconds);
    const bool cutout=next.command.emergency||next.command.brake>0||next.appliedBrake>1e-9||next.command.direction==0;
    next.appliedThrottle=Approach(next.appliedThrottle,cutout?0:next.command.throttle,dt/tractionSeconds);
    double grade=0,rolling=0,aero=0,brakeCapacity=0;
    for(size_t i=0;i<formation_.members.size();++i){const auto id=formation_.members[i].definition;
        const auto& v=vehicles_.at(id);const auto& d=definitions_.at(id);Sample point;
        auto r=spatial.History().SampleAt(spatial.Network(),spatial.Current().vehicles[i].centreStationMetres,point);if(!r.ok)return r;
        grade-=v.massKg*Gravity*point.tangent.z;
        rolling+=d.rollingNPerKg*v.massKg;aero+=d.aeroNPerMpsSquared;
        brakeCapacity+=next.appliedBrake*std::min((next.command.emergency?d.emergencyBrakeNPerKg:d.serviceBrakeNPerKg)*v.massKg,
                                               d.adhesionCoefficient*v.massKg*Gravity);
    }
    auto evaluate=[&](double speed){
        Forces force;force.grade=grade;
        if(!cutout){for(const auto& member:formation_.members){const auto& v=vehicles_.at(member.definition);const auto& d=definitions_.at(member.definition);
            if(!v.powered||d.maxTractionN==0)continue;
            const double magnitude=std::abs(speed);
            const double base=d.version>=2?d.tractionCutoffMps*.95:std::min(d.tractionCutoffMps*.5,d.maxPowerWatts/d.maxTractionN);
            const double taper=std::clamp((d.tractionCutoffMps-magnitude)/(d.tractionCutoffMps-base),0.0,1.0);
            const double available=std::min({d.maxTractionN,d.maxPowerWatts/std::max(magnitude,.1),d.adhesionCoefficient*v.massKg*d.adhesiveMassFraction*Gravity});
            force.traction+=next.command.direction*next.appliedThrottle*available*taper;
        }}
        if(std::abs(speed)>1e-12){force.brake=-Sign(speed)*brakeCapacity;force.resistance=-Sign(speed)*(rolling+aero*speed*speed);}
        else {const double drive=force.traction+force.grade;
            force.brake=-Sign(drive)*std::min(brakeCapacity,std::abs(drive));
            force.resistance=-Sign(drive)*std::min(rolling,std::max(0.0,std::abs(drive)-brakeCapacity));
        }
        return force;
    };
    const double oldSpeed=state_.speedMps;Forces force=evaluate(oldSpeed);
    double acceleration=force.Net()/next.massKg;
    double speed=oldSpeed+acceleration*dt;
    double distance=oldSpeed*dt+.5*acceleration*dt*dt;
    if(oldSpeed*speed<0){
        const double stopTime=-oldSpeed/acceleration,remaining=dt-stopTime;
        const auto restForce=evaluate(0);const double restAcceleration=restForce.Net()/next.massKg;
        distance=oldSpeed*stopTime+.5*acceleration*stopTime*stopTime+.5*restAcceleration*remaining*remaining;
        speed=restAcceleration*remaining;force=force.Blend(restForce,stopTime/dt);
    }
    if(std::abs(speed)<1e-12)speed=0;
    if(!std::isfinite(speed)||!std::isfinite(distance)||!std::isfinite(force.Net()))return Result::Fail("Nonfinite handling integration");
    // S2 rejects invalid extent atomically. Never derive position from a visual Actor.
    auto result=spatial.AdvanceDistance(distance);if(!result.ok)return result;
    result=spatial.SetCommandedSpeed(speed);if(!result.ok)return result;
    next.speedMps=speed;next.accelerationMps2=(speed-oldSpeed)/dt;
    next.tractionN=force.traction;next.brakingN=force.brake;next.resistanceN=force.resistance;
    next.gradeN=force.grade;next.netN=force.Net();next.held=speed==0;
    state_=std::move(next);return {};
}
Result HandlingModel::AdvanceFixedStep(Simulation& spatial,double seconds){
    if(!initialized_||suspended_||!std::isfinite(seconds)||seconds<=0)return Result::Fail("Invalid externally clocked handling step");
    return Step(spatial,seconds);
}
std::string HandlingModel::Json(const Snapshot& s) const {
    std::ostringstream out;out<<std::setprecision(17);
    out<<"{\"tick\":"<<s.tick<<",\"headMetres\":"<<s.headStationMetres<<",\"edge\":"<<s.head.edge.value
       <<",\"speedMps\":"<<state_.speedMps<<",\"accelerationMps2\":"<<state_.accelerationMps2
       <<",\"throttle\":"<<state_.command.throttle<<",\"brake\":"<<state_.command.brake
       <<",\"direction\":"<<state_.command.direction<<",\"emergency\":"<<(state_.command.emergency?"true":"false")
       <<",\"appliedThrottle\":"<<state_.appliedThrottle<<",\"appliedBrake\":"<<state_.appliedBrake
       <<",\"tractionN\":"<<state_.tractionN<<",\"brakingN\":"<<state_.brakingN<<",\"resistanceN\":"<<state_.resistanceN
       <<",\"gradeN\":"<<state_.gradeN<<",\"netN\":"<<state_.netN<<",\"massKg\":"<<state_.massKg
       <<",\"lastSequence\":"<<state_.lastAppliedSequence<<",\"rejected\":"<<state_.rejectedCommands
       <<",\"paused\":"<<((suspended_)?"true":"false")<<"}";return out.str();
}
HandlingDefinitions MakeHandlingDefinitions(){
    HandlingDefinition powered;powered.id=DefinitionId(1);powered.maxTractionN=150000;powered.maxPowerWatts=900000;powered.aeroNPerMpsSquared=25;
    HandlingDefinition coach;coach.id=DefinitionId(2);
    return {{powered.id,powered},{coach.id,coach}};
}
}
