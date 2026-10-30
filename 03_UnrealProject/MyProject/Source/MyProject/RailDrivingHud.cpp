#include "RailDrivingHud.h"
#include "RailSimulationSubsystem.h"
#include "RailLocomotiveView.h"
#include "RailControlPresentation.h"
#include "Simulation/RailDrivingControlModel.h"
#include "Simulation/RailDrivingAdvice.h"
#include "Simulation/RailJourneyPresentation.h"
#include "Simulation/RailTambaramDriving.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Styling/CoreStyle.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "InputCoreTypes.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/AutomationTest.h"

namespace {
enum Action { Power=0,Brake,Horn,Camera,Pause,Emergency,ResetEmergency,Drawer,CloseDrawer,Lamps,Pantograph,Wipers,CabLamp,Instruments,Reverse,Neutral,DirectionForward,Depart,PowerMinus,PowerPlus,BrakeMinus,BrakePlus };
struct FDriveDisplay {
 double Speed=0,Limit=0,Power=0,Brake=1,AppliedPower=0,AppliedBrake=1,Progress=0;
 bool Assisted=false,Emergency=false,CanReset=false,CanDepart=false,Enabled=false,Alert=false;
 FString Station,Location,Status,Detail,Time,Direction,Feedback;
};
const FLinearColor Ink(.012f,.017f,.025f,.87f),Edge(.30f,.36f,.41f,.60f),White(.94f,.96f,.98f,1),Muted(.57f,.65f,.70f,1),Amber(1.f,.56f,.16f,1),Mint(.28f,.86f,.72f,1),Red(1.f,.19f,.15f,1);
struct FHit { int32 Kind;FSlateRect Rect;FHit(int32 K,float L,float T,float R,float B):Kind(K),Rect(L,T,R,B){} };
struct FHold {int32 Kind;double StartY,Value;};
FString DistanceText(double Metres){return FMath::Abs(Metres)>=1000?FString::Printf(TEXT("%.1f km"),FMath::Abs(Metres)/1000):FString::Printf(TEXT("%.0f m"),FMath::Abs(Metres));}
bool TraceHudInput(){
#if !UE_BUILD_SHIPPING
 static const bool Enabled=FParse::Param(FCommandLine::Get(),TEXT("RailHudInputTrace"));return Enabled;
#else
 return false;
#endif
}
}

