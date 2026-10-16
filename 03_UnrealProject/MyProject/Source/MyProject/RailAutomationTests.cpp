#include "Misc/AutomationTest.h"
#include "Simulation/RailKernel.h"
#include "Simulation/RailHandling.h"
#include "Simulation/RailOperations.h"
#include "Simulation/RailCheckpointStore.h"
#include "Simulation/RailService.h"
#include "RailReviewPolicy.h"
#include "RailControlPresentation.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailControlPresentationTest,"IndianRailway.S8.ControlPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailControlPresentationTest::RunTest(const FString& Parameters){
 Rail::LongitudinalState H;H.command.throttle=.35;H.command.brake=0;H.command.emergency=true;H.command.direction=1;H.appliedThrottle=0;H.appliedBrake=1;
 auto P=Rail::ControlPresentation::From(H);
 TestEqual(TEXT("Emergency HUD/cab use applied brake despite zero request"),P.brake,1.0);
 TestEqual(TEXT("Emergency HUD/cab use applied power despite nonzero request"),P.power,0.0);
 TestTrue(TEXT("Emergency is explicit before and after full application"),P.emergency);
 H.appliedBrake=.3;P=Rail::ControlPresentation::From(H);TestEqual(TEXT("Brake ramp is not falsified as instantly full"),P.brake,.3);
 H.command.emergency=false;H.appliedBrake=.5;H.appliedThrottle=.2;H.command.direction=-1;P=Rail::ControlPresentation::From(H);
 TestEqual(TEXT("Normal applied power is preserved"),P.power,.2);TestEqual(TEXT("Service brake is preserved"),P.brake,.5);TestFalse(TEXT("Reset removes emergency indication"),P.emergency);TestEqual(TEXT("Authoritative direction preserved"),P.direction,-1);
 TestEqual(TEXT("Projection does not alter driver request"),H.command.brake,0.0);
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailReviewPolicyTest,"IndianRailway.S8.IsolatedRecoveryPolicy",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailReviewPolicyTest::RunTest(const FString& Parameters){
 for(int32 Bits=0;Bits<16;++Bits){
  const bool Development=(Bits&1)!=0,Review=(Bits&2)!=0,Egmore=(Bits&4)!=0,Resume=(Bits&8)!=0;
  const auto P=Rail::ReviewPolicy::FromFlags(Development,Review,Egmore,Resume);
  TestEqual(TEXT("Only development review isolates storage"),P.isolated,Development&&Review);
  TestEqual(TEXT("Resume requires development, review and Egmore"),P.resumeRequired,Development&&Review&&Egmore&&Resume);
  TestEqual(TEXT("Fresh review alone auto-starts"),P.Fresh(),Development&&Review&&!(Egmore&&Resume));
  TestFalse(TEXT("Successful recovery never blocks"),P.RecoveryBlocked(true,true));
  TestEqual(TEXT("Missing resume checkpoint blocks instead of starting"),P.RecoveryBlocked(false,false),P.resumeRequired);
  TestEqual(TEXT("Failed recovery preserves existing slots"),P.RecoveryBlocked(false,true),!P.Fresh());
 }
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailKernelTest,"IndianRailway.S2.Kernel",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailKernelTest::RunTest(const FString& Parameters){
 const auto Report=Rail::RunKernelTests();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);return Report.Passed();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailHandlingTest,"IndianRailway.S3.Handling",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailHandlingTest::RunTest(const FString& Parameters){
 const auto Report=Rail::RunHandlingTests();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);return Report.Passed();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailOperationsTest,"IndianRailway.S4.Operations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailOperationsTest::RunTest(const FString& Parameters){const auto Report=Rail::RunOperationsTests();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);return Report.Passed();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailAITest,"IndianRailway.S5.AI",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailAITest::RunTest(const FString& Parameters){const auto Report=Rail::RunAITests();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);return Report.Passed();}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailContinuityTest,"IndianRailway.S6.Continuity",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailContinuityTest::RunTest(const FString& Parameters){bool Passed=true;const Rail::TestReport Reports[]={Rail::RunContentTests(),Rail::RunSnapshotRecordTests(),Rail::RunWorldSnapshotTests(),Rail::RunCheckpointTests()};for(const auto& Report:Reports){Passed&=Report.Passed();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);}return Passed;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailServiceTest,"IndianRailway.S7.Service",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailServiceTest::RunTest(const FString& Parameters){const auto Report=Rail::RunServiceTests();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);return Report.Passed();}
#include "RailChunkView.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/InstancedStaticMeshComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailLhbPresentationTest,"IndianRailway.S8.LHBPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailLhbPresentationTest::RunTest(const FString& Parameters){
 const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!World){AddError(TEXT("No test world"));return false;}
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 Rail::OperationsWorld Logical;Logical.InitializeContent(Rail::FormationContent::LhbId);Rail::SnapshotRecord Before,After;
 Rail::WorldSnapshotCodec::Capture(Logical,1,Before);
 for(int32 Cycle=0;Cycle<2;++Cycle){int32 Total=0;
  for(int32 Chunk=1;Chunk<=2;++Chunk){auto* View=World->SpawnActor<ARailChunkView>();View->ChunkId=Chunk;
   TestTrue(TEXT("Coach assets load"),View->LoadCoach());View->Present(Logical,false);Total+=View->PresentedLhbVehicles;
   TestEqual(TEXT("Seven independently movable mesh parts"),View->CoachParts.Num(),7);
   if(View->CoachParts.Num()==7)for(const auto& Part:View->CoachParts)TestEqual(TEXT("Part instance count follows owned vehicles"),Part->GetInstanceCount(),View->PresentedLhbVehicles);
   TestTrue(TEXT("Presentation actor destroyed"),View->Destroy());
  }
  TestEqual(TEXT("Both chunks present all eight coaches exactly once"),Total,8);
 }
 Rail::WorldSnapshotCodec::Capture(Logical,1,After);TestTrue(TEXT("Presentation recreation preserves logical snapshot"),Before.Bytes()==After.Bytes());
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailLhbTest,"IndianRailway.S8.LHB",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailLhbTest::RunTest(const FString& Parameters){const auto Report=Rail::RunLhbTests();for(const auto& C:Report.cases)TestTrue(UTF8_TO_TCHAR(C.name.c_str()),C.passed);return Report.Passed();}
#include "Misc/CommandLine.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailStationTest,"IndianRailway.S8.StationPresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailStationTest::RunTest(const FString& Parameters){
 const FString Original=FCommandLine::Get();
 const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!World){AddError(TEXT("No station test world"));return false;}
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 Rail::OperationsWorld Logical;Logical.InitializeContent(Rail::FormationContent::LhbId);Rail::SnapshotRecord Before,After;
 Rail::WorldSnapshotCodec::Capture(Logical,1,Before);
 for(int32 Variant=0;Variant<2;++Variant){FCommandLine::Set(*(Original+(Variant?TEXT(" -S8Station -S8StationB"):TEXT(" -S8Station"))));
  for(int32 Cycle=0;Cycle<2;++Cycle)for(int32 Chunk=1;Chunk<=2;++Chunk){auto* View=World->SpawnActor<ARailChunkView>();View->ChunkId=Chunk;
   TestTrue(TEXT("Station package loads"),View->LoadStation(Logical.Spatial(Rail::TrainId(1))->Network()));
   TestTrue(TEXT("Chunk owns station presentation"),!View->StationParts.IsEmpty());
   int32 Count=0,PlatformCount=0,MastCount=0,RaisedMastCount=0;for(const auto& Part:View->StationParts){Count+=Part->GetInstanceCount();TestTrue(TEXT("Station mesh reference"),Part->GetStaticMesh()!=nullptr);TestTrue(TEXT("Scenery never supplies collision authority"),Part->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
    if(Part->GetStaticMesh()&&Part->GetStaticMesh()->GetName()==TEXT("SM_Station_OHEMast"))for(int32 I=0;I<Part->GetInstanceCount();++I){
     FTransform Transform;Part->GetInstanceTransform(I,Transform,true);const FVector P=Transform.GetLocation()/100.;++MastCount;
     const double RailZ=P.X>160?(P.X-160)*.02/FMath::Sqrt(1.-.02*.02):0.;
     if(P.X>160)++RaisedMastCount;
     TestTrue(TEXT("OHE foundation follows true arc-length grade"),FMath::Abs(P.Z-RailZ)<.0001);
     TestTrue(TEXT("OHE arm reaches the track centreline"),FMath::Abs(P.Y+3.3)<.0001);
     TestTrue(TEXT("OHE mast stays vertical on grade"),Transform.GetUnitAxis(EAxis::Z).Equals(FVector::UpVector,.0001));
     TestTrue(TEXT("OHE support meets rail-relative 5.5 m contact height"),FMath::Abs(Transform.TransformPosition(FVector(0,330,550)).Z/100.-RailZ-5.5)<.0001);
    }
    if(Part->GetStaticMesh()&&Part->GetStaticMesh()->GetName()==TEXT("SM_Station_Platform5m"))for(int32 I=0;I<Part->GetInstanceCount();++I){
     FTransform Transform;Part->GetInstanceTransform(I,Transform,true);const FBox Bounds=Part->GetStaticMesh()->GetBoundingBox().TransformBy(Transform);++PlatformCount;
     TestTrue(TEXT("Imported coping agrees with 1.675 m track offset"),FMath::Abs(Bounds.Max.Y+167.5)<.02);
     TestTrue(TEXT("Imported platform stays within S7 95..145 m extent"),Bounds.Min.X>=9499.98&&Bounds.Max.X<=14500.02);
     TestTrue(TEXT("Imported platform top includes 2 mm painted strip"),FMath::Abs(Bounds.Max.Z-84.2)<.02);
    }
   }
   TestEqual(TEXT("Ten five-metre platform modules owned by station chunk"),PlatformCount,Chunk==2?10:0);
   TestEqual(TEXT("Twelve OHE masts retain chunk ownership"),MastCount,Chunk==2?8:4);
   TestEqual(TEXT("Four masts follow the climbing track"),RaisedMastCount,Chunk==2?4:0);
   TestTrue(TEXT("Track and scenery instances populated"),Count>100);
   const int32 Components=View->StationParts.Num();View->LoadStation(Logical.Spatial(Rail::TrainId(1))->Network());
   TestEqual(TEXT("Repeated presentation load is idempotent"),View->StationParts.Num(),Components);
   TestTrue(TEXT("Station owner destroyed"),View->Destroy());
  }
 }
 FCommandLine::Set(*Original);Rail::WorldSnapshotCodec::Capture(Logical,1,After);
 TestTrue(TEXT("Both visual arrangements preserve logical snapshot"),Before.Bytes()==After.Bytes());
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return !HasAnyErrors();
}
#include "RailEgmoreSceneView.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailEgmoreSceneTest,"IndianRailway.S8.EgmoreScenePresentation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailEgmoreSceneTest::RunTest(const FString& Parameters){
 const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!World){AddError(TEXT("No Egmore test world"));return false;}
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 for(int32 Cycle=0;Cycle<2;++Cycle){
  auto* View=World->SpawnActor<ARailEgmoreSceneView>();
  if(!View){AddError(TEXT("Egmore actor creation failed"));break;}
  for(int32 Rebuild=0;Rebuild<2;++Rebuild){
   TestTrue(TEXT("Separate Egmore family loads"),View->RebuildScene());
   TestEqual(TEXT("Manifest placements recreated exactly"),View->PlacementCount,1316);
   TArray<UHierarchicalInstancedStaticMeshComponent*> Components;View->GetComponents(Components);
   TestEqual(TEXT("Shared modules do not accumulate across rebuilds"),Components.Num(),40);
   int32 Instances=0;
   for(auto* Component:Components){
    Instances+=Component->GetInstanceCount();TestNotNull(TEXT("Imported module bound"),Component->GetStaticMesh().Get());
    TestTrue(TEXT("Art cannot change simulation collision"),Component->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
    for(int32 I=0;I<Component->GetInstanceCount();++I){FTransform T;Component->GetInstanceTransform(I,T,false);TestTrue(TEXT("Finite positive instance transform"),!T.ContainsNaN()&&T.GetScale3D().GetMin()>0);}
   }
   TestEqual(TEXT("Every placement has one instance"),Instances,1316);
  }
  TestTrue(TEXT("Disposable station actor destroyed"),View->Destroy());
 }
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return !HasAnyErrors();
}
#include "Simulation/RailEgmoreLayout.h"
#include "Simulation/RailEgmoreService.h"
#include "Simulation/RailTambaramRoute.h"
#include "Simulation/RailTambaramService.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailEgmoreLayoutTest,"IndianRailway.S8.EgmoreSpatialContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailEgmoreLayoutTest::RunTest(const FString& Parameters){
 using namespace Rail;
 Fixture FixtureData;Formation Train;auto R=EgmoreLayout::ResolveSpatial(FixtureData,Train);
 TestTrue(TEXT("Candidate station graph resolves independently"),R.ok);
  TestFalse(TEXT("Scenario identity is not aliased to the old formation registry"),FormationContent::Supports(EgmoreLayout::ReservedContentId));
 FormationProperties Properties;TestTrue(TEXT("WAP7 plus seven LHB derives"),Train.Derive(FixtureData.definitions,Properties).ok);
 TestEqual(TEXT("Eight vehicles use existing dimensions"),int32(Properties.vehicleCount),8);
 TestTrue(TEXT("Native formation fits authored platform"),EgmoreLayout::BerthHeadX-Properties.lengthMetres>=EgmoreLayout::PlatformFromX&&EgmoreLayout::BerthHeadX<=EgmoreLayout::PlatformToX);
 for(size_t Track=0;Track<EgmoreLayout::TrackY.size();++Track){
  Simulation S;R=S.Initialize(FixtureData.graph,FixtureData.definitions,Train,{EdgeId(EgmoreLayout::TrackEdges[Track]),1},EgmoreLayout::StationMetres(EgmoreLayout::StartHeadX),{},TrainId(2));
  TestTrue(TEXT("Full formation starts within the approach"),R.ok);if(!R.ok)continue;
  bool Aligned=true,Bogies=true,Advanced=true;
  for(int32 Step=0;Step<220;++Step){
   if(!S.AdvanceDistance(1).ok){Advanced=false;break;}
   for(const auto& V:S.Current().vehicles){
    Aligned&=FMath::Abs(V.bodyCentre.y-EgmoreLayout::TrackY[Track])<1e-8&&FMath::Abs(V.bodyCentre.z)<1e-8;
    const auto& D=FixtureData.definitions.at(V.definition);
    Bogies&=FMath::Abs((V.frontBogie-V.bodyCentre).Length()-D.frontBogieOffsetMetres)<1e-8&&FMath::Abs((V.rearBogie-V.bodyCentre).Length()+D.rearBogieOffsetMetres)<1e-8;
   }
  }
  TestTrue(TEXT("Entire approach advances with unchanged spatial algorithm"),Advanced);
  TestTrue(TEXT("Moving bodies follow authored track centres and rail datum"),Aligned);
  TestTrue(TEXT("Moving bogies retain native offsets"),Bogies);
  Sample Head;S.Network().SamplePosition(S.Current().head,Head);
  TestTrue(TEXT("Berth maps to world X100 without visual scaling"),FMath::Abs(Head.position.x-EgmoreLayout::BerthHeadX)<1e-8);
  TestTrue(TEXT("Reversal remains on committed straight alignment"),S.AdvanceDistance(-220).ok);
 }
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailTambaramRouteTest,"IndianRailway.S8.EgmoreTambaramRouteContract",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailTambaramRouteTest::RunTest(const FString& Parameters){
 using namespace Rail;Fixture F;Formation Player,Traffic;auto R=TambaramRoute::ResolveSpatial(F,Player,Traffic);
 TestTrue(TEXT("ROUTE-T03 graph connectivity validates"),R.ok);
 TestEqual(TEXT("Official reference sequence has fifteen stations"),int32(TambaramRoute::StationCount),15);
 const double Length=TambaramRoute::RouteLength(1);
 TestTrue(TEXT("Source-backed corridor preserves full metric scale including endpoint envelopes"),Length>24000&&Length<26000);
 bool Smooth=true;int32 Curves=0;
 for(size_t Track=0;Track<4;++Track)for(size_t I=0;I<TambaramRoute::SegmentCount;++I){
  const auto* E=F.graph.Find(TambaramRoute::EdgeFor(Track,I));if(!E){Smooth=false;continue;}
  if(std::abs(E->geometry.curvaturePerMetre)>1e-8)++Curves;
  if(I){const auto* Prior=F.graph.Find(TambaramRoute::EdgeFor(Track,I-1));const auto End=Prior->geometry.At(Prior->lengthMetres),Start=E->geometry.At(0);
   Smooth&=(End.position-Start.position).Length()<1e-5&&(End.tangent-Start.tangent).Length()<1e-5;}
 }
 TestTrue(TEXT("ROUTE-T08 analytic curves have continuous positions and tangents"),Smooth&&Curves>0);
 TestTrue(TEXT("ROUTE-T02 stable Tambaram station identity"),FString(TambaramRoute::DestinationStationId)==TEXT("Station.Tambaram.01"));
 TestEqual(TEXT("Four parallel tracks plus two crossover arcs contain stable segment edges"),int32(F.graph.edges.size()),int32(4*TambaramRoute::SegmentCount+2));
 TestTrue(TEXT("Crossover graph edges are present"),F.graph.Find(EdgeId(90000))&&F.graph.Find(EdgeId(90001)));
 const auto Service=MakeEgmoreTambaramService();R=Service.Validate();TestTrue(TEXT("Distinct extended service validates"),R.ok);
 if(!R.ok){AddError(UTF8_TO_TCHAR(R.error.c_str()));return false;}
 OperationsWorld W;R=W.InitializeContent(TambaramRoute::ContentId);TestTrue(TEXT("ROUTE-T09 authoritative operations initialize"),R.ok);
 if(!R.ok){AddError(UTF8_TO_TCHAR(R.error.c_str()));return false;}
 TestEqual(TEXT("WAP-7 plus LHB player formation retained"),int32(W.Spatial(TrainId(2))->Current().vehicles.size()),8);
 ServiceRuntime Driver(Service);R=Driver.Select();if(R.ok)R=Driver.ConfirmFormation();if(R.ok)R=Driver.Start(W);
 if(R.ok)R=Driver.Input(W,CommandKind::Brake,0);
 if(R.ok)R=Driver.Input(W,CommandKind::Throttle,1);
 bool SawPlayerProceed=false,SawConflictStop=false;
 for(int I=0;I<14000&&R.ok;++I){R=Driver.Update(W,1.0/30.0);SawPlayerProceed|=W.Current().signals.at(SignalId(2)).aspect==SignalAspect::Proceed;
  const auto Reply=W.Current().lastRequest.find(TrainId(2));
  SawConflictStop|=Reply!=W.Current().lastRequest.end()&&!Reply->second.granted&&W.Current().routes.at(RouteId(1)).owner.Active()&&W.Current().signals.at(SignalId(2)).aspect==SignalAspect::Stop;
 }
 TestTrue(TEXT("ROUTE-T10 occupied shared throat denies player and retains stop"),R.ok&&SawConflictStop);
 TestTrue(TEXT("ROUTE-T10 player signal clears after AI whole-train release"),R.ok&&SawPlayerProceed);
 const auto* MovedPlayer=W.Spatial(TrainId(2));
 const auto HistoryCheck=MovedPlayer->History().Validate(MovedPlayer->Network());
 TestTrue(FString::Printf(TEXT("ROUTE-T05 player moved beyond initial segment (head %.3f)"),MovedPlayer->Current().headStationMetres),R.ok&&MovedPlayer->Current().headStationMetres>TambaramRoute::StartHead+300);
 TestTrue(FString::Printf(TEXT("ROUTE-T05 committed history validates: %s"),UTF8_TO_TCHAR(HistoryCheck.error.c_str())),HistoryCheck.ok);
 SnapshotRecord Saved;R=WorldSnapshotCodec::Capture(W,7,Saved);OperationsWorld Cold;if(R.ok)R=WorldSnapshotCodec::Restore(Saved,Cold);
 TestTrue(TEXT("ROUTE-T18 extended logical state cold-restores"),R.ok&&Cold.ContentId()==TambaramRoute::ContentId&&Cold.Json()==W.Json());
 OperationsWorld Legacy;Legacy.InitializeContent(EgmoreLayout::ReservedContentId);const auto Before=Legacy.Json();
 ServiceRuntime Wrong(MakeEgmoreService()),Extended(Service);SnapshotRecord ExtendedRecord;
 R=Extended.Select();if(R.ok)R=Extended.ConfirmFormation();if(R.ok)R=Extended.Start(W);if(R.ok)R=Extended.Capture(W,1,ExtendedRecord);
 TestTrue(TEXT("Extended service snapshot captured"),R.ok);
 ServiceRuntime Out;TestFalse(TEXT("ROUTE-T19 Egmore service rejects extended save"),ServiceRuntime::Restore(ExtendedRecord,MakeEgmoreService(),Legacy,Out).ok);
 TestTrue(TEXT("Rejected save preserves Egmore world"),Legacy.Json()==Before);
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailEgmoreServiceTest,"IndianRailway.S8.EgmoreServiceRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailEgmoreServiceTest::RunTest(const FString& Parameters){
 using namespace Rail;const auto D=MakeEgmoreService();auto R=D.Validate();
 TestTrue(TEXT("Independent service configuration validates"),R.ok);if(!R.ok){AddError(UTF8_TO_TCHAR(R.error.c_str()));return false;}
 OperationsWorld W;ServiceRuntime Service(D);
 R=Service.Select();if(R.ok)R=Service.ConfirmFormation();if(R.ok)R=Service.Start(W);
 TestTrue(TEXT("Egmore service starts with authoritative operations"),R.ok);if(!R.ok){AddError(UTF8_TO_TCHAR(R.error.c_str()));return false;}
 TestEqual(TEXT("Player has full eight vehicle formation"),int32(W.Spatial(D.player)->Current().vehicles.size()),8);
 TestEqual(TEXT("Scenario owns its content identity"),FString(W.ContentId().c_str()),FString(EgmoreLayout::ReservedContentId));
 Service.Input(W,CommandKind::Brake,0);Service.Input(W,CommandKind::Throttle,1);
 for(int32 I=0;I<900&&R.ok;++I)R=Service.Update(W,1.0/30);
 SnapshotRecord Saved;if(R.ok)R=Service.Capture(W,1,Saved);
 OperationsWorld Recovered;ServiceRuntime Restored(D);if(R.ok)R=ServiceRuntime::Restore(Saved,D,Recovered,Restored);
 TestTrue(TEXT("Running Egmore service cold-restores"),R.ok);if(!R.ok){AddError(UTF8_TO_TCHAR(R.error.c_str()));return false;}
 bool Same=true;for(int32 I=0;I<120&&Same;++I){
  Same=Service.Update(W,1.0/30).ok&&Restored.Update(Recovered,1.0/30).ok;
  SnapshotRecord A,B;Same=Same&&Service.Capture(W,2,A).ok&&Restored.Capture(Recovered,2,B).ok&&A.Bytes()==B.Bytes();
 }
 TestTrue(TEXT("Recovered service continues deterministically"),Same);
 OperationsWorld Untouched;Untouched.InitializeFixture();const auto Before=Untouched.Json();ServiceRuntime Legacy;
 TestFalse(TEXT("Egmore save rejected by legacy service"),ServiceRuntime::Restore(Saved,ServiceDefinition{},Untouched,Legacy).ok);
 TestTrue(TEXT("Incompatible recovery retains existing world"),Untouched.Json()==Before);
 std::set<StationPhase> CheckedPhases;bool PhaseRecovery=true,CheckedTrafficRelease=false,TrafficRouteSeen=false;
 for(int32 I=0;I<20000&&Service.Current().phase==ServicePhase::Running&&R.ok;++I){
  // The player can finish before independent adjacent traffic clears. Hold at
  // the berth in this recovery scenario so release is genuinely exercised.
  if(Service.Current().station==StationPhase::ReadyToDepart&&!Service.Current().departureAuthorized&&CheckedTrafficRelease)R=Service.Depart(W);
  if(R.ok)R=Service.Update(W,1.0/30);
  const auto Phase=Service.Current().station;
  const bool TrafficActive=W.Current().routes.at(D.trafficRoute).owner.Active();
  TrafficRouteSeen|=TrafficActive;
  const bool Released=TrafficRouteSeen&&!TrafficActive;
  if((Phase>=StationPhase::Arrival&&CheckedPhases.insert(Phase).second)||(Released&&!CheckedTrafficRelease)){
   SnapshotRecord PhaseRecord;OperationsWorld PhaseWorld;ServiceRuntime PhaseService(D);
   const auto Captured=Service.Capture(W,3,PhaseRecord);
   const auto Loaded=Captured.ok?ServiceRuntime::Restore(PhaseRecord,D,PhaseWorld,PhaseService):Captured;
   PhaseRecovery&=Loaded.ok&&PhaseWorld.Json()==W.Json()&&PhaseService.Json()==Service.Json();
   if(Released)CheckedTrafficRelease=true;
  }
 }
 TestTrue(TEXT("Full approach/dwell/departure completes"),R.ok&&Service.Current().success);
 TestTrue(TEXT("Station phases and AI route release cold-restore exactly"),PhaseRecovery&&CheckedPhases.count(StationPhase::Dwelling)&&CheckedPhases.count(StationPhase::ReadyToDepart)&&CheckedPhases.count(StationPhase::Departed)&&CheckedPhases.count(StationPhase::Completed)&&CheckedTrafficRelease);
 if(!Service.Current().success){AddError(UTF8_TO_TCHAR(Service.Json().c_str()));return false;}
 std::array<std::vector<uint8_t>,2> Slots;
 CheckpointIO IO{[&](unsigned I){return Slots[I];},[&](unsigned I,const std::vector<uint8_t>& Bytes){Slots[I]=Bytes;return Result{};}};
 CheckpointCommit Commit;R=Service.Settle(W,IO,Commit);
 TestTrue(TEXT("Egmore reward/checkpoint settlement succeeds"),R.ok);
 OperationsWorld Cold;ServiceRuntime ColdService(D);RecoverySelection Selected;R=ColdService.Recover(IO,Cold,Selected);
 TestTrue(TEXT("Completed Egmore service recovers its own save"),R.ok&&ColdService.Current().success&&ColdService.Current().settled);
 const auto SafeSlots=Slots;ServiceRuntime Other;OperationsWorld OtherWorld;OtherWorld.InitializeFixture();RecoverySelection Refused;
 TestFalse(TEXT("Legacy recovery cannot select Egmore saves"),Other.Recover(IO,OtherWorld,Refused).ok);
 TestTrue(TEXT("Incompatible slots are never overwritten"),Slots==SafeSlots);
 SnapshotRecord Unknown;TestFalse(TEXT("Unknown scenario content rejected"),SnapshotRecord::Create(1,{1},"IR-S8-EGMORE-999",Unknown).ok);
 return !HasAnyErrors();
}
#endif


