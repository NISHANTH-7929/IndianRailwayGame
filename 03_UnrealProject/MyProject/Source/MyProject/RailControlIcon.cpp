#include "RailControlIcon.h"
#include "Widgets/SLeafWidget.h"
#include "Rendering/DrawElements.h"

class SRailControlIcon : public SLeafWidget {
public:
 SLATE_BEGIN_ARGS(SRailControlIcon) {} SLATE_ARGUMENT(FName, Kind) SLATE_END_ARGS()
 void Construct(const FArguments& Args){Kind=Args._Kind;}
 virtual FVector2D ComputeDesiredSize(float) const override{return FVector2D(20,20);}
 virtual int32 OnPaint(const FPaintArgs&,const FGeometry& G,const FSlateRect&,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle&,bool) const override {
  const FVector2D Size=G.GetLocalSize();
  auto Line=[&](std::initializer_list<FVector2D> Points){TArray<FVector2D> P;for(const auto& V:Points)P.Add(FVector2D(V.X*Size.X,V.Y*Size.Y));FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),P,ESlateDrawEffect::None,FLinearColor(.82f,.91f,.94f),true,1.5f);};
  if(Kind=="Pause"){Line({{.3,.2},{.3,.8}});Line({{.7,.2},{.7,.8}});}
  else if(Kind=="CabCamera"){Line({{.1,.3},{.32,.3},{.4,.18},{.63,.18},{.7,.3},{.9,.3},{.9,.8},{.1,.8},{.1,.3}});Line({{.4,.45},{.6,.45},{.6,.65},{.4,.65},{.4,.45}});}
  else if(Kind=="Horn"){Line({{.1,.4},{.4,.4},{.7,.2},{.7,.8},{.4,.6},{.1,.6},{.1,.4}});Line({{.85,.35},{.95,.5},{.85,.65}});}
  else if(Kind=="Emergency"||Kind=="ResetEmergency"){Line({{.32,.12},{.68,.12},{.88,.32},{.88,.68},{.68,.88},{.32,.88},{.12,.68},{.12,.32},{.32,.12}});Line({{.5,.3},{.5,.57}});Line({{.5,.7},{.5,.73}});}
  else if(Kind=="Pantograph"){Line({{.15,.85},{.85,.85}});Line({{.5,.8},{.2,.5},{.5,.2},{.8,.5},{.5,.8}});Line({{.25,.15},{.75,.15}});}
  else if(Kind=="Wipers"){Line({{.1,.75},{.1,.2},{.9,.2},{.9,.75}});Line({{.2,.8},{.65,.35},{.8,.5}});}
  else if(Kind=="Lamps"||Kind=="CabLamp"){Line({{.5,.2},{.25,.3},{.2,.55},{.4,.75},{.6,.75},{.8,.55},{.75,.3},{.5,.2}});Line({{.4,.88},{.6,.88}});}
  else if(Kind=="Instruments"||Kind=="Debug"){Line({{.12,.2},{.88,.2},{.88,.8},{.12,.8},{.12,.2}});Line({{.27,.38},{.73,.38}});Line({{.27,.57},{.58,.57}});}
  else if(Kind=="Exit"){Line({{.5,.12},{.15,.12},{.15,.88},{.5,.88}});Line({{.4,.5},{.9,.5},{.7,.3}});Line({{.9,.5},{.7,.7}});}
  else {Line({{.2,.5},{.8,.5},{.6,.3}});Line({{.8,.5},{.6,.7}});}
  return Layer;
 }
private:FName Kind;
};
TSharedRef<SWidget> URailControlIcon::RebuildWidget(){return SNew(SRailControlIcon).Kind(Kind);}