class SRailDrivingHud : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SRailDrivingHud){} SLATE_END_ARGS()
 void Construct(const FArguments&) {SetCanTick(false);}
 TFunction<void(int32,double)> Execute;
 FDriveDisplay Data;
 bool CabOpen=false;
 double FeedbackUntil=0;
 void Update(const FDriveDisplay& Display){
  if(!Display.Enabled||Display.Emergency!=Data.Emergency){Holds.Empty();CabOpen=false;}
  if(Display.Feedback!=Data.Feedback)FeedbackUntil=FPlatformTime::Seconds()+6;
  Data=Display;Invalidate(EInvalidateWidgetReason::Paint);
 }
 virtual FVector2D ComputeDesiredSize(float)const override{return FVector2D(1280,720);}
 virtual bool SupportsKeyboardFocus()const override{return true;}
 virtual void OnFocusLost(const FFocusEvent&) override{if(TraceHudInput())UE_LOG(LogTemp,Display,TEXT("HUD_INPUT focus-lost holds=%d"),Holds.Num());Holds.Empty();}
 virtual void OnMouseCaptureLost(const FCaptureLostEvent& E) override{if(TraceHudInput())UE_LOG(LogTemp,Display,TEXT("HUD_INPUT capture-lost pointer=%u holds=%d"),uint32(E.PointerIndex),Holds.Num());Holds.Remove(uint32(E.PointerIndex)==FSlateApplication::CursorPointerIndex?100:E.PointerIndex);}
 virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)override{return E.GetEffectingButton()==EKeys::LeftMouseButton?Begin(G,E):FReply::Unhandled();}
 virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E)override{return Move(G,E);}
 virtual FReply OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E)override{return E.GetEffectingButton()==EKeys::LeftMouseButton?End(G,E):FReply::Unhandled();}
 virtual FReply OnTouchStarted(const FGeometry& G,const FPointerEvent& E)override{return Begin(G,E);}
 virtual FReply OnTouchMoved(const FGeometry& G,const FPointerEvent& E)override{return Move(G,E);}
 virtual FReply OnTouchEnded(const FGeometry& G,const FPointerEvent& E)override{return End(G,E);}
 virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E)override{
  if(!Data.Enabled)return FReply::Unhandled();
  const FKey Key=E.GetKey();
  if(Key==EKeys::Escape){if(CabOpen){CabOpen=false;Holds.Empty();}else Fire(Pause);return FReply::Handled();}
  if(CabOpen)return FReply::Unhandled();
  if(Key==EKeys::Up){Fire(Power,Rail::DrivingControl::StepValue(Data.Power,1));return FReply::Handled();}
  if(Key==EKeys::Down){Fire(Power,Rail::DrivingControl::StepValue(Data.Power,-1));return FReply::Handled();}
  if(Key==EKeys::Right){Fire(Brake,Rail::DrivingControl::StepValue(Data.Brake,1));return FReply::Handled();}
  if(Key==EKeys::Left){Fire(Brake,Rail::DrivingControl::StepValue(Data.Brake,-1));return FReply::Handled();}
  if(Key==EKeys::SpaceBar&&!E.IsRepeat()){Fire(Horn);return FReply::Handled();}
  return FReply::Unhandled();
 }
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& Geometry,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool)const override{
  const float Scale=Geometry.GetLocalSize().Y/720.f;
  if(Scale<=0)return Layer;
  const float W=Geometry.GetLocalSize().X/Scale;
  const FGeometry G=Geometry.MakeChild(FVector2D(W,720),FSlateLayoutTransform(Scale));
  auto Box=[&](float X,float Y,float Width,float Height,const FLinearColor& Color,float Radius=6.f){FSlateRoundedBoxBrush Brush(FLinearColor::White,Radius);FSlateDrawElement::MakeBox(Out,Layer++,G.ToPaintGeometry(FVector2D(Width,Height),FSlateLayoutTransform(FVector2D(X,Y))),&Brush,ESlateDrawEffect::None,Color);};
  auto Line=[&](const TArray<FVector2D>& Points,const FLinearColor& Color,float Width=1.f){FSlateDrawElement::MakeLines(Out,Layer++,G.ToPaintGeometry(),Points,ESlateDrawEffect::None,Color,true,Width);};
  auto Text=[&](const FString& Value,float X,float Y,int Size,const FLinearColor& Color=White,bool Centre=false){
   const FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),Size);
   const FVector2D Extent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value,Font);
   if(Centre)X-=Extent.X*.5f;
   FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(Extent,FSlateLayoutTransform(FVector2D(X+1,Y+1))),Value,Font,ESlateDrawEffect::None,FLinearColor(0,0,0,.6f));
   FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(Extent,FSlateLayoutTransform(FVector2D(X,Y))),Value,Font,ESlateDrawEffect::None,Color);
  };
  auto Fitted=[&](FString Value,float X,float Y,int Size,float Width,const FLinearColor& Color){const auto Font=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),Size);const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();if(Measure->Measure(Value,Font).X>Width){while(Value.Len()>1&&Measure->Measure(Value+TEXT("…"),Font).X>Width)Value.LeftChopInline(1);Value+=TEXT("…");}Text(Value,X,Y,Size,Color);};
  auto DetailLines=[&](const FString& Value,float X,float Y,float Width){
   TArray<FString> Words;Value.ParseIntoArray(Words,TEXT(" "),true);FString First,Second;bool Next=false;
   const auto Font=FCoreStyle::GetDefaultFontStyle(TEXT("Regular"),12);const auto Measure=FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
   for(const FString& Word:Words){const FString Candidate=First.IsEmpty()?Word:First+TEXT(" ")+Word;if(!Next&&Measure->Measure(Candidate,Font).X<=Width)First=Candidate;else{Next=true;Second+=(Second.IsEmpty()?TEXT(""):TEXT(" "))+Word;}}
   Fitted(First,X,Y,12,Width,Muted);if(!Second.IsEmpty())Fitted(Second,X,Y+20,12,Width,Muted);
  };
  auto Arc=[&](FVector2D C,float Radius,float Start,float End,const FLinearColor& Color,float Width){TArray<FVector2D> P;for(int I=0;I<=48;++I){const float A=FMath::DegreesToRadians(FMath::Lerp(Start,End,I/48.f));P.Add(C+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius);}Line(P,Color,Width);};
  auto Button=[&](int Kind,const FString& Label,const FLinearColor& Color=White){for(const auto& Hit:Layout(W))if(Hit.Kind==Kind){const auto& R=Hit.Rect;Box(R.Left,R.Top,R.Right-R.Left,R.Bottom-R.Top,Ink);Line({{R.Left+8,R.Bottom-1},{R.Right-8,R.Bottom-1}},Color.CopyWithNewOpacity(.6f),1.f);Text(Label,(R.Left+R.Right)/2,R.Top+(R.Bottom-R.Top-20)/2,14,Color,true);}};

  // Journey information occupies one corner; the windscreen is the background.
  Box(24,24,382,96,Ink,5);Box(24,24,3,96,Amber,0);
  Text(TEXT("CHENNAI EGMORE  /  TAMBARAM"),40,34,12,Muted);
  Text(Data.Station,40,54,24);Text(Data.Location,40,88,13,Muted);
  Box(24,119,382,2,Edge,0);Box(24,119,382*float(Data.Progress),2,Amber,0);
  Text(Data.Assisted?TEXT("ASSISTED"):TEXT("LOCO PILOT"),W-224,29,13,White,true);Text(Data.Time,W-224,50,18,White,true);Text(Data.Direction,W-224,79,12,Muted,true);
  Button(Pause,TEXT("II"));Button(Drawer,TEXT("CAB"));

  auto Lever=[&](bool IsPower){
   const float X=IsPower?W-132:28;
   const double Requested=IsPower?Data.Power:Data.Brake,Applied=IsPower?Data.AppliedPower:Data.AppliedBrake;
   const FLinearColor Color=IsPower?Mint:Amber;
   Box(X,229,104,316,Ink,5);Box(X,229,104,2,Color.CopyWithNewOpacity(.8f),0);
   Text(IsPower?(Data.Assisted?TEXT("POWER LIMIT"):TEXT("POWER")):TEXT("TRAIN BRAKE"),X+52,240,12,White,true);
   Text(FString::Printf(TEXT("%.0f"),Requested*100),X+49,259,27,White,true);Text(TEXT("%"),X+75,273,12,Muted);
   const float Top=316,Bottom=493,Travel=Bottom-Top;
   Box(X+44,Top-4,16,Travel+8,FLinearColor(.004f,.006f,.009f,.96f),7);
   for(int I=0;I<=10;++I){const float Y=Bottom-Travel*I/10.f;Line({{X+15,Y},{X+(I%5?25:32),Y}},I%5?Edge:Muted,1);}
   Box(X+78,Bottom-Travel*float(Applied),3,FMath::Max(3.f,Travel*float(Applied)),Color,1);
   const float HandleY=Bottom-Travel*float(Requested);
   Box(X+15,HandleY-18,74,38,FLinearColor(0,0,0,.7f),5);
   Box(X+16,HandleY-19,72,32,FLinearColor(.14f,.18f,.22f,1),4);
   Box(X+18,HandleY-18,68,3,FLinearColor(.45f,.51f,.55f,1),1);
   Box(X+18,HandleY+7,68,5,FLinearColor(.035f,.045f,.055f,1),1);
   Box(X+38,HandleY-10,28,3,Color,1);
   Line({{X+37,HandleY-2},{X+67,HandleY-2}},FLinearColor(.04f,.055f,.07f,1),2);
   Text(FString::Printf(TEXT("APPLIED %.0f%%"),Applied*100),X+52,516,11,Muted,true);
   Button(IsPower?PowerMinus:BrakeMinus,TEXT("−"));Button(IsPower?PowerPlus:BrakePlus,TEXT("+"));
  };
  Lever(false);Lever(true);

  // Speed dial: actual speed, current whole-train limit and direction remain
  // readable independently of cab camera angle or physical gauge visibility.
  const FVector2D C(248,622);Box(C.X-77,C.Y-77,154,154,Ink,77);
  Arc(C,64,140,400,Edge,3);Arc(C,64,140,140+260*FMath::Clamp(float(Data.Speed/160.),0.f,1.f),Data.Speed>Data.Limit+.36?Red:White,3);
  for(int I=0;I<=16;++I){const float A=FMath::DegreesToRadians(140.f+260.f*I/16.f);const FVector2D V(FMath::Cos(A),FMath::Sin(A));Line({C+V*(I%4?56:52),C+V*60},Muted,I%4?1:2);}
  const float LimitAngle=FMath::DegreesToRadians(140.f+260.f*FMath::Clamp(float(Data.Limit/160.),0.f,1.f));
  const FVector2D LimitV(FMath::Cos(LimitAngle),FMath::Sin(LimitAngle));Line({C+LimitV*61,C+LimitV*70},Amber,4);
  Text(FString::Printf(TEXT("%.0f"),Data.Speed),C.X,C.Y-31,40,White,true);Text(TEXT("km/h"),C.X,C.Y+19,12,Muted,true);
  Text(FString::Printf(TEXT("LIMIT %.0f"),Data.Limit),C.X,C.Y+47,13,Amber,true);
  Button(Horn,TEXT("HORN"));Button(Camera,TEXT("VIEW"));Button(Emergency,TEXT("EMERGENCY"),Red);
  if(Data.CanReset)Button(ResetEmergency,TEXT("RESET BRAKE"),Amber);

  const float CentreWidth=FMath::Clamp(W-720.f,300.f,680.f),CentreX=(W-CentreWidth)*.5f;
  Box(CentreX,613,CentreWidth,82,Ink,5);Box(CentreX,613,3,82,Data.Alert?Amber:Edge,0);
  const float NoticeWidth=CentreWidth-(Data.CanDepart?154:32);
  Fitted(Data.Status,CentreX+16,623,16,NoticeWidth,Data.Alert?Amber:White);DetailLines(Data.Detail,CentreX+16,650,NoticeWidth);
  if(Data.CanDepart)Button(Depart,TEXT("DEPART"),Mint);

  if(CabOpen){
   Box(0,0,W,720,FLinearColor(0,0,0,.55f),0);
   const float X=W-486;Box(X,140,462,424,FLinearColor(.018f,.024f,.034f,.99f),7);
   Text(TEXT("CAB CONTROLS"),X+24,161,23);Text(TEXT("Driving mode is chosen in the service briefing."),X+24,201,13,Muted);
   Button(CloseDrawer,TEXT("CLOSE"));Button(Lamps,TEXT("HEADLIGHTS"));Button(Pantograph,TEXT("PANTOGRAPH"));Button(Wipers,TEXT("WIPERS"));Button(CabLamp,TEXT("CAB LIGHT"));Button(Instruments,TEXT("INSTRUMENTS"));Button(Reverse,TEXT("REVERSE"));Button(Neutral,TEXT("NEUTRAL"));Button(DirectionForward,TEXT("FORWARD"));
   Text(TEXT("Direction changes require a stopped train."),X+24,524,13,Muted);
   Button(Pause,TEXT("II"));Button(Emergency,TEXT("EMERGENCY"),Red);
  }
  if(!Data.Feedback.IsEmpty()&&FPlatformTime::Seconds()<FeedbackUntil){Box(CentreX,570,CentreWidth,38,Ink,5);Fitted(Data.Feedback,CentreX+12,579,12,CentreWidth-24,Amber);}
  return Layer;
 }
