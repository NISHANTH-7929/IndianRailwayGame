#include "RailSimulationSubsystem.h"
#include "Kismet/KismetSystemLibrary.h"
#include "RailKernelView.h"
#include "RailOperationsView.h"
#include "RailVehicleDefinition.h"
#include "RailHandlingDefinition.h"
#include "RailDrivingWidget.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Misc/CoreDelegates.h"
#include "HAL/FileManager.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(RailS5,true);
DEFINE_LOG_CATEGORY_STATIC(LogRailKernel,Log,All);
void URailSimulationSubsystem::OnWorldBeginPlay(UWorld& W){
 if(W.GetMapName().Contains(TEXT("L_RailService_Egmore"))&&!FParse::Param(FCommandLine::Get(),TEXT("S8Egmore"))){UE_LOG(LogRailKernel,Error,TEXT("Egmore content flag required; refusing legacy save initialization"));return;}
 Super::OnWorldBeginPlay(W);if(W.GetMapName().Contains(TEXT("L_RailService"))){ServiceMode=true;InitializeS6();return;}if(W.GetMapName().Contains(TEXT("L_RailContinuity"))){InitializeS6();return;}AIMode=W.GetMapName().Contains(TEXT("L_RailAI"));OperationsMode=AIMode||W.GetMapName().Contains(TEXT("L_RailOperations"));HandlingMode=W.GetMapName().Contains(TEXT("L_TrainHandling"));Active=OperationsMode||HandlingMode||W.GetMapName().Contains(TEXT("L_RailKernel"));if(!Active)return;
 FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&URailSimulationSubsystem::SuspendFixture);
 FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,&URailSimulationSubsystem::ResumeFixture);
 FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&URailSimulationSubsystem::SuspendFixture);
 FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this,&URailSimulationSubsystem::ResumeFixture);
 const auto Tests=Rail::RunKernelTests();const FString Dir=FPaths::ProjectSavedDir()/(AIMode?TEXT("S5"):OperationsMode?TEXT("S4"):HandlingMode?TEXT("S3"):TEXT("S2"));IFileManager::Get().MakeDirectory(*Dir,true);
 FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Tests.Json().c_str()),*(Dir/TEXT("kernel-tests.json")));
 UE_LOG(LogRailKernel,Display,TEXT("S2_KERNEL_TESTS %s cases=%d"),Tests.Passed()?TEXT("PASS"):TEXT("FAIL"),int(Tests.cases.size()));
 if(OperationsMode){
  const auto H=Rail::RunHandlingTests(),O=Rail::RunOperationsTests();
  FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(H.Json().c_str()),*(Dir/TEXT("handling-tests.json")));
  FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(O.Json().c_str()),*(Dir/TEXT("operations-tests.json")));
  UE_LOG(LogRailKernel,Display,TEXT("S4_STARTUP S2=%d/%d S3=%d/%d S4=%d/%d"),Tests.Passed(),int(Tests.cases.size()),H.Passed(),int(H.cases.size()),O.Passed(),int(O.cases.size()));
  if(!Tests.Passed()||!H.Passed()||!O.Passed()){Status=TEXT("FAIL startup tests");return;}
  if(AIMode){const auto AI=Rail::RunAITests();FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(AI.Json().c_str()),*(Dir/TEXT("ai-tests.json")));
   UE_LOG(LogRailKernel,Display,TEXT("S5_STARTUP %s cases=%d"),AI.Passed()?TEXT("PASS"):TEXT("FAIL"),int(AI.cases.size()));if(!AI.Passed()){Status=TEXT("FAIL S5 startup");return;}}
  ResetHandlingFixture();CreateView();OperationsView=W.SpawnActor<ARailOperationsView>();
  if(auto* PC=W.GetFirstPlayerController()){DrivingWidget=CreateWidget<URailDrivingWidget>(PC);DrivingWidget->AddToViewport();PC->SetInputMode(FInputModeGameAndUI());PC->bShowMouseCursor=true;}
  return;
 }
 if(HandlingMode){
  const auto HandlingTests=Rail::RunHandlingTests();
  FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(HandlingTests.Json().c_str()),*(Dir/TEXT("handling-tests.json")));
  UE_LOG(LogRailKernel,Display,TEXT("S3_HANDLING_TESTS %s cases=%d"),HandlingTests.Passed()?TEXT("PASS"):TEXT("FAIL"),int(HandlingTests.cases.size()));
  ResetHandlingFixture();if(Status.StartsWith(TEXT("FAIL")))return;CreateView();
  if(auto* PC=W.GetFirstPlayerController()){DrivingWidget=CreateWidget<URailDrivingWidget>(PC);DrivingWidget->AddToViewport();PC->SetInputMode(FInputModeGameAndUI());PC->bShowMouseCursor=true;}
  return;
 }
 auto Fixture=Rail::MakeFixture();Fixture.definitions.clear();
 for(const TCHAR* Path:{TEXT("/Game/IndianRailway/S2/DA_Powered.DA_Powered"),TEXT("/Game/IndianRailway/S2/DA_Coach.DA_Coach")}){
  auto* Asset=LoadObject<URailVehicleDefinition>(nullptr,Path);if(!Asset){Status=FString::Printf(TEXT("FAIL missing data asset: %s"),Path);UE_LOG(LogRailKernel,Error,TEXT("%s"),*Status);return;}auto Record=Asset->Record();Fixture.definitions.emplace(Record.id,Record);
 }
 auto Result=Simulation.Initialize(Fixture.graph,Fixture.definitions,Fixture.Long(),{Rail::EdgeId(1),1},200,Fixture.SelectUpper());
 if(!Result.ok){Status=UTF8_TO_TCHAR(Result.error.c_str());UE_LOG(LogRailKernel,Error,TEXT("S2 initialization failed: %s"),*Status);return;}
 Simulation.SetCommandedSpeed(8);Status=Tests.Passed()?TEXT("Tests PASS | choice T2 upper | fixed30Hz | commanded motion only"):TEXT("Tests FAIL");CreateView();
}
void URailSimulationSubsystem::CreateView(){View=GetWorld()->SpawnActor<ARailKernelView>();if(auto* PC=GetWorld()->GetFirstPlayerController()){PC->SetViewTarget(View);PC->SetVirtualJoystickVisibility(false);}}
void URailSimulationSubsystem::Deinitialize(){
#if PLATFORM_ANDROID
 if(ServiceKeepsScreenAwake)UKismetSystemLibrary::ControlScreensaver(true);
#endif
 DestroyServiceAudio();
 if(ContinuityMode&&!Operations.Faulted())SaveS6(TEXT("world-exit"));
 FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
 FCoreDelegates::ApplicationHasEnteredForegroundDelegate.RemoveAll(this);
 FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
 FCoreDelegates::ApplicationHasReactivatedDelegate.RemoveAll(this);
 Super::Deinitialize();
}
void URailSimulationSubsystem::Tick(float DeltaTime){
 if(ContinuityMode){TickS6(DeltaTime);return;}
 if(OperationsMode){TickOperations(DeltaTime);return;}
 if(!View)return;
 if(HandlingMode&&CancelInputRequested.Exchange(false)){Handling.CancelInput();WriteHandlingEvidence();}
 if(AppBackground.Load())return;
 if(SkipNextDelta.Exchange(false))DeltaTime=0;
 // Startup loading and background suspension are outside the fixture clock.
 Warmup+=DeltaTime;if(!Started&&Warmup>=2){Started=true;DeltaTime=0;}
 if(Started&&!Simulation.Paused()){
  const auto R=HandlingMode?Handling.Update(Simulation,DeltaTime):Simulation.Update(DeltaTime,[](Rail::Simulation& S){
   if(S.Current().tick%3000==1500)S.SetCommandedSpeed(-8);
   if(S.Current().tick%3000==0)S.SetCommandedSpeed(8);
  });if(!R.ok){Status=TEXT("PAUSED: ")+FString(UTF8_TO_TCHAR(R.error.c_str()));UE_LOG(LogRailKernel,Warning,TEXT("%s"),*Status);}
 }
 if(!HandlingMode&&!ChangedChoice&&Simulation.Current().headStationMetres>250){Simulation.SetChoice({Rail::EdgeId(1),1},Rail::TransitionId(1));ChangedChoice=true;Status=TEXT("Tests PASS | future choice T1 lower; committed T2 remains | command motion only");}
 if(!HandlingMode&&!Recreated&&Simulation.Current().tick>=300){
  const auto Before=Simulation.Current();View->Destroy();CreateView();const auto& After=Simulation.Current();
  const bool Pass=Before.tick==After.tick&&Before.headStationMetres==After.headStationMetres&&Before.vehicles.size()==After.vehicles.size();
  const FString Evidence=FString::Printf(TEXT("{\"passed\":%s,\"tick\":%llu,\"headMetres\":%.17g,\"vehicles\":%d}"),Pass?TEXT("true"):TEXT("false"),After.tick,After.headStationMetres,int(After.vehicles.size()));
  FFileHelper::SaveStringToFile(Evidence,*(FPaths::ProjectSavedDir()/TEXT("S2/view-recreation.json")));UE_LOG(LogRailKernel,Display,TEXT("S2_VIEW_RECREATION %s"),*Evidence);Recreated=true;
 }
 Rail::Snapshot Presentation;if(Simulation.Presentation(Simulation.InterpolationAlpha(),Presentation).ok)View->Present(Simulation.Network(),Simulation.History(),Presentation,Status);
 if(HandlingMode&&Simulation.Current().tick>=LastEvidenceTick+30)WriteHandlingEvidence();
}
void URailSimulationSubsystem::ResetHandlingFixture(){
 if(ServiceMode){ServiceNotice=TEXT("Use Retry or Replay in the service result to start another run.");return;}
 if(ContinuityMode){Operations.InitializeAIFixture();S6Paused=true;S6Milestone=false;S6RecoveryBlocked=false;S6Recovery=TEXT("Explicit fixture reset; existing checkpoints retained");return;}
 if(OperationsMode){const auto R=AIMode?Operations.InitializeAIFixture():Operations.InitializeFixture();Status=R.ok?(AIMode?TEXT("S5 player + AI protected railway"):TEXT("S4 protected simulation fixture")):FString(UTF8_TO_TCHAR(R.error.c_str()));SelectedTrain=Rail::TrainId(1);LastEvidenceTick=0;Warmup=0;Started=false;Recreated=false;WriteOperationsEvidence();return;}
 if(!HandlingMode)return;
 auto Fixture=Rail::MakeFixture();Fixture.definitions.clear();
 for(const TCHAR* Path:{TEXT("/Game/IndianRailway/S2/DA_Powered.DA_Powered"),TEXT("/Game/IndianRailway/S2/DA_Coach.DA_Coach")}){
  auto* Asset=LoadObject<URailVehicleDefinition>(nullptr,Path);if(!Asset){Status=TEXT("FAIL missing S2 vehicle definition");return;}const auto Record=Asset->Record();Fixture.definitions.emplace(Record.id,Record);
 }
 Rail::HandlingDefinitions Definitions;
 for(const TCHAR* Path:{TEXT("/Game/IndianRailway/S3/DA_PoweredHandling.DA_PoweredHandling"),TEXT("/Game/IndianRailway/S3/DA_CoachHandling.DA_CoachHandling")}){
  auto* Asset=LoadObject<URailHandlingDefinition>(nullptr,Path);if(!Asset){Status=TEXT("FAIL missing S3 handling definition");return;}const auto Record=Asset->Record();Definitions.emplace(Record.id,Record);
 }
 auto Result=Simulation.Initialize(Fixture.graph,Fixture.definitions,Fixture.Long(),{Rail::EdgeId(1),1},200,Fixture.SelectUpper());
 if(Result.ok)Result=Handling.Initialize(Simulation,Fixture.definitions,Fixture.Long(),Definitions);
 Status=Result.ok?TEXT("S3 HANDLING | provisional parameters | upper path | finite fixture: RESET TEST at end"):TEXT("FAIL ")+FString(UTF8_TO_TCHAR(Result.error.c_str()));
 CommandSequence=0;LastEvidenceTick=0;Warmup=0;Started=false;WriteHandlingEvidence();
 UE_LOG(LogRailKernel,Display,TEXT("S3_RESET %s"),*Status);
}
Rail::Result URailSimulationSubsystem::SubmitPlayerCommand(Rail::CommandKind Kind,double Value){
 if(ServiceMode){if(S6Paused||S6RecoveryBlocked)return Rail::Result::Fail("Resume the service before driving");const auto R=Service.Input(Operations,Kind,Value);ServiceNotice=R.ok?TEXT(""):UTF8_TO_TCHAR(R.error.c_str());return R;}
 if(OperationsMode){const auto R=Operations.Command(SelectedTrain,Kind,Value);UE_LOG(LogRailKernel,Display,TEXT("S4_COMMAND T%llu kind%d value%.2f accepted%d reason%s"),SelectedTrain.value,int(Kind),Value,R.ok,UTF8_TO_TCHAR(R.error.c_str()));return R;}
 if(!HandlingMode)return Rail::Result::Fail("S3 map required");
 const Rail::TrainCommand Command{Simulation.Current().train,Handling.Source(),++CommandSequence,Simulation.Current().tick+1,Kind,Value};
 const auto Result=Handling.Submit(Simulation,Command);
 UE_LOG(LogRailKernel,Display,TEXT("S3_COMMAND seq=%llu applyTick=%llu kind=%d value=%.2f accepted=%d reason=%s"),Command.sequence,Command.applyTick,int(Kind),Value,Result.ok,UTF8_TO_TCHAR(Result.error.c_str()));
 return Result;
}
void URailSimulationSubsystem::ToggleHandlingPause(){if(ContinuityMode){if(S6RecoveryBlocked){S6Paused=true;return;}S6Paused=!S6Paused;SkipNextDelta.Store(true);if(S6Paused)SaveS6(TEXT("controlled-pause"));return;}if(OperationsMode){Operations.SetSuspended(!Operations.Suspended());WriteOperationsEvidence();return;}if(HandlingMode){Handling.SetSuspended(!Handling.Suspended());WriteHandlingEvidence();UE_LOG(LogRailKernel,Display,TEXT("S3_PAUSE %d"),Handling.Suspended());}}
void URailSimulationSubsystem::WriteHandlingEvidence(){
 const FString Json=UTF8_TO_TCHAR(Handling.Json(Simulation.Current()).c_str());
 FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("S3/telemetry.json")));
 LastEvidenceTick=Simulation.Current().tick;
}
FString URailSimulationSubsystem::HandlingDisplay() const {
 if(ContinuityMode)return S6Display();
 if(OperationsMode)return OperationsDisplay();
 const auto& H=Handling.Current();const auto& S=Simulation.Current();
 return FString::Printf(TEXT("S3 | Train %llu | v %+.2f m/s | a %+.3f m/s2 | tick %llu | head %.2f m\nPower %.0f%% (%.0f%% applied) | Brake %.0f%% (%.0f%% applied) | Reverser %+d | Emergency %s | %s\nTraction %+.0f N | Brake %+.0f N | Resistance %+.0f N | Grade %+.0f N | %d vehicles / %.0f kg / %.0f m | rejected %llu %s"),
  S.train.value,H.speedMps,H.accelerationMps2,S.tick,S.headStationMetres,H.command.throttle*100,H.appliedThrottle*100,H.command.brake*100,H.appliedBrake*100,H.command.direction,H.command.emergency?TEXT("ON"):TEXT("off"),
  Simulation.Paused()?TEXT("FAULT: RESET TEST"):Handling.Suspended()?TEXT("PAUSED"):H.held?TEXT("STOPPED"):TEXT("MOVING"),H.tractionN,H.brakingN,H.resistanceN,H.gradeN,int(S.properties.vehicleCount),H.massKg,S.properties.lengthMetres,H.rejectedCommands,UTF8_TO_TCHAR(H.lastRejection.c_str()));
}

