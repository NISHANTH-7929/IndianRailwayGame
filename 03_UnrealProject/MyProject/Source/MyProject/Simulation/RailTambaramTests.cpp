#include "RailTambaramService.h"
#include "RailTambaramDriving.h"
#include "RailJourneyPresentation.h"
#include "RailEgmoreService.h"
#include "RailContent.h"
#include "RailScenarioContent.h"
#include <cmath>
#include <set>

namespace Rail {
namespace {
struct TambaramHandlingRig {
 Simulation spatial;HandlingModel handling;uint64_t sequence=0;
 bool Initialize(){
  Fixture F;Formation Player,Traffic;auto R=TambaramRoute::ResolveSpatial(F,Player,Traffic);Choices Straight;
  for(size_t I=0;I+1<TambaramRoute::SegmentCount;++I)Straight.emplace(DirectedEdge{TambaramRoute::EdgeFor(1,I),1},TambaramRoute::TransitionFor(1,I,false));
  if(R.ok)R=spatial.Initialize(F.graph,F.definitions,Player,{TambaramRoute::EdgeFor(1,0),1},TambaramRoute::StartHead,Straight);
  if(R.ok)R=handling.Initialize(spatial,F.definitions,Player,TambaramDriving::Definitions());return R.ok;
 }
 bool Command(CommandKind K,double V){return handling.Submit(spatial,{spatial.Current().train,1,++sequence,spatial.Current().tick+1,K,V}).ok;}
 bool Steps(unsigned Count){for(unsigned I=0;I<Count;++I)if(!handling.Update(spatial,1.0/30).ok)return false;return true;}
 bool Stop(unsigned Maximum=2400){for(unsigned I=0;I<Maximum&&std::abs(handling.Current().speedMps)>1e-9;++I)if(!Steps(1))return false;return handling.Current().speedMps==0;}
};
}
TestReport RunTambaramJourneyTests(){
 TestReport Report;
 auto Check=[&](const char* Name,bool Pass,const std::string& Detail=""){Report.cases.push_back({Name,Pass,0,0,Detail});};
 const auto Network=TambaramRoute::MakeGraph();const auto Safety=ScenarioContent::OperationsFor(TambaramRoute::ContentId);
 {
  const auto Start=JourneyPresentation::At(TambaramRoute::StartHead,189.33);
  const auto Alongside=JourneyPresentation::At(TambaramRoute::StationMetres[4]+100,189.33);
  const auto Cleared=JourneyPresentation::At(TambaramRoute::StationMetres[4]+300,189.33);
  const auto End=JourneyPresentation::At(TambaramRoute::TambaramBerthHead,189.33);
  Check("Tambaram.JourneyHud.Origin",Start.station==0&&Start.progress==0&&!Start.terminal);
  Check("Tambaram.JourneyHud.WholeFormationPassing",Alongside.station==4&&Alongside.alongside&&Cleared.station==5);
  Check("Tambaram.JourneyHud.TerminalStop",End.terminal&&End.distance==0&&End.progress==1);
  const double Mambalam=TambaramRoute::StationMetres[4];
  Check("Tambaram.JourneyAudio.IntermediateStationFade",JourneyPresentation::StationAmbience(Mambalam)==1&&JourneyPresentation::StationAmbience(Mambalam+150)==.5&&JourneyPresentation::StationAmbience(Mambalam+300)==0);
  Check("Tambaram.JourneyHud.BoundedOutsideRoute",JourneyPresentation::At(-1000,189.33).progress==0&&JourneyPresentation::At(TambaramRoute::FinishHead+1000,189.33).progress==1);
 }
 Check("Tambaram.Topology.ResearchedRepresentativeCrossoverProtected",Safety.points.size()==2&&Safety.Validate(Network).ok,Safety.Validate(Network).error);
 Check("Tambaram.Recovery.ObsoleteScaffoldNotReinterpreted",!ScenarioContent::Supports("IR-S8-MSTBM-1"));
 Check("Tambaram.Recovery.PreJunctionContentRejected",!ScenarioContent::Supports("IR-S8-MSTBM-3"));
 Check("Tambaram.Recovery.PreDrivingContentRejected",!ScenarioContent::Supports("IR-S8-MSTBM-4"));
 Check("Tambaram.Recovery.PreOpposingRosterRejected",!ScenarioContent::Supports("IR-S8-MSTBM-5"));
 {
  TambaramHandlingRig Seed;bool Good=Seed.Initialize()&&Seed.Command(CommandKind::Throttle,1);
  for(unsigned I=0;I<6000&&Good&&Seed.handling.Current().speedMps<25;++I)Good=Seed.Steps(1);
  const auto AtSpeed=Seed.handling.Current();
  Check("Tambaram.Driving.FormationMass403Tonnes",Good&&AtSpeed.massKg==403000,std::to_string(AtSpeed.massKg));
  Check("Tambaram.Driving.PowerAvailableAt90kmh",Good&&AtSpeed.speedMps>=25&&AtSpeed.speedMps<25.1&&AtSpeed.tractionN>170000&&AtSpeed.tractionN*AtSpeed.speedMps>4400000&&AtSpeed.tractionN*AtSpeed.speedMps<4530000,std::to_string(AtSpeed.tractionN));
  auto Service=Seed,Emergency=Seed,Mild=Seed;
  Good=Good&&Service.Command(CommandKind::Throttle,0)&&Service.Command(CommandKind::Brake,1)&&Emergency.Command(CommandKind::Emergency,1)&&Mild.Command(CommandKind::Throttle,0)&&Mild.Command(CommandKind::Brake,.3);
  Good=Good&&Service.Steps(1)&&Emergency.Steps(1)&&Mild.Steps(1);
  Check("Tambaram.Driving.PneumaticApplicationAndPowerCutout",Good&&Service.handling.Current().appliedBrake>0&&Service.handling.Current().appliedBrake<.02&&Service.handling.Current().tractionN==0&&Emergency.handling.Current().tractionN==0);
  Good=Good&&Service.Steps(119)&&Mild.Steps(119)&&Emergency.Steps(119);
  Check("Tambaram.Driving.GraduatedBrakingMonotonic",Good&&Mild.handling.Current().speedMps>Service.handling.Current().speedMps&&Mild.handling.Current().brakingN>Service.handling.Current().brakingN);
  const double Start=Seed.spatial.Current().headStationMetres;
  Good=Good&&Service.Stop()&&Emergency.Stop();
  const double SD=Service.spatial.Current().headStationMetres-Start,ED=Emergency.spatial.Current().headStationMetres-Start;
  Check("Tambaram.Braking.90kmhServiceStopsBeforeDesignEnvelope",Good&&SD>450&&SD<TambaramDriving::AuthorityStoppingDistance(AtSpeed.speedMps),std::to_string(SD));
  Check("Tambaram.Braking.90kmhEmergencyShorterWithoutBounce",Good&&ED>250&&ED<SD,std::to_string(ED));
  const double Stopped=Service.spatial.Current().headStationMetres;
  Good=Good&&Service.Steps(300)&&Service.Command(CommandKind::Brake,0)&&Service.Command(CommandKind::Throttle,1)&&Service.Steps(270);
  Check("Tambaram.Driving.BrakeReleaseRetainsCutoutNineSeconds",Good&&Service.handling.Current().appliedBrake>.49&&Service.handling.Current().appliedBrake<.51&&Service.handling.Current().tractionN==0&&Service.spatial.Current().headStationMetres==Stopped);
  Good=Good&&Service.Steps(272);
  Check("Tambaram.Driving.PowerReturnsAfterCompleteBrakeRelease",Good&&Service.handling.Current().appliedBrake==0&&Service.handling.Current().tractionN>0&&Service.handling.Current().speedMps>0);
  auto Moving=Seed;const auto Rejections=Moving.handling.Current().rejectedCommands;
  bool Controls=Moving.Command(CommandKind::Direction,-1)&&Moving.Command(CommandKind::Emergency,1)&&Moving.Command(CommandKind::Emergency,0)&&Moving.Steps(1);
  Check("Tambaram.Driving.MovingReverserAndEmergencyResetDenied",Controls&&Moving.handling.Current().command.direction==1&&Moving.handling.Current().command.emergency&&Moving.handling.Current().rejectedCommands>=Rejections+2&&Moving.handling.Current().tractionN==0);
  Controls=Controls&&Moving.Command(CommandKind::Throttle,0)&&Moving.Stop()&&Moving.Command(CommandKind::Emergency,0)&&Moving.Command(CommandKind::Direction,-1)&&Moving.Steps(1);
  Check("Tambaram.Driving.StoppedEmergencyResetAndReverserAllowed",Controls&&!Moving.handling.Current().command.emergency&&Moving.handling.Current().command.direction==-1&&Moving.handling.Current().speedMps==0);
 }
 {
  constexpr double Ceiling=100.0/3.6,Length=200;
  bool Anticipates=false,RearRestricts=false,Bounded=true;
  for(double Head=1201;Head<TambaramRoute::TambaramBerthHead;Head+=20){
   const double Predict=TambaramDriving::PermittedSpeed(Head,Length,Ceiling),Active=TambaramDriving::ApplicableLimit(Head,Length,Ceiling),Local=TambaramDriving::LocalLimit(Head,Ceiling);
   Anticipates|=Predict<Active-.05;RearRestricts|=Active<Local-.05;Bounded&=std::isfinite(Predict)&&Predict>0&&Predict<=Active+1e-9&&Active<=Ceiling;
  }
  Check("Tambaram.SpeedProfile.BrakesBeforeFutureRestriction",Anticipates&&Bounded);
  Check("Tambaram.SpeedProfile.RestrictionUntilWholeRearClears",RearRestricts&&TambaramDriving::ApplicableLimit(1250,Length,Ceiling)<=30.0/3.6);
 }
 {
  const auto A=TambaramRoute::CrossoverGeometry(false),B=TambaramRoute::CrossoverGeometry(true),Target=TambaramRoute::GeometryFor(0,6);
  const auto End=B.At(B.lengthMetres),Expected=Target.At(Target.lengthMetres);
  Check("Tambaram.Junction.TangentContinuousTwoArcFit",(A.At(A.lengthMetres).position-B.origin).Length()<1e-7&&(End.position-Expected.position).Length()<1e-7&&std::abs(B.yawRadians+B.curvaturePerMetre*B.lengthMetres-Target.yawRadians)<1e-9);
 }
 {
  const auto Junction=MakeFixture();auto Protected=MakeOperationsDefinitions();
  Check("Tambaram.Topology.ExistingJunctionStillValid",Protected.Validate(Junction.graph).ok);
  Protected.points.erase(PointId(1));
  for(auto& Route:Protected.routes)Route.second.points.erase(PointId(1));
  Check("Tambaram.Topology.UnguardedBranchRejected",!Protected.Validate(Junction.graph).ok);
  Protected=MakeOperationsDefinitions();Protected.points.at(PointId(1)).positions={{0,{TransitionId(1),TransitionId(2)}}};
  for(auto& Route:Protected.routes)if(Route.second.points.count(PointId(1)))Route.second.points[PointId(1)]=0;
  Check("Tambaram.Topology.NonexclusiveBranchRejected",!Protected.Validate(Junction.graph).ok);
 }
 const auto D=MakeEgmoreTambaramService();OperationsWorld W;ServiceRuntime S(D);
 {
  Fixture F;Formation Player,Traffic;Simulation Spatial;HandlingModel Handling;
  auto B=TambaramRoute::ResolveSpatial(F,Player,Traffic);
  Choices Straight;Straight.emplace(DirectedEdge{TambaramRoute::EdgeFor(1,5),1},TambaramRoute::TransitionFor(1,5,false));
  if(B.ok)B=Spatial.Initialize(F.graph,F.definitions,Player,{TambaramRoute::EdgeFor(1,0),1},TambaramRoute::StartHead,Straight);
  auto Physics=TambaramDriving::Definitions();
  if(B.ok)B=Handling.Initialize(Spatial,F.definitions,Player,Physics);
  uint64_t Sequence=0;
  auto Command=[&](HandlingModel& H,const Simulation& P,CommandKind K,double V){return H.Submit(P,{P.Current().train,1,++Sequence,P.Current().tick+1,K,V});};
  if(B.ok)B=Command(Handling,Spatial,CommandKind::Throttle,1);
  for(unsigned I=0;I<18000&&B.ok&&Handling.Current().speedMps<12;++I)B=Handling.Update(Spatial,1.0/30);
  const double Speed=Handling.Current().speedMps,Start=Spatial.Current().headStationMetres;
  auto EmergencySpatial=Spatial;auto Emergency=Handling;
  if(B.ok)B=Command(Handling,Spatial,CommandKind::Throttle,0);
  if(B.ok)B=Command(Handling,Spatial,CommandKind::Brake,1);
  if(B.ok)B=Command(Emergency,EmergencySpatial,CommandKind::Emergency,1);
  for(unsigned I=0;I<1800&&B.ok;++I){B=Handling.Update(Spatial,1.0/30);if(B.ok)B=Emergency.Update(EmergencySpatial,1.0/30);}
  const double ServiceDistance=Spatial.Current().headStationMetres-Start,EmergencyDistance=EmergencySpatial.Current().headStationMetres-Start;
  Check("Tambaram.Braking.CruiseServiceStopsWithin200m",B.ok&&Speed>=12&&Handling.Current().speedMps==0&&ServiceDistance>0&&ServiceDistance<200,B.error+std::to_string(ServiceDistance));
  Check("Tambaram.Braking.EmergencyShorterThanService",B.ok&&Emergency.Current().speedMps==0&&EmergencyDistance>0&&EmergencyDistance<ServiceDistance,std::to_string(EmergencyDistance));
 }
 {
  TrackPosition Berth;Sample Expected,Actual;
  const auto Path=TambaramRoute::Path(1);
  const bool Bound=PositionOnRoute(Network,Path,D.berthHead,Berth).ok&&Network.SamplePosition(Berth,Expected).ok&&SampleRoute(Network,Path,D.berthHead,Actual).ok;
  Check("Tambaram.Platform.DestinationEdgeBinding",Bound&&D.platformEdge==Berth.edge&&D.platformEdge!=EdgeId(2)&&(Actual.position-Expected.position).Length()<1e-8);
  auto Wrong=D;Wrong.platformEdge=EdgeId(2);Check("Tambaram.Platform.OriginEdgeRejected",!Wrong.Validate().ok);
  TrackPosition Invalid;
  Check("Tambaram.Path.OutOfBoundsRejected",!PositionOnRoute(Network,Path,-1,Invalid).ok&&!PositionOnRoute(Network,Path,RouteLength(Network,Path)+1,Invalid).ok);
  TrackPosition Seam;const double FirstLength=Network.Find(Path.front().edge)->lengthMetres;
  Check("Tambaram.Path.SeamUsesNextOccurrence",PositionOnRoute(Network,Path,FirstLength,Seam).ok&&Seam.edge==Path[1].edge&&Seam.distanceMetres==0);
 }
 auto R=S.Select();if(R.ok)R=S.ConfirmFormation();if(R.ok)R=S.Start(W);
 Check("Tambaram.Journey.Start",R.ok,R.error);if(!R.ok)return Report;
 {
  SnapshotRecord Current,Obsolete;auto Saved=S.Capture(W,1,Current);auto OldBytes=Current.Bytes();
  // Produce a checksum-valid obsolete envelope independently, because the
  // current writer correctly refuses to create unsupported content records.
  const std::string OldId="IR-S8-MSTBM-4";
  if(Saved.ok&&OldBytes.size()>28+OldId.size()+4){
   for(size_t I=0;I<OldId.size();++I)OldBytes[28+I]=uint8_t(OldId[I]);
   uint32_t CRC=0xffffffffu;for(size_t I=0;I<OldBytes.size()-4;++I){CRC^=OldBytes[I];for(unsigned Bit=0;Bit<8;++Bit)CRC=(CRC>>1)^((CRC&1)?0xedb88320u:0u);}CRC=~CRC;
   for(unsigned I=0;I<4;++I)OldBytes[OldBytes.size()-4+I]=uint8_t(CRC>>(8*I));
  }
  const auto Retained=OldBytes;RecoverySelection Selected;
  auto Recovery=SelectSnapshot({OldBytes},[](const SnapshotRecord&){return Result{};},Selected);
  Check("Tambaram.Recovery.Version4RecordNotReinterpreted",Saved.ok&&!Recovery.ok&&Selected.slot==-1&&OldBytes==Retained&&!Selected.diagnostics.empty()&&Selected.diagnostics.front().find("content is unavailable")!=std::string::npos);
 }
 Check("Tambaram.Terminal.DepartBeforeArrivalRejected",!S.Depart(W).ok);
 auto Legacy=MakeEgmoreService();Legacy.terminalStop=true;Legacy.finishHead=Legacy.berthHead;
 Check("Tambaram.Terminal.LegacyOptInRejected",!Legacy.Validate().ok);
 S.Input(W,CommandKind::Brake,0);S.Input(W,CommandKind::Throttle,1);
 std::set<EdgeId> Visited;std::set<StationPhase> Phases;
 bool Authority=true,Spanning=false,BoundaryRecovery=false,PhaseRecovery=true,Monotonic=true;
 bool ConflictDenied=false,AIPointsLocked=false,AICleared=false,PlayerThroatReleased=false,OnwardGranted=false,AIHeadClearRearLocked=false;
 bool PointRequestSent=false,LockedPointDenied=false;EdgeId PreviousAI;bool PreviousAILock=false;
 double Previous=D.startHead,PeakSpeed=0;unsigned Recoveries=0;bool DwellInterrupted=false;
 bool OpposingGranted=false,OpposingSafe=true,OpposingPassed=false,OpposingRecovered=false,EmergencyEncounterSafe=false;
 double ClosestOpposing=1e30,OpposingPeak=0,PreviousOpposing=200;unsigned OpposingRecoveryCount=0;
 for(unsigned Step=0;Step<230000&&R.ok&&S.Current().phase==ServicePhase::Running;++Step){
  R=S.Update(W,1.0/30.0);if(!R.ok)break;
  const auto& T=W.Spatial(D.player)->Current();const auto& A=W.Current().authorities.at(D.player);
  const auto& Ops=W.Current();const auto& AITrain=W.Spatial(D.traffic)->Current();
  const auto& Opposing=W.Spatial(TrainId(3))->Current();const auto& OA=Ops.authorities.at(TrainId(3));
  OpposingGranted|=Ops.routes.at(RouteId(4)).owner.Active();
  OpposingSafe&=OA.valid&&Opposing.head.direction==-1&&Opposing.headStationMetres>=PreviousOpposing-1e-8&&Opposing.headStationMetres<=OA.maximumHead+1e-8&&Opposing.headStationMetres-Opposing.properties.lengthMetres>=OA.minimumRear-1e-8&&W.AI(TrainId(3))->Phase()!=AIPhase::FailSafe;
  PreviousOpposing=Opposing.headStationMetres;OpposingPeak=std::max(OpposingPeak,W.Handling(TrainId(3))->speedMps);
  Sample PlayerHead,OpposingHead;Network.SamplePosition(T.head,PlayerHead);Network.SamplePosition(Opposing.head,OpposingHead);
  ClosestOpposing=std::min(ClosestOpposing,(PlayerHead.position-OpposingHead.position).Length());
  OpposingPassed|=ClosestOpposing<8&&(PlayerHead.position-OpposingHead.position).Length()>600;
  auto PointReply=Ops.lastRequest.find(D.player);if(PointReply!=Ops.lastRequest.end()&&PointReply->second.request.kind==OperationsRequestKind::Point&&!PointReply->second.granted)LockedPointDenied=true;
  if(!PointRequestSent&&Ops.routes.at(RouteId(1)).owner.Active()){auto PointResult=W.RequestPoint(D.player,PointId(1),0);PointRequestSent=PointResult.ok;}
  auto Last=Ops.lastRequest.find(D.player);if(Last!=Ops.lastRequest.end()&&!Last->second.granted&&Ops.routes.at(RouteId(1)).owner.Active())ConflictDenied=true;
  if(Ops.routes.at(RouteId(1)).owner.Active()){
   AIPointsLocked|=Ops.points.at(PointId(1)).lock==Ops.routes.at(RouteId(1)).owner&&Ops.points.at(PointId(2)).lock==Ops.routes.at(RouteId(1)).owner;
   if(AITrain.head.edge==TambaramRoute::EdgeFor(0,10))AIHeadClearRearLocked=true;
  }
  AICleared|=!Ops.routes.at(RouteId(1)).releaseReason.empty();
  PlayerThroatReleased|=!Ops.routes.at(RouteId(2)).releaseReason.empty()&&!Ops.points.at(PointId(1)).lock.Active();
  OnwardGranted|=Ops.routes.at(RouteId(3)).owner.Active();
  PeakSpeed=std::max(PeakSpeed,std::abs(W.Handling(D.player)->speedMps));
  Authority&=A.valid&&T.headStationMetres<=A.maximumHead+1e-8&&T.headStationMetres-T.properties.lengthMetres>=A.minimumRear-1e-8;
  Monotonic&=T.headStationMetres>=Previous-1e-8;Previous=T.headStationMetres;
  const bool NewEdge=Visited.insert(T.head.edge).second,NewPhase=Phases.insert(S.Current().station).second;
  double Min=1e30,Max=-1e30;for(const auto& V:T.vehicles){Min=std::min(Min,std::min(V.frontReference.x,V.rearReference.x));Max=std::max(Max,std::max(V.frontReference.x,V.rearReference.x));}
  const bool AtBoundary=Min<0&&Max>0;
  if(AtBoundary){Spanning=true;const auto Demand=RequiredPresentation(T,W.Handling(D.player)->speedMps,true,{});Authority&=Demand.valid&&Demand.chunks==std::set<unsigned>{1,2};}
  const bool AIChanged=PreviousAI!=AITrain.head.edge||PreviousAILock!=Ops.routes.at(RouteId(1)).owner.Active();PreviousAI=AITrain.head.edge;PreviousAILock=Ops.routes.at(RouteId(1)).owner.Active();
  const bool AtOpposingPass=(PlayerHead.position-OpposingHead.position).Length()<100&&OpposingRecoveryCount==0;
  if(AtOpposingPass){
   auto EmergencyWorld=W;const double AIHead=Opposing.headStationMetres;
   bool Good=EmergencyWorld.Command(D.player,CommandKind::Emergency,1).ok;
   Good=Good&&!EmergencyWorld.Command(TrainId(3),CommandKind::Throttle,1).ok;
   for(unsigned I=0;I<300&&Good;++I)Good=EmergencyWorld.Update(1.0/30).ok;
   EmergencyEncounterSafe=Good&&EmergencyWorld.Handling(D.player)->command.emergency&&EmergencyWorld.Handling(D.player)->tractionN==0&&EmergencyWorld.Spatial(TrainId(3))->Current().headStationMetres>AIHead+50&&EmergencyWorld.ValidateInvariants().ok;
  }
  if(NewEdge||NewPhase||AIChanged||AtOpposingPass||(AtBoundary&&!BoundaryRecovery)){
   SnapshotRecord Record;OperationsWorld Cold;ServiceRuntime ColdService(D);
   auto Saved=S.Capture(W,Recoveries+1,Record);if(Saved.ok)Saved=ServiceRuntime::Restore(Record,D,Cold,ColdService);
   const bool Same=Saved.ok&&Cold.Json()==W.Json()&&ColdService.Json()==S.Json();PhaseRecovery&=Same;
   if(Same){W=std::move(Cold);S=std::move(ColdService);++Recoveries;}
   if(AtBoundary)BoundaryRecovery=Same;
   if(AtOpposingPass){OpposingRecovered=Same;++OpposingRecoveryCount;}
   if(!Same){R=Saved.ok?Result::Fail("Cold state differs"):Saved;break;}
  }
  // This traversal must never issue Depart or move by assignment.
  if(S.Current().station==StationPhase::Dwelling)DwellInterrupted|=S.Current().departureAuthorized;
 }
 Check("Tambaram.Journey.NormalSimulationCompletes",R.ok&&S.Current().success,R.error+S.Json());
 Check("Tambaram.Opposing.AuthorityReverseMotionAndNoFailsafe",OpposingGranted&&OpposingSafe&&OpposingPeak>9.5&&OpposingPeak<40.0/3.6,std::to_string(OpposingPeak));
 Check("Tambaram.Opposing.RealAdjacentTrainPass",OpposingPassed&&ClosestOpposing>=4.9&&ClosestOpposing<8,std::to_string(ClosestOpposing));
 Check("Tambaram.Opposing.ExactColdRecoveryDuringPass",OpposingRecovered);
 Check("Tambaram.Opposing.PlayerEmergencyAndAIControlIsolation",EmergencyEncounterSafe);
 {
  auto TrafficWorld=W;bool Safe=R.ok;TrafficWorld.SetSuspended(true);
  const auto Suspended=TrafficWorld.Json();Safe=Safe&&TrafficWorld.Update(10).ok&&TrafficWorld.Json()==Suspended;
  TrafficWorld.SetSuspended(false);Safe=Safe&&TrafficWorld.Update(10).ok;
  for(unsigned I=0;I<60000&&Safe;++I){
   const auto Phase=TrafficWorld.AI(TrainId(3))->Phase();
   if((Phase==AIPhase::Completed||Phase==AIPhase::Stopped)&&std::abs(TrafficWorld.Handling(TrainId(3))->speedMps)<.02)break;
   Safe=TrafficWorld.Update(1.0/30).ok;
  }
  const auto* AI=TrafficWorld.AI(TrainId(3));const auto* H=TrafficWorld.Handling(TrainId(3));
  Check("Tambaram.Opposing.PauseResumeAndEndpointBraking",Safe&&AI&&H&&(AI->Phase()==AIPhase::Completed||AI->Phase()==AIPhase::Stopped)&&std::abs(H->speedMps)<.02&&TrafficWorld.ValidateInvariants().ok,TrafficWorld.Error());
  SnapshotRecord Record;OperationsWorld Cold;auto Saved=WorldSnapshotCodec::Capture(TrafficWorld,1000,Record);if(Saved.ok)Saved=WorldSnapshotCodec::Restore(Record,Cold);
  Check("Tambaram.Opposing.StoppedColdRecovery",Saved.ok&&Cold.Json()==TrafficWorld.Json(),Saved.error);
 }
 Check("Tambaram.Junction.ConflictingPlayerRouteDenied",ConflictDenied);
 Check("Tambaram.Junction.AIBothPointsLocked",AIPointsLocked);
 Check("Tambaram.Junction.LockedPointMovementDenied",PointRequestSent&&LockedPointDenied);
 Check("Tambaram.Junction.HeadClearRetainsRearLock",AIHeadClearRearLocked);
 Check("Tambaram.Junction.AIWholeRearRelease",AICleared);
 Check("Tambaram.Junction.PlayerThroatReleasesBeforeOnward",PlayerThroatReleased&&OnwardGranted);
 Check("Tambaram.Journey.AllAlignmentEdgesTraversed",Visited.size()==TambaramRoute::SegmentCount);
 Check("Tambaram.Journey.AuthorityAndWholeTrainFootprint",Authority);
 Check("Tambaram.Journey.MonotonicForwardMotion",Monotonic);
 Check("Tambaram.Streaming.PolicySpanningAndLookback",Spanning);
 Check("Tambaram.Streaming.BoundaryColdRecovery",BoundaryRecovery);
 Check("Tambaram.Recovery.EveryEdgeAndStationPhase",PhaseRecovery&&Recoveries>=TambaramRoute::SegmentCount);
 Check("Tambaram.Terminal.NoDeparturePhase",!DwellInterrupted&&!S.Current().departureAuthorized&&!Phases.count(StationPhase::Departed)&&!Phases.count(StationPhase::ReadyToDepart));
 Check("Tambaram.Terminal.FullDwell",S.Current().dwellTicks==D.dwellTicks);
 Check("Tambaram.Pace.CruiseReachedWithoutOverspeed",PeakSpeed>D.assistedCruise-.5&&PeakSpeed<D.speedLimit&&S.Current().overspeedTicks==0,std::to_string(PeakSpeed));
 Check("Tambaram.Pace.CompletesWithinPlannedDuration",S.Current().success&&S.Current().ticks<=uint64_t(D.expectedDurationSeconds*30));
 Check("Tambaram.Terminal.DepartAfterCompletionRejected",!S.Depart(W).ok);
 if(!S.Current().success)return Report;
 std::array<std::vector<uint8_t>,2> Slots;
 CheckpointIO IO{[&](unsigned I){return Slots[I];},[&](unsigned I,const std::vector<uint8_t>& B){Slots[I]=B;return Result{};}};
 CheckpointCommit Commit;R=S.Settle(W,IO,Commit);const auto Once=Slots;
 if(R.ok)R=S.Settle(W,IO,Commit);
 Check("Tambaram.Result.SettlementIdempotent",R.ok&&Slots==Once,R.error);
 OperationsWorld Cold;ServiceRuntime Restored(D);RecoverySelection Selected;R=Restored.Recover(IO,Cold,Selected);
 Check("Tambaram.Result.SettledColdRecovery",R.ok&&Restored.Current().success&&Restored.Current().settled&&Restored.Json()==S.Json(),R.error);
 return Report;
}
}
