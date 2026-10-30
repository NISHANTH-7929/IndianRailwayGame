#include "CoreMinimal.h"
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "RailSimulationSubsystem.h"
#include "RailTambaramCorridorView.h"
#include "RailLocomotiveView.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "ShaderCompiler.h"
#include "UnrealClient.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace {
// A short, real-time traversal of a copied checkpoint. This ticker observes the
// normal subsystem; it never steps, teleports, or substitutes the simulation.
struct FCorridorMotionReview {
 FString Output,Pending;
 FString Samples=TEXT("wallSeconds,wallFrameMs,engineDeltaMs,tick,headMetres,speedMps,activeCells,rebuiltCells,evictedCells,blocked,paused,capturePending\n");
 TWeakObjectPtr<URailSimulationSubsystem> Sim;
 TArray<double> FrameMs;
 TMap<uint32,TPair<uint64,uint64>> CellCounters;
 double Created=FPlatformTime::Seconds(),ReadySince=0,Started=0,Previous=0,LastAudit=0;
 double Duration=90,StartHead=0,EndHead=0,LastShot=0,StopTime=0;
 uint64 StartTick=0,EndTick=0,Rebuilt=0,Evicted=0;
 int32 Captures=0,FramesOver50=0,FramesOver100=0,MaximumActiveCells=0;
 bool Running=false,Measuring=false,Finishing=false,SourceMatched=false,ForwardWarming=false,CameraReady=false;
 bool FaultSeen=false,BlockedSeen=false,PauseSeen=false,OverloadSeen=false;
 bool AuthorityValid=true,OccupancyValid=true,InvariantsValid=true,CadenceValid=true;

