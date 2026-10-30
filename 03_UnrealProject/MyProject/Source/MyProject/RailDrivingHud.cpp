#include "RailDrivingHud.h"
#include "RailSimulationSubsystem.h"
#include "RailLocomotiveView.h"
#include "RailControlPresentation.h"
#include "Simulation/RailDrivingControlModel.h"
#include "Simulation/RailDrivingAdvice.h"
#include "Simulation/RailJourneyPresentation.h"
#include "Simulation/RailTambaramDriving.h"
#include "Simulation/RailAutomaticSignals.h"
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
 double Speed=0,Limit=0,Power=0,Brake=1,AppliedPower=0,AppliedBrake=1,Progress=0,TractionKn=0,SignalDistance=0,BrakePipe=5;
 bool Assisted=false,Emergency=false,CanReset=false,CanDepart=false,Enabled=false,Alert=false,SignalFound=false,SignalAutomatic=false;
 int32 SignalAspect=0; // Rail::LinesideAspect
 FString Station,Location,Status,Detail,Time,Direction,Feedback;
};
// Cab-instrument palette: smoked glass, hairlines, DDU green numerals, amber for the air brake.
const FLinearColor Glass(.016f,.02f,.024f,.62f),Hair(1,1,1,.16f),White(.93f,.95f,.96f,1),Muted(.62f,.68f,.72f,1),
 Amber(1.f,.62f,.18f,1),Lcd(.47f,.95f,.62f,1),Red(.95f,.16f,.12f,1),Screen(.006f,.014f,.012f,.86f);
