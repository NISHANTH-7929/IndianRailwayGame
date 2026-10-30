#include "RailServiceWidget.h"
#include "RailControlIcon.h"
#include "RailDrivingHud.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "RailLocomotiveView.h"
#include "RailSimulationSubsystem.h"
#include "Simulation/RailScenarioContent.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "Components/Slider.h"
#include "Engine/World.h"
void URailServiceWidget::NativeOnInitialized(){
 Super::NativeOnInitialized();
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
 auto MakeText=[&](int Size){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetFont(FSlateFontInfo(T->GetFont().FontObject,Size));T->SetColorAndOpacity(FSlateColor(FLinearColor::White));T->SetAutoWrapText(true);return T;};
 Hud=MakeText(21);Hud->SetShadowColorAndOpacity(FLinearColor(0,0,0,.9f));Hud->SetShadowOffset(FVector2D(1,1));auto* H=Canvas->AddChildToCanvas(Hud);H->SetAnchors(FAnchors(0,0,1,0));H->SetOffsets(FMargin(20,15,20,250));
 Menu=WidgetTree->ConstructWidget<UVerticalBox>();MenuBackdrop=WidgetTree->ConstructWidget<UBorder>();MenuBackdrop->SetBrushColor(FLinearColor(.025f,.04f,.06f,.97f));MenuBackdrop->SetPadding(FMargin(18));MenuBackdrop->AddChild(Menu);auto* M=Canvas->AddChildToCanvas(MenuBackdrop);M->SetAnchors(FAnchors(.17f,.22f,.83f,.88f));M->SetOffsets(FMargin(0));
 ServiceMenu=WidgetTree->ConstructWidget<UVerticalBox>();Menu->AddChildToVerticalBox(ServiceMenu)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 PanelText=MakeText(22);
 // Long briefing/results scroll inside the panel; actions remain reachable.
 auto* Description=WidgetTree->ConstructWidget<UScrollBox>();Description->AddChild(PanelText);ServiceMenu->AddChildToVerticalBox(Description)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 auto MenuButton=[&](UTextBlock*& Label,void(URailServiceWidget::*Handler)(),FName Name){auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(65);auto* B=WidgetTree->ConstructWidget<UButton>();Box->AddChild(B);Label=MakeText(22);Label->SetAutoWrapText(false);Label->SetColorAndOpacity(FSlateColor(FLinearColor::Black));B->AddChild(Label);B->OnClicked.__Internal_AddDynamic(this,Handler,Name);ServiceMenu->AddChildToVerticalBox(Box);return B;};
 UTextBlock* Label=nullptr;MenuButton(Label,&URailServiceWidget::Primary,TEXT("Primary"));PrimaryText=Label;
 SecondaryButton=MenuButton(Label,&URailServiceWidget::Secondary,TEXT("Secondary"));SecondaryText=Label;
 // Replace the briefing content while adjusting sound instead of growing the pause menu.
 auto* AudioToggleBox=WidgetTree->ConstructWidget<USizeBox>();AudioToggleBox->SetHeightOverride(42);Menu->AddChildToVerticalBox(AudioToggleBox);
 auto* AudioToggle=WidgetTree->ConstructWidget<UButton>();AudioToggleBox->AddChild(AudioToggle);
 AudioToggleText=MakeText(18);AudioToggleText->SetAutoWrapText(false);AudioToggleText->SetColorAndOpacity(FSlateColor(FLinearColor::Black));AudioToggleText->SetText(FText::FromString(TEXT("SOUND SETTINGS")));AudioToggle->AddChild(AudioToggleText);
 AudioToggle->OnClicked.AddDynamic(this,&URailServiceWidget::ToggleAudioSettings);
 AudioControls=WidgetTree->ConstructWidget<UVerticalBox>();Menu->AddChildToVerticalBox(AudioControls);AudioControls->SetVisibility(ESlateVisibility::Collapsed);
 auto* AudioHelp=MakeText(16);AudioHelp->SetText(FText::FromString(TEXT("Adjust listening volume. 0% mutes; 100% is the full default mix. Engine controls traction; effects controls rolling, horn, brakes and alerts.")));AudioControls->AddChildToVerticalBox(AudioHelp);
 auto AddVolume=[&](const TCHAR* Caption,void(URailServiceWidget::*Handler)(float),FName Name,int32 Channel){
  auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(56);AudioControls->AddChildToVerticalBox(Box);
  auto* VolumeRow=WidgetTree->ConstructWidget<UHorizontalBox>();Box->AddChild(VolumeRow);
  auto* VolumeLabel=MakeText(18);VolumeLabel->SetAutoWrapText(false);AudioLabels.Add(VolumeLabel);
  auto* LabelBox=WidgetTree->ConstructWidget<USizeBox>();LabelBox->SetWidthOverride(165);LabelBox->AddChild(VolumeLabel);auto* LabelSlot=VolumeRow->AddChildToHorizontalBox(LabelBox);LabelSlot->SetVerticalAlignment(VAlign_Center);
  auto* Slider=WidgetTree->ConstructWidget<USlider>();Slider->SetMinValue(0);Slider->SetMaxValue(1);Slider->SetStepSize(.05f);Slider->SetValue(GetWorld()->GetSubsystem<URailSimulationSubsystem>()->GetServiceAudioVolume(Channel));
  Slider->SetToolTipText(FText::FromString(FString::Printf(TEXT("%s volume: drag, or focus and use arrow keys. 0%% mutes."),Caption)));
  Slider->OnValueChanged.__Internal_AddDynamic(this,Handler,Name);AudioSliders.Add(Slider);
  auto* SliderSlot=VolumeRow->AddChildToHorizontalBox(Slider);SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));SliderSlot->SetVerticalAlignment(VAlign_Center);SliderSlot->SetPadding(FMargin(12,0,12,0));
 };
 AddVolume(TEXT("Master"),&URailServiceWidget::MasterVolumeChanged,TEXT("MasterVolumeChanged"),0);
 AddVolume(TEXT("Engine"),&URailServiceWidget::EngineVolumeChanged,TEXT("EngineVolumeChanged"),1);
 AddVolume(TEXT("Effects"),&URailServiceWidget::EffectsVolumeChanged,TEXT("EffectsVolumeChanged"),2);
 Controls=WidgetTree->ConstructWidget<UVerticalBox>();auto* C=Canvas->AddChildToCanvas(Controls);C->SetAnchors(FAnchors(0,1,1,1));C->SetAlignment(FVector2D(0,1));C->SetOffsets(FMargin(15,0,15,176));
 auto* Row=WidgetTree->ConstructWidget<UHorizontalBox>();Controls->AddChildToVerticalBox(Row);PowerRow=Row;
 auto Add=[&](const TCHAR* Caption,void(URailServiceWidget::*Handler)(),FName Name){auto* Box=WidgetTree->ConstructWidget<USizeBox>();Box->SetHeightOverride(40);auto* B=WidgetTree->ConstructWidget<UButton>();Box->AddChild(B);auto* T=MakeText(13);T->SetAutoWrapText(false);T->SetText(FText::FromString(Caption));T->SetColorAndOpacity(FSlateColor(FLinearColor::Black));B->AddChild(T);B->OnClicked.__Internal_AddDynamic(this,Handler,Name);auto* Cell=Row->AddChildToHorizontalBox(Box);Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Cell->SetPadding(FMargin(3));};
