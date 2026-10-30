#include "RailService.h"
#include "RailDispatchDiagnostics.h"
#include "RailScenarioContent.h"
#include "RailSnapshotArchive.h"
#include "RailRouteSampling.h"
#include "RailTambaramDriving.h"
#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>
namespace Rail {
namespace {
std::string Q(const std::string& value){std::string s="\"";for(char c:value){if(c=='\\'||c=='\"')s+='\\';if(c=='\n')s+="\\n";else s+=c;}return s+'\"';}
void TransferService(SnapshotArchive& a,ServiceState& s,ServiceProfile& p){
 a.Fields(s.phase,s.station,s.mode,s.run,s.ticks,s.trafficTicks,s.overspeedTicks,s.harshTicks,s.dwellTicks,s.arrivalTick,s.departTick,s.lastRequestTick,s.cue,
  s.formationConfirmed,s.trafficSeen,s.trafficCleared,s.departureAuthorized,s.success,s.settled,s.requestedPower,s.requestedBrake,s.berthError,s.score,s.playerDelaySeconds,s.accumulator,s.reason,s.grade);
 for(auto& objective:s.objectives)a.Value(objective);
 a.Fields(p.nextRun,p.lastSettledRun,p.awards,p.reputation,p.unlocked);
}
bool Header(SnapshotArchive& a,const ServiceDefinition& d){std::string magic="IR-S7-APPLICATION-1",configuration=d.Json();a.Fields(magic,configuration);return a.Good()&&magic=="IR-S7-APPLICATION-1"&&configuration==d.Json();}
}
const char* ServicePhaseName(ServicePhase p){switch(p){case ServicePhase::Selection:return "Selection";case ServicePhase::Briefing:return "Briefing";case ServicePhase::Running:return "Running";default:return "Result";}}
const char* StationPhaseName(StationPhase p){switch(p){case StationPhase::Approaching:return "Approaching";case StationPhase::Arrival:return "Arrival";case StationPhase::Berthing:return "Berthing";case StationPhase::Dwelling:return "Dwelling";case StationPhase::ReadyToDepart:return "ReadyToDepart";case StationPhase::Departed:return "Departed";default:return "Completed";}}
Result ServiceDefinition::Validate() const {
 if(id.empty()||version.empty()||name.empty()||station.empty()||formation.empty()||unlock.empty()||player!=TrainId(2)||traffic!=TrainId(1)||playerRoute!=RouteId(2)||trafficRoute!=RouteId(1))return Result::Fail("Unsupported service/railway binding");
 for(double value:{startHead,platformFrom,platformTo,berthHead,berthTolerance,finishHead,speedLimit,assistedCruise,arrivalWindowSeconds,departureWindowSeconds,expectedDurationSeconds,completionPoints,safetyPoints,accuracyPoints,speedPoints,punctualityPoints})if(!std::isfinite(value)||value<0)return Result::Fail("Invalid service number");
 const bool egmore=ScenarioContent::IsEgmore(contentId),tambaram=ScenarioContent::IsTambaram(contentId);
 if(terminalStop&&!tambaram)return Result::Fail("Terminal contract requires the versioned Tambaram service");
 if(requireTrafficHold==egmore)return Result::Fail("Traffic objective does not match scenario");
 Fixture fixture;Formation shortTrain,longTrain;auto r=ScenarioContent::Resolve(contentId,fixture,shortTrain,longTrain);if(!r.ok)return r;FormationProperties properties;r=shortTrain.Derive(fixture.definitions,properties);if(!r.ok)return r;
 if(tambaram){TrackPosition Berth;auto Binding=PositionOnRoute(fixture.graph,ScenarioContent::PlayerPath(contentId),berthHead,Berth);
  if(!Binding.ok||platformEdge!=Berth.edge)return Result::Fail("Destination platform edge does not match route berth");
 }else if(platformEdge!=EdgeId(2))return Result::Fail("Unsupported legacy platform binding");
 const double PlatformLimit=tambaram?RouteLength(fixture.graph,ScenarioContent::PlayerPath(contentId)):(egmore?1200:160),FinishLimit=tambaram?PlatformLimit-10:(egmore?1190:165);
 if(startHead!=ScenarioContent::Start(contentId,2)||platformFrom<0||platformTo>PlatformLimit||platformTo-platformFrom<properties.lengthMetres||berthHead-properties.lengthMetres<platformFrom||berthHead>platformTo||berthTolerance<=0||berthTolerance>5||(!terminalStop&&(finishHead<=berthHead+2*berthTolerance||finishHead+berthTolerance>=FinishLimit))||(terminalStop&&finishHead!=berthHead)||assistedCruise<=0||assistedCruise>speedLimit||speedLimit>(tambaram?100.0/3.6:5)||dwellTicks<30||dwellTicks>1800||routeRetryTicks<3||routeRetryTicks>300||!reputationReward||reputationReward>100||departureWindowSeconds<=arrivalWindowSeconds)return Result::Fail("Incompatible platform, timing or formation");
 if(std::abs(completionPoints+safetyPoints+accuracyPoints+speedPoints+punctualityPoints-100)>1e-9)return Result::Fail("Score weights must total 100");
 std::set<ObjectiveKind> seen;for(const auto& o:objectives)if(o.kind<ObjectiveKind::Departure||o.kind>=ObjectiveKind::Count||o.title.empty()||!seen.insert(o.kind).second)return Result::Fail("Invalid objective configuration");
 for(const auto& step:tutorial)if(step.empty())return Result::Fail("Missing tutorial step");return {};
}
std::string ServiceDefinition::Json() const {
 std::ostringstream o;o<<std::setprecision(17)<<"{\"id\":"<<Q(id)<<",\"name\":"<<Q(name)<<",\"version\":"<<Q(version)<<",\"formation\":"<<Q(formation)<<",\"station\":"<<Q(station)<<",\"player\":"<<player.value<<",\"traffic\":"<<traffic.value<<",\"playerRoute\":"<<playerRoute.value<<",\"trafficRoute\":"<<trafficRoute.value<<",\"platformEdge\":"<<platformEdge.value<<",\"startHead\":"<<startHead<<",\"platformFrom\":"<<platformFrom<<",\"platformTo\":"<<platformTo<<",\"berthHead\":"<<berthHead<<",\"berthTolerance\":"<<berthTolerance<<",\"finishHead\":"<<finishHead<<",\"speedLimit\":"<<speedLimit<<",\"assistedCruise\":"<<assistedCruise<<",\"arrivalWindowSeconds\":"<<arrivalWindowSeconds<<",\"departureWindowSeconds\":"<<departureWindowSeconds<<",\"expectedDurationSeconds\":"<<expectedDurationSeconds<<",\"dwellTicks\":"<<dwellTicks<<",\"routeRetryTicks\":"<<routeRetryTicks<<",\"reputationReward\":"<<reputationReward<<",\"unlock\":"<<Q(unlock)<<",\"scoreWeights\":["<<completionPoints<<','<<safetyPoints<<','<<accuracyPoints<<','<<speedPoints<<','<<punctualityPoints<<"],\"objectives\":[";
 bool first=true;for(const auto& objective:objectives){if(!first)o<<',';first=false;o<<"{\"kind\":"<<int(objective.kind)<<",\"title\":"<<Q(objective.title)<<'}';}o<<"],\"tutorial\":[";first=true;for(const auto& t:tutorial){if(!first)o<<',';first=false;o<<Q(t);}o<<"]";if(!requireTrafficHold)o<<",\"requireTrafficHold\":false";if(terminalStop)o<<",\"terminalStop\":true";o<<"}";return o.str();
}
Result ServiceRuntime::Select(){auto r=definition_.Validate();if(!r.ok)return r;if(state_.phase!=ServicePhase::Selection)return Result::Fail("Choose Replay before selecting another run");state_.phase=ServicePhase::Briefing;return {};}
Result ServiceRuntime::ConfirmFormation(){if(state_.phase!=ServicePhase::Briefing)return Result::Fail("Select a service first");state_.formationConfirmed=true;return {};}
Result ServiceRuntime::SetMode(DrivingMode mode){if(state_.phase!=ServicePhase::Briefing||(mode!=DrivingMode::Assisted&&mode!=DrivingMode::Standard))return Result::Fail("Choose Assisted or Standard before departure");state_.mode=mode;return {};}
Result ServiceRuntime::Start(OperationsWorld& world){
 auto r=definition_.Validate();if(!r.ok)return r;if(state_.phase!=ServicePhase::Briefing||!state_.formationConfirmed||profile_.nextRun>=1000000)return Result::Fail("Confirm formation before starting");
 OperationsWorld candidate;r=candidate.InitializeContent(definition_.contentId);if(r.ok)r=candidate.EnableAI(definition_.traffic,definition_.trafficRoute);
 if(ScenarioContent::IsTambaram(definition_.contentId))for(uint64_t Id=3;Id<=ScenarioContent::TrainCount(definition_.contentId)&&r.ok;++Id)r=candidate.EnableAI(TrainId(Id),RouteId(Id+1));if(!r.ok)return r;
 const auto mode=state_.mode;state_=ServiceState{};state_.mode=mode;state_.formationConfirmed=true;state_.phase=ServicePhase::Running;state_.run=profile_.nextRun++;state_.reason=definition_.requireTrafficHold?"Express has priority at the junction; approach the restriction and wait":"Obtain your route, serve "+definition_.station+" and observe adjacent protected traffic";world=std::move(candidate);return {};
}
Result ServiceRuntime::Input(OperationsWorld& world,CommandKind kind,double value){
 if(state_.phase!=ServicePhase::Running)return Result::Fail("Start or resume a running service before driving");
 auto r=world.Command(definition_.player,kind,value);if(!r.ok)return r;
 if(kind==CommandKind::Throttle)state_.requestedPower=value;else if(kind==CommandKind::Brake)state_.requestedBrake=value;return {};
}
Result ServiceRuntime::Depart(const OperationsWorld& world){
 const auto& authority=world.Current().authorities.at(definition_.player);
 if(state_.phase!=ServicePhase::Running||state_.station!=StationPhase::ReadyToDepart||!authority.valid||!authority.routeGranted)return Result::Fail("Complete dwell and obtain authority before departure");
 if(ScenarioContent::IsS8Route(definition_.contentId)){
  const auto* Train=world.Spatial(definition_.player);const auto* Handling=world.Handling(definition_.player);
  if(!Train||!Handling||std::abs(Handling->speedMps)>.05||std::abs(Train->Current().headStationMetres-definition_.berthHead)>definition_.berthTolerance||Train->Current().headStationMetres-Train->Current().properties.lengthMetres<definition_.platformFrom||Train->Current().headStationMetres>definition_.platformTo)return Result::Fail("Remain stopped and fully berthed before departure permission");
 }
 state_.departureAuthorized=true;state_.reason="Departure permitted. Stop at the finish marker";++state_.cue;return {};
}
std::string ServiceRuntime::HoldReason(const OperationsWorld& world) const {
 const auto& route=world.Current().routes.at(definition_.trafficRoute);const auto& authority=world.Current().authorities.at(definition_.player);
 if(definition_.requireTrafficHold&&!authority.routeGranted&&route.owner.Active()&&route.owner.train==definition_.traffic)return "Express owns the shared junction. Wait for its complete rear to clear";
 if(state_.station==StationPhase::Dwelling)return "Station dwell: remain stopped";
 if(state_.station==StationPhase::ReadyToDepart&&!state_.departureAuthorized)return "Dwell complete. Press Depart, then release brake and apply power";
 if(!authority.routeGranted){
    for(const auto& hold:InspectDispatch(world.DefinitionsView(),world.Current()))if(hold.train==definition_.player){
     std::string text="Route hold: "+hold.reason;
     if(!hold.blockers.empty()){const auto& b=hold.blockers.front();text+=" ("+b.resource+", train "+std::to_string(b.train.value)+")";}
     return text+". Remain stopped until permission clears";
    }
    return "Waiting for protected route permission. Remain stopped";
   }
 return "Route available. Observe speed and stopping targets";
}
void ServiceRuntime::Assistance(OperationsWorld& world){
 if(state_.mode!=DrivingMode::Assisted)return;
 const auto* h=world.Handling(definition_.player);const double head=world.Spatial(definition_.player)->Current().headStationMetres;
 const auto& authority=world.Current().authorities.at(definition_.player);
 double target=state_.departureAuthorized?definition_.finishHead:definition_.berthHead;
 if(!authority.routeGranted)target=std::min(target,authority.maximumHead-10);
 const double remaining=target-head;
 double power=0,brake=0;
 const bool dwell=(state_.station==StationPhase::Dwelling||state_.station==StationPhase::ReadyToDepart)&&!state_.departureAuthorized;
 if(dwell||state_.requestedBrake>0||state_.requestedPower<=0||remaining<=.35){brake=std::max(state_.requestedBrake,(dwell||remaining<=.35)?1.0:0.0);}
 else {
  const bool Tambaram=ScenarioContent::IsTambaram(definition_.contentId);
  const double Profile=Tambaram?TambaramDriving::PermittedSpeed(head,world.Spatial(definition_.player)->Current().properties.lengthMetres,definition_.speedLimit):definition_.speedLimit;
  const double StopSpeed=Tambaram?std::max(.08,-1.8+std::sqrt(3.24+std::max(0.0,remaining-.5)*.6)):std::max(.25,std::sqrt(std::max(0.0,remaining-.4)*.45));
  const double targetSpeed=std::min({definition_.assistedCruise,std::max(.08,Profile-(Tambaram?1.0:0.0)),StopSpeed});
  if(Tambaram){
   // Coast before a reduction; retained brake effort continues while air releases.
   const double Error=h->speedMps-targetSpeed;
   if(Error>.04)brake=std::clamp(Error*.8+.15,.15,.65);
   else if(Error<-.12)power=std::min(state_.requestedPower,remaining<10?.06:remaining<60?.15:.55);
  }else{
   if(h->speedMps>targetSpeed+.08)brake=.65;
   else if(h->speedMps<targetSpeed-.08)power=std::min(state_.requestedPower,remaining<3?.18:.65);
  }
 }
 if(h->command.throttle!=power)world.Command(definition_.player,CommandKind::Throttle,power);
 if(h->command.brake!=brake)world.Command(definition_.player,CommandKind::Brake,brake);
}
std::array<double,5> ServiceRuntime::ScoreComponents() const {
 if(!state_.success)return {};
 return {definition_.completionPoints,definition_.safetyPoints,
  definition_.accuracyPoints*std::max(0.0,1-std::abs(state_.berthError)/definition_.berthTolerance),
  definition_.speedPoints*std::max(0.0,1-double(state_.overspeedTicks)/300.0),
  definition_.punctualityPoints*std::max(0.0,1-state_.playerDelaySeconds/60.0)};
}
void ServiceRuntime::ResultFor(bool success,const std::string& reason){
 state_.phase=ServicePhase::Result;state_.success=success;state_.reason=reason;state_.accumulator=0;++state_.cue;
 if(success){
  state_.objectives[1]=1;state_.objectives[2]=state_.overspeedTicks?2:1;state_.objectives[7]=1;
  state_.playerDelaySeconds=std::max(0.0,(double(state_.departTick)-double(state_.trafficTicks))/30.0-definition_.departureWindowSeconds);
  state_.score=0;for(double component:ScoreComponents())state_.score+=component;
  state_.grade=state_.score>=90?"A":state_.score>=75?"B":"C";
 }else{state_.score=0;state_.grade="Retry";}
}
void ServiceRuntime::Fail(const std::string& reason){state_.objectives[7]=2;ResultFor(false,reason);}
void ServiceRuntime::Evaluate(const OperationsWorld& world){
 const auto& player=world.Spatial(definition_.player)->Current();const auto* h=world.Handling(definition_.player);const double head=player.headStationMetres;
 const auto& authority=world.Current().authorities.at(definition_.player);const auto& traffic=world.Current().routes.at(definition_.trafficRoute);
 // This authored passenger itinerary permits forward departures only. The
 // railway still retains valid physical state; the application ends the run.
 if(h->speedMps<-.2){state_.objectives[1]=2;Fail("Safety stop: reverse movement is outside this forward-only service");return;}
 if(!authority.routeGranted&&traffic.owner.Active()&&traffic.owner.train==definition_.traffic){state_.trafficSeen=true;++state_.trafficTicks;}
 if(state_.trafficSeen&&authority.routeGranted){state_.trafficCleared=true;state_.objectives[8]=1;}
 if(!definition_.requireTrafficHold&&traffic.owner.Active()&&traffic.owner.train==definition_.traffic&&world.AI(definition_.traffic)){state_.trafficSeen=state_.trafficCleared=true;state_.objectives[8]=1;}
 const double Limit=ScenarioContent::IsTambaram(definition_.contentId)?TambaramDriving::ApplicableLimit(head,player.properties.lengthMetres,definition_.speedLimit):definition_.speedLimit;
 if(std::abs(h->speedMps)>Limit+.1)++state_.overspeedTicks;
 if(std::abs(h->accelerationMps2)>.9)++state_.harshTicks;
 if(head>definition_.startHead+3)state_.objectives[0]=1;
 if(head>definition_.berthHead+definition_.berthTolerance&&!state_.departureAuthorized){Fail("Station overshoot or departure before permission. Retry the service; existing reputation is retained");return;}
 const bool berth=std::abs(head-definition_.berthHead)<=definition_.berthTolerance&&head-player.properties.lengthMetres>=definition_.platformFrom&&head<=definition_.platformTo&&std::abs(h->speedMps)<=.05;
 switch(state_.station){
 case StationPhase::Approaching:if(authority.routeGranted&&head>=definition_.berthHead-18){state_.station=StationPhase::Arrival;state_.arrivalTick=state_.ticks;++state_.cue;}break;
 case StationPhase::Arrival:if(head>=definition_.berthHead-6)state_.station=StationPhase::Berthing;break;
 case StationPhase::Berthing:if(berth){state_.station=StationPhase::Dwelling;state_.berthError=head-definition_.berthHead;state_.objectives[3]=state_.objectives[4]=1;state_.dwellTicks=0;++state_.cue;}break;
 case StationPhase::Dwelling:
  if(!berth){state_.dwellTicks=0;state_.station=StationPhase::Berthing;}
  else if(++state_.dwellTicks>=definition_.dwellTicks){state_.objectives[5]=1;++state_.cue;if(definition_.terminalStop){state_.station=StationPhase::Completed;state_.departTick=state_.ticks;state_.objectives[6]=1;ResultFor(true,"Terminal service completed at "+definition_.station);}else state_.station=StationPhase::ReadyToDepart;}break;
 case StationPhase::ReadyToDepart:if(state_.departureAuthorized&&head>definition_.berthHead+3){state_.station=StationPhase::Departed;state_.departTick=state_.ticks;state_.objectives[6]=1;}break;
 case StationPhase::Departed:if(std::abs(head-definition_.finishHead)<=definition_.berthTolerance&&std::abs(h->speedMps)<=.05){state_.station=StationPhase::Completed;ResultFor(true,"Service completed. Traffic hold time is excluded from player lateness");}break;
 default:break;
 }
}
Result ServiceRuntime::Fixed(OperationsWorld& world){
 if(world.Current().tick>=state_.lastRequestTick+definition_.routeRetryTicks&&!world.Current().authorities.at(definition_.player).routeGranted){
  const bool Onward=ScenarioContent::IsTambaram(definition_.contentId)&&world.Spatial(definition_.player)->Current().head.edge==TambaramRoute::EdgeFor(ScenarioContent::IsLiveJourney(definition_.contentId)?0:1,14);
  auto r=world.RequestRoute(definition_.player,Onward?RouteId(3):definition_.playerRoute);if(!r.ok)return r;state_.lastRequestTick=world.Current().tick;}
 Assistance(world);
 auto r=world.Update(1.0/30.0);if(!r.ok)return r;++state_.ticks;Evaluate(world);return {};
}
Result ServiceRuntime::Update(OperationsWorld& world,double seconds){
 if(state_.phase!=ServicePhase::Running)return {};
 if(!std::isfinite(seconds)||seconds<0)return Result::Fail("Invalid service clock delta");
 state_.accumulator+=seconds;
 if(state_.accumulator>4.0/30.0+1e-9)return Result::Fail("Service fixed-step overload: pause and retain time debt");
 for(unsigned steps=0;state_.phase==ServicePhase::Running&&state_.accumulator+1e-12>=1.0/30.0&&steps<4;++steps){
  auto candidate=*this;auto railway=world;candidate.state_.accumulator=std::max(0.0,candidate.state_.accumulator-1.0/30.0);
  auto result=candidate.Fixed(railway);
  if(!result.ok){state_.objectives[1]=2;Fail("Safety stop: "+result.error);return {};}
  *this=std::move(candidate);world=std::move(railway);
 }return {};
}
Result ServiceRuntime::Replay(){if(state_.phase!=ServicePhase::Result||(state_.success&&!state_.settled))return Result::Fail("Settle the completed result before replay");state_=ServiceState{};return {};}
Result ServiceRuntime::Validate(const OperationsWorld& world) const {
 if(world.ContentId()!=definition_.contentId)return Result::Fail("Service and world content identity mismatch");
 auto r=definition_.Validate();if(!r.ok)return r;r=world.ValidateInvariants();if(!r.ok)return r;
 if(state_.phase<ServicePhase::Selection||state_.phase>ServicePhase::Result||state_.station<StationPhase::Approaching||state_.station>StationPhase::Completed||state_.mode<DrivingMode::Assisted||state_.mode>DrivingMode::Standard||!profile_.nextRun||profile_.nextRun>1000000||state_.run>=profile_.nextRun||profile_.lastSettledRun>=profile_.nextRun||profile_.awards>profile_.lastSettledRun||profile_.reputation!=profile_.awards*definition_.reputationReward||profile_.unlocked!=(profile_.awards>0))return Result::Fail("Invalid service/profile identity or progression");
 if(state_.ticks>world.Current().tick||state_.trafficTicks>state_.ticks||state_.overspeedTicks>state_.ticks||state_.harshTicks>state_.ticks||state_.dwellTicks>definition_.dwellTicks||state_.arrivalTick>state_.ticks||state_.departTick>state_.ticks||state_.lastRequestTick>world.Current().tick||state_.requestedPower<(ScenarioContent::IsTambaram(definition_.contentId)?-1:0)||state_.requestedPower>1||state_.requestedBrake<0||state_.requestedBrake>1||state_.accumulator<0||state_.accumulator>4.0/30.0+1e-9||state_.score<0||state_.score>100||state_.playerDelaySeconds<0)return Result::Fail("Invalid service clock/control/result values");
 for(auto objective:state_.objectives)if(objective>2)return Result::Fail("Invalid objective status");
 if(state_.phase==ServicePhase::Running&&(!state_.run||!state_.formationConfirmed||!world.AI(definition_.traffic)))return Result::Fail("Running service has no valid driver/formation");
 if(state_.settled&&(!state_.success||state_.phase!=ServicePhase::Result||profile_.lastSettledRun!=state_.run))return Result::Fail("Settlement/profile mismatch");
 if(definition_.terminalStop&&(state_.departureAuthorized||state_.station==StationPhase::ReadyToDepart||state_.station==StationPhase::Departed))return Result::Fail("Terminal service cannot authorize departure");
 if(state_.success&&(state_.phase!=ServicePhase::Result||state_.station!=StationPhase::Completed||(!definition_.terminalStop&&!state_.departureAuthorized)||!state_.trafficCleared||state_.dwellTicks!=definition_.dwellTicks||state_.objectives[7]!=1))return Result::Fail("Incomplete service cannot be rewarded");
 if(state_.success){
  const auto* train=world.Spatial(definition_.player);const auto* handling=world.Handling(definition_.player);
  if(std::abs(train->Current().headStationMetres-definition_.finishHead)>definition_.berthTolerance||std::abs(handling->speedMps)>.05||!state_.trafficSeen||!state_.arrivalTick||state_.departTick<state_.arrivalTick+definition_.dwellTicks)return Result::Fail("Result contradicts railway/station history");
  if(definition_.terminalStop&&(train->Current().headStationMetres-train->Current().properties.lengthMetres<definition_.platformFrom||train->Current().headStationMetres>definition_.platformTo))return Result::Fail("Terminal result must retain the complete formation inside the platform");
  auto derived=*this;derived.ResultFor(true,state_.reason);if(derived.state_.score!=state_.score||derived.state_.grade!=state_.grade||derived.state_.playerDelaySeconds!=state_.playerDelaySeconds)return Result::Fail("Result scoring differs from logical evidence");
 }
 return {};
}
Result ServiceRuntime::Capture(const OperationsWorld& world,uint64_t generation,SnapshotRecord& out) const {
 auto r=Validate(world);if(!r.ok)return r;SnapshotRecord railway;r=WorldSnapshotCodec::Capture(world,1,railway);if(!r.ok)return r;
 std::vector<uint8_t> payload;SnapshotArchive archive(payload,false);Header(archive,definition_);auto bytes=railway.Bytes();archive.Blob(bytes);auto state=state_;auto profile=profile_;TransferService(archive,state,profile);r=archive.Finish();return r.ok?SnapshotRecord::Create(generation,payload,definition_.contentId,out):r;
}
bool ServiceRuntime::Compatible(const SnapshotRecord& record,const ServiceDefinition& definition){if(record.ContentId()!=definition.contentId)return false;auto payload=record.Payload();SnapshotArchive a(payload,true);return Header(a,definition);}
Result ServiceRuntime::Restore(const SnapshotRecord& record,const ServiceDefinition& definition,OperationsWorld& world,ServiceRuntime& out){
 SnapshotRecord checked;auto r=SnapshotRecord::Read(record.Bytes(),checked);if(!r.ok)return r;
 if(checked.ContentId()!=definition.contentId)return Result::Fail("Unsupported S7 content identity retained");
 auto payload=checked.Payload();SnapshotArchive a(payload,true);if(!Header(a,definition))return Result::Fail("Unsupported S7 service configuration/version retained");
 std::vector<uint8_t> bytes;a.Blob(bytes);ServiceRuntime candidate(definition);TransferService(a,candidate.state_,candidate.profile_);r=a.Finish();if(!r.ok)return r;
 SnapshotRecord railway;r=SnapshotRecord::Read(bytes,railway);OperationsWorld restored;if(r.ok)r=WorldSnapshotCodec::Restore(railway,restored);if(r.ok)r=candidate.Validate(restored);if(!r.ok)return r;
 world=std::move(restored);out=std::move(candidate);return {};
}
Result ServiceRuntime::Save(const OperationsWorld& world,const CheckpointIO& io,CheckpointCommit& committed) const {
 return CommitSnapshot(io,[&](uint64_t generation,SnapshotRecord& record){return Capture(world,generation,record);},[&](const SnapshotRecord& record){OperationsWorld railway;ServiceRuntime service;return Restore(record,definition_,railway,service);},committed,[&](const SnapshotRecord& record){return Compatible(record,definition_);});
}
Result ServiceRuntime::Recover(const CheckpointIO& io,OperationsWorld& world,RecoverySelection& selected){
 if(!io.read)return Result::Fail("Missing checkpoint reader");
 auto r=SelectSnapshot({io.read(0),io.read(1)},[&](const SnapshotRecord& record){OperationsWorld railway;ServiceRuntime service;return Restore(record,definition_,railway,service);},selected);
 return r.ok?Restore(selected.record,definition_,world,*this):r;
}
Result ServiceRuntime::Settle(const OperationsWorld& world,const CheckpointIO& io,CheckpointCommit& committed){
 if(state_.phase!=ServicePhase::Result||!state_.success)return Result::Fail("Only a successful service can settle");
 if(state_.settled)return {};
 if(profile_.lastSettledRun>=state_.run)return Result::Fail("Stale settlement ID");
 auto candidate=*this;candidate.profile_.lastSettledRun=state_.run;++candidate.profile_.awards;candidate.profile_.reputation+=definition_.reputationReward;candidate.profile_.unlocked=true;candidate.state_.settled=true;
 auto r=candidate.Save(world,io,committed);if(r.ok)*this=std::move(candidate);return r;
}
std::string ServiceRuntime::Json() const {
 std::ostringstream o;o<<std::setprecision(17)<<"{\"serviceId\":"<<Q(definition_.id)<<",\"run\":"<<state_.run<<",\"phase\":"<<Q(ServicePhaseName(state_.phase))<<",\"station\":"<<Q(StationPhaseName(state_.station))<<",\"mode\":"<<int(state_.mode)<<",\"ticks\":"<<state_.ticks<<",\"trafficTicks\":"<<state_.trafficTicks<<",\"trafficSeen\":"<<(state_.trafficSeen?"true":"false")<<",\"trafficCleared\":"<<(state_.trafficCleared?"true":"false")<<",\"dwellTicks\":"<<state_.dwellTicks<<",\"arrivalTick\":"<<state_.arrivalTick<<",\"departureTick\":"<<state_.departTick<<",\"departureAuthorized\":"<<(state_.departureAuthorized?"true":"false")<<",\"berthError\":"<<state_.berthError<<",\"overspeedTicks\":"<<state_.overspeedTicks<<",\"harshTicks\":"<<state_.harshTicks<<",\"playerDelaySeconds\":"<<state_.playerDelaySeconds<<",\"score\":"<<state_.score<<",\"grade\":"<<Q(state_.grade)<<",\"success\":"<<(state_.success?"true":"false")<<",\"settled\":"<<(state_.settled?"true":"false")<<",\"settlementId\":"<<Q(definition_.id+"/"+std::to_string(state_.run))<<",\"reason\":"<<Q(state_.reason)<<",\"objectives\":[";
 for(size_t i=0;i<state_.objectives.size();++i){if(i)o<<',';o<<state_.objectives[i];}o<<"],\"profile\":{\"nextRun\":"<<profile_.nextRun<<",\"lastSettledRun\":"<<profile_.lastSettledRun<<",\"awards\":"<<profile_.awards<<",\"reputation\":"<<profile_.reputation<<",\"unlocked\":"<<(profile_.unlocked?"true":"false")<<"}}";return o.str();
}
}