// The WAP-7 master controller is centre-zero: forward for tractive effort, back for regenerative
// braking. The gate position g in 0..1 maps to the demand 2g-1, with a detent around zero.
double GateToDemand(double g){const double d=2*g-1;return FMath::Abs(d)<.04?0:FMath::Clamp(d,-1.0,1.0);}
double DemandToGate(double d){return (FMath::Clamp(d,-1.0,1.0)+1)*.5;}
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
  if(Key==EKeys::Up){Fire(Power,GateToDemand(Rail::DrivingControl::StepValue(DemandToGate(Data.Power),1)));return FReply::Handled();}
  if(Key==EKeys::Down){Fire(Power,GateToDemand(Rail::DrivingControl::StepValue(DemandToGate(Data.Power),-1)));return FReply::Handled();}
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
  auto Arc=[&](FVector2D C,float Radius,float Start,float End,const FLinearColor& Color,float Width){TArray<FVector2D> P;for(int I=0;I<=48;++I){const float A=FMath::DegreesToRadians(FMath::Lerp(Start,End,I/48.f));P.Add(C+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius);}Line(P,Color,Width);};

  auto Mono=[&](const FString& Value,float X,float Y,int Size,const FLinearColor& Color,int Align=0){
   const FSlateFontInfo Font=FCoreStyle::GetDefaultFontStyle(TEXT("Mono"),Size);
   const FVector2D Extent=FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value,Font);
   X-=Align==1?Extent.X*.5f:Align==2?Extent.X:0;
   FSlateDrawElement::MakeText(Out,Layer++,G.ToPaintGeometry(Extent,FSlateLayoutTransform(FVector2D(X,Y))),Value,Font,ESlateDrawEffect::None,Color);
  };
  auto Frame=[&](float X,float Y,float Width,float Height,const FLinearColor& Fill=Glass){Box(X,Y,Width,Height,Fill,2);Line({{X,Y},{X+Width,Y},{X+Width,Y+Height},{X,Y+Height},{X,Y}},Hair,1);};
  auto Caption=[&](const FString& Value,float X,float Y,int Align=0){Mono(Value,X,Y,9,Muted,Align);};
  auto Ring=[&](FVector2D C,float R,const FLinearColor& Color,float Width){Arc(C,R,0,360,Color,Width);};
  auto Rect=[&](int Kind,FSlateRect& R){for(const auto& Hit:Layout(W))if(Hit.Kind==Kind){R=Hit.Rect;return true;}return false;};

  // Route strip, top left, over the cab ceiling rather than the track.
  Frame(16,14,300,56);Box(16,14,2,56,Amber,0);
  Caption(Data.Assisted?TEXT("ASSISTED   NEXT"):TEXT("LOCO PILOT   NEXT"),28,20);Mono(Data.Time,306,20,10,Muted,2);
  Fitted(Data.Station.ToUpper(),28,34,15,180,White);Mono(Data.Location,306,38,10,Muted,2);
  Box(28,62,278,1,Hair,0);Box(28,62,278*float(Data.Progress),1,Amber,0);

  // DDU strip, top centre: speed, direction, maximum permissible speed and the message line.
  const float DduW=FMath::Clamp(W-760.f,380.f,460.f),DduX=(W-DduW)*.5f,DduY=12;
  Frame(DduX-4,DduY-4,DduW+8,104,FLinearColor(.05f,.055f,.06f,.78f));Box(DduX,DduY,DduW,96,Screen,2);
  const bool Over=Data.Speed>Data.Limit+.36;
  Mono(FString::Printf(TEXT("%3.0f"),Data.Speed),DduX+10,DduY+2,30,Over?Red:Lcd);Mono(TEXT("km/h"),DduX+86,DduY+22,9,Lcd.CopyWithNewOpacity(.7f));
  Mono(Data.Direction.Left(1),DduX+122,DduY+16,13,Lcd);
  if(Data.SignalFound){
   const float HX=DduX+DduW*.5f-58,HY=DduY+4;Box(HX,HY,16,40,FLinearColor(0,0,0,.9f),4);
   const auto A=Rail::LinesideAspect(Data.SignalAspect);
   const bool Lit[]={A==Rail::LinesideAspect::Attention,A==Rail::LinesideAspect::Proceed,A==Rail::LinesideAspect::Caution||A==Rail::LinesideAspect::Attention,A==Rail::LinesideAspect::Stop};
   const FLinearColor Lamp[]={Amber,Lcd,Amber,Red};
   for(int32 L=0;L<4;++L)Box(HX+4,HY+3+L*9.f,8,8,Lit[L]?Lamp[L]:FLinearColor(.12f,.12f,.12f,1),4);
   const TCHAR* Names[]={TEXT("DANGER"),TEXT("CAUTION"),TEXT("ATTENTION"),TEXT("PROCEED")};
   Mono(FString::Printf(TEXT("%s%s"),Names[Data.SignalAspect&3],Data.SignalAutomatic?TEXT("  A"):TEXT("")),HX+24,HY+2,10,Lit[3]?Red:Lit[2]?Amber:Lcd);
   Mono(FString::Printf(TEXT("SIG %s"),*DistanceText(Data.SignalDistance)),HX+24,HY+20,12,White);
  }
  Mono(TEXT("MPS"),DduX+DduW-74,DduY+8,9,Amber.CopyWithNewOpacity(.75f),2);Mono(FString::Printf(TEXT("%3.0f"),Data.Limit),DduX+DduW-10,DduY+2,24,Amber,2);
  const float BarX=DduX+10,BarW=DduW-20,BarY=DduY+44;
  Box(BarX,BarY,BarW,4,FLinearColor(1,1,1,.08f),1);
  Box(BarX,BarY,BarW*FMath::Clamp(float(Data.Speed/140.),0.f,1.f),4,Over?Red:Lcd,1);
  Box(BarX+BarW*FMath::Clamp(float(Data.Limit/140.),0.f,1.f)-1,BarY-4,3,12,Amber,0);
  const bool ShowFeedback=!Data.Feedback.IsEmpty()&&FPlatformTime::Seconds()<FeedbackUntil;
  Fitted(ShowFeedback?Data.Feedback:Data.Status.ToUpper(),DduX+10,DduY+54,11,DduW-20,Data.Alert||ShowFeedback?Amber:Lcd);
  Fitted(Data.Detail,DduX+10,DduY+73,9,DduW-20,Muted);

  // Icon keys, top right: cab switches, view, pause.
  for(int Kind:{(int)Drawer,(int)Camera,(int)Pause}){FSlateRect R;if(!Rect(Kind,R))continue;
   const FVector2D C((R.Left+R.Right)/2,(R.Top+R.Bottom)/2-5);Frame(R.Left,R.Top,R.Right-R.Left,R.Bottom-R.Top);
   if(Kind==Pause){Box(C.X-7,C.Y-8,4,16,White,0);Box(C.X+3,C.Y-8,4,16,White,0);}
   else if(Kind==Camera){Line({{C.X-11,C.Y-6},{C.X+5,C.Y-6},{C.X+5,C.Y+6},{C.X-11,C.Y+6},{C.X-11,C.Y-6}},White,1.5f);Line({{C.X+5,C.Y-2},{C.X+11,C.Y-6},{C.X+11,C.Y+6},{C.X+5,C.Y+2}},White,1.5f);}
   else for(int I=0;I<2;++I)for(int J=0;J<2;++J){const float L=C.X-9+I*10.f,T=C.Y-8+J*9.f;Line({{L,T},{L+7,T},{L+7,T+6},{L,T+6},{L,T}},White,1.2f);}
   Caption(Kind==Pause?TEXT("PAUSE"):Kind==Camera?TEXT("VIEW"):TEXT("CAB"),C.X,R.Bottom-15,1);
  }

  // Notched gates in the lower corners: handle in a slot, notch scale, and the actual
  // output beside it (tractive effort in kN, brake-cylinder pressure in kg/cm2).
  auto Gate=[&](bool IsPower){
   const float X=IsPower?W-136:16,Top=GateTop,Bottom=GateBottom,Travel=Bottom-Top;
   const double Requested=IsPower?DemandToGate(Data.Power):Data.Brake;
   const FLinearColor Accent=IsPower?Lcd:Amber;
   Frame(X,402,120,302,FLinearColor(.016f,.02f,.024f,.48f));
   Caption(IsPower?(Data.Assisted?TEXT("TE LIMIT"):TEXT("MASTER CTRL")):TEXT("A9 BRAKE"),X+60,410,1);
   const float Slot=IsPower?X+70:X+56,Scale=IsPower?X+18:X+104;
   Box(Slot-4,Top-6,8,Travel+12,FLinearColor(0,0,0,.85f),3);Line({{Slot,Top-4},{Slot,Bottom+4}},Hair,1);
   if(IsPower){
    const float Zero=Bottom-Travel*.5f;
    for(int I=-10;I<=10;++I){const float Y=Zero-Travel*.5f*I/10.f;Line({{Slot-8,Y},{Slot-(I%5?13:20),Y}},I==0?White:I%5?Hair:Muted,1);}
    Mono(TEXT("TE"),Slot+28,Top-7,9,Lcd);Mono(TEXT("0"),Slot+28,Zero-7,9,White);Mono(TEXT("BE"),Slot+28,Bottom-7,9,Amber);
   }else{
    for(int I=0;I<=10;++I){const float Y=Bottom-Travel*I/10.f;Line({{Slot+8,Y},{Slot+(I%5?14:20),Y}},I%5?Hair:Muted,1);}
    const struct {double At;const TCHAR* Name;} Marks[]={{0,TEXT("REL")},{Rail::AirBrake::RunningZone,TEXT("MIN")},{1,TEXT("FS")}};
    for(const auto& M:Marks)Mono(M.Name,Slot-26,Bottom-Travel*float(M.At)-7,9,M.At>=1?Amber:Muted,2);
   }
   Box(Scale-2,Top,4,Travel,FLinearColor(1,1,1,.07f),1);
   if(IsPower){ // realised effort from the centre: up for TE, down for BE
    const float Zero=Bottom-Travel*.5f,Amount=Travel*.5f*float(FMath::Clamp(FMath::Abs(Data.AppliedPower),0.0,1.0));
    if(Data.AppliedPower>=0)Box(Scale-2,Zero-Amount,4,FMath::Max(2.f,Amount),Lcd.CopyWithNewOpacity(.85f),1);
    else Box(Scale-2,Zero,4,FMath::Max(2.f,Amount),Amber.CopyWithNewOpacity(.85f),1);
   }else Box(Scale-2,Bottom-Travel*float(Data.AppliedBrake),4,FMath::Max(2.f,Travel*float(Data.AppliedBrake)),Accent.CopyWithNewOpacity(.85f),1);
   // T-grip handle; the slot stays visible either side of it.
   const float HandleY=Bottom-Travel*float(Requested);
   Box(Slot-23,HandleY-10,46,21,FLinearColor(0,0,0,.55f),3);
   Box(Slot-22,HandleY-11,44,19,FLinearColor(.10f,.11f,.12f,1),3);
   Box(Slot-22,HandleY-11,44,2,FLinearColor(.44f,.47f,.49f,1),1);
   Box(Slot-14,HandleY-3,28,2,Accent,0);
   if(IsPower){
    Mono(Data.Power<0?FString::Printf(TEXT("BE %.0f%%"),-Data.Power*100):FString::Printf(TEXT("TE %.0f%%"),Data.Power*100),X+8,Bottom+20,13,White);
    Mono(FString::Printf(TEXT("%.0fkN"),FMath::Abs(Data.TractionKn)),X+112,Bottom+24,10,Data.TractionKn<0?Amber:Lcd,2);
    Caption(Data.TractionKn<0?TEXT("demand   BE"):TEXT("demand   TE"),X+60,Bottom+44,1);
   }else{
    Mono(FString::Printf(TEXT("BP %.1f"),Data.BrakePipe),X+8,Bottom+22,11,Data.BrakePipe<4.95?Amber:White);
    Mono(FString::Printf(TEXT("BC %.1f"),Data.AppliedBrake*Rail::AirBrake::MaxCylinder),X+112,Bottom+22,11,Accent,2);
    Caption(TEXT("kg/cm2"),X+60,Bottom+44,1);
   }
   for(int Kind:{IsPower?(int)PowerPlus:(int)BrakePlus,IsPower?(int)PowerMinus:(int)BrakeMinus}){FSlateRect R;if(!Rect(Kind,R))continue;
    Frame(R.Left,R.Top,R.Right-R.Left,R.Bottom-R.Top);const FVector2D C((R.Left+R.Right)/2,(R.Top+R.Bottom)/2);const bool Up=Kind==PowerPlus||Kind==BrakePlus;
    Line(Up?TArray<FVector2D>{{C.X-8,C.Y+4},{C.X,C.Y-4},{C.X+8,C.Y+4}}:TArray<FVector2D>{{C.X-8,C.Y-4},{C.X,C.Y+4},{C.X+8,C.Y-4}},White,2);
   }
  };
  Gate(false);Gate(true);

  // Push buttons: red emergency mushroom by the brake, horn by the master controller.
  for(int Kind:{(int)Emergency,(int)Horn}){FSlateRect R;if(!Rect(Kind,R))continue;
   const FVector2D C((R.Left+R.Right)/2,(R.Top+R.Bottom)/2);const float Radius=(R.Bottom-R.Top)*.5f-2;
   Box(C.X-Radius,C.Y-Radius,Radius*2,Radius*2,Kind==Emergency?FLinearColor(.30f,.02f,.015f,.92f):Glass,Radius);
   Ring(C,Radius,Kind==Emergency?Red:Hair,Kind==Emergency?2.f:1.f);
   if(Kind==Emergency){Box(C.X-Radius*.64f,C.Y-Radius*.64f,Radius*1.28f,Radius*1.28f,Red,Radius*.64f);Mono(TEXT("EMG"),C.X,C.Y-7,10,White,1);}
   else {Line({{C.X-10,C.Y-4},{C.X-4,C.Y-4},{C.X+8,C.Y-11},{C.X+8,C.Y+11},{C.X-4,C.Y+4},{C.X-10,C.Y+4},{C.X-10,C.Y-4}},White,1.5f);Caption(TEXT("HORN"),C.X,C.Y+14,1);}
  }
  {FSlateRect R;if(Rect(ResetEmergency,R)){Frame(R.Left,R.Top,R.Right-R.Left,R.Bottom-R.Top,FLinearColor(.25f,.12f,.02f,.9f));Mono(TEXT("RESET"),(R.Left+R.Right)/2,R.Top+14,11,Amber,1);}}
  {FSlateRect R;if(Rect(Depart,R)){Frame(R.Left,R.Top,R.Right-R.Left,R.Bottom-R.Top,FLinearColor(.04f,.18f,.09f,.92f));Mono(TEXT("DEPART"),(R.Left+R.Right)/2,R.Top+14,12,Lcd,1);}}

  if(CabOpen){
   Box(0,0,W,720,FLinearColor(0,0,0,.5f),0);
   const float X=W-486;Frame(X,140,462,424,FLinearColor(.02f,.024f,.028f,.97f));Box(X,140,462,2,Amber,0);
   Mono(TEXT("CAB SWITCHES"),X+24,156,16,White);Caption(TEXT("Driving mode is chosen in the service briefing."),X+24,186);
   auto Key=[&](int Kind,const FString& Label){FSlateRect R;if(Rect(Kind,R)){Frame(R.Left,R.Top,R.Right-R.Left,R.Bottom-R.Top);Mono(Label,(R.Left+R.Right)/2,(R.Top+R.Bottom)/2-8,11,White,1);}};
   Key(CloseDrawer,TEXT("CLOSE"));Key(Lamps,TEXT("HEADLIGHT"));Key(Pantograph,TEXT("PANTOGRAPH"));Key(Wipers,TEXT("WIPER"));Key(CabLamp,TEXT("CAB LIGHT"));Key(Instruments,TEXT("INSTRUMENT VIEW"));
   Caption(TEXT("REVERSER"),X+24,436);Key(Reverse,TEXT("R"));Key(Neutral,TEXT("0"));Key(DirectionForward,TEXT("F"));
   Caption(TEXT("Direction changes require a stopped train."),X+24,524);
  }
  return Layer;
 }
