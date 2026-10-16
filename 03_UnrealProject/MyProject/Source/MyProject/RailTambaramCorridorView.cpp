#include "RailTambaramCorridorView.h"
#include "RailTambaramTurnoutView.h"
#include "Simulation/RailTambaramRoute.h"
#include "Simulation/RailRouteSampling.h"
#include "Simulation/RailScenarioContent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"

namespace {
constexpr double CellMetres=120.0;
constexpr int32 MaxCells=112; // two separated trains, four tracks, bounded 13-cell windows
bool Owns(int32 Chunk,double X){return Chunk==1?X<0:X>=0;}
// OSM bridge endpoints projected offline onto alignment v3, nearest sample 1m.
// Deck/river elevations and structural dimensions below are representative.
struct FWaterCrossing {double From,To;};
constexpr FWaterCrossing WaterCrossings[]={{3675.88,3748.98},{9606.31,9726.84}};
bool AtWater(double D,double Margin=0){for(const auto& B:WaterCrossings)if(D>=B.From-Margin&&D<=B.To+Margin)return true;return false;}
// Reference extents use player-track chainage, not the true (different) length
// of a parallel circular arc. Convert by authoritative segment and fraction.
double BaseChainage(int32 Track,double Distance){
 double Base=0;
 for(size_t I=0;I<Rail::TambaramRoute::SegmentCount;++I){
  const double Length=Rail::TambaramRoute::GeometryFor(Track,I).lengthMetres;
  const double BaseLength=Rail::TambaramRoute::GeometryFor(1,I).lengthMetres;
  if(Distance<=Length||I+1==Rail::TambaramRoute::SegmentCount)return Base+Distance*BaseLength/Length;
  Distance-=Length;Base+=BaseLength;
 }
 return Base;
}
bool AtStationShelter(double Base){
 if(Base<600)return true; // retained Egmore terminal precinct
 if(Base>=Rail::TambaramRoute::TambaramPlatformFrom-18&&Base<=Rail::TambaramRoute::TambaramPlatformTo+18)return true;
 for(size_t I=1;I+1<Rail::TambaramRoute::StationCount;++I)
  if(FMath::Abs(Base-Rail::TambaramRoute::StationMetres[I])<=180)return true;
 return false;
}
FVector Cm(Rail::Vec3 V){return FVector(V.x*100,V.y*100,V.z*100);}
FQuat Facing(Rail::Vec3 V){return FRotationMatrix::MakeFromXZ(Cm(V),FVector::UpVector).ToQuat();}
}
ARailTambaramCorridorView::ARailTambaramCorridorView(){
 PrimaryActorTick.bCanEverTick=false;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
bool ARailTambaramCorridorView::LoadAssets(){
 if(bAssetsAttempted)return Meshes.Num()==6&&Materials.Num()==22;
 bAssetsAttempted=true;
 LeafCluster=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/SM_ChennaiLeafCluster.SM_ChennaiLeafCluster"));
 LeafMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiLeafSurface.M_ChennaiLeafSurface"));
 if(auto* Dense=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/SM_ChennaiDenseCanopy.SM_ChennaiDenseCanopy")))LeafCluster=Dense;
 if(auto* DenseMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiDenseLeaf.M_ChennaiDenseLeaf")))LeafMaterial=DenseMaterial;
 GroundMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiGroundSurface.M_ChennaiGroundSurface"));
 WaterMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiRiverSurface.M_ChennaiRiverSurface"));
 const TCHAR* MeshPaths[]={TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_Track6m.SM_Track6m"),
  TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_Ballast6m.SM_Ballast6m"),
  TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_TreeBroadleaf.SM_TreeBroadleaf"),
  TEXT("/Engine/BasicShapes/Sphere.Sphere"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder")};
 for(const auto* Path:MeshPaths){auto* Asset=LoadObject<UStaticMesh>(nullptr,Path);if(!Asset){Diagnostic=FString::Printf(TEXT("Missing corridor mesh: %s"),Path);Meshes.Empty();return false;}Meshes.Add(Asset);}
 for(const TCHAR* Name:{TEXT("Ballast"),TEXT("Galvanized"),TEXT("Soil"),TEXT("Cream"),TEXT("RedOxide"),TEXT("Concrete"),TEXT("Glass"),TEXT("Roof"),TEXT("Black"),TEXT("Wood"),TEXT("Paving")}){
  const FString Path=FString::Printf(TEXT("/Game/IndianRailway/S8/Stations/Kit01/Materials/%s.%s"),Name,Name);
  auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path);if(!Material){Diagnostic=TEXT("Missing corridor material: ")+Path;Materials.Empty();return false;}Materials.Add(Material);
 }
 // Original project-authored lit colour master; optional during asset migration.
 auto* Tint=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_SceneryTint.M_SceneryTint"));
 const FLinearColor Colors[]={FLinearColor(.36f,.53f,.58f),FLinearColor(.64f,.38f,.32f),FLinearColor(.44f,.54f,.36f),
  FLinearColor(.075f,.19f,.035f),FLinearColor(.16f,.27f,.045f),FLinearColor(.12f,.075f,.035f),
  FLinearColor(.11f,.17f,.14f),FLinearColor(.07f,.075f,.075f),FLinearColor(.68f,.66f,.55f),
  FLinearColor(.20f,.25f,.11f),FLinearColor(.38f,.30f,.18f)};
 for(const auto& Color:Colors){
  if(Tint){auto* MID=UMaterialInstanceDynamic::Create(Tint,this);MID->SetVectorParameterValue(TEXT("Color"),Color);Materials.Add(MID);}
  else Materials.Add(Materials[3]);
 }
 Diagnostic=TEXT("Ready: bounded graph-sampled corridor cells");return true;
}
void ARailTambaramCorridorView::RemoveCell(int32 Key){
 if(auto* Cell=Cells.Find(Key))for(auto* Part:Cell->Parts){Components.Remove(Part);RemoveInstanceComponent(Part);Part->DestroyComponent();}
 if(Cells.Remove(Key))++ReleasedCells;
}
void ARailTambaramCorridorView::Release(){
 TArray<int32> Keys;Cells.GetKeys(Keys);for(int32 Key:Keys)RemoveCell(Key);
 ActiveCells=InstanceCount=StoneInstances=0;bReady=false;
}
void ARailTambaramCorridorView::BuildCell(const Rail::Graph& Graph,int32 Track,int32 CellIndex,int32 ChunkId,FCell& Cell){
 const auto Path=Rail::TambaramRoute::Path(Track);
 const double Start=CellIndex*CellMetres,End=FMath::Min(Start+CellMetres,Rail::RouteLength(Graph,Path));
 if(End<=Start)return;
 // Family order: track, shaped ballast, close stones, OHE steel, ground, cream buildings,
 // oxide buildings, trees. No collision and no gameplay state in any family.
 constexpr int32 FamilyCount=31; // four irregular dry-grass, scrub, damp-silt and gravel families
 TArray<FTransform> Instances[FamilyCount];
 auto Sample=[&](double D,Rail::Sample& S){return Rail::SampleRoute(Graph,Path,D,S).ok;};
 auto Add=[&](int32 Family,const Rail::Sample& S,FVector Offset,FVector Scale,FRotator Local=FRotator::ZeroRotator){
  if(!Owns(ChunkId,S.position.x))return;
  const FQuat Q=Facing(S.tangent);Instances[Family].Add(FTransform(Q*Local.Quaternion(),Cm(S.position)+Q.RotateVector(Offset*100),Scale));
 };
 // Cylinders have local Z axes. Branch endpoints are in the sampled railway
 // frame, so neither origin shifts nor curved cells can detach the crown.
 auto Limb=[&](const Rail::Sample& S,FVector A,FVector B,double Radius){
  const FVector V=B-A;
  Add(19,S,(A+B)*.5,FVector(Radius,Radius,V.Size()),FRotationMatrix::MakeFromZ(V).Rotator());
 };
 FRandomStream Rng(7301+Track*100000+CellIndex);
 for(double D=Start;D<End-.001;D+=6){
  Rail::Sample A,B;if(!Sample(D,A)||!Sample(FMath::Min(D+6,End),B))continue;
  const auto Delta=B.position-A.position;const double Length=Delta.Length();
  A.tangent=Delta.Unit(); // endpoints on authoritative geometry; <=6m render chord only
  if(!ARailTambaramTurnoutView::ReplacesPlainTrack(Track,D))Add(0,A,FVector::ZeroVector,FVector(Length/6,1,1));Add(1,A,FVector::ZeroVector,FVector(Length/6,1,1));
 }
 // The crossover is part of the graph, not an invented diagonal overlay.
 // Its one owner is the cell containing its entry on track 1; normal demand
 // windows keep the complete 140m connector present before either train arrives.
 double CrossoverEntry=0;
 for(size_t I=0;I<Rail::TambaramRoute::CrossoverSegment;++I)CrossoverEntry+=Rail::TambaramRoute::GeometryFor(1,I).lengthMetres;
 if(Track==1&&CrossoverEntry>=Start&&CrossoverEntry<End){
  for(bool Second:{false,true}){
   const auto Geometry=Rail::TambaramRoute::CrossoverGeometry(Second);
   for(double D=0;D<Geometry.lengthMetres;D+=3){
    auto A=Geometry.At(D);const auto B=Geometry.At(FMath::Min(D+3,Geometry.lengthMetres));
    const auto Delta=B.position-A.position;A.tangent=Delta.Unit();
    // Detailed connector rails and shared bearers belong to TurnoutView.
    Add(1,A,FVector(0,0,-.025),FVector(Delta.Length()/6,1,1));
    // Both electrified routes need continuous wire above the actual connector;
    // straight-track wires alone leave the AI pantograph metres off the wire.
    for(double Z:{5.5,6.3})Add(3,A,FVector(Delta.Length()/2,0,Z),FVector(Delta.Length()+.02,.012,.012));
   }
  }
  auto A=Rail::TambaramRoute::CrossoverGeometry(false).At(0);
  Add(8,A,FVector(0,-3.15,-.3),FVector(1.5,.8,.3));
  Add(11,A,FVector(0,-3.15,-.04),FVector(.85,.5,.26));
  Add(3,A,FVector(0,-1.8,-.10),FVector(.06,2.2,.07));
 }
 // Textured shoulder underlay supplies the continuous gravel field. Individually
 // instanced close stones are angular 25-65mm accents, not giant decorative rocks.
 for(double D=Start;D<End;D+=1.0/12){
  Rail::Sample S;if(!Sample(D,S))continue;
  const double Y=Rng.FRandRange(-1.9f,1.9f),Size=Rng.FRandRange(.025f,.065f);
  const double Z=-.33-FMath::Max(0.0,FMath::Abs(Y)-1.43)*.43;
  Add(2,S,FVector(0,Y,Z),FVector(Size,Size*Rng.FRandRange(.7f,1.1f),Size*.65),FRotator(Rng.FRandRange(-25.f,25.f),Rng.FRandRange(0.f,180.f),Rng.FRandRange(-25.f,25.f)));
 }
 // OHE contact wire follows short graph chords, with 60m cantilever supports.
 for(double D=Start;D<End;D+=6){
  Rail::Sample A,B;if(!Sample(D,A)||!Sample(FMath::Min(D+6,End),B))continue;
  const auto Delta=B.position-A.position;A.tangent=Delta.Unit();
  Add(3,A,FVector(Delta.Length()/2,0,5.5),FVector(Delta.Length()+.02,.012,.012));
  Add(3,A,FVector(Delta.Length()/2,0,6.3),FVector(Delta.Length()+.02,.012,.012));
 }
 for(double D=FMath::CeilToDouble(Start/60)*60;D<End;D+=60){
  if(AtStationShelter(BaseChainage(Track,D)))continue;
  Rail::Sample S;if(!Sample(D,S))continue;
  // A normal +2.55m mast would be inside the crossover train envelope.
  // Representative throat hardware stands on the outside of both routes.
  const bool Throat=Track==1&&D>=CrossoverEntry-6&&D<=CrossoverEntry+Rail::TambaramRoute::GeometryFor(1,Rail::TambaramRoute::CrossoverSegment).lengthMetres+6;
  const double MastY=Throat?-3.15:2.55;
  Add(3,S,FVector(0,MastY,3.05),FVector(.15,.15,7.2));
  Add(3,S,FVector(0,MastY/2,6.3),FVector(.08,FMath::Abs(MastY)+.15,.08));
  Add(3,S,FVector(0,0,5.9),FVector(.025,.025,.8));
 }
 // Station shelters occupy the outer platform edges. Shared double cantilevers
 // stand in the two 5m intertrack gaps, never through a canopy. Tracks1/2 own
 // one pair each; tracks0/3 deliberately contribute no duplicate station post.
 // Shared base chainage puts both arms exactly over the parallel graph section.
 if(Track==1||Track==2){
  const double BaseStart=BaseChainage(Track,Start),BaseEnd=BaseChainage(Track,End);
  const double LeftWire=Track==1?0:16.35;
  const auto BasePath=Rail::TambaramRoute::Path(1);
  for(double D=FMath::CeilToDouble(BaseStart/60)*60;D<BaseEnd;D+=60){
   if(!AtStationShelter(D))continue;
   Rail::Sample S;if(!Rail::SampleRoute(Graph,BasePath,D,S).ok)continue;
   Add(3,S,FVector(0,LeftWire+2.5,3.05),FVector(.15,.15,7.2));
   Add(3,S,FVector(0,LeftWire+2.5,6.3),FVector(.08,5.15,.08));
   for(double Y:{LeftWire,LeftWire+5})Add(3,S,FVector(0,Y,5.9),FVector(.025,.025,.8));
  }
 }
 // Geographic neighbourhood character changes along the corridor. These are
 // individually composed original buildings, not a surveyed cadastral model.
 // Outer landscape ownership uses player track 1 (offset 0), never track 0
 // (offset +5), so the southern side cannot grow into the player's formation.
 if(Track==1){
  for(double D=Start;D<End;D+=12){Rail::Sample S;if(!Sample(D,S))continue;
   if(!AtWater(D+6,8))Add(4,S,FVector(6,10,-.8),FVector(16,32,.3));}
 }
 // Concrete girder bridges at source-backed river crossings. Every deck and
 // pier remains graph sampled, and only the player-side cell owns the water.
 for(double D=Start;D<End;D+=6)if(AtWater(BaseChainage(Track,D+3))){
  Rail::Sample S;if(!Sample(D,S))continue;
  Add(8,S,FVector(3,0,-.95),FVector(6.2,4.4,1.0));
  for(double Side:{-1.,1.}){
   Add(8,S,FVector(3,Side*2.05,-.22),FVector(6.1,.18,.5));
   Add(3,S,FVector(3,Side*2.1,.58),FVector(6.1,.055,.055));
   Add(3,S,FVector(0,Side*2.1,.23),FVector(.06,.06,.8));
  }
  if(int32(D/6)%4==0)Add(8,S,FVector(0,0,-3.1),FVector(1.4,3.2,4.7));
 }
 // Water/bed extend under both banks, including the terrain's suppression
 // margin. The bridge deck itself retains the reference-backed span above.
 // Without this overlap the widened scenery exposed white voids at the banks.
 if(Track==1)for(double D=Start;D<End;D+=6)if(AtWater(D+3,30)){
  Rail::Sample S;if(!Sample(D,S))continue;
  Add(21,S,FVector(3,10,-4.2),FVector(6.5,440,.14));
  Add(25,S,FVector(3,10,-5),FVector(7,450,1));
 }
 // Original representative graded embankment, not a surveyed elevation.
 // Each bank has ONE continuous slope and dry backing. Independently shifted
 // 5m slabs made a stair-step crest even when their slope planes agreed.
 // Retain irregular wet-edge accents, but never use them to define the crest.
 // Each complete bank still belongs to exactly one streamed cell.
 if(Track==1)for(const auto& Water:WaterCrossings)for(int32 EndIndex=0;EndIndex<2;++EndIndex){
  const double Edge=EndIndex?Water.To:Water.From,Into=EndIndex?-1.:1.;
  if(Edge<Start||Edge>=End)continue;
  Rail::Sample S;if(!Sample(Edge,S))continue;
  const double Run=25,Drop=3.55;
  const double SlopeLength=FMath::Sqrt(Run*Run+Drop*Drop);
  const FRotator BankRotation(-Into*FMath::RadiansToDegrees(FMath::Atan2(Drop,Run)),0,0);
  // Same lateral extent as the old 89 abutting strips: -212.5m to +232.5m.
  // The backing top meets the surrounding ground at -0.65m. Account for
  // slab thickness so the slope's upper face meets that crest, not a raised lip.
  const double SlopeCentreZ=-.65-Drop*.5-.3*SlopeLength/Run;
  Add(25,S,FVector(-5.5*Into,10,SlopeCentreZ),FVector(SlopeLength+.6,445,.6),BankRotation);
  // Stop the backing at the crest. Extending it riverward over the ramp
  // exposes its vertical front face as a second, raised terrace.
  Add(25,S,FVector(-22.25*Into,10,-.925),FVector(8.5,445,.55));
  for(int32 Panel=0;Panel<89;++Panel){
   const double Y=-210+Panel*5,Noise=FMath::Sin(Y*.071+Edge*.01);
   // Continuous low-frequency meander, feathered to zero under the railway.
   // Only the wet/dry accents meander; the structural bank stays continuous.
   const double Outside=FMath::Clamp((FMath::Abs(Y-10)-26)/24.,0.,1.);
   const double BankShift=Outside*(4.1*Noise+1.8*FMath::Sin(Y*.029+Edge*.017));
   // Broken vegetation ribbons soften the representative engineered crest.
   // Wet silt, grass and bushes are outside the 24m railway/bridge envelope.
   if(FMath::Abs(Y-10)>24){
    // The surface is a continuous graded section. Former flattened spheres
    // read as giant pebbles from the bridge camera, especially in sunlight.
    // A shallow wet toe follows the same slope as the bank, with no raised rim.
    Add(29,S,FVector((3+BankShift)*Into,Y,-.65-(21+BankShift)*Drop/Run+.025),FVector(7.5,5.,.035),BankRotation);
    for(int32 Bush=0;Bush<2;++Bush){
     if((Panel+Bush)%4==0)continue;
     const FVector P((-12+Bush*5+BankShift)*Into,Y+Noise*1.4,-.85-Bush*.71);
     Add(28,S,P,FVector(2.3+Noise*.5,2.1+Noise*.4,1.5+Noise*.3),FRotator(0,Panel*47+Bush*73,0));
    }
   }
   // Irregular silt tongues and coarse riprap; never on the rail formation.
   if(FMath::Abs(Y-10)>24){
    Add(25,S,FVector((6+BankShift)*Into,Y,-4.02),FVector(5+Noise,5.8,.25));
    for(int32 Rock=0;Rock<2;++Rock){
     const double Along=-6+Rock*6.1+Noise*2;
     const double X=(Along+BankShift)*Into,RY=Y+FMath::Sin(Rock*2.1+Panel)*2;
     Add(2,S,FVector(X,RY,-2.1-(Along+6)*.14),FVector(.35+.18*Rock,.5,.35),FRotator(11*Rock,37*Panel+Rock*19,17));
    }
    // Unequal clumps, not regularly spaced square green posts. Thin crossed
    // blades stay in the existing grass family and terminate at the wet toe.
    for(int32 Reed=0;Reed<2;++Reed){
     const double RY=Y-1.5+Reed*2+FMath::Sin(Panel*2.7+Reed)*.6;
     const double ReedHeight=.65+.55*FMath::Abs(FMath::Sin(Panel+Reed*1.7));
     for(int32 Blade=0;Blade<3;++Blade)
      Add(24,S,FVector((3+BankShift)*Into+Blade*.13,RY+Blade*.17,-4.1+ReedHeight*.5),FVector(.035,.13,ReedHeight),FRotator(Noise*12+Blade*5,Reed*49+Blade*61,Noise*6));
    }
   }
  }
 }
 if(Track==1||Track==3){
  const double Side=Track==1?-1:1;
  const double Mid=BaseChainage(Track,(Start+End)*.5);
  // Only endpoint scenes own wide architectural precincts. Intermediate
  // stations have 12m clear envelopes; city fabric continues behind them.
  const bool Terminal=Mid<540||Mid>Rail::TambaramRoute::StationMetres.back()-400;
  bool NearStation=false;
  for(double Station:Rail::TambaramRoute::StationMetres)if(FMath::Abs(Mid-Station)<260)NearStation=true;
  const bool Airport=Mid>14000&&Mid<17900;
  const bool Industrial=(Mid>10700&&Mid<12500)||(Mid>20500&&Mid<21900);
  const double FenceY=NearStation?18:9.0;
  // Broad ground extends beyond the visible building rows, avoiding sliced
  // rectangular verges. Small overlaps hide seams on the tightest curves.
  for(double D=Start;D<End;D+=12){Rail::Sample S;if(!Sample(D,S))continue;
   if(AtWater(BaseChainage(Track,D+6),12))continue;
   Add(4,S,FVector(6,Side*87,-1.05),FVector(18,178,.8));
   // Continuous textured soil replaces the old rows of raised ellipsoids.
   // Surface variation comes from the world-space ground material; geometric
   // relief is reserved for drainage and the river slope. This avoids both
   // circular patch silhouettes and a new checkerboard of coloured slabs.
   if(Terminal)continue;
   Add(22,S,FVector(6,Side*(FenceY+6),-.58),FVector(15,6,.12));
   Add(13,S,FVector(6,Side*(FenceY+10),-.46),FVector(15,1.4,.18));
   // Open drain trough: invert and two raised sides, with access slabs.
   Add(11,S,FVector(6,Side*(FenceY-1.4),-.55),FVector(15,.7,.12));
   for(double Edge:{-1.,1.})Add(8,S,FVector(6,Side*(FenceY-1.4)+Edge*.43,-.30),FVector(15,.16,.55));
   if(!NearStation){
    Add(8,S,FVector(6,Side*FenceY,.25),FVector(12,.20,1.7));
    Add(6,S,FVector(6,Side*FenceY,1.14),FVector(12,.28,.12));
    for(double X:{0.,6.})Add(8,S,FVector(X,Side*FenceY,.30),FVector(.34,.34,1.9));
   }
   if(int32(D/12)%4==0){
    Add(13,S,FVector(5,Side*(FenceY-1.4),-.04),FVector(1,.95,.12));
    Add(23,S,FVector(5,Side*(FenceY+6),-.509),FVector(3,.09,.012));
   }
  }
  for(double D=Start+8;D<End;D+=Rng.FRandRange(18.f,27.f)){
   if(AtWater(BaseChainage(Track,D),45))continue;
   Rail::Sample S;if(!Sample(D,S))continue;
   // Independent width, setback, storeys, colour and facade rhythm avoid
   // repeated silhouettes. Composite construction stays HISM-batched.
   const double W=Rng.FRandRange(6.f,15.f),Depth=Rng.FRandRange(7.f,14.f);
   const double Setback=(Terminal?88:NearStation?65:FenceY+16)+Depth*.5+Rng.FRandRange(0.f,10.f);
   const int32 Floors=Airport?Rng.RandRange(1,2):Rng.RandRange(1,4);
   const double H=Floors*3.0,Base=-.48;
   const int32 Style=Rng.RandRange(0,5),Wall=Style<2?5:Style==2?6:Style==3?14:Style==4?15:16;
   auto Box=[&](int32 F,double X,double Y,double Z,double SX,double SY,double SZ){Add(F,S,FVector(X,Side*(Setback+Y),Base+Z),FVector(SX,SY,SZ));};
   const bool Shed=Industrial&&Rng.FRand()<.6f;
   if(Shed){
    Box(8,0,0,2.7,W+3,Depth+4,5.4);
    // Pitched sheet roof, clerestory, roller doors, structural ribs.
    for(double Sign:{-1.,1.})Add(10,S,FVector(0,Side*(Setback+Sign*(Depth+4)*.25),Base+5.8),FVector(W+4,(Depth+5)*.52,.16),FRotator(0,0,Sign*12));
    Box(11,0,-(Depth+4)*.5-.035,1.8,3.6,.10,3.6);
    for(double X=-W*.5;X<=W*.5;X+=2.5){Box(3,X,-(Depth+4)*.5-.10,2.8,.12,.12,5.6);Box(9,X,-(Depth+4)*.5-.12,4.8,1.7,.08,.65);}
    for(double Z=.35;Z<3.5;Z+=.3)Box(3,0,-(Depth+4)*.5-.10,Z,3.6,.04,.035);
    Box(13,0,-Depth*.5-4,.03,W+4,5,.06);
   }else{
    Box(Wall,0,0,H*.5,W,Depth,H);
    Box(8,0,0,.15,W+.25,Depth+.25,.3);
    const int32 Bays=FMath::Max(2,FMath::FloorToInt(W/2.6));
    const double Front=-Depth*.5-.07;
    for(int32 Floor=0;Floor<Floors;++Floor){
     const double Z=Floor*3;
     Box(8,0,0,Z+2.96,W+.25,Depth+.25,.16);
     for(int32 Bay=0;Bay<Bays;++Bay){
      const double X=-W*.5+(Bay+.5)*W/Bays;
      const bool Shop=Floor==0&&(NearStation||Style==3);
      Box(8,X,Front-.035,Z+1.65,Shop?2.25:1.55,.14,Shop?2.5:1.5);
      Box(Shop?11:9,X,Front-.13,Z+1.65,Shop?2.05:1.33,.07,Shop?2.3:1.28);
      Box(8,X,Front-.18,Z+1.65,.065,.06,1.3);
      Box(8,X,Front-.18,Z+1.65,1.35,.06,.065);
      // Concrete sunshade and side returns give depth in grazing views.
      Box(5,X,Front-.35,Z+2.48,1.9,.75,.11);
      if(Shop){Box(Style%2?10:6,X,Front-.75,2.8,2.5,1.6,.12);Box(23,X,Front-.2,2.65,1.8,.05,.3);}
      if(Floor>0&&Bay%2==Style%2){
       Box(8,X,Front-.6,Z+.12,2.1,1.3,.18);
       Box(5,X,Front-1.18,Z+.65,2.1,.10,1.0);
       for(double E:{-1.,1.})Box(5,X+E,Front-.6,Z+.65,.10,1.2,1.0);
      }
     }
     // Side windows avoid blank end walls in exterior cameras.
     for(double Sign:{-1.,1.})for(double Y:{-Depth*.25,Depth*.25})Box(9,Sign*(W*.5+.03),Y,Z+1.65,.07,1.2,1.25);
    }
    // Flat roof parapet, stair headroom, cylindrical black rooftop tank.
    for(double Sign:{-1.,1.}){Box(Wall,Sign*(W*.5-.09),0,H+.35,.18,Depth,.7);Box(Wall,0,Sign*(Depth*.5-.09),H+.35,W,.18,.7);}
    const double StairX=(Style%2?-.28:.25)*W;
    Box(Wall,StairX,Depth*.2,H+1,2.7,2.8,2);
    if(Floors<=2&&(Style==2||Style==5)){
     // Small tiled lean-to rooms mix low roof profiles into the flat terraces.
     for(double RoofSide:{-1.,1.})
      Add(6,S,FVector(-StairX,Side*(Setback-Depth*.18+RoofSide*Depth*.11),Base+H+.65),FVector(W*.46,Depth*.24,.12),FRotator(0,0,RoofSide*Side*18));
    }
    Box(8,-W*.25,Depth*.18,H+.3,1.8,1.8,.6);
    Add(20,S,FVector(-W*.25,Side*(Setback+Depth*.18),Base+H+1.15),FVector(1.45,1.45,1.2));
    Add(20,S,FVector(-W*.25,Side*(Setback+Depth*.18),Base+H+1.79),FVector(1.5,1.5,.10));
    if(Style==1)Box(10,-W*.1,-Depth*.2,H+.16,W*.45,Depth*.4,.15);
    // Exposed rainwater downpipes and roof outlets give the elevations a
    // believable service scale without adding a new mesh/material family.
    const double PipeX=W*.5-.38;
    Box(8,PipeX,Front-.2,H*.5,.10,.10,H+.2);
    Box(8,PipeX-.19,Front-.2,.23,.48,.10,.10);
    for(double Z=1.;Z<H;Z+=3.)Box(3,PipeX,Front-.17,Z,.22,.13,.06);
    if(Style==1||Style==4){
     Box(8,-W*.29,Depth*.18,H+1.85,.07,1.8,.07);
     Box(8,-W*.29,Depth*.18+.85,H+1.18,.07,.07,1.4);
    }
    // Mixed low annexes, verandas and rooftop service fittings break the
    // repeated full-height box silhouette typical of the earlier corridor.
    if(Style==0||Style==4){
     Box(Wall,W*.38,-Depth*.42,1.45,W*.45,Depth*.52,2.9);
     Box(8,W*.38,-Depth*.42,2.96,W*.47,Depth*.54,.13);
     Box(11,W*.38,-Depth*.69-.05,1.1,1.15,.08,2.1);
    }
    if(Style==2||Style==5){
     Box(10,-W*.12,Front-1.3,2.9,W*.65,2.5,.12);
     for(double X:{-W*.4,W*.2})Box(8,X,Front-2.3,1.4,.18,.18,2.8);
    }
    for(int32 Unit=0;Unit<(Floors>2?2:1);++Unit){
     Box(8,W*.31-Unit*1.3,Depth*.45+.14,H-1.05,.9,.35,.55);
     Box(11,W*.31-Unit*1.3,Depth*.45+.34,H-1.05,.7,.035,.4);
    }
   }
   // A second, staggered row builds city depth without crowding railway land.
   if(!Airport&&Rng.FRand()<.65f){
    const double FarH=Rng.RandRange(2,5)*3.,FarY=Setback+Depth+Rng.FRandRange(12.f,22.f);
    const double FarX=Rng.FRandRange(-7.f,10.f),FarW=W+Rng.FRandRange(0.f,5.f);
    Add(Wall,S,FVector(FarX,Side*FarY,FarH*.5+Base),FVector(FarW,Depth+2,FarH));
    for(double Z=1.5;Z<FarH;Z+=3)for(double X=-W*.4;X<W*.5;X+=2.8)
     for(double Face:{-1.,1.})Add(9,S,FVector(X+FarX,Side*(FarY+Face*((Depth+2)*.5+.05)),Z+Base),FVector(1.35,.10,1.3));
    for(double Z=1.5;Z<FarH;Z+=3)for(double Face:{-1.,1.})for(double Y:{-Depth*.25,Depth*.25})
     Add(9,S,FVector(FarX+Face*(FarW*.5+.04),Side*(FarY+Y),Z+Base),FVector(.08,1.3,1.3));
    Add(8,S,FVector(FarX,Side*FarY,FarH+Base),FVector(FarW+.4,Depth+2.4,.2));
    // Parapets and an offset stair room articulate the previously blank far
    // boxes visible above the riverbank. All faces receive window openings.
    for(double Face:{-1.,1.}){
     Add(Wall,S,FVector(FarX+Face*(FarW*.5-.1),Side*FarY,FarH+Base+.3),FVector(.2,Depth+2,.6));
     Add(Wall,S,FVector(FarX,Side*(FarY+Face*(Depth*.5+.9)),FarH+Base+.3),FVector(FarW,.2,.6));
    }
    Add(Wall,S,FVector(FarX+FarW*.25,Side*(FarY+Depth*.2),FarH+Base+.9),FVector(2.5,2.6,1.8));
   }
  }
  // Original tropical silhouettes, with exposed branch structure, asymmetric
  // flattened leaf clusters and occasional segmented palm fronds. These are
  // botanical adaptations, not individually surveyed/species-certified trees.
  for(double D=Start+4;D<End;D+=Rng.FRandRange(8.f,16.f)){
   if(AtWater(BaseChainage(Track,D),25))continue;
   Rail::Sample S;if(!Sample(D,S))continue;
   // Tree trunks stand outside the road, avoiding the earlier row of trees
   // physically planted through the asphalt. Crowns may overhang the verge.
   const double Y=Side*(Terminal?82:NearStation?60:Rng.FRandRange(19.f,23.f)),H=Rng.FRandRange(4.5f,9.5f);
   const bool Palm=Rng.FRand()<.22f;
   const double Lean=Rng.FRandRange(-.45f,.45f),Yaw=Rng.FRandRange(0.f,6.283185f);
   const FVector Root(0,Y,-.65),Crown(Lean,Y,H-.6);
   if(Palm){
    for(int32 K=0;K<5;++K){
     const double A=K/5.,B=(K+1)/5.;
     Limb(S,Root+FVector(Lean*A*A,0,H*A),Root+FVector(Lean*B*B,0,H*B),.24-K*.02);
    }
    for(int32 Frond=0;Frond<9;++Frond){
     const double A=Yaw+Frond*6.283185/9;
     const FVector Dir(FMath::Cos(A),FMath::Sin(A),0);
     for(int32 K=0;K<4;++K){
      const double T=(K+.5)/4.;
      const FVector P=Crown+Dir*(T*3.2)+FVector(0,0,1.3*FMath::Sin(T*3.14159)-T*.9);
      Add(17+Frond%2,S,P,FVector(1.15,.45*(1-T)+.1,.12),FRotator(25-45*T,FMath::RadiansToDegrees(A),0));
     }
    }
   }else{
    // Alternate spreading shade-tree and narrower upright habits. Crown
    // height/spread vary together instead of every tree sharing one ball.
    const bool Upright=Rng.FRand()<.32f;
    const double CrownWidth=Upright?.68:1.0,CrownHeight=Upright?1.30:.85;
    Limb(S,Root,FVector(Lean*.5,Y,H*.6-.6),Rng.FRandRange(.30f,.48f));
    const int32 Arms=Rng.RandRange(5,7);
    for(int32 Arm=0;Arm<Arms;++Arm){
     const double A=Yaw+Arm*2.39996,Spread=Rng.FRandRange(1.6f,3.0f)*CrownWidth;
     const FVector Tip(Lean+FMath::Cos(A)*Spread,Y+FMath::Sin(A)*Spread,H-.9+Rng.FRandRange(-1.2f,.5f));
     Limb(S,FVector(Lean*.3,Y,H*.42-.6),Tip,.13);
     for(int32 L=0;L<3;++L){
      const double LA=A+L*2.1;
      Add(17+(Arm+L)%2,S,Tip+FVector(FMath::Cos(LA)*.7,FMath::Sin(LA)*.7,L*.48),FVector(Rng.FRandRange(2.8f,4.2f)*CrownWidth,Rng.FRandRange(2.5f,3.7f)*CrownWidth,Rng.FRandRange(1.8f,2.8f)*CrownHeight),FRotator(Rng.FRandRange(-16.f,16.f),FMath::RadiansToDegrees(LA),Rng.FRandRange(-20.f,20.f)));
     }
    }
   }
   // A staggered far group adds depth behind the architecture without
   // colliding with front-row buildings. It uses the same bounded HISM cells.
   if(!Terminal&&Rng.FRand()<.85f){
    const double GroveY=Rng.FRandRange(108.f,143.f);
    for(int32 Tree=0;Tree<3;++Tree){
     const double FY=Side*(GroveY+Tree*5+Rng.FRandRange(-2.f,2.f)),FH=Rng.FRandRange(4.f,9.f);
     const double FX=Rng.FRandRange(-6.f,7.f),Width=Rng.FRandRange(3.f,5.f);
     Limb(S,FVector(FX,FY,-.7),FVector(FX+.3,FY,FH-.5),.25);
     for(int32 K=0;K<5;++K){
      const double A=K*2.4+Tree;
      Add(17+K%2,S,FVector(FX+FMath::Cos(A)*1.6,FY+FMath::Sin(A)*1.6,FH+FMath::Sin(A)*.5),FVector(Width,Width*.85,2.7),FRotator(0,K*71+Tree*23,0));
     }
     if(Tree%2==0)Add(28,S,FVector(FX+2,FY,-.05),FVector(2.8,2.3,1.3),FRotator(0,Tree*61,0));
    }
   }
   if(!NearStation&&!Terminal&&Rng.FRand()<.70f)for(int32 L=0;L<4;++L){
    const double ScrubY=Side*Rng.FRandRange(6.3f,7.6f),ScrubX=Rng.FRandRange(-2.f,3.f);
    const double ScrubHeight=Rng.FRandRange(.35f,.85f);
    Add(28,S,FVector(ScrubX,ScrubY,-.65+ScrubHeight*.5),FVector(Rng.FRandRange(.65f,1.35f),.8,ScrubHeight),FRotator(0,L*47,0));
    Add(24,S,FVector(ScrubX+.5,ScrubY,-.35),FVector(.035,.13,.6),FRotator(5,L*73,14));
   }
  }
 }
 const int32 MeshIndex[]={0,1,2,2,2,2,2,3,2,2,2,2,2,2,2,2,2,4,4,5,5,2,2,2,2,2,2,2,4,2,2};
 const int32 MaterialIndex[]={-1,-1,0,1,2,3,4,-1,5,6,7,8,9,10,11,12,13,14,15,16,8,17,18,19,20,21,5,20,14,21,0};
 for(int32 Family=0;Family<FamilyCount;++Family){
  if(Instances[Family].IsEmpty())continue;
  auto* Part=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);
  Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Meshes[MeshIndex[Family]]);
  if(MaterialIndex[Family]>=0)Part->SetMaterial(0,Materials[MaterialIndex[Family]]);
  if(Family==21&&WaterMaterial)Part->SetMaterial(0,WaterMaterial);
  if((Family==17||Family==18||Family==28)&&LeafCluster&&LeafMaterial){
   Part->SetStaticMesh(LeafCluster);auto* M=UMaterialInstanceDynamic::Create(LeafMaterial,this);
   // Linear-space leaf albedo plus deterministic per-cell variation. Previous
   // bright green values washed out to lime under the route's daylight.
   const float Shade=.78f+.22f*FMath::Abs(FMath::Sin(CellIndex*1.71f+Family*.93f));
   M->SetVectorParameterValue(TEXT("Color"),(Family==17?FLinearColor(.022f,.065f,.018f):Family==18?FLinearColor(.045f,.085f,.026f):FLinearColor(.036f,.057f,.019f))*Shade);Part->SetMaterial(0,M);
  }
  if((Family==4||Family==25||Family==24||Family==27||Family==29||Family==30)&&GroundMaterial){
   auto* M=UMaterialInstanceDynamic::Create(GroundMaterial,this);
   const FLinearColor C=Family==24?FLinearColor(.075f,.12f,.035f):Family==27?FLinearColor(.19f,.22f,.075f):Family==29?FLinearColor(.055f,.065f,.035f):Family==30?FLinearColor(.24f,.20f,.135f):Family==25?FLinearColor(.14f,.105f,.06f):FLinearColor(.20f,.16f,.095f);
   M->SetVectorParameterValue(TEXT("Color"),C);Part->SetMaterial(0,M);
  }
  Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);
  Part->SetCastShadow(Family!=2&&Family!=4);Part->SetCullDistances(Family==2?7000:70000,Family==2?11000:100000);
  Part->bAutoRebuildTreeOnInstanceChanges=false;Part->RegisterComponent();Part->AddInstances(Instances[Family],false,true);
  Part->BuildTreeIfOutdated(true,true);Part->bAutoRebuildTreeOnInstanceChanges=true;
  Components.Add(Part);Cell.Parts.Add(Part);Cell.Instances+=Instances[Family].Num();if(Family==2)Cell.Stones+=Instances[Family].Num();
 }
 ++RebuildCount;
}
bool ARailTambaramCorridorView::PresentSignals(const Rail::OperationsWorld& World,int32 ChunkId){
 const auto* Train=World.Spatial(Rail::TrainId(2));if(!Train)return false;
 TArray<uint64> Ids;TArray<FTransform> Poses;TArray<bool> Proceed;
 for(const auto& Entry:World.DefinitionsView().signals){
  Rail::Sample S;const auto State=World.Current().signals.find(Entry.first);
  if(State==World.Current().signals.end()||!Train->Network().SamplePosition(Entry.second.boundary,S).ok)return false;
  if(!Owns(ChunkId,S.position.x))continue;
  const FVector Forward=Cm(S.tangent).GetSafeNormal(),Right(-Forward.Y,Forward.X,0);
  Ids.Add(Entry.first.value);Poses.Add(FTransform(FRotator(0,Forward.Rotation().Yaw+90,0),Cm(S.position)-Right*270));
  Proceed.Add(State->second.aspect==Rail::SignalAspect::Proceed);
 }
 if(Ids!=SignalIds){
  auto* Housing=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_SignalUnbound.SM_SignalUnbound"));
  auto* Lens=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Egmore/M_SignalLens.M_SignalLens"));
  if(!Housing||!Lens||!Material)return false;
  for(const auto& Part:SignalParts){RemoveInstanceComponent(Part.Get());Part->DestroyComponent();}
  SignalParts.Empty();SignalLamps.Empty();SignalIds=Ids;
  for(int32 I=0;I<Ids.Num();++I)for(int32 P=0;P<4;++P){
   auto* Part=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);
   Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(P?Lens:Housing);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   Part->SetCastShadow(P==0);Part->SetCullDistance(100000);Part->RegisterComponent();SignalParts.Add(Part);
   if(P){auto* Lamp=UMaterialInstanceDynamic::Create(Material,this);Part->SetMaterial(0,Lamp);SignalLamps.Add(Lamp);}
  }
 }
 for(int32 I=0;I<Ids.Num();++I){
  SignalParts[I*4]->SetWorldTransform(Poses[I]);
  for(int32 L=0;L<3;++L){
   SignalParts[I*4+L+1]->SetWorldTransform(FTransform(Poses[I].GetRotation(),Poses[I].TransformPosition(FVector(0,19,330+45*L)),FVector(.25,.025,.25)));
   const auto Color=L==0&&!Proceed[I]?FLinearColor(6,0,0):L==2&&Proceed[I]?FLinearColor(0,6,0):FLinearColor(.005f,.005f,.005f);
   SignalLamps[I*3+L]->SetVectorParameterValue(TEXT("Color"),Color);
  }
 }
 return true;
}
void ARailTambaramCorridorView::Present(const Rail::OperationsWorld& World,int32 ChunkId){
 if(!Rail::ScenarioContent::IsTambaram(World.ContentId())){if(Cells.Num())Release();return;}
 if(ChunkId!=1&&ChunkId!=2){Diagnostic=TEXT("Invalid chunk ownership");bReady=false;return;}
 if(!LoadAssets()){bReady=false;return;}
 if(!PresentSignals(World,ChunkId)){Diagnostic=TEXT("Signal presentation unavailable");bReady=false;return;}
 const auto* Player=World.Spatial(Rail::TrainId(2));if(!Player){bReady=false;return;}
 const auto& Graph=Player->Network();
 if(OwnerChunk&&OwnerChunk!=ChunkId)Release();OwnerChunk=ChunkId;
 TSet<int32> Wanted;
 for(uint64 TrainId:{1ull,2ull})if(const auto* Train=World.Spatial(Rail::TrainId(TrainId))){
  const double Head=Train->Current().headStationMetres+(TrainId==1?Rail::TambaramRoute::TrafficStationOffset():0),Rear=Head-Train->Current().properties.lengthMetres;
  for(int32 Track=0;Track<4;++Track){
   const double Length=Rail::RouteLength(Graph,Rail::TambaramRoute::Path(Track));
   const int32 First=FMath::Max(0,FMath::FloorToInt((Rear-360)/CellMetres));
   const int32 Last=FMath::Min(FMath::CeilToInt(Length/CellMetres)-1,FMath::FloorToInt((Head+720)/CellMetres));
   for(int32 I=First;I<=Last;++I)Wanted.Add(Track*100000+I);
  }
 }
 if(Wanted.Num()>MaxCells){Diagnostic=TEXT("Corridor cell bound exceeded; preserving previous presentation");bReady=false;return;}
 TArray<int32> Keys;Cells.GetKeys(Keys);for(int32 Key:Keys)if(!Wanted.Contains(Key))RemoveCell(Key);
 for(int32 Key:Wanted)if(!Cells.Contains(Key)){FCell Cell;BuildCell(Graph,Key/100000,Key%100000,ChunkId,Cell);Cells.Add(Key,MoveTemp(Cell));}
 ActiveCells=Cells.Num();InstanceCount=StoneInstances=0;
 for(const auto& Pair:Cells){InstanceCount+=Pair.Value.Instances;StoneInstances+=Pair.Value.Stones;}
 bReady=true;
}
