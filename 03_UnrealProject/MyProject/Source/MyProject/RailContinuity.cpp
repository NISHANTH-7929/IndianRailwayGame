#include "RailSimulationSubsystem.h"
#include "RailReviewPolicy.h"
#include "Simulation/RailEgmoreService.h"
#include "Simulation/RailTambaramService.h"
#include "RailChunkView.h"
#include "RailTambaramCorridorView.h"
#include "RailTambaramTurnoutView.h"
#include "RailTambaramStationView.h"
#include "RailEgmoreSceneView.h"
#include "RailDrivingWidget.h"
#include "RailServiceWidget.h"
#include "RailServiceDefinition.h"
#include "RailServiceView.h"
#include "RailLocomotiveView.h"
#include "EngineUtils.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Simulation/RailCheckpointStore.h"
#include "Engine/LevelStreamingDynamic.h"
#include "Engine/Level.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CoreDelegates.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundWave.h"
CSV_DEFINE_CATEGORY(RailS6,true);
CSV_DEFINE_CATEGORY(RailS7,true);
DEFINE_LOG_CATEGORY_STATIC(LogRailContinuity,Log,All);
namespace {
Rail::ReviewPolicy ReviewPolicy(){
#if !UE_BUILD_SHIPPING
 return Rail::ReviewPolicy::FromFlags(true,
  FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview")),
  FParse::Param(FCommandLine::Get(),TEXT("S8Egmore"))||FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram")),
  FParse::Param(FCommandLine::Get(),TEXT("S8AssetReviewResume")));
#else
 return {};
#endif
}
bool AssetReview(){
#if !UE_BUILD_SHIPPING
 return FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview"));
#else
 return false;
#endif
}
bool EgmoreContent(){return FParse::Param(FCommandLine::Get(),TEXT("S8Egmore"));}
bool TambaramContent(){return FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram"));}
bool LhbContent(){return EgmoreContent()||TambaramContent()||FParse::Param(FCommandLine::Get(),TEXT("S8LHB"));}
FString Directory(bool ServiceStage=false){
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
 if(TambaramContent()&&AssetReview()){
  FString ReviewRoot;
  if(FParse::Value(FCommandLine::Get(),TEXT("TambaramEvidence="),ReviewRoot)&&!FPaths::IsRelative(ReviewRoot))
   return ReviewRoot+TEXT("-isolated-saves");
 }
#endif
 // New traffic content uses separate slots; retain prior ServiceV5 saves intact.
 if(TambaramContent()&&FParse::Param(FCommandLine::Get(),TEXT("S9LiveJourney"))&&!AssetReview())return FPaths::ProjectSavedDir()/TEXT("S9/EgmoreTambaram/LiveServiceV1");
 if(TambaramContent()&&!AssetReview())return FPaths::ProjectSavedDir()/TEXT("S8/EgmoreTambaram/ServiceV6");
 if(AssetReview()&&FParse::Param(FCommandLine::Get(),TEXT("S8AudioReview")))return FPaths::ProjectSavedDir()/TEXT("S8/AudioReview");if(TambaramContent())return FPaths::ProjectSavedDir()/(AssetReview()?TEXT("S8/EgmoreTambaram/AssetReview"):TEXT("S8/EgmoreTambaram/ServiceV5"));if(EgmoreContent())return FPaths::ProjectSavedDir()/(AssetReview()?TEXT("S8/Egmore/AssetReview"):TEXT("S8/Egmore/Service"));if(FParse::Param(FCommandLine::Get(),TEXT("S8Station")))return FPaths::ProjectSavedDir()/(AssetReview()?TEXT("S8/Station/AssetReview"):TEXT("S8/Station/Service"));if(LhbContent())return FPaths::ProjectSavedDir()/(AssetReview()?TEXT("S8/LHB/AssetReview"):TEXT("S8/LHB/Service"));if(AssetReview())return FPaths::ProjectSavedDir()/TEXT("S8/AssetReview");return FPaths::ProjectSavedDir()/(ServiceStage?TEXT("S7"):TEXT("S6"));}
FString Slot(unsigned Index,bool ServiceStage=false){return Directory(ServiceStage)/FString::Printf(TEXT("checkpoint-%u.irs"),Index);}
Rail::CheckpointIO Files(bool ServiceStage=false){return {
 [ServiceStage](unsigned Index){const int64 Size=IFileManager::Get().FileSize(*Slot(Index,ServiceStage));if(Size<0)return std::vector<uint8_t>{};
  if(Size==0||Size>static_cast<int64>(Rail::SnapshotRecord::MaximumBytes))return std::vector<uint8_t>{0};
  TArray<uint8> Bytes;if(!FFileHelper::LoadFileToArray(Bytes,*Slot(Index,ServiceStage))||Bytes.IsEmpty())return std::vector<uint8_t>{0};return std::vector<uint8_t>(Bytes.GetData(),Bytes.GetData()+Bytes.Num());},
 [ServiceStage](unsigned Index,const std::vector<uint8_t>& Bytes){
  TUniquePtr<FArchive> File(IFileManager::Get().CreateFileWriter(*Slot(Index,ServiceStage)));
  if(!File)return Rail::Result::Fail("Cannot open checkpoint slot");
  File->Serialize(const_cast<uint8_t*>(Bytes.data()),static_cast<int64>(Bytes.size()));File->Flush();
  const bool Error=File->IsError();const bool Closed=File->Close();
  return !Error&&Closed?Rail::Result{}:Rail::Result::Fail("Checkpoint write/flush failed");}
 };}
FString Escaped(FString Value){Value.ReplaceInline(TEXT("\\"),TEXT("\\\\"));Value.ReplaceInline(TEXT("\""),TEXT("\\\""));Value.ReplaceInline(TEXT("\n"),TEXT("\\n"));Value.ReplaceInline(TEXT("\r"),TEXT("\\r"));return Value;}
FString LogicalFingerprint(const Rail::OperationsWorld& World,const Rail::ServiceRuntime* Service=nullptr){
 Rail::SnapshotRecord Record;const auto Result=Service?Service->Capture(World,1,Record):Rail::WorldSnapshotCodec::Capture(World,1,Record);if(!Result.ok)return TEXT("INVALID");
 uint64 Hash=14695981039346656037ull;for(auto Byte:Record.Bytes()){Hash^=Byte;Hash*=1099511628211ull;}
 return FString::Printf(TEXT("%016llx"),Hash);
}
}
void URailSimulationSubsystem::InitializeS6(){
 ContinuityMode=OperationsMode=AIMode=Active=true;S6Paused=true;S6Levels.SetNum(2);
 IFileManager::Get().MakeDirectory(*Directory(ServiceMode),true);
 bool Passed=true;
#if !UE_BUILD_SHIPPING
 const Rail::TestReport Tests[]={Rail::RunKernelTests(),Rail::RunHandlingTests(),Rail::RunOperationsTests(),Rail::RunAITests(),Rail::RunContentTests(),Rail::RunSnapshotRecordTests(),Rail::RunWorldSnapshotTests(),Rail::RunCheckpointTests()};
 const TCHAR* Names[]={TEXT("kernel"),TEXT("handling"),TEXT("operations"),TEXT("ai"),TEXT("content"),TEXT("record"),TEXT("world-snapshot"),TEXT("checkpoint")};
 for(int32 I=0;I<8;++I){Passed&=Tests[I].Passed();FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Tests[I].Json().c_str()),*(Directory(ServiceMode)/(FString(Names[I])+TEXT("-tests.json"))));}
 if(LhbContent()){const auto Test=Rail::RunLhbTests();Passed&=Test.Passed();FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Test.Json().c_str()),*(Directory(ServiceMode)/TEXT("lhb-tests.json")));}
 if(ServiceMode){const auto Test=Rail::RunServiceTests();Passed&=Test.Passed();FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Test.Json().c_str()),*(Directory(true)/TEXT("service-tests.json")));}
