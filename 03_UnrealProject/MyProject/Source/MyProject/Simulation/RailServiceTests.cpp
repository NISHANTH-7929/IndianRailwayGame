#include "RailService.h"
#include <cmath>
#include <limits>
namespace Rail {
TestReport RunServiceTests(){
 TestReport report;auto add=[&](const char* name,bool passed,const std::string& detail=""){report.cases.push_back({name,passed,0,0,detail});};
 ServiceDefinition definition;add("S7-definition",definition.Validate().ok);
 auto bad=definition;bad.platformTo=120;add("S7-platform-too-short",!bad.Validate().ok);
 bad=definition;bad.player=TrainId(1);add("S7-formation-binding-rejected",!bad.Validate().ok);
 bad=definition;bad.berthHead=std::numeric_limits<double>::quiet_NaN();add("S7-nonfinite-definition",!bad.Validate().ok);
 ServiceRuntime service;OperationsWorld world;world.InitializeFixture();
 add("S7-selection",service.Select().ok&&service.Current().phase==ServicePhase::Briefing);
 add("S7-confirm-required",!service.Start(world).ok);
 add("S7-mode-boundary",!service.SetMode(DrivingMode::ExpertReserved).ok&&service.SetMode(DrivingMode::Standard).ok&&service.SetMode(DrivingMode::Assisted).ok);
 add("S7-confirm-formation",service.ConfirmFormation().ok);
 add("S7-start",service.Start(world).ok&&world.ValidateInvariants().ok&&world.Spatial(TrainId(2))->Current().vehicles.size()==2);
 add("S7-existing-AI",world.AI(TrainId(1))&&world.AI(TrainId(1))->Route()==RouteId(1));
 add("S7-input",service.Input(world,CommandKind::Brake,0).ok&&service.Input(world,CommandKind::Throttle,.5).ok);
 add("S7-early-depart-denied",!service.Depart(world).ok);
 std::vector<uint8_t> slots[2];bool interrupt=false;unsigned writes=0;
 CheckpointIO io{[&](unsigned i){return slots[i];},[&](unsigned i,const std::vector<uint8_t>& bytes){++writes;slots[i]=bytes;if(interrupt){slots[i].resize(bytes.size()/2);return Result::Fail("Injected interrupted settlement");}return Result{};}};
 CheckpointCommit commit;add("S7-start-checkpoint",service.Save(world,io,commit).ok);
 bool okay=true,hold=false,restrictive=false,cleared=false;std::set<StationPhase> phases;
 ServiceRuntime moving;OperationsWorld movingWorld;bool captured=false;
 for(unsigned i=0;i<14000&&okay&&service.Current().phase==ServicePhase::Running;++i){
  okay=service.Update(world,1.0/30.0).ok;phases.insert(service.Current().station);
  const auto& ops=world.Current();
  if(service.Current().trafficSeen&&!service.Current().trafficCleared){hold=true;restrictive|=!ops.authorities.at(TrainId(2)).routeGranted&&ops.signals.at(SignalId(2)).aspect==SignalAspect::Stop&&ops.routes.at(RouteId(1)).owner.train==TrainId(1);}
  if(service.Current().trafficCleared)cleared=true;
  if(i==2000){moving=service;movingWorld=world;captured=true;}
  if(service.Current().station==StationPhase::ReadyToDepart&&!service.Current().departureAuthorized)okay=okay&&service.Depart(world).ok;
 }
 add("S7-real-traffic-hold",hold&&restrictive&&service.Current().trafficTicks>0);
 add("S7-rear-clear-allows-player",cleared&&world.AI(TrainId(1))->Phase()==AIPhase::Completed);
 add("S7-complete-itinerary",okay&&service.Current().success,service.Json());
 add("S7-station-state-contract",phases.size()==7);
 add("S7-berth",std::abs(service.Current().berthError)<=definition.berthTolerance&&service.Current().objectives[4]==1);
 add("S7-dwell",service.Current().dwellTicks==definition.dwellTicks&&service.Current().objectives[5]==1);
 add("S7-departure",service.Current().departureAuthorized&&service.Current().objectives[6]==1);
 add("S7-objective-events",service.Current().objectives[0]==1&&service.Current().objectives[7]==1&&service.Current().objectives[8]==1);
 add("S7-explained-result",service.Current().score>0&&service.Current().grade!="Not graded"&&service.Current().reason.find("Traffic")!=std::string::npos);
 double points=0;for(double component:service.ScoreComponents())points+=component;
 add("S7-score-components-reconcile",points==service.Current().score&&service.ScoreComponents()[3]==definition.speedPoints);
 add("S7-delay-attribution",service.Current().trafficTicks>0&&service.Current().playerDelaySeconds==std::max(0.0,(double(service.Current().departTick)-double(service.Current().trafficTicks))/30.0-definition.departureWindowSeconds));
 SnapshotRecord snapshot;OperationsWorld restored;ServiceRuntime recovery;
 auto result=moving.Capture(movingWorld,20,snapshot);if(result.ok)result=ServiceRuntime::Restore(snapshot,definition,restored,recovery);
 add("S7-cold-restore-full-state",captured&&result.ok&&recovery.Json()==moving.Json()&&restored.Json()==movingWorld.Json(),result.error);
 bool same=result.ok;for(unsigned i=0;i<300&&same;++i){same=moving.Update(movingWorld,1.0/30).ok&&recovery.Update(restored,1.0/30).ok&&moving.Json()==recovery.Json()&&movingWorld.Json()==restored.Json();}
 add("S7-restored-continuation",same);
 auto thirty=moving,sixty=moving;auto w30=movingWorld,w60=movingWorld;
 for(unsigned i=0;i<120;++i){thirty.Update(w30,1.0/30);sixty.Update(w60,1.0/60);sixty.Update(w60,1.0/60);}
 add("S7-render-rate-independence",thirty.Json()==sixty.Json()&&w30.Json()==w60.Json());
 const auto frozen=moving.Json();moving.Update(movingWorld,0);add("S7-zero-time-freezes",moving.Json()==frozen);
 auto future=definition;future.version="future";add("S7-configuration-mismatch-retained",!ServiceRuntime::Restore(snapshot,future,restored,recovery).ok&&!ServiceRuntime::Compatible(snapshot,future));
 add("S7-result-checkpoint",service.Save(world,io,commit).ok);
 interrupt=true;const auto profileBefore=service.Profile().reputation;add("S7-interrupted-settlement-not-published",!service.Settle(world,io,commit).ok&&service.Profile().reputation==profileBefore&&!service.Current().settled);
 RecoverySelection selected;ServiceRuntime resumed;OperationsWorld resumedWorld;result=resumed.Recover(io,resumedWorld,selected);
 add("S7-interrupted-settlement-recover",result.ok&&resumed.Current().success&&!resumed.Current().settled,result.error);
 interrupt=false;add("S7-settlement",resumed.Settle(resumedWorld,io,commit).ok&&resumed.Profile().reputation==definition.reputationReward&&resumed.Profile().unlocked);
 const auto writeCount=writes;add("S7-duplicate-settlement",resumed.Settle(resumedWorld,io,commit).ok&&writes==writeCount&&resumed.Profile().reputation==definition.reputationReward);
 ServiceRuntime paid;OperationsWorld paidWorld;result=paid.Recover(io,paidWorld,selected);add("S7-paid-cold-restore",result.ok&&paid.Current().settled&&paid.Profile().reputation==definition.reputationReward);
 add("S7-replay-preserves-profile",paid.Replay().ok&&paid.Select().ok&&paid.ConfirmFormation().ok&&paid.Start(paidWorld).ok&&paid.Current().run==2&&paid.Profile().reputation==definition.reputationReward);
 auto forged=service;forged.profile_.reputation=999;add("S7-invalid-progression-rejected",!forged.Capture(world,100,snapshot).ok);
 forged=service;forged.state_.score=101;add("S7-invalid-result-rejected",!forged.Capture(world,100,snapshot).ok);
 ServiceRuntime overshoot;OperationsWorld ow;ow.InitializeFixture();overshoot.Select();overshoot.ConfirmFormation();overshoot.SetMode(DrivingMode::Standard);overshoot.Start(ow);overshoot.Input(ow,CommandKind::Brake,0);overshoot.Input(ow,CommandKind::Throttle,1);
 for(unsigned i=0;i<12000&&overshoot.Current().phase==ServicePhase::Running;++i){if(ow.Current().authorities.at(TrainId(2)).routeGranted){overshoot.Input(ow,CommandKind::Brake,0);overshoot.Input(ow,CommandKind::Throttle,1);}overshoot.Update(ow,1.0/30);}
 add("S7-overshoot-failure",overshoot.Current().phase==ServicePhase::Result&&!overshoot.Current().success&&overshoot.Current().reason.find("overshoot")!=std::string::npos);
 add("S7-failure-no-reward",!overshoot.Settle(ow,io,commit).ok&&overshoot.Profile().reputation==0);
 add("S7-retry",overshoot.Replay().ok&&overshoot.Select().ok&&overshoot.ConfirmFormation().ok&&overshoot.Start(ow).ok&&overshoot.Current().run==2);
 ServiceRuntime safety;OperationsWorld sw;sw.InitializeFixture();safety.Select();safety.ConfirmFormation();safety.SetMode(DrivingMode::Standard);safety.Start(sw);safety.Input(sw,CommandKind::Direction,-1);safety.Input(sw,CommandKind::Brake,0);safety.Input(sw,CommandKind::Throttle,1);
 for(unsigned i=0;i<5000&&safety.Current().phase==ServicePhase::Running;++i)safety.Update(sw,1.0/30);
 add("S7-forward-service-safety-failure",safety.Current().phase==ServicePhase::Result&&!safety.Current().success&&safety.Current().reason.find("Safety stop")!=std::string::npos&&sw.ValidateInvariants().ok,safety.Json()+" world="+sw.Error());
 ServiceRuntime standard;OperationsWorld manual;manual.InitializeFixture();standard.Select();standard.ConfirmFormation();standard.SetMode(DrivingMode::Standard);standard.Start(manual);
 bool manualOkay=true;
 for(unsigned i=0;i<16000&&manualOkay&&standard.Current().phase==ServicePhase::Running;++i){
  const auto& authority=manual.Current().authorities.at(TrainId(2));const auto* h=manual.Handling(TrainId(2));
  const double head=manual.Spatial(TrainId(2))->Current().headStationMetres;
  if(standard.Current().station==StationPhase::ReadyToDepart&&!standard.Current().departureAuthorized)manualOkay=standard.Depart(manual).ok;
  const double target=!authority.routeGranted?authority.maximumHead-10:standard.Current().departureAuthorized?definition.finishHead:definition.berthHead;
  const double distance=target-head;const bool dwell=standard.Current().station==StationPhase::Dwelling;
  // Test driver uses ordinary player commands; Standard itself supplies no braking assistance.
  const double desired=std::min(1.0,std::sqrt(std::max(0.0,distance-.6)*.3));
  const double brake=dwell||distance<.6||h->speedMps>desired+.05?1:0;
  const double power=!brake&&h->speedMps<desired-.05?.25:0;
  if(h->command.throttle!=power)manualOkay&=standard.Input(manual,CommandKind::Throttle,power).ok;
  if(h->command.brake!=brake)manualOkay&=standard.Input(manual,CommandKind::Brake,brake).ok;
  manualOkay&=standard.Update(manual,1.0/30).ok;
 }
 add("S7-standard-complete-itinerary",manualOkay&&standard.Current().success&&standard.Current().mode==DrivingMode::Standard,standard.Json());
 bad=definition;bad.completionPoints=150;add("S7-invalid-score-weights",!bad.Validate().ok);
 ServiceRuntime invalidMode;invalidMode.Select();add("S7-invalid-mode",!invalidMode.SetMode(static_cast<DrivingMode>(99)).ok);
 auto overload=moving;auto overloadedWorld=movingWorld;const auto beforeDebt=overloadedWorld.Json();
 add("S7-overload-retains-railway",!overload.Update(overloadedWorld,1).ok&&overloadedWorld.Json()==beforeDebt&&overload.Current().accumulator>=1);
 return report;
}
}