private:
 TMap<int32,FHold> Holds;
 TArray<FHit> Layout(float W)const{
  if(CabOpen){const float X=W-486;return {{CloseDrawer,W-126,151,W-40,194},{Lamps,X+24,239,X+220,294},{Pantograph,X+238,239,W-48,294},{Wipers,X+24,309,X+220,364},{CabLamp,X+238,309,W-48,364},{Instruments,X+24,379,W-48,434},{Reverse,X+24,454,X+154,506},{Neutral,X+166,454,X+296,506},{DirectionForward,X+308,454,W-48,506},{Pause,W-88,24,W-24,90},{Emergency,W-132,613,W-24,679}};}
  TArray<FHit> Out{{Pause,W-88,24,W-24,90},{Drawer,W-162,24,W-98,90},{Brake,28,297,132,513},{Power,W-132,297,W-28,513},{BrakeMinus,28,550,77,596},{BrakePlus,83,550,132,596},{PowerMinus,W-132,550,W-83,596},{PowerPlus,W-77,550,W-28,596},{Horn,28,613,132,679},{Emergency,W-132,613,W-24,679}};
  if(Data.CanReset)Out.Add({ResetEmergency,W-290,613,W-148,679});else Out.Add({Camera,W-210,613,W-148,679});
  if(Data.CanDepart){const float Right=(W+FMath::Clamp(W-720.f,300.f,680.f))*.5f;Out.Add({Depart,Right-124,613,Right,679});}
  return Out;
 }
 void Fire(int32 Kind,double Value=0){if(Data.Emergency&&(Kind==Power||Kind==Brake))return;if(Execute)Execute(Kind,Value);}
 static int32 Key(const FPointerEvent& E){return E.IsTouchEvent()?int32(E.GetPointerIndex()):100;}
 FVector2D Position(const FGeometry& G,const FPointerEvent& E)const{return G.AbsoluteToLocal(E.GetScreenSpacePosition())/(G.GetLocalSize().Y/720.f);}
 FReply Begin(const FGeometry& G,const FPointerEvent& E){
  if(!Data.Enabled)return FReply::Unhandled();
  const auto P=Position(G,E);const float W=G.GetLocalSize().X/(G.GetLocalSize().Y/720.f);
  if(TraceHudInput())UE_LOG(LogTemp,Display,TEXT("HUD_INPUT begin key=%d x=%.1f y=%.1f holds=%d"),Key(E),P.X,P.Y,Holds.Num());
  for(const auto& Hit:Layout(W))if(Hit.Rect.ContainsPoint(P)){
   const int32 Kind=Hit.Kind;
   if(Kind==Power||Kind==Brake){
    if(Data.Emergency)return FReply::Handled();
    for(const auto& Pair:Holds)if(Pair.Value.Kind==Kind)return FReply::Handled();
    Holds.Add(Key(E),{Kind,P.Y,Kind==Power?Data.Power:Data.Brake});
    return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this));
   }
   if(Kind==Drawer||Kind==CloseDrawer){CabOpen=Kind==Drawer;Holds.Empty();}
   else if(Kind==PowerMinus||Kind==PowerPlus)Fire(Power,Rail::DrivingControl::StepValue(Data.Power,Kind==PowerPlus?1:-1));
   else if(Kind==BrakeMinus||Kind==BrakePlus)Fire(Brake,Rail::DrivingControl::StepValue(Data.Brake,Kind==BrakePlus?1:-1));
   else {if(Kind==Pause||Kind==Emergency)Holds.Empty();Fire(Kind);}
   Invalidate(EInvalidateWidgetReason::Paint);return FReply::Handled().SetUserFocus(SharedThis(this));
  }
  if(CabOpen){CabOpen=false;return FReply::Handled();}
  return FReply::Unhandled();
 }
 FReply Move(const FGeometry& G,const FPointerEvent& E){
  const auto* Hold=Holds.Find(Key(E));if(!Hold)return FReply::Unhandled();
  if(!Data.Enabled||Data.Emergency){Holds.Empty();return FReply::Handled().ReleaseMouseCapture();}
  const double Value=Rail::DrivingControl::DragValue(Hold->Value,Hold->StartY,Position(G,E).Y,177);
  if(TraceHudInput())UE_LOG(LogTemp,Display,TEXT("HUD_INPUT move key=%d kind=%d start=%.1f y=%.1f value=%.2f"),Key(E),Hold->Kind,Hold->StartY,Position(G,E).Y,Value);
  Fire(Hold->Kind,Value);return FReply::Handled();
 }
 FReply End(const FGeometry& G,const FPointerEvent& E){
  if(TraceHudInput())UE_LOG(LogTemp,Display,TEXT("HUD_INPUT end key=%d y=%.1f holds=%d"),Key(E),Position(G,E).Y,Holds.Num());
  if(const auto* Hold=Holds.Find(Key(E))){
   // Fast pointer gestures can begin/end within one frame, coalescing every
   // move event. Commit the release position before relinquishing capture.
   if(Data.Enabled&&!Data.Emergency)Fire(Hold->Kind,Rail::DrivingControl::DragValue(Hold->Value,Hold->StartY,Position(G,E).Y,177));
   Holds.Remove(Key(E));return FReply::Handled().ReleaseMouseCapture();
  }
  return FReply::Unhandled();
 }
};