#endif
 // Runtime content validation and protected operations remain enabled in Shipping.
 if(ServiceMode){
  const auto* Asset=LoadObject<URailServiceDefinition>(nullptr,TEXT("/Game/IndianRailway/S7/DA_Shantipur.DA_Shantipur"));if(!Asset||!Asset->Record().Validate().ok){Active=false;return;}
  auto Definition=Asset->Record();if(TambaramContent())Definition=FParse::Param(FCommandLine::Get(),TEXT("S9LiveJourney"))?Rail::MakeLiveTambaramService():Rail::MakeEgmoreTambaramService();else if(EgmoreContent())Definition=Rail::MakeEgmoreService();else if(LhbContent()){Definition.contentId=Rail::FormationContent::LhbId;Definition.berthHead=Rail::FormationContent::ServiceBerthHead;Definition.id="S8-Shantipur-LHB-01";Definition.version="IR-S8-LHB-service-1";Definition.formation="WAP-7 presentation + LHB AC 3-tier";}
  if(!Definition.Validate().ok){Active=false;return;}Service=Rail::ServiceRuntime(Definition);if(!Operations.InitializeContent(Definition.contentId).ok){Active=false;return;}SelectedTrain=Service.Definition().player;
  FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Service.Definition().Json().c_str()),*(Directory(true)/TEXT("service-definition.json")));
 }else Operations.InitializeAIFixture();
 auto Policy=ReviewPolicy();Policy.resumeRequired=Policy.resumeRequired&&ServiceMode;
 Rail::RecoverySession Restored;Rail::RecoverySelection Selected;const auto Recovery=Policy.Fresh()?Rail::Result::Fail("Fresh isolated asset review"):ServiceMode?Service.Recover(Files(true),Operations,Selected):Rail::RecoverCheckpoint(Files(),Restored,Selected);
 if(Recovery.ok){if(!ServiceMode)Operations=Restored.World();S6Generation=Selected.record.Generation();S6Recovery=FString::Printf(TEXT("Restored generation %llu paused"),S6Generation);}
 else S6Recovery=UTF8_TO_TCHAR(Recovery.error.c_str());
 S6RecoveryBlocked=Policy.RecoveryBlocked(Recovery.ok,IFileManager::Get().FileExists(*Slot(0,ServiceMode))||IFileManager::Get().FileExists(*Slot(1,ServiceMode)));
 if(Policy.resumeRequired)UE_LOG(LogRailContinuity,Display,TEXT("S8_ASSET_REVIEW_RESUME isolated=1 recovered=%d blocked=%d generation=%llu"),Recovery.ok,S6RecoveryBlocked,S6Generation);
 for(const auto& Diagnostic:Selected.diagnostics)S6Recovery+=TEXT(" | ")+FString(UTF8_TO_TCHAR(Diagnostic.c_str()));
 if(!Passed){Status=TEXT("S6 startup tests failed");Active=false;return;}
 FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&URailSimulationSubsystem::SuspendFixture);
 FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&URailSimulationSubsystem::SuspendFixture);
 FCoreDelegates::ApplicationHasEnteredForegroundDelegate.AddUObject(this,&URailSimulationSubsystem::ResumeFixture);
 FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this,&URailSimulationSubsystem::ResumeFixture);
 if(AssetReview()&&FParse::Param(FCommandLine::Get(),TEXT("S8LowLight"))){
  for(TActorIterator<ADirectionalLight> It(GetWorld());It;++It)It->GetLightComponent()->SetIntensity(.06f);
  for(TActorIterator<ASkyLight> It(GetWorld());It;++It)It->GetLightComponent()->SetIntensity(.12f);
 }
 S6Camera=GetWorld()->SpawnActor<ACameraActor>();
 S6Camera->SetActorLocation(FVector(-5000,-26000,30000));S6Camera->SetActorRotation((FVector(-5000,3000,0)-S6Camera->GetActorLocation()).Rotation());S6Camera->GetCameraComponent()->SetFieldOfView(45);
 if(auto* PC=GetWorld()->GetFirstPlayerController()){PC->SetViewTarget(S6Camera);PC->SetVirtualJoystickVisibility(false);if(ServiceMode){ServiceWidget=CreateWidget<URailServiceWidget>(PC);ServiceWidget->AddToViewport();ServiceView=GetWorld()->SpawnActor<ARailServiceView>();ServiceView->Configure(Service.Definition());}else{DrivingWidget=CreateWidget<URailDrivingWidget>(PC);DrivingWidget->AddToViewport();}PC->SetInputMode(FInputModeGameAndUI());PC->bShowMouseCursor=true;}
 if(!FParse::Param(FCommandLine::Get(),TEXT("S8DisableLocomotive")))LocomotiveView=GetWorld()->SpawnActor<ARailLocomotiveView>();
 if(LocomotiveView){FString Diagnostic,ClassId=TEXT("WAP7");FParse::Value(FCommandLine::Get(),TEXT("S8LocoClass="),ClassId);if(LocomotiveView->LoadPresentation(ClassId,Diagnostic)){if(FParse::Param(FCommandLine::Get(),TEXT("S8Cab")))LocomotiveView->ToggleCamera();if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetViewTarget(LocomotiveView);}else UE_LOG(LogRailContinuity,Warning,TEXT("S8 %s; retaining proxy"),*Diagnostic);}
 if(EgmoreContent()||TambaramContent()){EgmoreTrafficLocomotive=GetWorld()->SpawnActor<ARailLocomotiveView>();FString Diagnostic;if(EgmoreTrafficLocomotive&&!EgmoreTrafficLocomotive->LoadPresentation(TEXT("WAP7"),Diagnostic)){Active=false;return;}}
 if(TambaramContent()&&Operations.Spatial(Rail::TrainId(3))){OpposingLocomotive=GetWorld()->SpawnActor<ARailLocomotiveView>();FString Diagnostic;if(!OpposingLocomotive||!OpposingLocomotive->LoadPresentation(TEXT("WAP7"),Diagnostic)){Active=false;return;}}
 for(uint64 Id=4;Id<=Rail::ScenarioContent::TrainCount(Operations.ContentId());++Id){auto* TrafficView=GetWorld()->SpawnActor<ARailLocomotiveView>();FString Diagnostic;if(!TrafficView||!TrafficView->LoadPresentation(TEXT("WAP7"),Diagnostic)){Active=false;return;}AdditionalTrafficLocomotives.Add(TrafficView);}

 if(Policy.Fresh()&&ServiceMode){
  const auto A=Service.Select();const auto B=Service.ConfirmFormation();const auto C=Service.Start(Operations);
  if(A.ok&&B.ok&&C.ok){S6Paused=false;S6Generation=0;SubmitPlayerCommand(Rail::CommandKind::Brake,0);SubmitPlayerCommand(Rail::CommandKind::Throttle,TambaramContent()?1:.35);}
  UE_LOG(LogRailContinuity,Display,TEXT("S8_ASSET_REVIEW isolated=1 started=%d"),A.ok&&B.ok&&C.ok);
 }
 WriteS6Evidence();
}
void URailSimulationSubsystem::SaveS6(const FString& Reason){
 CSV_SCOPED_TIMING_STAT(RailS7,CheckpointMs);
 if(ServiceMode&&Service.Current().accumulator>4.0/30.0+1e-9){S6SaveStatus=TEXT("Checkpoint retained: restore after frame overload before saving");WriteS6Evidence();return;}
 if(ReviewPolicy().resumeRequired&&S6RecoveryBlocked){S6SaveStatus=TEXT("Save refused: isolated recovery blocked; checkpoint files retained");WriteS6Evidence();return;}
 const double Begin=FPlatformTime::Seconds();Rail::CheckpointCommit Commit;
 const auto Result=ServiceMode?Service.Save(Operations,Files(true),Commit):Rail::CommitCheckpoint(Operations,Files(),Commit);
 S6SaveMilliseconds=(FPlatformTime::Seconds()-Begin)*1000;SkipNextDelta.Store(true);
 if(Result.ok){S6Generation=Commit.generation;S6SaveStatus=Reason+TEXT(" verified");}
 else S6SaveStatus=Reason+TEXT(": ")+FString(UTF8_TO_TCHAR(Result.error.c_str()));
 UE_LOG(LogRailContinuity,Display,TEXT("S6_SAVE generation=%llu tick=%llu ms=%.3f %s"),S6Generation,Operations.Current().tick,S6SaveMilliseconds,*S6SaveStatus);
 WriteS6Evidence();
 if(Result.ok)IFileManager::Get().Copy(*(Directory(ServiceMode)/TEXT("last-checkpoint.json")),*(Directory(ServiceMode)/TEXT("telemetry.json")));
}
ARailLocomotiveView* URailSimulationSubsystem::PlayerLocomotive() const {return LocomotiveView.Get();}
void URailSimulationSubsystem::SaveS6Checkpoint(){if(ContinuityMode)SaveS6(TEXT("explicit-checkpoint"));}
void URailSimulationSubsystem::SuspendS6(){
 AppBackground.Store(true);CancelInputRequested.Store(true);
 // Android delegates normally dispatch on the game thread. If a platform calls
 // elsewhere, defer capture to that thread instead of racing mutable state.
 if(IsInGameThread()){CancelInputRequested.Store(false);S6Paused=true;StopServiceAudio();SaveS6(TEXT("background"));}
}
void URailSimulationSubsystem::ToggleS6LookBack(){
 if(!ContinuityMode)return;S6LookBack=!S6LookBack;
 // Tick rebinds either orientation to the current complete formation, so the
 // camera cannot remain behind at a historical train location after movement.
 WriteS6Evidence();
}
void URailSimulationSubsystem::CycleS6Chunks(){
 if(!ContinuityMode)return;S6Paused=true;WriteS6Evidence();
 FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Operations.Json().c_str()),*(Directory(ServiceMode)/TEXT("before-unload.json")));
 for(int32 I=0;I<2;++I)if(S6Levels[I]){
  if(auto* Level=S6Levels[I]->GetLoadedLevel())for(const auto& Actor:Level->Actors)if(auto* Chunk=Cast<ARailChunkView>(Actor)){S6RetiredViews.AddUnique(Chunk);S6RetiredMeshes.AddUnique(Chunk->Bodies->GetStaticMesh());for(const auto& Part:Chunk->CoachParts)if(Part)S6RetiredMeshes.AddUnique(Part->GetStaticMesh());for(const auto& Part:Chunk->StationParts)if(Part)S6RetiredMeshes.AddUnique(Part->GetStaticMesh());}
  S6Levels[I]->SetShouldBeVisible(false);S6Levels[I]->SetShouldBeLoaded(false);S6Ready[I]=false;}
 S6ReloadAfter=FPlatformTime::Seconds()+3;S6Blocked=true;
}
void URailSimulationSubsystem::TickS6(float DeltaTime){
 CSV_SCOPED_TIMING_STAT(RailS7,ContinuityMs);
 // Record intent before presentation work so loading stalls during travel stay
 // in the driving sample. Exclude menus, explicit pause, results and background.
 const bool DrivingFrame=ServiceMode&&Service.Current().phase==Rail::ServicePhase::Running&&!S6Paused&&!AppBackground.Load();
 CSV_CUSTOM_STAT(RailS7,DrivingFrame,DrivingFrame?1:0,ECsvCustomStatOp::Set);
 CSV_CUSTOM_STAT(RailS7,PresentationBlocked,0,ECsvCustomStatOp::Set);
 CSV_CUSTOM_STAT(RailS7,PresentationTransition,0,ECsvCustomStatOp::Set);
 if(CancelInputRequested.Exchange(false)){S6Paused=true;StopServiceAudio();SaveS6(TEXT("deferred-background"));}
 if(AppBackground.Load())return;
 const double Now=FPlatformTime::Seconds();
 const auto PlayerId=ServiceMode?Service.Definition().player:Rail::TrainId(1);const auto* Player=Operations.Spatial(PlayerId);if(!Player)return;
 std::vector<Rail::Vec3> Ahead;const auto& State=Player->Current();
 if(!State.vehicles.empty()){
  const auto Centre=(State.vehicles.front().bodyCentre+State.vehicles.back().bodyCentre)*.5;
  const FVector Focus(Centre.x*100,Centre.y*100,-5000);
  if(S6Camera){S6Camera->SetActorLocation(Focus+FVector(0,S6LookBack?26000:-26000,35000));S6Camera->SetActorRotation((Focus-S6Camera->GetActorLocation()).Rotation());}
  // The 45-degree inspection camera's railway viewing band is included in
  // demand, in addition to the train footprint, route ahead and overlap margin.
  Ahead.push_back({Centre.x-160,Centre.y,0});Ahead.push_back({Centre.x+160,Centre.y,0});
 }
 const double Speed=Operations.Handling(PlayerId)->speedMps;
 const double Horizon=FMath::Max(40.0,FMath::Abs(Speed)*8.0);
 for(double Distance=0;Distance<=Horizon;Distance+=5){Rail::Sample Sample;if(Player->History().SampleAt(Player->Network(),State.headStationMetres+(Speed<0?-Distance:Distance),Sample).ok)Ahead.push_back(Sample.position);}
 const auto Demand=Rail::RequiredPresentation(State,Speed,S6LookBack,Ahead);
 std::set<unsigned> Loaded;bool Transition=false;int32 Views=0;
 for(int32 I=0;I<2;++I){auto* Level=S6Levels[I].Get();const bool Wanted=Demand.chunks.count(static_cast<unsigned>(I+1))&&Now>=S6ReloadAfter;
  if(Level&&!Level->ShouldBeLoaded()&&!Level->IsLevelLoaded()){
   Level->SetIsRequestingUnloadAndRemoval(true);S6Levels[I]=nullptr;Level=nullptr;++S6Unloads;Transition=true;
   GEngine->ForceGarbageCollection(true);SkipNextDelta.Store(true);
  }
  if(Wanted&&!Level){bool Success=false;Level=ULevelStreamingDynamic::LoadLevelInstance(this,EgmoreContent()?(I==0?TEXT("/Game/IndianRailway/S8/Egmore/L_EgmoreChunkA"):TEXT("/Game/IndianRailway/S8/Egmore/L_EgmoreChunkB")):UTF8_TO_TCHAR(Rail::ContentManifest::Chunks()[static_cast<size_t>(I)].level),FVector::ZeroVector,FRotator::ZeroRotator,Success);
   S6Levels[I]=Level;S6LoadStarted[I]=Now;S6Ready[I]=false;++S6Loads;Transition=true;
   if(!Success)S6StreamStatus=TEXT("Essential chunk request failed; simulation paused");}
  if(Level&&!Wanted&&Level->ShouldBeLoaded()){
   if(auto* LoadedLevel=Level->GetLoadedLevel())for(const auto& Actor:LoadedLevel->Actors)if(auto* Chunk=Cast<ARailChunkView>(Actor)){S6RetiredViews.AddUnique(Chunk);S6RetiredMeshes.AddUnique(Chunk->Bodies->GetStaticMesh());for(const auto& Part:Chunk->CoachParts)if(Part)S6RetiredMeshes.AddUnique(Part->GetStaticMesh());for(const auto& Part:Chunk->StationParts)if(Part)S6RetiredMeshes.AddUnique(Part->GetStaticMesh());}
   Level->SetShouldBeVisible(false);Level->SetShouldBeLoaded(false);S6Ready[I]=false;Transition=true;}
  if(Level&&Level->IsLevelVisible()&&Level->GetLoadedLevel()&&Wanted){
   // Present may create child scenery actors in this level: iterate a snapshot.
   const auto LevelActors=Level->GetLoadedLevel()->Actors;
   uint64 BeforeCells=0;for(const auto& Actor:LevelActors)if(auto* CellView=Cast<ARailChunkView>(Actor)){if(CellView->TambaramCorridor)BeforeCells+=CellView->TambaramCorridor->RebuildCount;if(CellView->TambaramStation)BeforeCells+=CellView->TambaramStation->RebuildCount;}
   bool Bound=false;for(const auto& Actor:LevelActors)if(auto* ViewActor=Cast<ARailChunkView>(Actor)){ViewActor->Present(Operations,!ServiceMode||ServiceDebug,HasLocomotivePresentation()?(ServiceMode?Service.Definition().player.value:1ull):0ull);Bound=ViewActor->Bodies&&ViewActor->Scenery&&ViewActor->Bodies->GetStaticMesh()&&ViewActor->Scenery->GetStaticMesh();if(LhbContent()&&!FParse::Param(FCommandLine::Get(),TEXT("S8DisableCoach")))Bound=Bound&&ViewActor->CoachParts.Num()==7;if(TambaramContent())Bound=Bound&&ViewActor->TambaramReady;if(FParse::Param(FCommandLine::Get(),TEXT("S8Station"))&&!EgmoreContent()&&!TambaramContent())Bound=Bound&&!ViewActor->StationParts.IsEmpty();++Views;}
   uint64 AfterCells=0;for(const auto& Actor:LevelActors)if(auto* CellView=Cast<ARailChunkView>(Actor)){if(CellView->TambaramCorridor)AfterCells+=CellView->TambaramCorridor->RebuildCount;if(CellView->TambaramStation)AfterCells+=CellView->TambaramStation->RebuildCount;}
   if(AfterCells!=BeforeCells)Transition=true;
   if(Bound){Loaded.insert(static_cast<unsigned>(I+1));if(!S6Ready[I]){S6Ready[I]=true;S6LoadMilliseconds[I]=(Now-S6LoadStarted[I])*1000;Transition=true;}}
  }
 }
 bool EgmoreReady=true;
 if(EgmoreContent()){
  if(!IsValid(EgmoreScene)){TActorIterator<ARailEgmoreSceneView> It(GetWorld());if(It)EgmoreScene=*It;}
  EgmoreReady=IsValid(EgmoreScene)&&EgmoreScene->PresentSignals(Operations)&&EgmoreScene->IsServiceReady()&&HasLocomotivePresentation()&&IsValid(EgmoreTrafficLocomotive)&&EgmoreTrafficLocomotive->IsReady();
 }
 const bool Blocked=!EgmoreReady||!Rail::EssentialPresentationReady(Demand,Loaded)||Now<S6ReloadAfter;
 const bool SkipResumeDelta=SkipNextDelta.Exchange(false);
 // This frame may synchronously finish presentation loading. Its elapsed cost
 // arrives as the NEXT frame's delta and is not elapsed simulation time.
 if(Blocked||Transition||S6Blocked){DeltaTime=0;SkipNextDelta.Store(true);}
 else if(SkipResumeDelta)DeltaTime=0;
 S6Blocked=Blocked;S6StreamStatus=Blocked?TEXT("Waiting for essential presentation; logical permission unchanged"):TEXT("Essential presentation ready");
 CSV_CUSTOM_STAT(RailS7,PresentationBlocked,Blocked?1:0,ECsvCustomStatOp::Set);
 CSV_CUSTOM_STAT(RailS7,PresentationTransition,Transition?1:0,ECsvCustomStatOp::Set);
 if(!Blocked&&!S6Paused&&!S6RecoveryBlocked&&!Operations.Faulted()){
  const auto BeforePhase=Service.Current().phase;const auto BeforeStation=Service.Current().station;
  const double ApplicationBegin=FPlatformTime::Seconds();
  const auto Result=ServiceMode?Service.Update(Operations,DeltaTime):Operations.Update(DeltaTime);if(!Result.ok){S6Paused=true;S6StreamStatus=UTF8_TO_TCHAR(Result.error.c_str());}
  if(ServiceMode){CSV_CUSTOM_STAT(RailS7,ServiceUpdateMs,(FPlatformTime::Seconds()-ApplicationBegin)*1000,ECsvCustomStatOp::Accumulate);}
  if(ServiceMode&&(Service.Current().phase!=BeforePhase||Service.Current().station!=BeforeStation||Operations.Current().tick>=ServiceLastSaveTick+300)){
   if(Service.Current().phase==Rail::ServicePhase::Result)S6Paused=true;SaveS6(TEXT("service-checkpoint"));ServiceLastSaveTick=Operations.Current().tick;
  }
  CSV_CUSTOM_STAT(RailS6,SimulationStepMs,Operations.Metrics().stepMilliseconds,ECsvCustomStatOp::Accumulate);
  CSV_CUSTOM_STAT(RailS6,AIUpdateMs,Operations.Metrics().aiMilliseconds,ECsvCustomStatOp::Accumulate);
 }
 // Update replaces the candidate state; reacquire train pointers after commit.
 if(!ServiceMode&&!S6Milestone&&Operations.Spatial(Rail::TrainId(1))->Current().headStationMetres>=240){S6Milestone=true;SaveS6(TEXT("boundary-milestone"));}
 if(AssetReview()&&ServiceMode&&!EgmoreContent()&&!TambaramContent()&&!S6Blocked&&!S6Paused&&!FParse::Param(FCommandLine::Get(),TEXT("S8CabManual"))){
  S8ReviewSeconds+=DeltaTime;const bool CabReview=FParse::Param(FCommandLine::Get(),TEXT("S8CabReview"));const int32 Phase=int32(S8ReviewSeconds/(CabReview?6:15));
  if(Phase!=S8ReviewPhase){S8ReviewPhase=Phase;
   if(CabReview){
    auto Command=[&](Rail::CommandKind Kind,double Value){const auto Result=SubmitPlayerCommand(Kind,Value);UE_LOG(LogRailContinuity,Display,TEXT("S8_CAB_TEST phase=%d kind=%d value=%.2f accepted=%d reason=%s"),Phase,int32(Kind),Value,Result.ok,UTF8_TO_TCHAR(Result.error.c_str()));};
    switch(Phase%12){
     case 0:Command(Rail::CommandKind::Throttle,0);Command(Rail::CommandKind::Brake,1);break;
     case 1:Command(Rail::CommandKind::Direction,1);Command(Rail::CommandKind::Brake,0);Command(Rail::CommandKind::Throttle,.25);break;
     case 2:Command(Rail::CommandKind::Throttle,.75);break;
     case 3:Command(Rail::CommandKind::Throttle,0);Command(Rail::CommandKind::Brake,.5);break;
     case 4:Command(Rail::CommandKind::Brake,1);Command(Rail::CommandKind::Direction,-1);break;
     case 5:Command(Rail::CommandKind::Direction,0);break;
     case 6:Command(Rail::CommandKind::Direction,1);break;
     case 7:LocomotiveAction(0);break;
     case 8:LocomotiveAction(0);break;
     case 9:LocomotiveAction(2);LocomotiveAction(5);break;
     case 10:Command(Rail::CommandKind::Emergency,1);break;
     case 11:Command(Rail::CommandKind::Emergency,0);ServiceAction(4);break;
    }
   }else{
   if(Phase>0&&HasLocomotivePresentation()){LocomotiveAction(0);LocomotiveAction(2);LocomotiveAction(4);LocomotiveAction(5);}
   ServiceAction(4);
   SubmitPlayerCommand(Rail::CommandKind::Throttle,Phase%3==2?0:.35);
   SubmitPlayerCommand(Rail::CommandKind::Brake,Phase%3==2?.25:0);
   }
   UE_LOG(LogRailContinuity,Display,TEXT("S8_REVIEW_PHASE phase=%d time=%.2f"),Phase,S8ReviewSeconds);
  }
 }
 if(EgmoreContent()&&IsValid(EgmoreScene))EgmoreScene->PresentSignals(Operations);
 // Readiness is established before Update, but every vehicle must render the
 // committed post-update pose, just like the separately presented locomotive.
 // Avoid rebuilding scenery here: it owns the earlier readiness/skip-delta gate.
 if(TambaramContent())for(const auto& Level:S6Levels)if(Level&&Level->IsLevelVisible()&&Level->GetLoadedLevel())
  for(const auto& Actor:Level->GetLoadedLevel()->Actors)if(auto* Chunk=Cast<ARailChunkView>(Actor))
   if(Chunk->PresentedTick!=Operations.Current().tick)Chunk->Present(Operations,ServiceDebug,HasLocomotivePresentation()?Service.Definition().player.value:0ull,false);
 if(ServiceMode)PresentService();
 if(HasLocomotivePresentation()){const auto* CurrentPlayer=Operations.Spatial(PlayerId);const auto* CurrentHandling=Operations.Handling(PlayerId);if(CurrentPlayer&&CurrentHandling)LocomotiveView->Present(*CurrentPlayer,*CurrentHandling,DeltaTime,S6Paused||S6Blocked);}
 if(EgmoreTrafficLocomotive){const auto* Train=Operations.Spatial(Rail::TrainId(1));const auto* TrafficHandling=Operations.Handling(Rail::TrainId(1));if(Train&&TrafficHandling)EgmoreTrafficLocomotive->Present(*Train,*TrafficHandling,DeltaTime,S6Paused||S6Blocked);}
 if(OpposingLocomotive){const auto* Train=Operations.Spatial(Rail::TrainId(3));const auto* H=Operations.Handling(Rail::TrainId(3));if(Train&&H)OpposingLocomotive->Present(*Train,*H,DeltaTime,S6Paused||S6Blocked);}
 for(int32 I=0;I<AdditionalTrafficLocomotives.Num();++I){const auto* Train=Operations.Spatial(Rail::TrainId(4+I));const auto* H=Operations.Handling(Rail::TrainId(4+I));if(Train&&H&&AdditionalTrafficLocomotives[I])AdditionalTrafficLocomotives[I]->Present(*Train,*H,DeltaTime,S6Paused||S6Blocked);}

 CSV_CUSTOM_STAT(RailS6,ResidentChunks,Views,ECsvCustomStatOp::Set);
 if(Transition||Now-S6LastEvidence>=1){WriteS6Evidence();S6LastEvidence=Now;}
}
void URailSimulationSubsystem::WriteS6Evidence(){
 CSV_SCOPED_TIMING_STAT(RailS7,TelemetryMs);
 S6RetiredViews.RemoveAll([](const auto& ViewRef){return !ViewRef.IsValid();});
 S6RetiredMeshes.RemoveAll([](const auto& MeshRef){return !MeshRef.IsValid();});
 if(S6RetiredViews.Num()>64||S6RetiredMeshes.Num()>64){S6Paused=true;S6StreamStatus=TEXT("Residency audit limit exceeded");}
 int32 Levels=0,Actors=0,Vehicles=0,Proxies=0,CoachVehicles=0,CoachComponents=0;
 int32 StationComponents=0,StationInstances=0;
 for(const auto& Level:S6Levels)if(Level&&Level->GetLoadedLevel())for(const auto& Actor:Level->GetLoadedLevel()->Actors)if(auto* Chunk=Cast<ARailChunkView>(Actor))for(const auto& Part:Chunk->StationParts)if(Part){++StationComponents;StationInstances+=Part->GetInstanceCount();}
 for(const auto& Level:S6Levels)if(Level&&Level->GetLoadedLevel()){++Levels;for(const auto& Actor:Level->GetLoadedLevel()->Actors)if(auto* Chunk=Cast<ARailChunkView>(Actor)){++Actors;Vehicles+=Chunk->PresentedVehicles;CoachVehicles+=Chunk->PresentedLhbVehicles;CoachComponents+=Chunk->CoachParts.Num();Proxies+=Chunk->Scenery?Chunk->Scenery->GetInstanceCount():0;}}
 const FString Json=FString::Printf(TEXT("{\"content\":\"%s\",\"schema\":1,\"generation\":%llu,\"paused\":%s,\"blocked\":%s,\"lookBack\":%s,\"loadedLevels\":%d,\"chunkActors\":%d,\"visualVehicles\":%d,\"proxyInstances\":%d,\"retiredActorsAlive\":%d,\"retiredMeshesAlive\":%d,\"loads\":%u,\"unloads\":%u,\"loadMs\":[%.3f,%.3f],\"saveMs\":%.3f,\"save\":\"%s\",\"recovery\":\"%s\",\"streaming\":\"%s\",\"logical\":%s}"),
 UTF8_TO_TCHAR(Operations.ContentId().c_str()),S6Generation,S6Paused?TEXT("true"):TEXT("false"),S6Blocked?TEXT("true"):TEXT("false"),S6LookBack?TEXT("true"):TEXT("false"),Levels,Actors,Vehicles,Proxies,S6RetiredViews.Num(),S6RetiredMeshes.Num(),S6Loads,S6Unloads,S6LoadMilliseconds[0],S6LoadMilliseconds[1],S6SaveMilliseconds,*Escaped(S6SaveStatus),*Escaped(S6Recovery),*Escaped(S6StreamStatus),UTF8_TO_TCHAR(Operations.Json().c_str()));
 const FString Application=FString::Printf(TEXT("\"stationComponents\":%d,\"stationInstances\":%d,"),StationComponents,StationInstances)+(ServiceMode?TEXT("\"service\":")+FString(UTF8_TO_TCHAR(Service.Json().c_str()))+TEXT(","):TEXT(""));
 const FString Loco=TEXT("\"locomotive\":")+(LocomotiveView?LocomotiveView->TelemetryJson():TEXT("{\"ready\":false}"))+TEXT(",");
 FString TambaramViews=TEXT("\"tambaramViews\":[");bool FirstView=true;
 for(const auto& Level:S6Levels)if(Level&&Level->GetLoadedLevel())for(const auto& Actor:Level->GetLoadedLevel()->Actors)if(auto* Chunk=Cast<ARailChunkView>(Actor))if(Chunk->TambaramCorridor){
  auto* C=Chunk->TambaramCorridor.Get();TambaramViews+=FString::Printf(TEXT("%s{\"chunk\":%d,\"ready\":%s,\"cells\":%d,\"instances\":%d,\"stones\":%d,\"cellBuilds\":%llu,\"cellReleases\":%llu,\"trackAssets\":%s,\"turnoutFinishes\":%s,\"station\":%s}"),FirstView?TEXT(""):TEXT(","),Chunk->ChunkId,Chunk->TambaramReady?TEXT("true"):TEXT("false"),C->ActiveCells,C->InstanceCount,C->StoneInstances,C->RebuildCount,C->ReleasedCells,*C->TrackAssetTelemetryJson(),Chunk->TambaramTurnout?*Chunk->TambaramTurnout->FinishTelemetryJson():TEXT("[]"),Chunk->TambaramStation?*Chunk->TambaramStation->TelemetryJson():TEXT("null"));FirstView=false;
 }
 TambaramViews+=TEXT("],");
 const FString Egmore=EgmoreContent()?TEXT("\"egmoreScene\":")+(IsValid(EgmoreScene)?EgmoreScene->TelemetryJson():TEXT("{\"ready\":false}"))+TEXT(",\"trafficLocomotive\":")+(IsValid(EgmoreTrafficLocomotive)?EgmoreTrafficLocomotive->TelemetryJson():TEXT("{\"ready\":false}"))+TEXT(","):TEXT("");
 const FString Evidence=TEXT("{")+Egmore+TambaramViews+TEXT("\"logicalFingerprint\":\"")+LogicalFingerprint(Operations,ServiceMode?&Service:nullptr)+TEXT("\",")+Application+Loco+FString::Printf(TEXT("\"lhbVehicles\":%d,\"lhbComponents\":%d,"),CoachVehicles,CoachComponents)+Json.Mid(1);
 FFileHelper::SaveStringToFile(Evidence,*(Directory(ServiceMode)/TEXT("telemetry.json")));
 UE_LOG(LogRailContinuity,Display,TEXT("S6_STATE tick=%llu levels=%d actors=%d generation=%llu paused=%d blocked=%d"),Operations.Current().tick,Levels,Actors,S6Generation,S6Paused,S6Blocked);
 if(ServiceMode&&AssetReview()&&FParse::Param(FCommandLine::Get(),TEXT("S8AudioReview"))){
  UE_LOG(LogRailContinuity,Display,TEXT("RAIL_AUDIO_STATE running=%d rail=%d station=%d horn=%d brake=%d railGain=%.4f stationGain=%.4f pitch=%.3f"),ServiceAudioWasRunning,ServiceMovementAudio&&ServiceMovementAudio->IsPlaying(),ServiceStationAudio&&ServiceStationAudio->IsPlaying(),ServiceHornAudio&&ServiceHornAudio->IsPlaying(),ServiceBrakeAudio&&ServiceBrakeAudio->IsPlaying(),ServiceRailGain,ServiceStationGain,ServiceRailPitch);
 }
}
FString URailSimulationSubsystem::S6Display() const {
 const auto* Player=Operations.Spatial(Rail::TrainId(1));const auto* H=Operations.Handling(Rail::TrainId(1));const auto* AI=Operations.AI(Rail::TrainId(2));if(!Player||!H)return TEXT("S6 unavailable");
 return FString::Printf(TEXT("S6 technical prototype | tick %llu | T1 head %.2fm | speed %.2fm/s | %s\nChunks A:%s B:%s | loads %u unloads %u | IR-S6-fixture-1\nPower %.0f%% Brake %.0f%% applied %.0f%% | AI T2 %s\nGeneration %llu | %s | %.2fms\n%s\n%s"),Operations.Current().tick,Player->Current().headStationMetres,H->speedMps,S6Paused?TEXT("PAUSED"):S6Blocked?TEXT("CONTENT PAUSE"):TEXT("RUNNING"),S6Ready[0]?TEXT("ready"):TEXT("absent"),S6Ready[1]?TEXT("ready"):TEXT("absent"),S6Loads,S6Unloads,H->command.throttle*100,H->command.brake*100,H->appliedBrake*100,AI?UTF8_TO_TCHAR(Rail::AIPhaseName(AI->Phase())):TEXT("none"),S6Generation,*S6SaveStatus,S6SaveMilliseconds,*S6StreamStatus,*S6Recovery.Left(160));
}
void URailSimulationSubsystem::ServiceAction(int32 Action){
 if(!ServiceMode)return;
 if(Action==4){PlayServiceHorn();return;}
 if(Action==5){ToggleS6LookBack();return;}
 if(Action==6){
#if !UE_BUILD_SHIPPING
  ServiceDebug=!ServiceDebug;
#endif
  return;
 }
 if(S6RecoveryBlocked){ServiceNotice=TEXT("No compatible checkpoint. Existing records are retained; restore a compatible backup before continuing.");return;}
 if(Action==2){if(Service.Current().phase==Rail::ServicePhase::Running)ToggleHandlingPause();return;}
 if(Action==7){SubmitPlayerCommand(Rail::CommandKind::Emergency,1);return;}
 if(Action==8||(Action==1&&Service.Current().phase==Rail::ServicePhase::Running)){
  if(Service.Current().accumulator>4.0/30.0+1e-9){S6Paused=true;ServiceNotice=TEXT("Your checkpoint is safe. Restore the last checkpoint before saving and exiting.");return;}
  S6Paused=true;SaveS6(TEXT("save-and-exit"));if(S6SaveStatus.EndsWith(TEXT("verified")))UKismetSystemLibrary::QuitGame(this,GetWorld()->GetFirstPlayerController(),EQuitPreference::Quit,false);else ServiceNotice=TEXT("Save failed. Remain paused and retry.");return;
 }
 if(Action==0&&Service.Current().phase==Rail::ServicePhase::Running){
  if(Service.Current().accumulator>4.0/30.0+1e-9){Rail::RecoverySelection Selected;const auto Restored=Service.Recover(Files(true),Operations,Selected);if(Restored.ok){ServiceDispatchWatchdog.Reset();ServiceDispatchTick=Operations.Current().tick;ServiceDispatchNotice.Empty();}ServiceNotice=Restored.ok?TEXT("Last verified checkpoint restored. Resume when ready."):UTF8_TO_TCHAR(Restored.error.c_str());S6Paused=true;SkipNextDelta.Store(true);WriteS6Evidence();return;}
  S6Paused=false;ServiceNotice.Empty();SkipNextDelta.Store(true);return;
 }
 Rail::CheckpointCommit Commit;Rail::Result Result;
 if(Action==0&&Service.Current().phase==Rail::ServicePhase::Result&&Service.Current().success&&!Service.Current().settled){
  Result=Service.Settle(Operations,Files(true),Commit);
 }else{
  auto Candidate=Service;auto Railway=Operations;bool Changed=true;
  if(Action==0){
   if(Candidate.Current().phase==Rail::ServicePhase::Selection)Result=Candidate.Select();
   else if(Candidate.Current().phase==Rail::ServicePhase::Briefing)Result=Candidate.Current().formationConfirmed?Candidate.Start(Railway):Candidate.ConfirmFormation();
   else Result=Candidate.Replay();
  }else if(Action==1&&Candidate.Current().phase==Rail::ServicePhase::Briefing){Result=Candidate.SetMode(Candidate.Current().mode==Rail::DrivingMode::Assisted?Rail::DrivingMode::Standard:Rail::DrivingMode::Assisted);}
  else if(Action==3)Result=Candidate.Depart(Railway);
  else Changed=false;
  if(!Changed)return;
  if(Result.ok)Result=Candidate.Save(Railway,Files(true),Commit);
  if(Result.ok){Service=std::move(Candidate);Operations=std::move(Railway);ServiceDispatchWatchdog.Reset();ServiceDispatchTick=Operations.Current().tick;ServiceDispatchNotice.Empty();S6Paused=Service.Current().phase!=Rail::ServicePhase::Running;ServiceLastSaveTick=Operations.Current().tick;}
 }
 if(Result.ok){if(Commit.generation)S6Generation=Commit.generation;S6SaveStatus=TEXT("service action verified");ServiceNotice.Empty();SkipNextDelta.Store(true);}
 else ServiceNotice=UTF8_TO_TCHAR(Result.error.c_str());
 WriteS6Evidence();
 if(Result.ok)IFileManager::Get().Copy(*(Directory(true)/TEXT("last-checkpoint.json")),*(Directory(true)/TEXT("telemetry.json")));
}

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
void URailSimulationSubsystem::FreezeForWheelMotionReview(){
 if(AssetReview()&&TambaramContent()&&FParse::Param(FCommandLine::Get(),TEXT("TambaramMotionReview")))Active=false;
}
#endif