#define ADD(Name,Caption) Add(TEXT(Caption),&URailServiceWidget::Name,TEXT(#Name))
 ADD(Power0,"POWER 0");ADD(Power25,"POWER 25%");ADD(Power50,"POWER 50%");ADD(Power100,"POWER 100%");ADD(Brake0,"RELEASE BRAKE");ADD(Brake50,"BRAKE 50%");ADD(Brake100,"BRAKE 100%");
 Row=WidgetTree->ConstructWidget<UHorizontalBox>();Controls->AddChildToVerticalBox(Row);
 ADD(Horn,"HORN");ADD(LookBack,"LOOK BACK");ADD(Pause,"PAUSE");ADD(Depart,"DEPART");ADD(Emergency,"EMERGENCY");ADD(ResetEmergency,"RESET E-BRAKE");
#if !UE_BUILD_SHIPPING
 ADD(Debug,"DEBUG");
#endif
 ADD(Exit,"SAVE + EXIT");
Row=WidgetTree->ConstructWidget<UHorizontalBox>();Controls->AddChildToVerticalBox(Row);
 ADD(CabCamera,"CAB / OUTSIDE");ADD(Lamps,"LAMPS");ADD(Pantograph,"PANTOGRAPH");ADD(Wipers,"WIPERS");ADD(CabLamp,"CAB LIGHT");
 Row=WidgetTree->ConstructWidget<UHorizontalBox>();Controls->AddChildToVerticalBox(Row);
 DirectionRow=Row;
 ADD(Reverse,"REVERSE");ADD(Neutral,"NEUTRAL");ADD(Forward,"FORWARD");
