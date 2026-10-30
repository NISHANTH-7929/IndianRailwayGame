#include "RailSimulationSubsystem.h"
#include "Simulation/RailScenarioContent.h"
#include "RailLocomotiveView.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
void URailSimulationSubsystem::CabControl(const FString& Control){
 const auto* H=Operations.Handling(ServiceMode?Service.Definition().player:Rail::TrainId(1));if(!H)return;
 const double Power=ServiceMode?Service.Current().requestedPower:H->command.throttle;const double Brake=ServiceMode?Service.Current().requestedBrake:H->command.brake;
 Rail::Result Result;
 if(Control==TEXT("throttle"))Result=SubmitPlayerCommand(Rail::CommandKind::Throttle,Power>=.99?0:FMath::Min(1.0,Power+.25));
 else if(Control==TEXT("train_brake"))Result=SubmitPlayerCommand(Rail::CommandKind::Brake,Brake>=.99?0:FMath::Min(1.0,Brake+.25));
 else if(Control==TEXT("reverser"))Result=SubmitPlayerCommand(Rail::CommandKind::Direction,H->command.direction==1?-1:H->command.direction+1);
 else if(Control==TEXT("switch_emergency"))Result=SubmitPlayerCommand(Rail::CommandKind::Emergency,H->command.emergency?0:1);
 else if(Control==TEXT("switch_lamps"))LocomotiveAction(2);
 else if(Control==TEXT("switch_wipers"))LocomotiveAction(4);
 else if(Control==TEXT("switch_cablight"))LocomotiveAction(5);
 else return;
 if(!Result.ok)ServiceNotice=UTF8_TO_TCHAR(Result.error.c_str());
 UE_LOG(LogTemp,Display,TEXT("S8_CAB_INTERACTION control=%s accepted=%d"),*Control,Result.ok);
}
bool URailSimulationSubsystem::HasLocomotivePresentation() const {return LocomotiveView&&LocomotiveView->IsReady();}
FString URailSimulationSubsystem::LocomotiveDisplay() const {return HasLocomotivePresentation()?LocomotiveView->Display():TEXT("Locomotive assets unavailable; proxy view");}
void URailSimulationSubsystem::LocomotiveAction(int32 Action){
 if(!LocomotiveView)return;
 if(Action==1){
  if(Rail::ScenarioContent::UsesLhb(Operations.ContentId())){ServiceNotice=TEXT("This formation uses WAP-7.");return;}
  const auto* H=Operations.Handling(ServiceMode?Service.Definition().player:Rail::TrainId(1));
  if(!S6Paused||(H&&FMath::Abs(H->speedMps)>.05)){ServiceNotice=TEXT("Pause at a standstill to change locomotive appearance.");return;}
  FString Diagnostic;LocomotiveView->LoadPresentation(LocomotiveView->ClassName()==TEXT("WAP7")?TEXT("WAG9"):TEXT("WAP7"),Diagnostic);ServiceNotice=Diagnostic;SkipNextDelta.Store(true);
 }else if(Action==0&&HasLocomotivePresentation()){
  if(Rail::ScenarioContent::IsTambaram(Operations.ContentId()))LocomotiveView->CycleJourneyCamera();else LocomotiveView->ToggleCamera();
  ServiceNotice=TEXT("Camera: ")+LocomotiveView->CameraName()+TEXT(" | CAMERA cycles cab, forward route, right and left sides");
 }
 else if(Action>=2&&Action<=5)LocomotiveView->ToggleSwitch(Action-2);
 else if(Action==6)LocomotiveView->ToggleInstrumentView();
 if(HasLocomotivePresentation())if(auto* PC=GetWorld()->GetFirstPlayerController())PC->SetViewTarget(LocomotiveView);
}

bool URailSimulationSubsystem::IsLocomotiveCabCamera() const {return HasLocomotivePresentation()&&LocomotiveView->IsCabCamera();}
