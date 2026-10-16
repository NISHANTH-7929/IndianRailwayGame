#include "RailService.h"
#include <cmath>
namespace Rail {
TestReport RunLhbTests(){
 TestReport report;auto recordCheck=[&](const char* n,bool p){report.cases.push_back({n,p,0,0,"Versioned LHB configuration boundary"});};
 Fixture f;Formation shortTrain,longTrain;
 recordCheck("LHB.Resolve",FormationContent::Resolve(FormationContent::LhbId,f,shortTrain,longTrain).ok);
 const auto& coach=f.definitions.at(DefinitionId(3001));
 const auto& locomotive=f.definitions.at(DefinitionId(3000));
 recordCheck("LHB.Wap7MeasuredEnvelope",locomotive.lengthMetres==FormationContent::Wap7EnvelopeLength&&locomotive.frontBogieOffsetMetres==5.85&&locomotive.rearBogieOffsetMetres==-5.85&&shortTrain.members.front().definition==DefinitionId(3000));
 recordCheck("LHB.Dimensions",coach.version==1&&coach.lengthMetres==24&&coach.frontBogieOffsetMetres==7.45&&coach.rearBogieOffsetMetres==-7.45);
 FormationProperties p;shortTrain.Derive(f.definitions,p);
 recordCheck("LHB.ShortFootprint",std::abs(p.lengthMetres-(24+FormationContent::Wap7EnvelopeLength))<1e-8&&shortTrain.gapMetres==0);
 longTrain.Derive(f.definitions,p);recordCheck("LHB.LongFootprint",std::abs(p.lengthMetres-(168+FormationContent::Wap7EnvelopeLength))<1e-8);
 OperationsWorld world;recordCheck("LHB.Initialize",world.InitializeContent(FormationContent::LhbId).ok);
 const auto& pose=world.Spatial(TrainId(2))->Current().vehicles.at(1);
 recordCheck("LHB.BogiePlacement",std::abs((pose.frontBogie-pose.rearBogie).Length()-14.9)<1e-8&&pose.id==VehicleId(102)&&pose.definition==DefinitionId(3001));
 recordCheck("LHB.FootprintAndOccupancy",world.ValidateInvariants().ok&&std::abs(world.Spatial(TrainId(2))->Current().properties.lengthMetres-(24+FormationContent::Wap7EnvelopeLength))<1e-8);
 SnapshotRecord record;recordCheck("LHB.Capture",WorldSnapshotCodec::Capture(world,1,record).ok&&record.ContentId()==FormationContent::LhbId);
 OperationsWorld cold;recordCheck("LHB.ColdRecovery",WorldSnapshotCodec::Restore(record,cold).ok&&cold.ContentId()==FormationContent::LhbId&&cold.Json()==world.Json());
 OperationsWorld old;old.InitializeFixture();SnapshotRecord legacy;WorldSnapshotCodec::Capture(old,1,legacy);
 recordCheck("Legacy.IdentityUnchanged",legacy.ContentId()=="IR-S6-fixture-1"&&old.Spatial(TrainId(2))->Current().vehicles.at(1).lengthMetres==22);
 recordCheck("Legacy.ColdRecovery",WorldSnapshotCodec::Restore(legacy,cold).ok&&cold.Json()==old.Json());
 const auto before=world.Json();recordCheck("LHB.UnknownInitializeTransactional",!world.InitializeContent("future").ok&&world.Json()==before);
 ServiceDefinition service;service.contentId=FormationContent::LhbId;service.id="S8-Shantipur-LHB-01";service.version="IR-S8-LHB-service-1";service.berthHead=FormationContent::ServiceBerthHead;
 recordCheck("LHB.PlatformCompatible",service.Validate().ok);
 auto bad=service;bad.platformFrom=100;recordCheck("LHB.ShortPlatformRejected",!bad.Validate().ok);
 ServiceRuntime runtime(service);runtime.Select();runtime.ConfirmFormation();
 recordCheck("LHB.ServiceStart",runtime.Start(world).ok&&world.ContentId()==service.contentId);
 SnapshotRecord saved;recordCheck("LHB.ServiceCapture",runtime.Capture(world,2,saved).ok);
 ServiceRuntime restored;recordCheck("LHB.ServiceColdRecovery",ServiceRuntime::Restore(saved,service,cold,restored).ok&&cold.ContentId()==service.contentId);
 recordCheck("LHB.ServiceMismatchRejected",!ServiceRuntime::Restore(saved,ServiceDefinition{},cold,restored).ok);
 std::vector<uint8_t> slots[2]={legacy.Bytes(),record.Bytes()};unsigned writes=0;
 CheckpointIO io{[&](unsigned i){return slots[i];},[&](unsigned i,const std::vector<uint8_t>& b){++writes;slots[i]=b;return Result{};}};CheckpointCommit commit;
 recordCheck("LHB.MixedSlotsPreserved",!CommitCheckpoint(old,io,commit).ok&&writes==0&&slots[1]==record.Bytes());
 recordCheck("LHB.MixedSlotsPreservedReverse",!CommitCheckpoint(world,io,commit).ok&&writes==0&&slots[0]==legacy.Bytes());
 bool okay=runtime.Input(world,CommandKind::Brake,0).ok&&runtime.Input(world,CommandKind::Throttle,.5).ok;
 bool movingRecovery=false;double greatestSpeed=0;
 for(unsigned i=0;i<16000&&okay&&runtime.Current().phase==ServicePhase::Running;++i){
  okay=runtime.Update(world,1.0/30.0).ok;greatestSpeed=std::max(greatestSpeed,world.Handling(TrainId(2))->speedMps);
  if(i==2000){SnapshotRecord moving;OperationsWorld recovered;ServiceRuntime resumed;
   movingRecovery=runtime.Capture(world,3,moving).ok&&ServiceRuntime::Restore(moving,service,recovered,resumed).ok;
   if(movingRecovery){world=std::move(recovered);runtime=std::move(resumed);}
  }
  if(runtime.Current().station==StationPhase::ReadyToDepart&&!runtime.Current().departureAuthorized)okay=okay&&runtime.Depart(world).ok;
 }
 recordCheck("LHB.MovingColdRecovery",movingRecovery);
 recordCheck("LHB.AcceleratesAndBrakes",greatestSpeed>0.5&&std::abs(world.Handling(TrainId(2))->speedMps)<.05);
 recordCheck("LHB.CompletePassengerService",okay&&runtime.Current().success);
 recordCheck("LHB.CurvedTrafficFootprint",world.AI(TrainId(1))&&world.AI(TrainId(1))->Phase()==AIPhase::Completed&&world.ValidateInvariants().ok);
 return report;
}
}


