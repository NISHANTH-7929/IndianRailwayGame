#include "CoreMinimal.h"
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "RailSimulationSubsystem.h"
#include "RailServiceWidget.h"
#include "RailLocomotiveView.h"
#include "RailEgmoreSceneView.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "InputCoreTypes.h"
#include "EngineUtils.h"
#include "UObject/UObjectIterator.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "UnrealClient.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"

namespace {
struct FDesktopStep {const TCHAR* Name;const TCHAR* Callback;};
const FDesktopStep Steps[]={
 {TEXT("baseline"),nullptr},{TEXT("power-zero"),TEXT("Power0")},{TEXT("brake-full-stop"),TEXT("Brake100")},
 {TEXT("emergency-engage"),TEXT("Emergency")},{TEXT("emergency-request-zero"),TEXT("Brake0")},
 {TEXT("emergency-reset"),TEXT("ResetEmergency")},{TEXT("brake-half"),TEXT("Brake50")},
 {TEXT("brake-full"),TEXT("Brake100")},{TEXT("brake-release"),TEXT("Brake0")},
 {TEXT("power-quarter"),TEXT("Power25")},{TEXT("power-half"),TEXT("Power50")},
 {TEXT("power-release"),TEXT("Power0")},{TEXT("stop-before-direction"),TEXT("Brake100")},
 {TEXT("neutral"),TEXT("Neutral")},{TEXT("forward"),TEXT("Forward")},
 {TEXT("reverse"),TEXT("Reverse")},{TEXT("restore-forward"),TEXT("Forward")},
 {TEXT("cab-light-toggle"),TEXT("CabLamp")},{TEXT("cab-light-restore"),TEXT("CabLamp")},
 {TEXT("label-view"),TEXT("Instruments")},{TEXT("outside-reset"),TEXT("CabCamera")},
 {TEXT("return-cab"),TEXT("CabCamera")},
 {TEXT("touch-throttle"),nullptr},{TEXT("touch-power-restore"),TEXT("Power0")},
 {TEXT("touch-brake"),nullptr},{TEXT("touch-brake-restore"),TEXT("Brake100")},
 {TEXT("touch-emergency"),nullptr},{TEXT("touch-emergency-reset"),TEXT("ResetEmergency")},
 {TEXT("touch-camera-drag"),nullptr},{TEXT("drag-outside-reset"),TEXT("CabCamera")},{TEXT("drag-return-reset"),TEXT("CabCamera")},
 {TEXT("pause"),TEXT("Pause")},{TEXT("resume"),TEXT("Pause")}
};

struct FDesktopReview : TSharedFromThis<FDesktopReview> {
 FString Directory,PendingScreenshot;TArray<TSharedPtr<FJsonValue>> Records;
 TWeakObjectPtr<UWorld> World;TWeakObjectPtr<URailSimulationSubsystem> Sim;
 TWeakObjectPtr<URailServiceWidget> Widget;TWeakObjectPtr<ARailLocomotiveView> Loco;
 double Started=FPlatformTime::Seconds(),ReadyAt=0,Due=0;int32 Step=0,Phase=0,Frames=0;bool AllChecks=true;
 FVector2D TouchPoint;FRotator CameraBefore;int32 InteractionsBefore=0;bool TouchActive=false;
 bool Inject(ETouchType::Type Type){
  auto* PC=World.IsValid()?World->GetFirstPlayerController():nullptr;if(!PC)return false;
  PC->InputTouch(FTouchId(FInputDeviceId::CreateFromInternalId(0),ETouchIndex::Touch1),Type,TouchPoint,Type==ETouchType::Ended?0.f:1.f,FPlatformTime::Cycles64());
  TouchActive=Type!=ETouchType::Ended;return true;
 }
 void Save(const FString& Path,const TSharedRef<FJsonObject>& Object){FString Text;FJsonSerializer::Serialize(Object,TJsonWriterFactory<>::Create(&Text));FFileHelper::SaveStringToFile(Text,*Path);}
 bool Finish(const FString& Error){
  if(TouchActive)Inject(ETouchType::Ended);
  auto Result=MakeShared<FJsonObject>();Result->SetBoolField(TEXT("passed"),Error.IsEmpty()&&AllChecks);
  Result->SetStringField(TEXT("error"),Error);Result->SetStringField(TEXT("coverage"),TEXT("Existing reflected UMG callbacks plus staged PlayerController InputTouch injection through cab ray-control/camera polling. Not OS mouse, physical touchscreen, Slate button-hit dispatch or Android verification."));
  Result->SetBoolField(TEXT("mouseInputVerified"),false);Result->SetArrayField(TEXT("steps"),Records);
  Save(Directory/TEXT("result.json"),Result);UE_LOG(LogTemp,Display,TEXT("RAIL_DESKTOP_REVIEW finished passed=%d error=%s"),Error.IsEmpty()&&AllChecks,*Error);
  FPlatformMisc::RequestExit(false);return false;
 }
 bool FindReady(){
  if(!GEngine)return false;
  for(const auto& Context:GEngine->GetWorldContexts()){
   UWorld* W=Context.World();if(!W||!W->IsGameWorld()||!W->HasBegunPlay()||!W->GetMapName().Contains(TEXT("L_RailService_Egmore")))continue;
   auto* S=W->GetSubsystem<URailSimulationSubsystem>();if(!S||!S->IsServiceMode()||!S->HasLocomotivePresentation()||S->IsServicePaused()||S->ReadOnlyService().Current().phase!=Rail::ServicePhase::Running)continue;
   bool SceneReady=false;for(TActorIterator<ARailEgmoreSceneView> It(W);It;++It)SceneReady|=It->IsServiceReady();if(!SceneReady)continue;
   for(TObjectIterator<URailServiceWidget> It;It;++It)if(It->GetWorld()==W&&It->IsInViewport()){Widget=*It;break;}
   for(TActorIterator<ARailLocomotiveView> It(W);It;++It)if(It->IsCabCamera()){Loco=*It;break;}
   if(!Widget.IsValid()||!Loco.IsValid()||!GEngine->GameViewport)continue;
   World=W;Sim=S;ReadyAt=FPlatformTime::Seconds();Due=ReadyAt+5;return true;
  }return false;
 }
 bool Tick(float){
  const double Now=FPlatformTime::Seconds();
  if(!ReadyAt){if(Now-Started>120)return Finish(TEXT("Ready-state timeout; no callbacks dispatched"));FindReady();return true;}
  if(Now-ReadyAt>240)return Finish(TEXT("240-second ready-state budget exhausted"));
  if(!World.IsValid()||!Sim.IsValid()||!Widget.IsValid()||!Loco.IsValid())return Finish(TEXT("Review world or presentation disappeared"));
  auto* S=Sim.Get();const auto& Service=S->ReadOnlyService();
  if(Service.Current().phase!=Rail::ServicePhase::Running)return Finish(TEXT("Service left Running phase"));
  TSharedPtr<FJsonObject> Logical;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(UTF8_TO_TCHAR(S->ReadOnlyOperations().Json().c_str())),Logical);
  if(!Logical.IsValid()||!Logical->GetBoolField(TEXT("valid"))||Logical->GetBoolField(TEXT("faulted")))return Finish(TEXT("Logical world invalid or faulted"));
  const auto* H=S->ReadOnlyOperations().Handling(Service.Definition().player);if(!H)return Finish(TEXT("Missing authoritative handling"));
  if(Now<Due||++Frames<6)return true;
  if(Phase==2){
   if(IFileManager::Get().FileSize(*PendingScreenshot)<=0){if(Now-Due>10)return Finish(TEXT("Screenshot did not complete"));return true;}
   ++Step;Phase=0;Frames=0;if(Step>=UE_ARRAY_COUNT(Steps))return Finish(TEXT(""));
  }
  const auto& Current=Steps[Step];
  if(Phase==3||Phase==4){
   if(Phase==3&&FString(Current.Name)==TEXT("touch-camera-drag")){
    TouchPoint.X+=90;TouchPoint.Y+=35;if(!Inject(ETouchType::Moved))return Finish(TEXT("No player controller for move"));Phase=4;Due=Now;Frames=0;return true;
   }
   if(!Inject(ETouchType::Ended))return Finish(TEXT("No player controller for touch release"));Phase=1;Due=Now+3;Frames=0;return true;
  }
  if(Phase==0){
   if(S->IsServicePaused()&&FString(Current.Name)!=TEXT("resume"))return Finish(TEXT("Unexpected pause before callback"));
   const FString Name=Current.Name;
   const bool RequireStop=Name==TEXT("emergency-engage")||Name==TEXT("emergency-reset")||Name==TEXT("neutral")||Name==TEXT("forward")||Name==TEXT("reverse")||Name==TEXT("restore-forward");
   if(RequireStop&&(FMath::Abs(H->speedMps)>=.02||H->command.throttle>0))return true;
   const bool TouchStep=Name==TEXT("touch-throttle")||Name==TEXT("touch-brake")||Name==TEXT("touch-emergency")||Name==TEXT("touch-camera-drag");
   if(TouchStep){
    if(FMath::Abs(H->speedMps)>=.02||Service.Current().requestedPower!=0||!Loco->IsCabCamera())return Finish(TEXT("Touch requires stopped zero-intent-power cab"));
    if(Name!=TEXT("touch-camera-drag")&&Service.Current().requestedBrake!=1)return Finish(TEXT("Control-touch setup requires full brake intent"));
    TSharedPtr<FJsonObject> T;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Loco->TelemetryJson()),T);
    if(!T.IsValid()||T->GetBoolField(TEXT("instrumentView")))return Finish(TEXT("Invalid normal-cab touch telemetry"));
    const auto& V=T->GetArrayField(TEXT("viewport"));if(V.Num()!=2)return Finish(TEXT("Invalid viewport array"));const double W=V[0]->AsNumber(),Height=V[1]->AsNumber();
    if(!FMath::IsFinite(W)||!FMath::IsFinite(Height)||W<=0||Height<=0)return Finish(TEXT("Invalid touch viewport"));
    InteractionsBefore=int32(T->GetNumberField(TEXT("interactions")));
    auto* Camera=Loco->FindComponentByClass<UCameraComponent>();if(!Camera)return Finish(TEXT("No cab camera component"));CameraBefore=Camera->GetRelativeRotation();
    if(Name==TEXT("touch-camera-drag"))TouchPoint=FVector2D(W*.5,Height*.5);
    else{
     const FString Role=Name==TEXT("touch-throttle")?TEXT("throttle"):Name==TEXT("touch-brake")?TEXT("train_brake"):TEXT("switch_emergency");bool Found=false;
     for(const auto& Value:T->GetArrayField(TEXT("controlPoses"))){const auto P=Value->AsObject();if(P->GetStringField(TEXT("role"))==Role){const auto& Point=P->GetArrayField(TEXT("screen"));if(Point.Num()!=2)return Finish(TEXT("Invalid control projection"));TouchPoint=FVector2D(Point[0]->AsNumber(),Point[1]->AsNumber());Found=true;break;}}
     if(!Found)return Finish(TEXT("Missing projected control"));
    }
    if(!FMath::IsFinite(TouchPoint.X)||!FMath::IsFinite(TouchPoint.Y)||TouchPoint.X<=0||TouchPoint.X>=W||TouchPoint.Y<Height*.18||TouchPoint.Y>=Height*.86)return Finish(TEXT("Control projection outside guarded cab region"));
    if(Name==TEXT("touch-camera-drag")&&(TouchPoint.X+90>=W||TouchPoint.Y+35>=Height*.86))return Finish(TEXT("Drag endpoint outside cab region"));
    if(!Inject(ETouchType::Began))return Finish(TEXT("No player controller for touch"));Phase=3;Due=Now;Frames=0;return true;
   }
   if(Current.Callback){auto* Function=Widget->FindFunction(FName(Current.Callback));if(!Function)return Finish(FString(TEXT("Missing existing UI callback: "))+Current.Callback);Widget->ProcessEvent(Function,nullptr);}
   Phase=1;Due=Now+3;Frames=0;return true;
  }
  auto Record=MakeShared<FJsonObject>();Record->SetStringField(TEXT("name"),Current.Name);Record->SetStringField(TEXT("callback"),Current.Callback?Current.Callback:TEXT("none"));
  Record->SetObjectField(TEXT("logical"),Logical);
  const FString Hud=S->ServiceHudText();Record->SetStringField(TEXT("hud"),Hud);Record->SetBoolField(TEXT("paused"),S->IsServicePaused());
  Record->SetNumberField(TEXT("speedMps"),H->speedMps);Record->SetNumberField(TEXT("requestedBrake"),Service.Current().requestedBrake);Record->SetNumberField(TEXT("requestedPower"),Service.Current().requestedPower);
  Record->SetNumberField(TEXT("appliedBrake"),H->appliedBrake);Record->SetNumberField(TEXT("appliedPower"),H->appliedThrottle);Record->SetBoolField(TEXT("emergency"),H->command.emergency);Record->SetNumberField(TEXT("direction"),H->command.direction);
  TSharedPtr<FJsonObject> Telemetry;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Loco->TelemetryJson()),Telemetry);if(!Telemetry.IsValid())return Finish(TEXT("Invalid locomotive telemetry"));Record->SetObjectField(TEXT("locomotive"),Telemetry);
  bool Passed=true;const FString Name=Current.Name;
  if(Name.StartsWith(TEXT("touch-"))&&Current.Callback==nullptr){
   Record->SetStringField(TEXT("inputMethod"),TEXT("PlayerController::InputTouch synthetic in-engine injection"));
   Record->SetNumberField(TEXT("interactionsBefore"),InteractionsBefore);
   Record->SetNumberField(TEXT("touchX"),TouchPoint.X);Record->SetNumberField(TEXT("touchY"),TouchPoint.Y);
   const int32 Count=int32(Telemetry->GetNumberField(TEXT("interactions")));
   if(Name==TEXT("touch-camera-drag")){auto* Camera=Loco->FindComponentByClass<UCameraComponent>();Passed=Camera&&!Camera->GetRelativeRotation().Equals(CameraBefore,.1)&&Count==InteractionsBefore;Record->SetStringField(TEXT("cameraBefore"),CameraBefore.ToString());if(Camera)Record->SetStringField(TEXT("cameraAfter"),Camera->GetRelativeRotation().ToString());}
   else {Passed=Count==InteractionsBefore+1;if(Name==TEXT("touch-throttle"))Passed&=Service.Current().requestedPower==.25&&Service.Current().requestedBrake==1&&FMath::Abs(H->speedMps)<.02;else if(Name==TEXT("touch-brake"))Passed&=Service.Current().requestedBrake==0;else Passed&=H->command.emergency&&H->appliedBrake>.999;}
  }
  else if(Name==TEXT("emergency-request-zero"))Passed=Service.Current().requestedBrake==0&&H->command.emergency&&H->appliedBrake>.999&&H->appliedThrottle<.00001&&Hud.Contains(TEXT("Brake 100%"))&&Hud.Contains(TEXT("EMERGENCY"));
  else if(Name==TEXT("emergency-reset"))Passed=!H->command.emergency;
  else if(Name==TEXT("power-quarter"))Passed=Service.Current().requestedPower==.25;
  else if(Name==TEXT("power-half"))Passed=Service.Current().requestedPower==.5;
  else if(Name==TEXT("power-release"))Passed=Service.Current().requestedPower==0;
  else if(Name==TEXT("touch-power-restore"))Passed=Service.Current().requestedPower==0;
  else if(Name==TEXT("touch-brake-restore"))Passed=Service.Current().requestedBrake==1;
  else if(Name==TEXT("touch-emergency-reset"))Passed=!H->command.emergency;
  else if(Name==TEXT("drag-outside-reset"))Passed=!Loco->IsCabCamera();
  else if(Name==TEXT("drag-return-reset")){auto* Camera=Loco->FindComponentByClass<UCameraComponent>();Passed=Loco->IsCabCamera()&&Camera&&Camera->GetRelativeRotation().Equals(CameraBefore,.1);}
  else if(Name==TEXT("brake-half"))Passed=Service.Current().requestedBrake==.5;
  else if(Name==TEXT("brake-full"))Passed=Service.Current().requestedBrake==1;
  else if(Name==TEXT("brake-release"))Passed=Service.Current().requestedBrake==0;
  else if(Name==TEXT("neutral"))Passed=H->command.direction==0;
  else if(Name==TEXT("reverse"))Passed=H->command.direction==-1;
  else if(Name==TEXT("forward")||Name==TEXT("restore-forward"))Passed=H->command.direction==1;
  else if(Name==TEXT("label-view"))Passed=Telemetry->GetBoolField(TEXT("instrumentView"))&&Telemetry->GetNumberField(TEXT("cameraFov"))==55;
  else if(Name==TEXT("outside-reset"))Passed=!Telemetry->GetBoolField(TEXT("cabCamera"))&&!Telemetry->GetBoolField(TEXT("instrumentView"))&&!Telemetry->GetBoolField(TEXT("cabLightVisible"));
  else if(Name==TEXT("return-cab"))Passed=Telemetry->GetBoolField(TEXT("cabCamera"))&&!Telemetry->GetBoolField(TEXT("instrumentView"));
  else if(Name==TEXT("cab-light-toggle")||Name==TEXT("cab-light-restore")){
   const bool Before=Records.Last()->AsObject()->GetObjectField(TEXT("locomotive"))->GetBoolField(TEXT("cabLight"));
   Passed=Telemetry->GetBoolField(TEXT("cabLight"))!=Before&&Telemetry->GetBoolField(TEXT("cabLightVisible"))==Telemetry->GetBoolField(TEXT("cabLight"));
  }
  else if(Name==TEXT("pause"))Passed=S->IsServicePaused();else if(Name==TEXT("resume"))Passed=!S->IsServicePaused();
  Record->SetBoolField(TEXT("checkPassed"),Passed);AllChecks&=Passed;
  PendingScreenshot=Directory/FString::Printf(TEXT("%02d-%s.png"),Step,Current.Name);Record->SetStringField(TEXT("screenshot"),PendingScreenshot);
  Records.Add(MakeShared<FJsonValueObject>(Record));Save(Directory/FString::Printf(TEXT("%02d-%s.json"),Step,Current.Name),Record);
  UE_LOG(LogTemp,Display,TEXT("RAIL_DESKTOP_REVIEW step=%s passed=%d HUD=%s"),Current.Name,Passed,*Hud);
  FScreenshotRequest::RequestScreenshot(PendingScreenshot,true,false);Phase=2;Due=Now+1;Frames=0;return true;
 }
};
bool StartedReview=false;
void StartDesktopReview(){
 FString Directory;const TCHAR* Cmd=FCommandLine::Get();
 if(StartedReview||!FParse::Param(Cmd,TEXT("S8Egmore"))||!FParse::Param(Cmd,TEXT("S8AssetReview"))||!FParse::Param(Cmd,TEXT("S8Cab"))||!FParse::Value(Cmd,TEXT("RailDesktopEvidence="),Directory)||FPaths::IsRelative(Directory)||Directory.IsEmpty()){
  UE_LOG(LogTemp,Error,TEXT("Rail.S8DesktopReview requires isolated S8Egmore S8AssetReview S8Cab and absolute RailDesktopEvidence"));return;
 }
 FPaths::NormalizeDirectoryName(Directory);
 if(IFileManager::Get().DirectoryExists(*Directory)||!IFileManager::Get().MakeDirectory(*Directory,true)){UE_LOG(LogTemp,Error,TEXT("Review output must be a new writable directory"));return;}
 StartedReview=true;auto Review=MakeShared<FDesktopReview>();Review->Directory=Directory;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Review](float Dt){return Review->Tick(Dt);}));
}
FAutoConsoleCommand DesktopReviewCommand(TEXT("Rail.S8DesktopReview"),TEXT("Isolated editor-only bounded cab callback and full-UI screenshot review"),FConsoleCommandDelegate::CreateStatic(&StartDesktopReview));
}
#endif