TSharedRef<SWidget> URailDrivingHud::RebuildWidget(){
 DrivingSlate=SNew(SRailDrivingHud);DrivingSlate->Execute=[WeakThis=TWeakObjectPtr<URailDrivingHud>(this)](int32 Kind,double Value){if(WeakThis.IsValid())WeakThis->Dispatch(Kind,Value);};return DrivingSlate.ToSharedRef();
}
void URailDrivingHud::ReleaseSlateResources(bool Children){Super::ReleaseSlateResources(Children);DrivingSlate.Reset();}
bool URailDrivingHud::IsCabDrawerOpen()const{return DrivingSlate.IsValid()&&DrivingSlate->CabOpen;}
void URailDrivingHud::Synchronize(URailSimulationSubsystem* S,bool Driving){
 Railway=S;if(!DrivingSlate.IsValid()||!S)return;
 FDriveDisplay D;D.Enabled=Driving;
 const auto& Service=S->ReadOnlyService();const auto& State=Service.Current();const auto& Def=Service.Definition();const auto& World=S->ReadOnlyOperations();
 const auto* Handling=World.Handling(Def.player);const auto* Train=World.Spatial(Def.player);
 if(!Handling||!Train){DrivingSlate->Update(D);return;}
 const auto Control=Rail::ControlPresentation::From(*Handling);const double Head=Train->Current().headStationMetres,Length=Train->Current().properties.lengthMetres;
 D.Speed=FMath::Abs(Handling->speedMps)*3.6;D.Limit=Rail::TambaramDriving::ApplicableLimit(Head,Length,Def.speedLimit)*3.6;
 D.Power=State.requestedPower;D.Brake=State.requestedBrake;D.AppliedPower=Control.power;D.AppliedBrake=Control.brake;D.Emergency=Control.emergency;
 D.CanReset=D.Emergency&&D.Speed<.036&&D.Power<=.001;D.CanDepart=State.station==Rail::StationPhase::ReadyToDepart&&!Def.terminalStop;
 D.Assisted=State.mode==Rail::DrivingMode::Assisted;
 D.Direction=Control.direction>0?TEXT("FORWARD"):Control.direction<0?TEXT("REVERSE"):TEXT("NEUTRAL");
 const auto Journey=Rail::JourneyPresentation::At(Head,Length);D.Progress=Journey.progress;D.Station=UTF8_TO_TCHAR(Rail::TambaramRoute::StationNames[Journey.station]);
 D.Location=(Journey.terminal?TEXT("SCHEDULED STOP  /  "):Journey.alongside?TEXT("PASSING STATION  /  "):TEXT("NEXT STATION  /  "))+DistanceText(Journey.distance)+(Journey.distance<0?TEXT(" behind"):TEXT(" ahead"));
 D.Time=FString::Printf(TEXT("%02llu:%02llu"),State.ticks/1800,(State.ticks/30)%60);
 D.Status=TEXT("Route authorised");D.Detail=TEXT("Observe lineside signals and speed limits");
 const auto Authority=World.Current().authorities.find(Def.player);
 const bool RouteHeld=Authority!=World.Current().authorities.end()&&!Authority->second.routeGranted;
 if(D.Power>0&&D.AppliedBrake>.02){D.Status=TEXT("Traction held");D.Detail=TEXT("Brakes are still applied. Release before powering.");}
 if(D.Assisted&&D.Power>0&&D.Brake==0&&D.AppliedBrake>.02){D.Status=TEXT("Assistance braking");D.Detail=TEXT("Your power setting is the ceiling; brake overrides assist.");}
 if(Control.direction==0){D.Status=TEXT("Neutral selected");D.Detail=TEXT("Select Forward in CAB before applying power");}
 if(RouteHeld){D.Status=TEXT("Route held");D.Detail=UTF8_TO_TCHAR(Service.HoldReason(World).c_str());D.Alert=true;if(!S->DrivingDispatchNotice().IsEmpty()){D.Status=TEXT("Dispatcher hold");D.Detail=S->DrivingDispatchNotice();}}
 const double Approach=Rail::TambaramDriving::PermittedSpeed(Head,Length,Def.speedLimit);
 switch(Rail::StandardDrivingAdvice(Handling->speedMps,D.Limit/3.6,Approach,Def.berthHead-Head,Def.berthTolerance,State.station==Rail::StationPhase::Dwelling)){
  case Rail::DrivingAdvice::Overspeed:D.Status=TEXT("Reduce speed");D.Detail=FString::Printf(TEXT("Current limit %.0f km/h — cut power and brake"),D.Limit);D.Alert=true;break;
  case Rail::DrivingAdvice::Restriction:D.Status=TEXT("Restriction ahead");D.Detail=FString::Printf(TEXT("Reduce towards %.0f km/h"),Approach*3.6);D.Alert=true;break;
  case Rail::DrivingAdvice::BrakeForBerth:D.Status=TEXT("Tambaram approach");D.Detail=DistanceText(Def.berthHead-Head)+TEXT(" to stop marker — begin braking");D.Alert=true;break;
  case Rail::DrivingAdvice::PositionAtBerth:D.Status=TEXT("Align with stop marker");D.Detail=FString::Printf(TEXT("%+.1f m to berth — creep into position"),Def.berthHead-Head);break;
  case Rail::DrivingAdvice::HoldDwell:D.Status=TEXT("Platform dwell");D.Detail=FString::Printf(TEXT("Remain stopped  %.0f / %.0f s"),double(State.dwellTicks)/30,double(Def.dwellTicks)/30);break;
  default:break;
 }
 if(!S->ServiceControlNotice().IsEmpty()&&!S->ServiceControlNotice().StartsWith(TEXT("Camera:")))D.Feedback=S->ServiceControlNotice();
 if(D.Emergency){D.Status=TEXT("Emergency brake applied");D.Detail=D.CanReset?TEXT("Stopped — reset is available"):TEXT("Power cut. Wait until the train is stopped.");D.Alert=true;}
 if(S->IsPresentationBlocked()){D.Status=TEXT("Loading scenery");D.Detail=TEXT("Service paused while the route loads");D.Alert=true;}
 DrivingSlate->Update(D);
}
void URailDrivingHud::Dispatch(int32 Kind,double Value){
 auto* S=Railway.Get();if(!S||S->IsServicePaused())return;
 switch(Kind){
  case Power:S->SubmitPlayerCommand(Rail::CommandKind::Throttle,Value);break;
  case Brake:S->SubmitPlayerCommand(Rail::CommandKind::Brake,Value);break;
  case Horn:S->ServiceAction(4);break;
  case Camera:S->LocomotiveAction(0);break;
  case Pause:S->ServiceAction(2);break;
  case Emergency:S->SubmitPlayerCommand(Rail::CommandKind::Throttle,0);S->ServiceAction(7);break;
  case ResetEmergency:S->SubmitPlayerCommand(Rail::CommandKind::Emergency,0);break;
  case Lamps:S->LocomotiveAction(2);break;
  case Pantograph:S->LocomotiveAction(3);break;
  case Wipers:S->LocomotiveAction(4);break;
  case CabLamp:S->LocomotiveAction(5);break;
  case Instruments:S->LocomotiveAction(6);break;
  case Reverse:S->SubmitPlayerCommand(Rail::CommandKind::Direction,-1);break;
  case Neutral:S->SubmitPlayerCommand(Rail::CommandKind::Direction,0);break;
  case DirectionForward:S->SubmitPlayerCommand(Rail::CommandKind::Direction,1);break;
  case Depart:S->ServiceAction(3);break;
  default:break;
 }
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailRapidHudDragTest,"IndianRailway.S9.DrivingHudRapidDrag",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailRapidHudDragTest::RunTest(const FString& Parameters){
 auto Hud=SNew(SRailDrivingHud);FDriveDisplay Display;Display.Enabled=true;Display.Brake=0;Hud->Update(Display);
 int32 Commands=0,LastKind=-1;double LastValue=-1;
 Hud->Execute=[&](int32 Kind,double Value){++Commands;LastKind=Kind;LastValue=Value;};
 const FGeometry Geometry=FGeometry::MakeRoot(FVector2D(1280,720),FSlateLayoutTransform());
 const TSet<FKey> Down{EKeys::LeftMouseButton},Up;
 const FVector2D Start(80,493),Finish(80,404.5);
 const FPointerEvent Press(0,Start,Start,Down,EKeys::LeftMouseButton,0,FModifierKeysState());
 const FPointerEvent Release(0,Finish,Start,Up,EKeys::LeftMouseButton,0,FModifierKeysState());
 Hud->OnMouseButtonDown(Geometry,Press);
 TestEqual(TEXT("Grabbing the handle does not jump the brake"),Commands,0);
 Hud->OnMouseButtonUp(Geometry,Release); // Deliberately no move event.
 TestEqual(TEXT("Rapid release commits one brake command"),Commands,1);
 TestEqual(TEXT("The correct lever receives the rapid drag"),LastKind,int32(Brake));
 TestEqual(TEXT("Release position is retained when movement was coalesced"),LastValue,.5);
 Hud->OnMouseButtonDown(Geometry,Press);Hud->OnFocusLost(FFocusEvent());Hud->OnMouseButtonUp(Geometry,Release);
 TestEqual(TEXT("A cancelled contact cannot commit on release"),Commands,1);
 Display.Emergency=true;Hud->Update(Display);Hud->OnMouseButtonDown(Geometry,Press);Hud->OnMouseButtonUp(Geometry,Release);
 TestEqual(TEXT("Emergency blocks rapid lever commands"),Commands,1);
 return true;
}
#endif