void URailSimulationSubsystem::WriteOperationsEvidence(){
 const FString Json=UTF8_TO_TCHAR(Operations.Json().c_str());
 FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/(AIMode?TEXT("S5/telemetry.json"):TEXT("S4/telemetry.json"))));
 LastEvidenceTick=Operations.Current().tick;
}
void URailSimulationSubsystem::TickOperations(float DeltaTime){
 if(!View||!OperationsView)return;
 if(CancelInputRequested.Exchange(false)){Operations.CancelInput();WriteOperationsEvidence();}
 if(AppBackground.Load())return;
 if(SkipNextDelta.Exchange(false))DeltaTime=0;
 Warmup+=DeltaTime;if(!Started&&Warmup>=2){Started=true;DeltaTime=0;}
 if(Started&&!Operations.Faulted()){
  const auto R=Operations.Update(DeltaTime);
  if(AIMode){CSV_CUSTOM_STAT(RailS5,AIUpdateMs,Operations.Metrics().aiMilliseconds,ECsvCustomStatOp::Accumulate);CSV_CUSTOM_STAT(RailS5,SimulationStepMs,Operations.Metrics().stepMilliseconds,ECsvCustomStatOp::Accumulate);}if(!R.ok){Status=TEXT("S4 FAULT: ")+FString(UTF8_TO_TCHAR(R.error.c_str()));UE_LOG(LogRailKernel,Error,TEXT("%s"),*Status);WriteOperationsEvidence();}
 }
 const auto* A=Operations.Spatial(Rail::TrainId(1));const auto* B=Operations.Spatial(Rail::TrainId(2));if(!A||!B)return;
 Rail::Snapshot Combined=A->Current();Combined.vehicles.insert(Combined.vehicles.end(),B->Current().vehicles.begin(),B->Current().vehicles.end());
 View->Present(A->Network(),A->History(),Combined,Status);
 OperationsView->Present(A->Network(),Operations.DefinitionsView(),Operations.Current());
 if(!Recreated&&Operations.Current().tick>=300){RecreateOperationsView();Recreated=true;}
 if(Operations.Current().tick>=LastEvidenceTick+30)WriteOperationsEvidence();
}
void URailSimulationSubsystem::SelectTrain(uint64 Id){if(OperationsMode&&Operations.Spatial(Rail::TrainId(Id))){SelectedTrain=Rail::TrainId(Id);UE_LOG(LogRailKernel,Display,TEXT("S4_SELECT T%llu"),Id);}}
void URailSimulationSubsystem::RequestPlayerRoute(uint64 Id){
 if(!OperationsMode)return;const auto R=Operations.RequestRoute(SelectedTrain,Rail::RouteId(Id));
 UE_LOG(LogRailKernel,Display,TEXT("S4_ROUTE_REQUEST T%llu R%llu accepted%d reason%s"),SelectedTrain.value,Id,R.ok,UTF8_TO_TCHAR(R.error.c_str()));
}
void URailSimulationSubsystem::RequestPlayerPoint(){if(OperationsMode)Operations.RequestPoint(SelectedTrain,Rail::PointId(2),Operations.Current().points.at(Rail::PointId(2)).position==0?1:0);}
void URailSimulationSubsystem::CancelPlayerRoute(){
 if(!OperationsMode)return;const auto A=Operations.Current().authorities.find(SelectedTrain);
 if(A!=Operations.Current().authorities.end()&&A->second.routeGranted)Operations.CancelRoute(SelectedTrain,A->second.owner.route,A->second.owner.generation);
}
void URailSimulationSubsystem::RecreateOperationsView(){
 if(ContinuityMode){CycleS6Chunks();return;}
 if(!OperationsMode||!OperationsView)return;
 const auto* S=Operations.Spatial(Rail::TrainId(1));if(!S)return;
 OperationsView->Present(S->Network(),Operations.DefinitionsView(),Operations.Current());
 const FString Before=UTF8_TO_TCHAR(Operations.Json().c_str()),Digest=OperationsView->PresentationDigest();
 const uint32 OldId=OperationsView->GetUniqueID();const bool Destroyed=OperationsView->Destroy();
 OperationsView=GetWorld()->SpawnActor<ARailOperationsView>();
 if(!OperationsView){UE_LOG(LogRailKernel,Error,TEXT("S4_VIEW_RECREATION spawn failed"));return;}
 OperationsView->Present(S->Network(),Operations.DefinitionsView(),Operations.Current());
 bool TrainViewPass=true;uint32 OldTrainViewId=0,NewTrainViewId=0;
 if(AIMode){OldTrainViewId=View->GetUniqueID();const bool TrainDestroyed=View->Destroy();CreateView();NewTrainViewId=View?View->GetUniqueID():0;
  TrainViewPass=TrainDestroyed&&NewTrainViewId&&OldTrainViewId!=NewTrainViewId;
  if(View){Rail::Snapshot Combined=S->Current();const auto* B=Operations.Spatial(Rail::TrainId(2));if(B)Combined.vehicles.insert(Combined.vehicles.end(),B->Current().vehicles.begin(),B->Current().vehicles.end());View->Present(S->Network(),S->History(),Combined,Status);}}
 const bool Pass=TrainViewPass&&Destroyed&&OldId!=OperationsView->GetUniqueID()&&Before==UTF8_TO_TCHAR(Operations.Json().c_str())&&Digest==OperationsView->PresentationDigest();
 const FString Evidence=FString::Printf(TEXT("{\"passed\":%s,\"destroyReturned\":%s,\"oldActorId\":%u,\"newActorId\":%u,\"tick\":%llu,\"presentationEqual\":%s,\"logicalBefore\":%s,\"logicalAfter\":%s}"),Pass?TEXT("true"):TEXT("false"),Destroyed?TEXT("true"):TEXT("false"),OldId,OperationsView->GetUniqueID(),Operations.Current().tick,Digest==OperationsView->PresentationDigest()?TEXT("true"):TEXT("false"),*Before,UTF8_TO_TCHAR(Operations.Json().c_str()));
 if(AIMode){const FString TrainEvidence=FString::Printf(TEXT("{\"passed\":%s,\"oldActorId\":%u,\"newActorId\":%u,\"logicalBefore\":%s,\"logicalAfter\":%s}"),Pass?TEXT("true"):TEXT("false"),OldTrainViewId,NewTrainViewId,*Before,UTF8_TO_TCHAR(Operations.Json().c_str()));FFileHelper::SaveStringToFile(TrainEvidence,*(FPaths::ProjectSavedDir()/TEXT("S5/train-view-recreation.json")));}
 FFileHelper::SaveStringToFile(Evidence,*(FPaths::ProjectSavedDir()/(AIMode?TEXT("S5/view-recreation.json"):TEXT("S4/view-recreation.json"))));
 UE_LOG(LogRailKernel,Display,TEXT("S4_VIEW_RECREATION %s tick%llu"),Pass?TEXT("PASS"):TEXT("FAIL"),Operations.Current().tick);
}
FString URailSimulationSubsystem::OperationsDisplay() const {
 const auto* S=Operations.Spatial(SelectedTrain);const auto* H=Operations.Handling(SelectedTrain);if(!S||!H)return TEXT("S4 missing train state");
 const auto& State=Operations.Current();const auto A=State.authorities.find(SelectedTrain);
 FString Text=FString::Printf(TEXT("S4 protected simulation fixture | T%llu E%llu %.1fm | v %+.2fm/s | tick %llu | %s\nPower %.0f%% Brake %.0f%% dir%+d | "),SelectedTrain.value,S->Current().head.edge.value,S->Current().head.distanceMetres,H->speedMps,State.tick,Operations.Faulted()?TEXT("FAULT - RESET"):Operations.Suspended()?TEXT("PAUSED"):H->held?TEXT("STOPPED"):TEXT("MOVING"),H->command.throttle*100,H->command.brake*100,H->command.direction);
 if(A!=State.authorities.end())Text+=FString::Printf(TEXT("R%llu owner T%llu | limit E%llu %.1fm | %s\n"),A->second.owner.route.value,A->second.owner.train.value,A->second.endpoint.edge.value,A->second.endpoint.distanceMetres,A->second.routeGranted?TEXT("ROUTE GRANTED"):TEXT("HOLDING AREA ONLY"));
 Text+=TEXT("Blocks orange=occupied purple=reserved: ");for(const auto& B:State.blocks)Text+=FString::Printf(TEXT("B%llu[%llu/R%llu] "),B.first.value,B.second.occupants.empty()?0:B.second.occupants.begin()->value,B.second.reservation.route.value);
 Text+=TEXT("\n");for(const auto& P:State.points)Text+=FString::Printf(TEXT("P%llu pos%d lockR%llu | "),P.first.value,P.second.position,P.second.lock.route.value);
 for(const auto& Signal:State.signals)Text+=FString::Printf(TEXT("S%llu %s | "),Signal.first.value,Signal.second.aspect==Rail::SignalAspect::Proceed?TEXT("PROCEED"):TEXT("STOP"));
 Text+=TEXT("\n");const auto Request=State.lastRequest.find(SelectedTrain);if(Request!=State.lastRequest.end())Text+=FString::Printf(TEXT("Request R%llu %s %s | "),Request->second.request.route.value,Request->second.granted?TEXT("OK"):TEXT("DENIED"),UTF8_TO_TCHAR(Request->second.reason.c_str()));
 if(!State.events.empty())Text+=UTF8_TO_TCHAR(State.events.back().c_str());if(Operations.Faulted())Text+=TEXT("\n")+FString(UTF8_TO_TCHAR(Operations.Error().c_str()));
 if(AIMode){Text.ReplaceInline(TEXT("S4 protected simulation fixture"),TEXT("S5 player + AI railway"));const auto* Driver=Operations.AI(Rail::TrainId(2));
  if(Driver)Text+=FString::Printf(TEXT("\nAI B %s | target %.1fm/s | %s"),UTF8_TO_TCHAR(Rail::AIPhaseName(Driver->Phase())),Driver->TargetSpeed(),UTF8_TO_TCHAR(Driver->Reason().c_str()));}
 return Text;
}

