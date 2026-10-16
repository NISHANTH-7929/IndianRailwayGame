#include "RailDrivingWidget.h"
#include "RailSimulationSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Engine/World.h"
void URailDrivingWidget::NativeOnInitialized(){
 Super::NativeOnInitialized();
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
 auto* Column=WidgetTree->ConstructWidget<UVerticalBox>();
 auto* CanvasSlot=Canvas->AddChildToCanvas(Column);CanvasSlot->SetAnchors(FAnchors(0,1,1,1));CanvasSlot->SetAlignment(FVector2D(0,1));CanvasSlot->SetOffsets(FMargin(20,0,20,GetWorld()->GetSubsystem<URailSimulationSubsystem>()->IsAIMode()?432:GetWorld()->GetSubsystem<URailSimulationSubsystem>()->IsOperationsMode()?406:276));CanvasSlot->SetAutoSize(false);
 Telemetry=WidgetTree->ConstructWidget<UTextBlock>();Telemetry->SetColorAndOpacity(FSlateColor(FLinearColor::White));Telemetry->SetFont(FSlateFontInfo(Telemetry->GetFont().FontObject,18));Column->AddChildToVerticalBox(Telemetry);
 auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Column->AddChildToVerticalBox(Row);
 auto Add=[&](const TCHAR* Label,void(URailDrivingWidget::*Handler)(),const FName Function){
  auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(58);
  auto* Button=WidgetTree->ConstructWidget<UButton>();Box->AddChild(Button);
  auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetText(FText::FromString(Label));Text->SetFont(FSlateFontInfo(Text->GetFont().FontObject,17));Text->SetColorAndOpacity(FSlateColor(FLinearColor::Black));Button->AddChild(Text);
  Button->OnClicked.__Internal_AddDynamic(this,Handler,Function);
  auto* Cell=Row->AddChildToHorizontalBox(Box);Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Cell->SetPadding(FMargin(3));
 };
 Add(TEXT("POWER 0"),&URailDrivingWidget::Throttle0,TEXT("Throttle0"));
 Add(TEXT("POWER 25%"),&URailDrivingWidget::Throttle25,TEXT("Throttle25"));
 Add(TEXT("POWER 50%"),&URailDrivingWidget::Throttle50,TEXT("Throttle50"));
 Add(TEXT("POWER 100%"),&URailDrivingWidget::Throttle100,TEXT("Throttle100"));
 Add(TEXT("BRAKE 0"),&URailDrivingWidget::Brake0,TEXT("Brake0"));
 Add(TEXT("BRAKE 50%"),&URailDrivingWidget::Brake50,TEXT("Brake50"));
 Add(TEXT("BRAKE 100%"),&URailDrivingWidget::Brake100,TEXT("Brake100"));
 Row=WidgetTree->ConstructWidget<UHorizontalBox>();Column->AddChildToVerticalBox(Row);
 Add(TEXT("REVERSE"),&URailDrivingWidget::Reverse,TEXT("Reverse"));
 Add(TEXT("NEUTRAL"),&URailDrivingWidget::Neutral,TEXT("Neutral"));
 Add(TEXT("FORWARD"),&URailDrivingWidget::Forward,TEXT("Forward"));
 Add(TEXT("EMERGENCY"),&URailDrivingWidget::Emergency,TEXT("Emergency"));
 Add(TEXT("RESET E-BRAKE"),&URailDrivingWidget::ResetEmergency,TEXT("ResetEmergency"));
 Add(TEXT("PAUSE / RESUME"),&URailDrivingWidget::Pause,TEXT("Pause"));
 Add(TEXT("RESET TEST"),&URailDrivingWidget::ResetTest,TEXT("ResetTest"));
 if(GetWorld()->GetSubsystem<URailSimulationSubsystem>()->IsOperationsMode()){
  Row=WidgetTree->ConstructWidget<UHorizontalBox>();Column->AddChildToVerticalBox(Row);
  if(GetWorld()->GetSubsystem<URailSimulationSubsystem>()->IsContinuityMode()){
   Add(TEXT("CHECKPOINT"),&URailDrivingWidget::Checkpoint,TEXT("Checkpoint"));
   Add(TEXT("LOOK BACK"),&URailDrivingWidget::LookBack,TEXT("LookBack"));
  }else{
   Add(TEXT("SELECT A"),&URailDrivingWidget::SelectA,TEXT("SelectA"));
   Add(TEXT("SELECT B"),&URailDrivingWidget::SelectB,TEXT("SelectB"));
  }
  Add(TEXT("ROUTE A"),&URailDrivingWidget::RouteA,TEXT("RouteA"));
  Add(TEXT("ROUTE B"),&URailDrivingWidget::RouteB,TEXT("RouteB"));
  Add(TEXT("POINT 2"),&URailDrivingWidget::Point,TEXT("Point"));
  Add(TEXT("CANCEL ROUTE"),&URailDrivingWidget::CancelRoute,TEXT("CancelRoute"));
  Add(TEXT("RECREATE VIEW"),&URailDrivingWidget::RecreateView,TEXT("RecreateView"));
 }
 Row=WidgetTree->ConstructWidget<UHorizontalBox>();Column->AddChildToVerticalBox(Row);
 Add(TEXT("CAB / OUTSIDE"),&URailDrivingWidget::CabCamera,TEXT("CabCamera"));
 Add(TEXT("WAP7 / WAG9"),&URailDrivingWidget::LocoClass,TEXT("LocoClass"));
 Add(TEXT("LAMPS"),&URailDrivingWidget::Lamps,TEXT("Lamps"));
 Add(TEXT("PANTOGRAPH"),&URailDrivingWidget::Pantograph,TEXT("Pantograph"));
 Add(TEXT("WIPERS"),&URailDrivingWidget::Wipers,TEXT("Wipers"));
 Add(TEXT("CAB LIGHT"),&URailDrivingWidget::CabLamp,TEXT("CabLamp"));

}
void URailDrivingWidget::NativeTick(const FGeometry& G,float Dt){Super::NativeTick(G,Dt);if(Telemetry)Telemetry->SetText(FText::FromString(GetWorld()->GetSubsystem<URailSimulationSubsystem>()->HandlingDisplay()));}
#define RAIL_INPUT(Name,Kind,Value) void URailDrivingWidget::Name(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SubmitPlayerCommand(Rail::CommandKind::Kind,Value);}
RAIL_INPUT(Throttle0,Throttle,0)
RAIL_INPUT(Throttle25,Throttle,.25)
RAIL_INPUT(Throttle50,Throttle,.5)
RAIL_INPUT(Throttle100,Throttle,1)
RAIL_INPUT(Brake0,Brake,0)
RAIL_INPUT(Brake50,Brake,.5)
RAIL_INPUT(Brake100,Brake,1)
RAIL_INPUT(Reverse,Direction,-1)
RAIL_INPUT(Neutral,Direction,0)
RAIL_INPUT(Forward,Direction,1)
RAIL_INPUT(Emergency,Emergency,1)
RAIL_INPUT(ResetEmergency,Emergency,0)
#undef RAIL_INPUT
void URailDrivingWidget::Pause(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->ToggleHandlingPause();}
void URailDrivingWidget::ResetTest(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->ResetHandlingFixture();}


void URailDrivingWidget::SelectA(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SelectTrain(1);}
void URailDrivingWidget::SelectB(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SelectTrain(2);}
void URailDrivingWidget::RouteA(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->RequestPlayerRoute(1);}
void URailDrivingWidget::RouteB(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->RequestPlayerRoute(2);}
void URailDrivingWidget::Point(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->RequestPlayerPoint();}
void URailDrivingWidget::CancelRoute(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->CancelPlayerRoute();}
void URailDrivingWidget::RecreateView(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->RecreateOperationsView();}
void URailDrivingWidget::Checkpoint(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SaveS6Checkpoint();}
void URailDrivingWidget::LookBack(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->ToggleS6LookBack();}


void URailDrivingWidget::CabCamera(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(0);}
void URailDrivingWidget::LocoClass(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(1);}
void URailDrivingWidget::Lamps(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(2);}
void URailDrivingWidget::Pantograph(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(3);}
void URailDrivingWidget::Wipers(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(4);}
void URailDrivingWidget::CabLamp(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(5);}