#undef ADD
 const auto& Content=GetWorld()->GetSubsystem<URailSimulationSubsystem>()->ReadOnlyOperations().ContentId();
 if(Rail::ScenarioContent::IsTambaram(Content)){
  DrivingHud=WidgetTree->ConstructWidget<URailDrivingHud>();
  auto* DrivingSlot=Canvas->AddChildToCanvas(DrivingHud);DrivingSlot->SetAnchors(FAnchors(0,0,1,1));DrivingSlot->SetOffsets(FMargin(0));DrivingSlot->SetZOrder(3);
 }else if(Rail::ScenarioContent::IsS8Route(Content))BuildEgmoreControls(Canvas);
 if(!Rail::ScenarioContent::UsesLhb(GetWorld()->GetSubsystem<URailSimulationSubsystem>()->ReadOnlyOperations().ContentId())){MenuButton(Label,&URailServiceWidget::LocoClass,TEXT("LocoClass"));Label->SetText(FText::FromString(TEXT("WAP7 / WAG9 (paused at standstill)")));}
}
void URailServiceWidget::BuildEgmoreControls(UCanvasPanel* Canvas){
 // Every interactive rectangle is inside the bottom 14%, reserved by cab input.
 EgmoreControls=WidgetTree->ConstructWidget<UCanvasPanel>();
 auto* Dock=Canvas->AddChildToCanvas(EgmoreControls);Dock->SetAnchors(FAnchors(0,.862f,1,.998f));Dock->SetOffsets(FMargin(0));Dock->SetZOrder(2);
 auto* Background=WidgetTree->ConstructWidget<UBorder>();Background->SetBrushColor(FLinearColor(.012f,.022f,.029f,.97f));auto* Bg=EgmoreControls->AddChildToCanvas(Background);Bg->SetAnchors(FAnchors(0,0,1,1));Bg->SetOffsets(FMargin(0));Background->SetVisibility(ESlateVisibility::HitTestInvisible);
 const FLinearColor Normal(.045f,.075f,.09f,1),Traction(.055f,.16f,.18f,1),Brake(.19f,.12f,.055f,1),Danger(.46f,.025f,.025f,1);
 auto Add=[&](const TCHAR* Caption,void(URailServiceWidget::*Handler)(),FName Name,float X0,float X1,int Row,FLinearColor Color,bool Icon){
  auto* Button=WidgetTree->ConstructWidget<UButton>();
  FButtonStyle Style=Button->GetStyle();
  // Solid brushes avoid the engine's grey beveled default button surface.
  Style.Normal.DrawAs=ESlateBrushDrawType::Box;Style.Normal.SetResourceObject(nullptr);Style.Normal.Margin=FMargin(0);Style.Normal.TintColor=FSlateColor(Color);
  Style.Hovered=Style.Normal;Style.Hovered.TintColor=FSlateColor(Color*1.35f);Style.Pressed=Style.Normal;Style.Pressed.TintColor=FSlateColor(Color*.7f);Style.Disabled=Style.Normal;
  Style.NormalPadding=FMargin(4,2);Style.PressedPadding=FMargin(4,3,4,1);Button->SetStyle(Style);
  Button->SetToolTipText(FText::FromString(Caption));Button->OnClicked.__Internal_AddDynamic(this,Handler,Name);
  auto* Cell=EgmoreControls->AddChildToCanvas(Button);Cell->SetAnchors(FAnchors(X0,Row==0?.055f:.53f,X1,Row==0?.47f:.945f));Cell->SetOffsets(FMargin(2,0,2,0));
  auto* Fit=WidgetTree->ConstructWidget<UScaleBox>();Fit->SetStretch(EStretch::ScaleToFit);Fit->SetStretchDirection(EStretchDirection::DownOnly);Button->AddChild(Fit);
  auto* Content=WidgetTree->ConstructWidget<UHorizontalBox>();Fit->AddChild(Content);
  if(Icon){auto* Mark=WidgetTree->ConstructWidget<URailControlIcon>();Mark->Kind=Name;auto* IS=Content->AddChildToHorizontalBox(Mark);IS->SetVerticalAlignment(VAlign_Center);IS->SetPadding(FMargin(0,0,5,0));}
  auto* Text=WidgetTree->ConstructWidget<UTextBlock>();Text->SetFont(FSlateFontInfo(Text->GetFont().FontObject,12));Text->SetText(FText::FromString(Caption));Text->SetColorAndOpacity(FSlateColor(FLinearColor(.9f,.95f,.97f)));auto* TS=Content->AddChildToHorizontalBox(Text);TS->SetVerticalAlignment(VAlign_Center);
 };
#define DOCK(Name,Caption,X0,X1,Row,Color,Icon) Add(TEXT(Caption),&URailServiceWidget::Name,TEXT(#Name),X0,X1,Row,Color,Icon)
 DOCK(Power0,"PWR 0",.005f,.061f,0,Traction,false);DOCK(Power10,"10%",.061f,.117f,0,Traction,false);DOCK(Power25,"25%",.117f,.173f,0,Traction,false);DOCK(Power50,"50%",.173f,.229f,0,Traction,false);DOCK(Power100,"100%",.229f,.285f,0,Traction,false);
 DOCK(Brake0,"RELEASE",.005f,.061f,1,Brake,false);DOCK(Brake15,"15%",.061f,.117f,1,Brake,false);DOCK(Brake30,"30%",.117f,.173f,1,Brake,false);DOCK(Brake50,"50%",.173f,.229f,1,Brake,false);DOCK(Brake100,"100%",.229f,.285f,1,Brake,false);
 DOCK(Reverse,"REV",.295f,.338f,0,Normal,false);DOCK(Neutral,"N",.338f,.381f,0,Normal,false);DOCK(Forward,"FWD",.381f,.425f,0,Normal,false);DOCK(Depart,"DEPART",.295f,.425f,1,Traction,true);
 DOCK(Horn,"HORN",.435f,.4975f,0,Normal,true);DOCK(Lamps,"LAMPS",.4975f,.56f,0,Normal,true);DOCK(Pantograph,"PANTO",.56f,.6225f,0,Normal,true);DOCK(Wipers,"WIPER",.6225f,.685f,0,Normal,true);
 DOCK(CabLamp,"CAB LIGHT",.435f,.5183f,1,Normal,true);DOCK(Instruments,"LABELS",.5183f,.6017f,1,Normal,true);DOCK(LookBack,"LOOK BACK",.6017f,.685f,1,Normal,true);
 DOCK(Pause,"PAUSE",.695f,.78f,0,Normal,true);DOCK(CabCamera,"CAMERA",.78f,.865f,0,Normal,true);DOCK(ToggleHud,"HUD",.695f,.78f,1,Normal,false);DOCK(Exit,"SAVE / EXIT",.78f,.865f,1,Normal,true);
 DOCK(Emergency,"EMERGENCY",.875f,.995f,0,Danger,true);DOCK(ResetEmergency,"RESET BRAKE",.875f,.995f,1,Brake,true);
#undef DOCK
}
void URailServiceWidget::NativeTick(const FGeometry& G,float Dt){
 Super::NativeTick(G,Dt);auto* S=GetWorld()->GetSubsystem<URailSimulationSubsystem>();const auto Phase=S->ReadOnlyService().Current().phase;
 const bool Running=Phase==Rail::ServicePhase::Running;
#if !UE_BUILD_SHIPPING
 // Review captures show the driving interface over the paused, unchanged service state.
 static const bool HudReview=FParse::Param(FCommandLine::Get(),TEXT("TambaramHudReview"));
#else
 constexpr bool HudReview=false;
#endif
 const bool Driving=Running&&(!S->IsServicePaused()||HudReview);
 if(Driving)bAudioSettingsOpen=false;
 ServiceMenu->SetVisibility(bAudioSettingsOpen?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
 AudioControls->SetVisibility(bAudioSettingsOpen?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 AudioToggleText->SetText(FText::FromString(bAudioSettingsOpen?TEXT("BACK TO SERVICE"):TEXT("SOUND SETTINGS")));
 const TCHAR* AudioNames[]={TEXT("Master"),TEXT("Engine"),TEXT("Effects")};
 for(int32 Channel=0;Channel<AudioSliders.Num();++Channel){const float Volume=S->GetServiceAudioVolume(Channel);if(!FMath::IsNearlyEqual(AudioSliders[Channel]->GetValue(),Volume,.001f))AudioSliders[Channel]->SetValue(Volume);AudioLabels[Channel]->SetText(FText::FromString(FString::Printf(TEXT("%s  %d%%"),AudioNames[Channel],FMath::RoundToInt(Volume*100))));}
 MenuBackdrop->SetVisibility(Driving?ESlateVisibility::Collapsed:ESlateVisibility::Visible);Controls->SetVisibility(Driving?ESlateVisibility::Visible:ESlateVisibility::Collapsed);Hud->SetVisibility(Driving?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
 const bool Cab=S->IsLocomotiveCabCamera();
 PowerRow->SetVisibility(Cab?ESlateVisibility::Collapsed:ESlateVisibility::Visible);DirectionRow->SetVisibility(Cab?ESlateVisibility::Collapsed:ESlateVisibility::Visible);
 if(EgmoreControls){Controls->SetVisibility(ESlateVisibility::Collapsed);EgmoreControls->SetVisibility(Driving?ESlateVisibility::Visible:ESlateVisibility::Collapsed);}
 else if(auto* Layout=Cast<UCanvasPanelSlot>(Controls->Slot))Layout->SetOffsets(FMargin(15,0,15,Cab?92:176));
 if(DrivingHud){
  Controls->SetVisibility(ESlateVisibility::Collapsed);Hud->SetVisibility(ESlateVisibility::Collapsed);
  DrivingHud->SetVisibility(Driving?ESlateVisibility::Visible:ESlateVisibility::Collapsed);
  DrivingHud->Synchronize(S,Driving);
  if(auto* Loco=S->PlayerLocomotive())Loco->SetDrivingHudInput(true,!Driving||DrivingHud->IsCabDrawerOpen());
 }
 FString HudText=S->ServiceHudText();
 // Hide only the two routine journey rows, never notices or approaching restrictions.
 if(bQuietHud&&Rail::ScenarioContent::IsTambaram(S->ReadOnlyOperations().ContentId())){
  TArray<FString> Lines;HudText.ParseIntoArrayLines(Lines);
  if(Lines.Num()>=4){HudText=Lines[0]+TEXT("\n")+Lines[1];for(int32 I=4;I<Lines.Num();++I)HudText+=TEXT("\n")+Lines[I];}
 }
 Hud->SetText(FText::FromString(HudText));PanelText->SetText(FText::FromString(S->ServicePanelText()));PrimaryText->SetText(FText::FromString(S->ServicePrimaryLabel()));
 const bool NeedsRestore=Running&&S->ReadOnlyService().Current().accumulator>4.0/30.0+1e-9;
 SecondaryButton->SetVisibility(Phase==Rail::ServicePhase::Briefing||(Running&&!NeedsRestore)?ESlateVisibility::Visible:ESlateVisibility::Collapsed);SecondaryText->SetText(FText::FromString(Running?TEXT("SAVE AND EXIT"):TEXT("SWITCH ASSISTED / STANDARD")));
}
void URailServiceWidget::ToggleAudioSettings(){bAudioSettingsOpen=!bAudioSettingsOpen;}
void URailServiceWidget::ToggleHud(){bQuietHud=!bQuietHud;}
void URailServiceWidget::MasterVolumeChanged(float Value){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SetServiceAudioVolume(0,Value);}
void URailServiceWidget::EngineVolumeChanged(float Value){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SetServiceAudioVolume(1,Value);}
void URailServiceWidget::EffectsVolumeChanged(float Value){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SetServiceAudioVolume(2,Value);}
#define INPUT(Name,Kind,Value) void URailServiceWidget::Name(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->SubmitPlayerCommand(Rail::CommandKind::Kind,Value);}
INPUT(Reverse,Direction,-1) INPUT(Neutral,Direction,0) INPUT(Forward,Direction,1)
INPUT(Power0,Throttle,0) INPUT(Power10,Throttle,.1) INPUT(Power25,Throttle,.25) INPUT(Power50,Throttle,.5) INPUT(Power100,Throttle,1)
INPUT(Brake0,Brake,0) INPUT(Brake15,Brake,.15) INPUT(Brake30,Brake,.30) INPUT(Brake50,Brake,.5) INPUT(Brake100,Brake,1) INPUT(ResetEmergency,Emergency,0)
#undef INPUT
#define ACTION(Name,Id) void URailServiceWidget::Name(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->ServiceAction(Id);}
ACTION(Primary,0) ACTION(Secondary,1) ACTION(Pause,2) ACTION(Depart,3) ACTION(Horn,4) ACTION(LookBack,5) ACTION(Debug,6) ACTION(Emergency,7) ACTION(Exit,8)
#undef ACTION


void URailServiceWidget::CabCamera(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(0);}
void URailServiceWidget::LocoClass(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(1);}
void URailServiceWidget::Lamps(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(2);}
void URailServiceWidget::Pantograph(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(3);}
void URailServiceWidget::Wipers(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(4);}
void URailServiceWidget::CabLamp(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(5);}
void URailServiceWidget::Instruments(){GetWorld()->GetSubsystem<URailSimulationSubsystem>()->LocomotiveAction(6);}