private:
 TMap<int32,FHold> Holds;
 static constexpr float GateTop=442,GateBottom=619; // 177 units of travel, as the drag model expects
 TArray<FHit> Layout(float W)const{
  if(CabOpen){const float X=W-486;return {{CloseDrawer,W-126,151,W-40,194},{Lamps,X+24,214,X+220,269},{Pantograph,X+238,214,W-48,269},{Wipers,X+24,284,X+220,339},{CabLamp,X+238,284,W-48,339},{Instruments,X+24,354,W-48,409},{Reverse,X+24,456,X+154,508},{Neutral,X+166,456,X+296,508},{DirectionForward,X+308,456,W-48,508},{Pause,W-72,14,W-16,70}};}
  TArray<FHit> Out{{Drawer,W-200,14,W-144,70},{Camera,W-136,14,W-80,70},{Pause,W-72,14,W-16,70},
   {Brake,18,424,110,632},{Power,W-134,424,W-42,632},
   {BrakePlus,142,442,196,492},{BrakeMinus,142,569,196,619},{PowerPlus,W-196,442,W-142,492},{PowerMinus,W-196,569,W-142,619},
   {Emergency,138,634,210,706},{Horn,W-210,634,W-138,706}};
  if(Data.CanReset)Out.Add({ResetEmergency,222,646,306,694});
  if(Data.CanDepart)Out.Add({Depart,W*.5f-64,648,W*.5f+64,696});
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
    Holds.Add(Key(E),{Kind,P.Y,Kind==Power?DemandToGate(Data.Power):Data.Brake});
    return FReply::Handled().CaptureMouse(SharedThis(this)).SetUserFocus(SharedThis(this));
   }
   if(Kind==Drawer||Kind==CloseDrawer){CabOpen=Kind==Drawer;Holds.Empty();}
   else if(Kind==PowerMinus||Kind==PowerPlus)Fire(Power,GateToDemand(Rail::DrivingControl::StepValue(DemandToGate(Data.Power),Kind==PowerPlus?1:-1)));
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
  Fire(Hold->Kind,Hold->Kind==Power?GateToDemand(Value):Value);return FReply::Handled();
 }
 FReply End(const FGeometry& G,const FPointerEvent& E){
  if(TraceHudInput())UE_LOG(LogTemp,Display,TEXT("HUD_INPUT end key=%d y=%.1f holds=%d"),Key(E),Position(G,E).Y,Holds.Num());
  if(const auto* Hold=Holds.Find(Key(E))){
   // Fast pointer gestures can begin/end within one frame, coalescing every
   // move event. Commit the release position before relinquishing capture.
   if(Data.Enabled&&!Data.Emergency){const double Value=Rail::DrivingControl::DragValue(Hold->Value,Hold->StartY,Position(G,E).Y,177);Fire(Hold->Kind,Hold->Kind==Power?GateToDemand(Value):Value);}
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
 D.Location=(Journey.terminal?TEXT("STOP "):Journey.alongside?TEXT("PASS "):TEXT(""))+DistanceText(Journey.distance)+(Journey.distance<0?TEXT(" back"):TEXT(""));
 D.TractionKn=Handling->tractionN/1000; // negative = regenerative braking effort
 D.BrakePipe=Rail::AirBrake::BrakePipe(Handling->command.brake,Handling->command.emergency);
 D.Time=FString::Printf(TEXT("%02llu:%02llu"),State.ticks/1800,(State.ticks/30)%60);
 D.Status=TEXT("Route authorised");D.Detail=TEXT("Observe lineside signals and speed limits");
 // Lineside signal ahead, from the same authoritative state that drives the lineside heads.
 const auto& Plan=Rail::SignalPlanFor(World);const auto Aspects=Rail::EvaluateSignals(Plan,World);
 const auto Ahead=Rail::NextSignal(Plan,Aspects,World,Def.player);
 D.SignalFound=Ahead.found;D.SignalDistance=Ahead.distance;D.SignalAspect=int32(Ahead.aspect);D.SignalAutomatic=Ahead.role==Rail::SignalRole::Automatic;
 if(Ahead.found&&Ahead.distance<2500)switch(Ahead.aspect){
  case Rail::LinesideAspect::Stop:D.Status=Ahead.role==Rail::SignalRole::Manual?TEXT("Station signal at danger"):TEXT("Automatic signal at danger");
   D.Detail=TEXT("Stop before the signal ")+DistanceText(Ahead.distance)+TEXT(" ahead");D.Alert=true;break;
  case Rail::LinesideAspect::Caution:D.Status=TEXT("Caution - single yellow");D.Detail=TEXT("Be prepared to stop at the next signal");D.Alert=true;break;
  case Rail::LinesideAspect::Attention:D.Status=TEXT("Attention - double yellow");D.Detail=TEXT("Be prepared to pass the next signal at restricted speed");break;
  default:D.Status=TEXT("Signal clear - proceed");D.Detail=TEXT("Observe the speed limit and the next signal");break;
 }
 const auto Authority=World.Current().authorities.find(Def.player);
 const bool RouteHeld=Authority!=World.Current().authorities.end()&&!Authority->second.routeGranted;
 if(D.Power>0&&D.BrakePipe<Rail::AirBrake::TractionInterlockPipe){D.Status=TEXT("Traction held");D.Detail=TEXT("Brake pipe below 4.7 - release the A9 before powering.");}
 if(D.Power<0&&D.Speed<10){D.Status=TEXT("Regeneration fading");D.Detail=TEXT("Below 10 km/h use the air brake to stop.");}
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
 // The protection system brakes any train inside its stopping curve to the end of authority
 // (a signal at danger). Tell the driver when it is overriding a release or power demand.
 if(Authority!=World.Current().authorities.end()&&Authority->second.valid&&Handling->speedMps>.5&&(State.requestedBrake<1||State.requestedPower>0)&&
    Authority->second.maximumHead-Head<=Rail::TambaramDriving::AuthorityStoppingDistance(Handling->speedMps)){
  D.Status=TEXT("TPWS brake intervention");D.Detail=TEXT("Train protection is stopping you short of the signal at danger");D.Alert=true;}
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
