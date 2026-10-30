#include "RailTambaramTurnoutView.h"
#include "Simulation/RailTambaramRoute.h"
#include "Simulation/RailScenarioContent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

namespace {
double Entry(){double D=0;for(size_t I=0;I<Rail::TambaramRoute::CrossoverSegment;++I)D+=Rail::TambaramRoute::GeometryFor(1,I).lengthMetres;return D;}
double EntryFor(int32 T){double D=0;for(size_t I=0;I<Rail::TambaramRoute::CrossoverSegment;++I)D+=Rail::TambaramRoute::GeometryFor(T,I).lengthMetres;return D;}
double Begin(int32 T){return FMath::FloorToDouble(EntryFor(T)/6)*6;}
double End(int32 T){return FMath::CeilToDouble((EntryFor(T)+Rail::TambaramRoute::GeometryFor(T,Rail::TambaramRoute::CrossoverSegment).lengthMetres)/6)*6;}
FVector V(Rail::Vec3 A){return FVector(A.x,A.y,A.z);}
FVector Side(const Rail::Sample& S){return FVector(-S.tangent.y,S.tangent.x,0);}
}
ARailTambaramTurnoutView::ARailTambaramTurnoutView(){PrimaryActorTick.bCanEverTick=false;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));}
bool ARailTambaramTurnoutView::ReplacesPlainTrack(int32 Track,double D){return Track<2&&D>=Begin(Track)-.001&&D<End(Track)-.001;}
FString ARailTambaramTurnoutView::FinishTelemetryJson() const {
 FString Result=TEXT("[");
 for(int32 I=0;I<Materials.Num();++I){if(I)Result+=TEXT(",");Result+=TEXT("\"")+(Materials[I]?Materials[I]->GetPathName():TEXT(""))+TEXT("\"");}
 return Result+TEXT("]");
}
bool ARailTambaramTurnoutView::LoadAssets(){
 if(Cube&&Materials.Num()==4)return true;
 Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 auto* Master=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_SceneryTint.M_SceneryTint"));
 if(!Cube||!Master){Diagnostic=TEXT("Missing turnout cube/tint material");return false;}
 Materials.Empty();
 const FLinearColor Colors[]={FLinearColor(.34f,.36f,.38f),FLinearColor(.16f,.075f,.038f),FLinearColor(.38f,.37f,.32f),FLinearColor(.075f,.085f,.09f)};
 for(int32 I=0;I<4;++I){auto* M=UMaterialInstanceDynamic::Create(Master,this);M->SetVectorParameterValue(TEXT("Color"),Colors[I]);M->SetScalarParameterValue(TEXT("Roughness"),I==0?.32f:.82f);Materials.Add(M);}
 const TCHAR* Finishes[]={TEXT("M_RailHead"),TEXT("M_RailRust"),TEXT("M_SleeperConcrete"),TEXT("M_Fastener")};
 for(int32 I=0;I<4;++I){
  const FString Path=FString::Printf(TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/MaterialsV4/%s.%s"),Finishes[I],Finishes[I]);
  if(auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path))Materials[I]=Material;
 }
 return true;
}
void ARailTambaramTurnoutView::Release(){for(const auto& P:Parts){RemoveInstanceComponent(P.Get());P->DestroyComponent();}Parts.Empty();InstanceCount=0;bBuilt=false;bReady=false;}
void ARailTambaramTurnoutView::Build(int32 Chunk,int32 Point1,int32 Point2){
 Release();TArray<FTransform> Geometry[4];
 auto Box=[&](int32 Family,FVector A,FVector B,double Width,double Height){
  const FVector Mid=(A+B)*.5; if((Chunk==1)!=(Mid.X<0))return;
  const FVector Delta=B-A;const double Length=Delta.Size();if(Length<.0001)return;
  Geometry[Family].Add(FTransform(FRotationMatrix::MakeFromXZ(Delta,FVector::UpVector).ToQuat(),Mid*100,FVector(Length,Width,Height)));
 };
 auto RailBeam=[&](FVector A,FVector B,double W=.07){
  Box(0,A+FVector(0,0,-.018),B+FVector(0,0,-.018),W,.036);
  Box(1,A+FVector(0,0,-.091),B+FVector(0,0,-.091),.016,.11);
  Box(1,A+FVector(0,0,-.158),B+FVector(0,0,-.158),.15,.026);
 };
 const auto Arc1=Rail::TambaramRoute::CrossoverGeometry(false),Arc2=Rail::TambaramRoute::CrossoverGeometry(true);
 const double Total=Arc1.lengthMetres+Arc2.lengthMetres;
 auto Cross=[&](double D){return D<=Arc1.lengthMetres?Arc1.At(D):Arc2.At(D-Arc1.lengthMetres);};
 // Shared long bearers under the complete 5m crossover: no overlapping duplicate
 // ordinary sleeper mesh. Rail head z=0 and inner-face gauge exactly 1.676m.
 for(int32 T=0;T<2;++T){
  for(double D=Begin(T);D<End(T)-.001;D+=.5){
   const auto A=Rail::TambaramRoute::SampleRoute(T,D),B=Rail::TambaramRoute::SampleRoute(T,FMath::Min(D+.5,End(T)));
   for(double Y:{-.873,.873})RailBeam(V(A.position)+Side(A)*Y,V(B.position)+Side(B)*Y);
  }
 }
 for(double D=Begin(1)+.3;D<End(1);D+=.6){
  const auto S=Rail::TambaramRoute::SampleRoute(1,D);const FVector P=V(S.position),R=Side(S),F=V(S.tangent);
  Box(2,P+R*(-1.4)+FVector(0,0,-.265),P+R*6.4+FVector(0,0,-.265),.25,.18);
  // Rail seats and paired clip silhouettes at each stock rail.
  for(double Y:{-.873,.873,4.127,5.873}){
   Box(3,P+R*(Y-.13)+FVector(0,0,-.177),P+R*(Y+.13)+FVector(0,0,-.177),.20,.022);
   for(double Clip:{-.065,.065})Box(1,P+R*(Y+Clip)+F*.07+FVector(0,0,-.14),P+R*(Y+Clip)-F*.07+FVector(0,0,-.14),.023,.035);
  }
 }
 // Continuously graph-sampled closure rails; short chords avoid faceted curves.
 for(double D=0;D<Total-.001;D+=.5){
  const auto A=Cross(D),B=Cross(FMath::Min(D+.5,Total));
  for(double Y:{-.873,.873}){
   const double FromToe=FMath::Min(D,Total-D);
   // Tapered tongues share the stock rail at the toe and acquire full width at heel.
   const double Width=.008+.062*FMath::Clamp(FromToe/12.,0.,1.);
   const bool AtExit=D>Total/2;const int32 Position=AtExit?Point2:Point1;
   // The simulation switches instantaneously while unoccupied; presentation reads
   // the committed position, never requested/ungranted point settings.
   const double Open=(Position==0?.12:0)*FMath::Max(0.,1.-FromToe/12.);
   const double Offset=Y+(Y<0?Open:-Open);
   RailBeam(V(A.position)+Side(A)*Offset,V(B.position)+Side(B)*Offset,Width);
  }
 }
 // Locate each crossing from actual rail intersection, not a hard-coded chainage.
 // A representative fixed V nose, flared wing rails, and opposite check rail.
 for(int32 EndIndex=0;EndIndex<2;++EndIndex){
  double Best=1e9,FrogD=0;
  const auto Toe=EndIndex?Cross(Total):Cross(0);const FVector R=Side(Toe),P=V(Toe.position);
  for(double D=0;D<=Total;D+=.05){const double Lat=FMath::Abs(FVector::DotProduct(V(Cross(D).position)-P,R));const double Error=FMath::Abs(Lat-1.746);if(Error<Best){Best=Error;FrogD=D;}}
  const auto S=Cross(FrogD);const FVector C=V(S.position)+Side(S)*(EndIndex?.873:-.873),F=V(Toe.tangent);
  const double Dir=EndIndex?-1.:1.;
  for(double Sign:{-1.,1.}){
   RailBeam(C-F*(Dir*2.1)+R*(Sign*.15),C-F*(Dir*.35)+R*(Sign*.055),.052);
   RailBeam(C-F*(Dir*.35)+R*(Sign*.055),C+F*(Dir*2.3)+R*(Sign*.12),.052);
  }
  // Tapered manganese nose represented by short increasing-width head sections.
  for(double D=.08;D<1.7;D+=.1)Box(0,C+F*(Dir*D)+FVector(0,0,-.02),C+F*(Dir*(D+.1))+FVector(0,0,-.02),FMath::Min(.075,.01+D*.04),.04);
  const FVector Guard=C+R*(EndIndex? -1.64:1.64);
  RailBeam(Guard-F*2,Guard+F*2,.052);
  RailBeam(Guard-F*2,Guard-F*2.5+R*.06,.052);RailBeam(Guard+F*2,Guard+F*2.5+R*.06,.052);
 }
 for(int32 I=0;I<2;++I){
  const auto S=Cross(I?Total:0);const FVector P=V(S.position),R=Side(S),F=V(S.tangent);const double Sign=I?1.:-1.;
  const FVector Machine=P+R*(Sign*2.25);
  Box(2,Machine-F*.65+FVector(0,0,-.29),Machine+F*.65+FVector(0,0,-.29),.9,.22);
  Box(3,Machine-F*.42+FVector(0,0,-.06),Machine+F*.42+FVector(0,0,-.06),.55,.24);
  Box(0,Machine-F*.36+FVector(0,0,.065),Machine+F*.36+FVector(0,0,.065),.49,.025);
  for(double X:{-.28,.28})Box(1,P+F*X+R*(Sign*2.0)+FVector(0,0,-.11),P+F*X-R*(Sign*.94)+FVector(0,0,-.11),.035,.035);
 }
 for(int32 I=0;I<4;++I){auto* Part=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Cube);Part->SetMaterial(0,Materials[I]);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCullDistances(70000,95000);Part->RegisterComponent();Part->AddInstances(Geometry[I],false,true);Parts.Add(Part);InstanceCount+=Geometry[I].Num();}
 bBuilt=true;LastPoint1=Point1;LastPoint2=Point2;OwnerChunk=Chunk;
}
void ARailTambaramTurnoutView::Present(const Rail::OperationsWorld& World,int32 ChunkId){
 if(!Rail::ScenarioContent::IsTambaram(World.ContentId())){Release();return;}
 bool Wanted=false;
 for(uint64 Id:{1ull,2ull})if(const auto* Train=World.Spatial(Rail::TrainId(Id))){const double Head=Train->Current().headStationMetres+(Id==1?Rail::TambaramRoute::TrafficStationOffset():0);const double Rear=Head-Train->Current().properties.lengthMetres;Wanted|=Head+900>=Entry()&&Rear-900<=Entry()+Rail::TambaramRoute::GeometryFor(1,6).lengthMetres;}
 if(!Wanted){if(bBuilt)Release();bReady=true;Diagnostic=TEXT("Turnout outside bounded demand");return;}
 if(ChunkId!=1&&ChunkId!=2){bReady=false;return;}
 const auto P1=World.Current().points.find(Rail::PointId(1)),P2=World.Current().points.find(Rail::PointId(2));
 if(P1==World.Current().points.end()||P2==World.Current().points.end()||!LoadAssets()){bReady=false;return;}
 if(!bBuilt||OwnerChunk!=ChunkId||LastPoint1!=P1->second.position||LastPoint2!=P2->second.position)Build(ChunkId,P1->second.position,P2->second.position);
 bReady=true;Diagnostic=FString::Printf(TEXT("Turnout: %d parts, committed points %d/%d, locks %d/%d"),InstanceCount,LastPoint1,LastPoint2,P1->second.lock.Active(),P2->second.lock.Active());
}