 void ObserveCells(UWorld* World,bool Baseline=false){
  int32 Active=0;
  for(TActorIterator<ARailTambaramCorridorView> It(World);It;++It){
   Active+=It->ActiveCells;
   auto* Old=CellCounters.Find(It->GetUniqueID());
   // Actors created after resume contribute all their actual cell builds.
   if(!Baseline){
    Rebuilt+=Old?(It->RebuildCount>=Old->Key?It->RebuildCount-Old->Key:0):It->RebuildCount;
    Evicted+=Old?(It->ReleasedCells>=Old->Value?It->ReleasedCells-Old->Value:0):It->ReleasedCells;
   }
   CellCounters.Add(It->GetUniqueID(),TPair<uint64,uint64>(It->RebuildCount,It->ReleasedCells));
  }
  MaximumActiveCells=FMath::Max(MaximumActiveCells,Active);
 }
 int32 ActiveCells(UWorld* World)const{
  int32 Count=0;for(TActorIterator<ARailTambaramCorridorView> It(World);It;++It)Count+=It->ActiveCells;return Count;
 }
 void RecordJson(const FString& Name,const TSharedRef<FJsonObject>& Json)const{
  FString Encoded;FJsonSerializer::Serialize(Json,TJsonWriterFactory<>::Create(&Encoded));
  FFileHelper::SaveStringToFile(Encoded,*(Output/Name));
 }
 bool Camera(bool Forward){
  auto* S=Sim.Get();auto* Loco=S?S->PlayerLocomotive():nullptr;
  auto* PC=S?S->GetWorld()->GetFirstPlayerController():nullptr;
  if(!Loco||!Loco->IsReady()||!PC)return false;
  // Toggle through exterior to reset instrument zoom and head look.
  if(Loco->IsCabCamera())Loco->ToggleCamera();
  Loco->ToggleCamera();if(Forward)Loco->CycleJourneyCamera();
  PC->SetViewTarget(Loco);
  return Loco->CameraName()==(Forward?TEXT("Forward route"):TEXT("Cab"));
 }
 bool Capture(const TCHAR* Label){
  if(!Pending.IsEmpty())return false;
  Pending=Output/FString::Printf(TEXT("%02d-%s.png"),Captures++,Label);
  LastShot=FPlatformTime::Seconds();FScreenshotRequest::RequestScreenshot(Pending,false,false);
  auto Json=MakeShared<FJsonObject>();Json->SetStringField(TEXT("label"),Label);
  Json->SetNumberField(TEXT("wallSeconds"),LastShot-Started);Json->SetNumberField(TEXT("headMetres"),EndHead);Json->SetNumberField(TEXT("tick"),EndTick);
  Json->SetBoolField(TEXT("simulationRunning"),!Sim->IsServicePaused()&&!Sim->IsPresentationBlocked());
  RecordJson(FString::Printf(TEXT("%02d-frame.json"),Captures-1),Json);
  FFileHelper::SaveStringToFile(Sim->PlayerLocomotive()->TelemetryJson(),*(Output/FString::Printf(TEXT("%02d-locomotive.json"),Captures-1)));
  return true;
 }
 bool Finish(bool RequestedPass,const FString& Reason){
  const double Elapsed=Started>0?(StopTime>0?StopTime:FPlatformTime::Seconds())-Started:0;
  if(Sim.IsValid()){
   auto* S=Sim.Get();const auto& World=S->ReadOnlyOperations();
   InvariantsValid&=World.ValidateInvariants().ok;
   // The pause is our teardown, after the measured interval and final frame.
   if(!S->IsServicePaused())S->ServiceAction(2);
   FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(World.Json().c_str()),*(Output/TEXT("after-logical.json")));
  }
  TArray<double> Sorted=FrameMs;Sorted.Sort();
  auto Percentile=[&Sorted](double Fraction){return Sorted.IsEmpty()?0.:Sorted[FMath::Clamp(FMath::CeilToInt(Fraction*Sorted.Num())-1,0,Sorted.Num()-1)];};
  double Total=0;for(double Ms:FrameMs)Total+=Ms;
  const double P95=Percentile(.95),P99=Percentile(.99);
  const bool Functional=RequestedPass&&SourceMatched&&EndHead-StartHead>300&&EndTick>StartTick&&Elapsed>=Duration&&Captures==3&&Pending.IsEmpty()
   &&FrameMs.Num()>300&&Rebuilt>0&&Evicted>0&&!FaultSeen&&!BlockedSeen&&!PauseSeen&&!OverloadSeen&&AuthorityValid&&OccupancyValid&&InvariantsValid&&CadenceValid;
  // Publish an explicit budget, separate from functional traversal. A failed
  // timing budget is useful evidence, never hidden by a successful screenshot.
  const bool FrameBudget=!Sorted.IsEmpty()&&P95<=50&&P99<=100;
  auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("passed"),Functional&&FrameBudget);
  Result->SetBoolField(TEXT("functionalPassed"),Functional);Result->SetBoolField(TEXT("frameBudgetPassed"),FrameBudget);
  Result->SetStringField(TEXT("reason"),Reason);Result->SetStringField(TEXT("scope"),TEXT("Bounded editor RenderOffscreen real-time traversal; not Shipping, mobile, manual driving, full journey or automatic flicker qualification"));
  Result->SetNumberField(TEXT("durationRequestedSeconds"),Duration);Result->SetNumberField(TEXT("wallSeconds"),Elapsed);
  Result->SetNumberField(TEXT("startHeadMetres"),StartHead);Result->SetNumberField(TEXT("endHeadMetres"),EndHead);Result->SetNumberField(TEXT("distanceAdvancedMetres"),EndHead-StartHead);
  Result->SetNumberField(TEXT("startTick"),StartTick);Result->SetNumberField(TEXT("endTick"),EndTick);
  Result->SetNumberField(TEXT("simulationAdvancedSeconds"),(EndTick-StartTick)/30.);
  Result->SetNumberField(TEXT("frameCapFps"),30);Result->SetBoolField(TEXT("fixedTimeStep"),false);Result->SetBoolField(TEXT("realTimeCadenceVerified"),CadenceValid);
  Result->SetNumberField(TEXT("frameCount"),FrameMs.Num());Result->SetNumberField(TEXT("wallFrameMeanMs"),FrameMs.IsEmpty()?0:Total/FrameMs.Num());
  Result->SetNumberField(TEXT("wallFrameP50Ms"),Percentile(.50));Result->SetNumberField(TEXT("wallFrameP95Ms"),P95);Result->SetNumberField(TEXT("wallFrameP99Ms"),P99);Result->SetNumberField(TEXT("wallFrameMaxMs"),Sorted.IsEmpty()?0:Sorted.Last());
  Result->SetNumberField(TEXT("frameP95BudgetMs"),50);Result->SetNumberField(TEXT("frameP99BudgetMs"),100);
  Result->SetNumberField(TEXT("framesOver50Ms"),FramesOver50);Result->SetNumberField(TEXT("framesOver100Ms"),FramesOver100);
  Result->SetBoolField(TEXT("captureAndLoadingOverheadIncluded"),true);Result->SetNumberField(TEXT("captures"),Captures);
  Result->SetNumberField(TEXT("rebuiltCellsDuringDrive"),Rebuilt);Result->SetNumberField(TEXT("evictedCellsDuringDrive"),Evicted);Result->SetNumberField(TEXT("maximumActiveCells"),MaximumActiveCells);
  Result->SetBoolField(TEXT("fixtureMatchedBeforeResume"),SourceMatched);Result->SetBoolField(TEXT("faultSeen"),FaultSeen);Result->SetBoolField(TEXT("presentationBlockedSeen"),BlockedSeen);
  Result->SetBoolField(TEXT("unexpectedPauseSeen"),PauseSeen);Result->SetBoolField(TEXT("frameOverloadSeen"),OverloadSeen);
  Result->SetBoolField(TEXT("authorityValid"),AuthorityValid);Result->SetBoolField(TEXT("occupancyValid"),OccupancyValid);Result->SetBoolField(TEXT("invariantsValid"),InvariantsValid);
  FFileHelper::SaveStringToFile(Samples,*(Output/TEXT("frames.csv")));RecordJson(TEXT("result.json"),Result);
  FPlatformMisc::RequestExit(false);return false;
 }
 bool Tick(float Delta){
  const double Now=FPlatformTime::Seconds();
  if(Now-Created>Duration+180)return Finish(false,TEXT("Motion review timeout"));
  if(!Sim.IsValid()){
   for(const auto& Context:GEngine->GetWorldContexts())if(auto* World=Context.World())if(World->IsGameWorld()&&World->HasBegunPlay()){
    auto* S=World->GetSubsystem<URailSimulationSubsystem>();if(S&&S->IsServiceMode()&&S->ReadOnlyService().Definition().terminalStop){Sim=S;break;}
   }return true;
  }
  auto* S=Sim.Get();auto* World=S->GetWorld();const auto& Railway=S->ReadOnlyOperations();const auto& Service=S->ReadOnlyService();
  const auto* Train=Railway.Spatial(Service.Definition().player);const auto* Handling=Railway.Handling(Service.Definition().player);
  if(!Train||!Handling)return true;
  EndHead=Train->Current().headStationMetres;EndTick=Railway.Current().tick;
  if(!Running){
   bool Ready=false;for(TActorIterator<ARailTambaramCorridorView> It(World);It;++It)Ready|=It->IsReady();
   if(!Ready||!S->HasLocomotivePresentation()||S->IsPresentationBlocked()||(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())){ReadySince=0;return true;}
   if(!CameraReady){if(!Camera(false))return Finish(false,TEXT("Actual cab camera unavailable"));CameraReady=true;ReadySince=Now;return true;}
   if(!ReadySince){ReadySince=Now;return true;}if(Now-ReadySince<3)return true;
   FString Expected;SourceMatched=FFileHelper::LoadFileToString(Expected,*(Output+TEXT("-logical.json")))&&Expected==UTF8_TO_TCHAR(Railway.Json().c_str());
   if(!SourceMatched||!S->IsServicePaused()||Service.Current().phase!=Rail::ServicePhase::Running||Service.Current().mode!=Rail::DrivingMode::Assisted)
    return Finish(false,TEXT("Requires an unchanged paused Assisted checkpoint"));
   CadenceValid=!FApp::UseFixedTimeStep()&&!FApp::IsBenchmarking()&&FMath::Abs(GEngine->GetMaxFPS()-30)<.01;
   if(!CadenceValid)return Finish(false,TEXT("Requires real-time cadence with t.MaxFPS 30, no benchmark or fixed-time-step"));
   InvariantsValid=Railway.ValidateInvariants().ok;if(!InvariantsValid)return Finish(false,TEXT("Checkpoint invariants invalid"));
   FFileHelper::SaveStringToFile(Expected,*(Output/TEXT("before-logical.json")));
   StartHead=EndHead;StartTick=EndTick;ObserveCells(World,true);Started=Previous=Now;Running=Measuring=true;S->ServiceAction(0);return true;
  }
  if(Measuring){
   const double WallMs=(Now-Previous)*1000;Previous=Now;FrameMs.Add(WallMs);FramesOver50+=WallMs>50;FramesOver100+=WallMs>100;
   ObserveCells(World);BlockedSeen|=S->IsPresentationBlocked();PauseSeen|=S->IsServicePaused();FaultSeen|=Railway.Faulted();OverloadSeen|=Service.Current().accumulator>4./30.+1e-9;
   CadenceValid&=!FApp::UseFixedTimeStep()&&!FApp::IsBenchmarking()&&FMath::Abs(GEngine->GetMaxFPS()-30)<.01;
   const auto Authority=Railway.Current().authorities.find(Service.Definition().player);
   AuthorityValid&=Authority!=Railway.Current().authorities.end()&&Authority->second.valid&&EndHead<=Authority->second.maximumHead+1e-6
    &&EndHead-Train->Current().properties.lengthMetres>=Authority->second.minimumRear-1e-6;
   for(const auto& Block:Railway.Current().blocks)OccupancyValid&=Block.second.occupants.size()<=1;
   if(Now-LastAudit>=1){InvariantsValid&=Railway.ValidateInvariants().ok;LastAudit=Now;}
   Samples+=FString::Printf(TEXT("%.6f,%.6f,%.6f,%llu,%.6f,%.6f,%d,%llu,%llu,%d,%d,%d\n"),Now-Started,WallMs,Delta*1000,EndTick,EndHead,Handling->speedMps,ActiveCells(World),Rebuilt,Evicted,S->IsPresentationBlocked()?1:0,S->IsServicePaused()?1:0,Pending.IsEmpty()?0:1);
   if(FaultSeen||BlockedSeen||PauseSeen||OverloadSeen||!AuthorityValid||!OccupancyValid||!InvariantsValid||!CadenceValid)
    return Finish(false,TEXT("Runtime pause, blockage, overload, cadence, authority or occupancy check failed"));
   if(Service.Current().phase!=Rail::ServicePhase::Running)return Finish(false,TEXT("Service ended before bounded review completed"));
  }
  if(!Pending.IsEmpty()){
   if(IFileManager::Get().FileSize(*Pending)<=0){if(Now-LastShot>20)return Finish(false,TEXT("Screenshot missing"));return true;}
   Pending.Empty();
  }
  if(Finishing){StopTime=Now;Measuring=false;return Finish(true,TEXT("Bounded moving review completed; inspect images and frame budget separately"));}
  if(Captures==0&&Now-Started>=2){Capture(TEXT("cab-start"));return true;}
  if(Captures==1&&!ForwardWarming&&Now-Started>=Duration*.5){
   if(!Camera(true))return Finish(false,TEXT("Actual forward camera unavailable"));
   // Allow a full second of normal moving frames before the next capture.
   LastShot=Now;ForwardWarming=true;return true;
  }
  if(Captures==1&&ForwardWarming&&Now-LastShot>=1){ForwardWarming=false;Capture(TEXT("forward-middle"));return true;}
  if(Captures==2&&Now-Started>=Duration-2&&S->PlayerLocomotive()->CameraName()!=TEXT("Cab")){
   if(!Camera(false))return Finish(false,TEXT("Actual final cab camera unavailable"));
  }
  if(Captures==2&&Now-Started>=Duration){Capture(TEXT("cab-end"));Finishing=true;return true;}
  return true;
 }
};
void BeginCorridorMotionReview(){
 FString Output;double Duration=90;
 if(!FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram"))||!FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview"))||!FParse::Param(FCommandLine::Get(),TEXT("S8AssetReviewResume"))
  ||!FParse::Param(FCommandLine::Get(),TEXT("CorridorMotionReview"))||!FParse::Value(FCommandLine::Get(),TEXT("TambaramEvidence="),Output)||FPaths::IsRelative(Output))return;
 FParse::Value(FCommandLine::Get(),TEXT("CorridorMotionSeconds="),Duration);if(Duration<60||Duration>90)return;
 IFileManager::Get().MakeDirectory(*Output,true);
 auto Review=MakeShared<FCorridorMotionReview>();Review->Output=Output;Review->Duration=Duration;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Review](float Delta){return Review->Tick(Delta);}));
}
FAutoConsoleCommand CorridorMotionReviewCommand(TEXT("Rail.CorridorMotionReview"),TEXT("Bounded real-time corridor traversal from an isolated checkpoint"),FConsoleCommandDelegate::CreateStatic(&BeginCorridorMotionReview));
}
#endif
