#include "RailTambaramCorridorView.h"
#include "RailTambaramTurnoutView.h"
#include "Simulation/RailTambaramRoute.h"
#include "Simulation/RailRouteSampling.h"
#include "Simulation/RailScenarioContent.h"
#include "Simulation/RailAutomaticSignals.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/StaticMeshComponent.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(RailScenery,true);

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
FString ARailTambaramCorridorView::TrackAssetTelemetryJson() const {
 const FString Rails=Meshes.IsValidIndex(0)&&Meshes[0]?Meshes[0]->GetPathName():TEXT("");
 const FString Bed=Meshes.IsValidIndex(1)&&Meshes[1]?Meshes[1]->GetPathName():TEXT("");
 const FString Stone=BallastStone?BallastStone->GetPathName():TEXT("");
 const FString Ground=GroundMaterial?GroundMaterial->GetPathName():TEXT("");
 FString Finishes=TEXT("[");
 for(int32 I=0;I<TrackFinishes.Num();++I){if(I)Finishes+=TEXT(",");Finishes+=TEXT("\"")+(TrackFinishes[I]?TrackFinishes[I]->GetPathName():TEXT(""))+TEXT("\"");}
 Finishes+=TEXT("]");
 FString Textures=TEXT("[");
 for(int32 I=0;I<TrackTextures.Num();++I){
  if(I)Textures+=TEXT(",");const auto* T=TrackTextures[I].Get();
  Textures+=T?FString::Printf(TEXT("{\"asset\":\"%s\",\"width\":%d,\"height\":%d,\"mips\":%d,\"streamable\":%s}"),*T->GetPathName(),T->GetSizeX(),T->GetSizeY(),T->GetNumMips(),T->IsStreamable()?TEXT("true"):TEXT("false")):TEXT("null");
 }
 Textures+=TEXT("]");
 FString Nature=TEXT("[");
 for(const UObject* Asset:{static_cast<const UObject*>(LeafCluster.Get()),static_cast<const UObject*>(TerrainStrip.Get()),static_cast<const UObject*>(GrassTuft.Get()),static_cast<const UObject*>(BranchMesh.Get()),static_cast<const UObject*>(LeafMaterial.Get()),static_cast<const UObject*>(TerrainMaterial.Get()),static_cast<const UObject*>(GrassMaterial.Get()),static_cast<const UObject*>(BranchMaterial.Get()),static_cast<const UObject*>(OuterGroundSurface.Get())}){
  if(Nature.Len()>1)Nature+=TEXT(",");Nature+=TEXT("\"")+(Asset?Asset->GetPathName():TEXT(""))+TEXT("\"");
 }
 Nature+=TEXT("]");
 return FString::Printf(TEXT("{\"rails\":\"%s\",\"bed\":\"%s\",\"stone\":\"%s\",\"ground\":\"%s\",\"finishes\":%s,\"textures\":%s,\"nature\":%s}"),*Rails,*Bed,*Stone,*Ground,*Finishes,*Textures,*Nature);
}
bool ARailTambaramCorridorView::LoadAssets(){
 if(bAssetsAttempted)return Meshes.Num()==6&&Materials.Num()==28;
 bAssetsAttempted=true;
 LeafCluster=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/SM_ChennaiLeafCluster.SM_ChennaiLeafCluster"));
 LeafMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiLeafSurface.M_ChennaiLeafSurface"));
 if(auto* Dense=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/SM_ChennaiDenseCanopy.SM_ChennaiDenseCanopy")))LeafCluster=Dense;
 if(auto* DenseMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiDenseLeaf.M_ChennaiDenseLeaf")))LeafMaterial=DenseMaterial;
 if(auto* Fine=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Meshes/SM_CorridorLeafClusterV1.SM_CorridorLeafClusterV1")))LeafCluster=Fine;
 if(auto* Dense=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Meshes/SM_CorridorLeafClusterV2.SM_CorridorLeafClusterV2")))LeafCluster=Dense;
 if(auto* Fine=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Materials/M_CorridorLeaf.M_CorridorLeaf")))LeafMaterial=Fine;
 TerrainStrip=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Meshes/SM_CorridorVerge12m.SM_CorridorVerge12m"));
 GrassTuft=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Meshes/SM_CorridorGrassPatchV1.SM_CorridorGrassPatchV1"));
 BranchMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Meshes/SM_CorridorBranchV1.SM_CorridorBranchV1"));
 TerrainMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Materials/M_CorridorNatureGround.M_CorridorNatureGround"));
 GrassMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Materials/M_CorridorDryGrass.M_CorridorDryGrass"));
 BranchMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Materials/M_CorridorBark.M_CorridorBark"));
 OuterGroundSurface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorNatureV1/Materials/M_CorridorOuterGround.M_CorridorOuterGround"));
 GroundMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiGroundSurface.M_ChennaiGroundSurface"));
 if(auto* Finish=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorFinishV2/M_CorridorGround.M_CorridorGround")))GroundMaterial=Finish;
 if(auto* Finish=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/MaterialsV4/M_DryGround.M_DryGround")))GroundMaterial=Finish;
 WaterMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_ChennaiRiverSurface.M_ChennaiRiverSurface"));
 const TCHAR* MeshPaths[]={TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_Track6m.SM_Track6m"),
  TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_Ballast6m.SM_Ballast6m"),
  TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_TreeBroadleaf.SM_TreeBroadleaf"),
  TEXT("/Engine/BasicShapes/Sphere.Sphere"),TEXT("/Engine/BasicShapes/Cylinder.Cylinder")};
 for(const auto* Path:MeshPaths){auto* Asset=LoadObject<UStaticMesh>(nullptr,Path);if(!Asset){Diagnostic=FString::Printf(TEXT("Missing corridor mesh: %s"),Path);Meshes.Empty();return false;}Meshes.Add(Asset);}
 if(auto* Track=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/Meshes/SM_Track6m_RealismV2.SM_Track6m_RealismV2")))Meshes[0]=Track;
 if(auto* Bed=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/Meshes/SM_Ballast6m_RealismV3.SM_Ballast6m_RealismV3")))Meshes[1]=Bed;
 BallastStone=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/Meshes/SM_AngularBallastStoneV1.SM_AngularBallastStoneV1"));
 for(const TCHAR* Name:{TEXT("M_RailHead"),TEXT("M_RailRust"),TEXT("M_SleeperConcrete"),TEXT("M_TrackStone")}){
  const FString Path=FString::Printf(TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/MaterialsV4/%s.%s"),Name,Name);
  TrackFinishes.Add(LoadObject<UMaterialInterface>(nullptr,*Path));
 }
 for(const TCHAR* Name:{TEXT("T_TrackBallast_BC"),TEXT("T_DryGround_BC"),TEXT("T_DryVerge_BC")}){
  const FString Path=FString::Printf(TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/TexturesV2/%s.%s"),Name,Name);
  TrackTextures.Add(LoadObject<UTexture2D>(nullptr,*Path));
 }
 for(const TCHAR* Name:{TEXT("Ballast"),TEXT("Galvanized"),TEXT("Soil"),TEXT("Cream"),TEXT("RedOxide"),TEXT("Concrete"),TEXT("Glass"),TEXT("Roof"),TEXT("Black"),TEXT("Wood"),TEXT("Paving")}){
  const FString Path=FString::Printf(TEXT("/Game/IndianRailway/S8/Stations/Kit01/Materials/%s.%s"),Name,Name);
  auto* Material=LoadObject<UMaterialInterface>(nullptr,*Path);if(!Material){Diagnostic=TEXT("Missing corridor material: ")+Path;Materials.Empty();return false;}Materials.Add(Material);
 }
 // Original project-authored lit colour master; optional during asset migration.
 auto* Tint=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_SceneryTint.M_SceneryTint"));
 if(auto* Finish=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S9/CorridorFinishV2/M_CorridorFinish.M_CorridorFinish")))Tint=Finish;
 // Last six: the weathered emulsion pastels that dominate Chennai suburban housing (lemon,
 // sky blue, rose, mint, off-white, peach). Linear values, sun-faded rather than catalogue new.
 const FLinearColor Colors[]={FLinearColor(.36f,.53f,.58f),FLinearColor(.64f,.38f,.32f),FLinearColor(.44f,.54f,.36f),
  FLinearColor(.075f,.19f,.035f),FLinearColor(.16f,.27f,.045f),FLinearColor(.12f,.075f,.035f),
  FLinearColor(.11f,.17f,.14f),FLinearColor(.07f,.075f,.075f),FLinearColor(.68f,.66f,.55f),
  FLinearColor(.20f,.25f,.11f),FLinearColor(.38f,.30f,.18f),
  FLinearColor(.66f,.58f,.30f),FLinearColor(.40f,.55f,.64f),FLinearColor(.64f,.42f,.43f),
  FLinearColor(.46f,.60f,.47f),FLinearColor(.70f,.69f,.63f),FLinearColor(.70f,.48f,.33f)};
 // Painted walls (teal, terracotta, green and the six pastels) get monsoon streaks, faded
 // paint and storey-base dust from a biplanar world-aligned mask: two fetches, mobile-safe.
 auto* Facade=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S10/ChennaiFacadeV1/M_ChennaiFacade.M_ChennaiFacade"));
 for(int32 Index=0;Index<UE_ARRAY_COUNT(Colors);++Index){
  const bool Wall=Index<3||Index>=11;
  auto* Parent=Wall&&Facade?Facade:Tint;
  if(Parent){auto* MID=UMaterialInstanceDynamic::Create(Parent,this);MID->SetVectorParameterValue(TEXT("Color"),Colors[Index]);
   if(Parent==Facade)MID->SetScalarParameterValue(TEXT("GrimeAmount"),1.25f); // a few monsoons since repaint
   Materials.Add(MID);}
  else Materials.Add(Materials[3]);
 }
 // Material count is constant during travel: twelve foliage shades and six
 // ground finishes instead of up to nine new MIDs for every streamed cell.
 if(LeafMaterial)for(int32 Kind=0;Kind<3;++Kind)for(int32 Shade=0;Shade<4;++Shade){
  auto* Material=UMaterialInstanceDynamic::Create(LeafMaterial,this);
  const FLinearColor Base=Kind==0?FLinearColor(.022f,.065f,.018f):Kind==1?FLinearColor(.045f,.085f,.026f):FLinearColor(.036f,.057f,.019f);
  Material->SetVectorParameterValue(TEXT("Color"),Base*(.78f+.22f*Shade/3.f));LeafPalette.Add(Material);
 }
 const FLinearColor GroundColors[]={FLinearColor(.14f,.125f,.07f),FLinearColor(.14f,.105f,.06f),FLinearColor(.075f,.12f,.035f),FLinearColor(.19f,.22f,.075f),FLinearColor(.055f,.065f,.035f),FLinearColor(.24f,.20f,.135f)};
 if(GroundMaterial)for(const auto& Color:GroundColors){
  auto* Material=UMaterialInstanceDynamic::Create(GroundMaterial,this);
  Material->SetVectorParameterValue(TEXT("Color"),Color);
  Material->SetScalarParameterValue(TEXT("VegetationCoverage"),GroundPalette.IsEmpty()?.62f:0.f);GroundPalette.Add(Material);
 }
 if(GroundMaterial){
  OuterGroundMaterial=UMaterialInstanceDynamic::Create(OuterGroundSurface?OuterGroundSurface:GroundMaterial,this);
  // Post-monsoon Chennai backland: weedy green over red-brown laterite, not bare desert.
  OuterGroundMaterial->SetVectorParameterValue(TEXT("Color"),FLinearColor(.085f,.095f,.04f));
  OuterGroundMaterial->SetScalarParameterValue(TEXT("VegetationCoverage"),.95f);
 }
 // S10 photographic surfaces (CC0 Poly Haven maps, world-aligned): red laterite and weedy
 // grass ground, weathered concrete for slabs/walls/posts/drains, cracked asphalt roads.
 // Optional during migration: absent assets leave the procedural finishes above in place.
 if(auto* Ground=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S10/PolyHavenV1/M_ChennaiGround.M_ChennaiGround"))){
  // Exposed laterite under Chennai dust reads brown-red, not the saturated orange of a fresh cut.
  const FLinearColor Dust(.80f,.86f,.92f);
  auto* Trackside=UMaterialInstanceDynamic::Create(Ground,this);Trackside->SetScalarParameterValue(TEXT("VegetationCoverage"),.40f); // trodden, dusty
  Trackside->SetVectorParameterValue(TEXT("Tint"),Dust);
  if(GroundPalette.Num())GroundPalette[0]=Trackside;
  OuterGroundMaterial=UMaterialInstanceDynamic::Create(Ground,this);OuterGroundMaterial->SetScalarParameterValue(TEXT("VegetationCoverage"),.74f);
  OuterGroundMaterial->SetVectorParameterValue(TEXT("Tint"),Dust);
 }
 if(auto* Concrete=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S10/PolyHavenV1/M_ChennaiConcrete.M_ChennaiConcrete")))Materials[5]=Concrete;
 if(auto* Asphalt=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S10/PolyHavenV1/M_ChennaiAsphalt.M_ChennaiAsphalt")))Materials[18]=Asphalt;
 Diagnostic=TEXT("Ready: bounded graph-sampled corridor cells");return true;
}
void ARailTambaramCorridorView::RemoveCell(int32 Key){
 CSV_SCOPED_TIMING_STAT(RailScenery,RemoveCellMs);
 if(auto* Cell=Cells.Find(Key))for(auto* Part:Cell->Parts){Components.Remove(Part);RemoveInstanceComponent(Part);Part->DestroyComponent();}
 if(Cells.Remove(Key))++ReleasedCells;
}
void ARailTambaramCorridorView::Release(){
 TArray<int32> Keys;Cells.GetKeys(Keys);for(int32 Key:Keys)RemoveCell(Key);
 ActiveCells=InstanceCount=StoneInstances=0;bReady=false;
}
void ARailTambaramCorridorView::BuildCell(const Rail::Graph& Graph,int32 Track,int32 CellIndex,int32 ChunkId,FCell& Cell){
 CSV_SCOPED_TIMING_STAT(RailScenery,BuildCellMs);
 CSV_CUSTOM_STAT(RailScenery,CellsBuilt,1,ECsvCustomStatOp::Accumulate);
 const auto Path=Rail::TambaramRoute::Path(Track);
 const double Start=CellIndex*CellMetres,End=FMath::Min(Start+CellMetres,Rail::RouteLength(Graph,Path));
 if(End<=Start)return;
 // Family order: track, shaped ballast, close stones, OHE steel, ground, cream buildings,
 // oxide buildings, trees. No collision and no gameplay state in any family.
 constexpr int32 FamilyCount=40; // ground relief, grass tufts, exterior soil finish and six facade pastels
 TArray<FTransform> Instances[FamilyCount];
 const Rail::PreparedRouteSampler Sampler(Graph,Path);
 auto Sample=[&](double D,Rail::Sample& S){return Sampler.SampleAt(D,S);};
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
 // Matches the authored 12m median strip. Sampling this only during cell
 // construction keeps grass roots on the shallow relief without runtime mesh
 // edits or collision queries. End/lateral edges are flat for mixed LODs.
 auto Smooth=[](double A,double B,double V){const double T=FMath::Clamp((V-A)/(B-A),0.,1.);return T*T*(3.-2.*T);};
 auto MedianHeight=[&](double D,double Y){
  const double X=FMath::Fmod(D,12.),LocalY=Y-10.675;
  const double Fade=Smooth(0.,1.,FMath::Min(X,12.-X))*(1.-Smooth(2.6,3.5,FMath::Abs(LocalY)));
  const double PathY=-1.3+.18*FMath::Sin(X*UE_TWO_PI/12.),DrainY=1.95+.14*FMath::Sin(X*UE_TWO_PI/12.+.7);
  const double PathMask=1.-Smooth(.30,.78,FMath::Abs(LocalY-PathY));
  const double DampMask=1.-Smooth(.14,.68,FMath::Abs(LocalY-DrainY));
  const double Mound=.095*FMath::Sin(X*.79+LocalY*1.6)+.045*FMath::Cos(X*1.37-LocalY*2.4);
  return -.65+Fade*FMath::Max(-.24,Mound*(1.-PathMask*.8)-.045*PathMask-.17*DampMask);
 };
 auto MedianPathY=[](double D){return 9.375+.18*FMath::Sin(FMath::Fmod(D,12.)*UE_TWO_PI/12.);};
 FRandomStream Rng(7301+Track*100000+CellIndex);
 for(double D=Start;D<End-.001;D+=6){
  Rail::Sample A,B;if(!Sample(D,A)||!Sample(FMath::Min(D+6,End),B))continue;
  const auto Delta=B.position-A.position;const double Length=Delta.Length();
  A.tangent=Delta.Unit(); // endpoints on authoritative geometry; <=6m render chord only
  if(!ARailTambaramTurnoutView::ReplacesPlainTrack(Track,D))Add(0,A,FVector::ZeroVector,FVector(Length/6,1,1));Add(1,A,FVector(-.04,0,0),FVector((Length+.08)/6,1,1));
 }
 // Ballast chords overlap by four centimetres at each end. On bends this
 // covers the outer-edge wedge between differently rotated adjacent modules;
 // world-space texture mapping keeps their overlapping surface continuous.
 // The crossover is part of the graph, not an invented diagonal overlay.
 // Its one owner is the cell containing its entry on track 1; normal demand
 // windows keep the complete 140m connector present before either train arrives.
 double CrossoverEntry=0;
 for(size_t I=0;I<Rail::TambaramRoute::CrossoverSegment;++I)CrossoverEntry+=Rail::TambaramRoute::GeometryFor(1,I).lengthMetres;
 if(Track==1&&CrossoverEntry>=Start&&CrossoverEntry<End){
  // Shared turnout bearers need ballast across both lines. A slightly lower
  // broad bed joins the existing shoulders without coplanar top surfaces.
  const double BedFrom=FMath::FloorToDouble(CrossoverEntry/6)*6;
  const double BedTo=FMath::CeilToDouble((CrossoverEntry+Rail::TambaramRoute::GeometryFor(1,Rail::TambaramRoute::CrossoverSegment).lengthMetres)/6)*6;
  for(double D=BedFrom;D<BedTo;D+=6){
   Rail::Sample A,B;if(!Sample(D,A)||!Sample(FMath::Min(D+6,BedTo),B))continue;
   const auto Delta=B.position-A.position;A.tangent=Delta.Unit();
   Add(1,A,FVector(-.04,2.5,-.015),FVector((Delta.Length()+.08)/6,2.2,1));
  }
  for(bool Second:{false,true}){
   const auto Geometry=Rail::TambaramRoute::CrossoverGeometry(Second);
   for(double D=0;D<Geometry.lengthMetres;D+=3){
    auto A=Geometry.At(D);const auto B=Geometry.At(FMath::Min(D+3,Geometry.lengthMetres));
    const auto Delta=B.position-A.position;A.tangent=Delta.Unit();
    // Detailed connector rails and shared bearers belong to TurnoutView.
    Add(1,A,FVector(-.04,0,-.025),FVector((Delta.Length()+.08)/6,1,1));
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
 // The ballast surface carries the aggregate field. A few half-embedded stones
 // catch grazing light along the outer flat shoulder, clear of sleeper ends.
 // Four accents per metre replaces twelve: the texture does the distant work.
 FRandomStream Ballast(76421+Track*100000+CellIndex);
 for(double D=Start;D<End;D+=.25){
  Rail::Sample S;if(!Sample(D,S))continue;
  const double Y=(Ballast.FRand()<.5f?-1:1)*Ballast.FRandRange(1.46f,1.62f),Size=Ballast.FRandRange(.025f,.055f);
  Add(2,S,FVector(0,Y,-.235-Size*.18),FVector(Size,Size*Ballast.FRandRange(.7f,1.1f),Size*.65),FRotator(Ballast.FRandRange(-25.f,25.f),Ballast.FRandRange(0.f,180.f),Ballast.FRandRange(-25.f,25.f)));
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
  for(double D=Start;D<End;D+=12){Rail::Sample S,B;if(!Sample(D,S))continue;
   if(AtWater(D+6,8))continue;
   if(!TerrainStrip||!TerrainMaterial||AtStationShelter(D+6)||AtWater(D+6,30)||!Sample(FMath::Min(D+12,End),B)){
    Add(4,S,FVector(6,10,-.8),FVector(16,32,.3));continue;
   }
   const auto Delta=B.position-S.position;S.tangent=Delta.Unit();
   // Preserve level foundations under each track pair. Only the open median
   // backing drops below the sculpted strip, allowing the drain to be an
   // actual depression instead of a dark rectangle painted on raised soil.
   Add(4,S,FVector(6,.0875,-.8),FVector(16,12.175,.3));
   Add(4,S,FVector(6,20.5875,-.8),FVector(16,10.825,.3));
   Add(4,S,FVector(6,10.675,-1.15),FVector(16,9,.3));
   Add(31,S,FVector(-.08,10.675,-.65),FVector((Delta.Length()+.16)/12,1,1));
  }
 }
 // This alignment retains an 11.35m strip between the two pairs of tracks.
 // Give the open maintenance land broken, low vegetation rather than a vast
 // uniform dirt runway. All roots stay at least 3m from either track centre;
 // station precincts and bridge approaches remain clear. No new surface slabs.
 const auto FoliageBounds=(LeafCluster&&LeafPalette.Num()==12?LeafCluster:Meshes[4])->GetBounds();
 const double FoliageBottom=(FoliageBounds.Origin.Z-FoliageBounds.BoxExtent.Z)*.01;
 if(Track==1)for(int32 Slot=FMath::CeilToInt(Start/6);Slot*6<End;++Slot){
  FRandomStream Median(78217+Slot*281);
  const double D=Slot*6+Median.FRandRange(.6f,4.8f);
  if(D>=End||AtStationShelter(D)||AtWater(D,28))continue;
  const double Patch=.5+.5*FMath::Sin(D*.035+.8);
  if(Median.FRand()>.32+.58*Patch)continue;
  Rail::Sample S;if(!Sample(D,S))continue;
  const double Y=Median.FRandRange(8.05f,13.2f);
  const bool Shaped=TerrainStrip&&TerrainMaterial&&!AtStationShelter(FMath::FloorToDouble(D/12)*12+6)&&!AtWater(FMath::FloorToDouble(D/12)*12+6,30);
  const int32 Clumps=Median.RandRange(3,6);
  for(int32 Clump=0;Clump<Clumps;++Clump){
   const double Height=Median.FRandRange(.16f,.42f);
   const double CY=FMath::Clamp(Y+Median.FRandRange(-.45f,.45f),8.05,13.2);
   const double X=Clump*.47+Median.FRandRange(-.24f,.24f);
   if(FMath::Abs(CY-MedianPathY(D+X))<.52)continue;
   const double Ground=Shaped?MedianHeight(D+X,CY):-.65;
   const int32 Family=GrassTuft&&GrassMaterial?32:28;
   Add(Family,S,FVector(X,CY,Ground-(Family==32?.015:.025+FoliageBottom*Height)),
    FVector(Median.FRandRange(.40f,.85f),Median.FRandRange(.35f,.8f),Height),FRotator(0,Median.FRandRange(0.f,360.f),0));
  }
  if(Slot%5==0&&FMath::Abs(Y-MedianPathY(D))>.8){
   const double Ground=Shaped?MedianHeight(D,Y):-.65,Height=Median.FRandRange(.40f,.72f);
   Add(28,S,FVector(0,Y,Ground-.035-FoliageBottom*Height),FVector(1.25,1.0,Height),FRotator(0,Median.FRandRange(0.f,360.f),0));
  }
  // Small spilled aggregate fragments sit in the soil, never large rocks or
  // geometric colour patches. Their independent seed keeps reloads identical.
  for(int32 Stone=0;Stone<3;++Stone){
   const double Size=Median.FRandRange(.035f,.085f);
   const double X=Median.FRandRange(-.8f,1.3f),SY=Y+Median.FRandRange(-.55f,.55f);
   Add(2,S,FVector(X,SY,(Shaped?MedianHeight(D+X,SY):-.65)-.01),
    FVector(Size,Size*.7,Size*.45),FRotator(0,Median.FRandRange(0.f,180.f),15));
  }
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
  // A separate stable stream keeps street furniture from reshuffling buildings
  // and trees when its density changes. Everything remains HISM batched.
  FRandomStream Detail(19391+Track*71000+CellIndex*137);
  // Broad ground extends beyond the visible building rows, avoiding sliced
  // rectangular verges. Small overlaps hide seams on the tightest curves.
  for(double D=Start;D<End;D+=12){Rail::Sample S;if(!Sample(D,S))continue;
   if(AtWater(BaseChainage(Track,D+6),12))continue;
   // At the 176m outer edge, the tightest route bend separates 12m rail
   // samples by up to 21.33m. A 26m tile keeps those distant corners covered;
   // retain the former 18m footprint near riverbanks so no extra level soil
   // extends over the graded slope or the water exclusion.
   const double GroundLength=AtWater(BaseChainage(Track,D+6),32)?18.:26.;
   Add(33,S,FVector(6,Side*87,-1.05),FVector(GroundLength,178,.8));
   // Continuous textured soil replaces the old rows of raised ellipsoids.
   // Surface variation comes from the world-space ground material; geometric
   // relief is reserved for drainage and the river slope. This avoids both
   // circular patch silhouettes and a new checkerboard of coloured slabs.
   if(Terminal)continue;
   Add(22,S,FVector(6,Side*(FenceY+6),-.58),FVector(15,6,.12));
   Add(13,S,FVector(6,Side*(FenceY+10),-.46),FVector(15,1.4,.18));
   // Low weathered drainage channel. The previous 55cm bright curbs read as
   // a second raised platform along the whole route. Its dark invert now
   // meets the soil, while the jointed sides project only 24cm above it.
   Add(11,S,FVector(6,Side*(FenceY-1.4),-.651),FVector(12.03,.56,.025));
   for(double Edge:{-1.,1.})Add(26,S,FVector(6,Side*(FenceY-1.4)+Edge*.36,-.54),FVector(11.98,.14,.26));
   if(!NearStation){
    Add(8,S,FVector(6,Side*FenceY,.25),FVector(12,.20,1.7));
    Add(6,S,FVector(6,Side*FenceY,1.14),FVector(12,.28,.12));
    for(double X:{0.,6.})Add(8,S,FVector(X,Side*FenceY,.30),FVector(.34,.34,1.9));
    // Cable troughs sit on the soil rather than float beside the sleepers.
    // Butt-jointed covers and occasional access covers retain their useful
    // railway scale without a shiny, uninterrupted raised stripe.
    Add(26,S,FVector(6,Side*3.35,-.58),FVector(12,.40,.14));
    for(double X=1.;X<12.;X+=2.){
     const bool Access=(int32(D/12)+int32(X))%13==0;
     Add(Access?13:26,S,FVector(X,Side*3.35,-.48),FVector(1.96,.46,.06));
    }
   }
   if(int32(D/12)%4==0){
    Add(26,S,FVector(5,Side*(FenceY-1.4),-.38),FVector(1,.92,.06));
    Add(23,S,FVector(5,Side*(FenceY+6),-.509),FVector(3,.09,.012));
   }
  }
  if(!Terminal&&!NearStation){
   Rail::Sample S;
   const double D=Start+Detail.FRandRange(28.f,86.f);
   if(!AtWater(BaseChainage(Track,D),25)&&Sample(D,S)){
    if(CellIndex%3==Track%3){
     // Relay cupboard on a raised concrete plinth. No invented signal aspect
     // or readable safety marking: signal state belongs to PresentSignals.
     const double Y=Side*4.8;
     Add(8,S,FVector(0,Y,-.27),FVector(1.65,1.05,.4));
     Add(3,S,FVector(0,Y,.55),FVector(1.25,.65,1.25));
     Add(10,S,FVector(0,Y,1.21),FVector(1.38,.8,.09));
     Add(11,S,FVector(0,Y-Side*.337,.55),FVector(.018,.015,1.1));
     for(double X:{-.10,.10})Add(11,S,FVector(X,Y-Side*.36,.56),FVector(.035,.04,.14));
     for(int32 Vent=0;Vent<4;++Vent)Add(11,S,FVector(.34,Y-Side*.34,.22+Vent*.07),FVector(.34,.015,.018));
    }
    // Sparse parked road vehicles, never mistaken for simulated railway
    // traffic. Wheels meet the road and the footpath remains unobstructed.
    if(Detail.FRand()<(Airport?.18f:Industrial?.45f:.62f)){
     const double Y=Side*(FenceY+8.15),Z=-.52;
     const bool Van=Industrial&&CellIndex%2==0;
     Add(CellIndex%3==0?6:5,S,FVector(0,Y,Z+.65),FVector(Van?4.6:3.8,1.65,.62));
     Add(9,S,FVector(-.15,Y,Z+1.18),FVector(Van?2.9:2.05,1.44,.60));
     Add(5,S,FVector(-.15,Y,Z+1.51),FVector(Van?3.:2.15,1.52,.09));
     for(double X:{-.9,.35})Add(5,S,FVector(X,Y,Z+1.18),FVector(.10,1.50,.62));
     for(double X:{-1.2,1.2})for(double E:{-1.,1.}){
      Add(20,S,FVector(X,Y+E*.80,Z+.34),FVector(.62,.62,.20),FRotator(0,0,90));
      Add(3,S,FVector(X,Y+E*.915,Z+.34),FVector(.30,.035,.30));
     }
     for(double E:{-1.,1.}){
      Add(8,S,FVector(Van?2.32:1.92,Y+E*.53,Z+.72),FVector(.035,.30,.15));
      Add(6,S,FVector(Van?-2.32:-1.92,Y+E*.55,Z+.72),FVector(.035,.22,.17));
     }
    }
    if(CellIndex%2==0){
     // Pavement lighting, not trackside OHE. Simple down-facing luminaire.
     const double Y=Side*(FenceY+10.5);
     Add(8,S,FVector(0,Y,-.18),FVector(.42,.42,.65));
     Add(3,S,FVector(0,Y,2.8),FVector(.10,.10,6.1));
     Add(3,S,FVector(0,Y-Side*.55,5.76),FVector(.08,1.2,.08));
     Add(10,S,FVector(0,Y-Side*1.1,5.69),FVector(.35,.65,.12));
     Add(8,S,FVector(0,Y-Side*1.1,5.62),FVector(.27,.52,.025));
    }
   }
  }
  // Coherent neighbourhoods span several streaming cells; a cell boundary must
  // not reset every street to the same first house. These are representative
  // streetscapes, not surveyed plots. All variation stays beyond the clearance.
  const double District=.5+.5*FMath::Sin(Mid*.0019+Side*2.3);
  // Chennai suburbs are packed plot-to-plot; gaps are the exception, not the rule.
  const double Density=Airport?.35:Industrial?.66:.86+.12*District;
  for(double D=Start+Rng.FRandRange(10.f,23.f);D<End-10;D+=Rng.FRandRange(17.f,27.f)){
   if(AtWater(BaseChainage(Track,D),45))continue;
   const double Plot=BaseChainage(Track,D);
   const bool OpenStretch=!NearStation&&!Terminal&&
    FMath::Sin(Plot*.0061+Side*1.7)>.83&&FMath::Sin(Plot*.0013-Side)>.1;
   if(OpenStretch||Rng.FRand()>Density){
    // This plot has no building. Low broken scrub gives the empty lot a
    // different silhouette from occupied paved yards without crossing the
    // street, railway fence or a neighbouring building footprint.
    if(!Terminal&&!NearStation){
     Rail::Sample Open;if(Sample(D,Open)){
      FRandomStream Lot(9137+Track*31000+FMath::FloorToInt(D)*71);
      const double PatchY=FenceY+Lot.FRandRange(20.f,29.f);
      for(int32 Clump=0;Clump<14;++Clump){
       const double X=Lot.FRandRange(-4.f,4.f),Y=Side*(PatchY+Lot.FRandRange(-3.f,3.f));
       if(GrassTuft&&GrassMaterial)Add(32,Open,FVector(X,Y,-.665),FVector(Lot.FRandRange(.65f,1.1f),Lot.FRandRange(.6f,.95f),Lot.FRandRange(.25f,.6f)),FRotator(0,Lot.FRandRange(0.f,360.f),0));
      }
      for(int32 Shrub=0;Shrub<3;++Shrub){
       const double Height=Lot.FRandRange(.65f,1.2f);
       Add(28,Open,FVector(Lot.FRandRange(-3.f,3.f),Side*(PatchY+Lot.FRandRange(-2.5f,2.5f)),-.67-FoliageBottom*Height),FVector(Lot.FRandRange(1.4f,2.2f),Lot.FRandRange(1.1f,1.7f),Height),FRotator(0,Lot.FRandRange(0.f,360.f),0));
      }
     }
    }
    continue;
   }
   Rail::Sample S;if(!Sample(D,S))continue;
   // Independent width, setback, storeys, colour and facade rhythm avoid
   // repeated silhouettes. Composite construction stays HISM-batched.
   const double W=Rng.FRandRange(7.f,17.f),Depth=Rng.FRandRange(7.f,14.f);
   const double Setback=(Terminal?88:NearStation?65:FenceY+16)+Depth*.5+Rng.FRandRange(0.f,10.f)+8*District;
   const int32 Floors=Airport?Rng.RandRange(1,2):Rng.RandRange(1,District>.62?4:2);
   const double H=Floors*3.0,Base=-.48;
   const int32 Style=Rng.RandRange(0,5);
   const int32 Palette=(int32(FMath::FloorToDouble(Plot/670))+Track)%3;
   // Mostly pastel emulsion, with cream, teal, terracotta and red-oxide minorities.
   static constexpr int32 WallChoices[]={34,35,36,37,38,39,5,14,15,6};
   const int32 Wall=WallChoices[(Style*3+Palette*5+int32(FMath::Abs(Plot)/31))%10];
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
    // Not every roof has the same tank at the same corner. Keep the
    // residential service fittings coherent with the house's stable style.
    if(Style!=3){
     const double TankX=(Style%2?-.27:.23)*W,TankY=Style<3?Depth*.18:-Depth*.12;
     Box(8,TankX,TankY,H+.3,1.8,1.8,.6);
     Add(20,S,FVector(TankX,Side*(Setback+TankY),Base+H+1.15),FVector(1.45,1.45,1.2));
     Add(20,S,FVector(TankX,Side*(Setback+TankY),Base+H+1.79),FVector(1.5,1.5,.10));
    }
    if(Style==3||Style==4){
     // Small roof-mounted panel bank and supports, outside railway land.
     for(int32 Panel=0;Panel<3;++Panel){
      Box(3,-W*.27+Panel*1.1,Depth*.12,H+.35,.08,1.45,.65);
      Add(9,S,FVector(-W*.27+Panel*1.1,Side*(Setback+Depth*.12),Base+H+.72),FVector(1.,1.65,.07),FRotator(0,0,Side*12));
     }
    }
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
    // Private forecourts with real openings, low boundary walls and paving
    // connect the houses to the street instead of leaving floating boxes.
    if(!Terminal&&!NearStation){
     const double Yard=FMath::Max(1.5,Setback-Depth*.5-(FenceY+11));
     const double GateY=-Depth*.5-Yard;
     Box(13,0,-Depth*.5-Yard*.5,.015,W,Yard,.06);
     for(double E:{-1.,1.}){
      Box(Wall,E*(W*.25+1.05),GateY,.55,FMath::Max(.5,W*.5-2.1),.18,1.1);
      Box(8,E*2.1,GateY,.75,.28,.28,1.5);
      Box(Wall,E*W*.5,-Depth*.5-Yard*.5,.55,.18,Yard,1.1);
     }
     // Open entrance; a visible route to the door, not a fence across it.
     if(Style==1||Style==4)for(int32 Step=0;Step<2;++Step)
      Box(8,0,Front-.7+Step*.23,.10+Step*.10,1.8,.65-Step*.23,.20+Step*.20);
    }
   }
   // A second, staggered row builds city depth without crowding railway land.
   if(!Airport&&Rng.FRand()<.50f+.32f*District){
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
  // Ground cover uses the actual foliage base; this mesh is flatter than a
  // unit sphere, so height/2 would leave its lowest leaves floating above soil.
  // Shape and rear-grove choices never consume the building/detail streams.
  FRandomStream Canopy(51743+Track*79000+CellIndex*173);
  // Anchor candidates to route distance, then keep coherent groves and gaps.
  // Jitter hides the candidate grid; the independent stream cannot reshuffle
  // building plots or change placement when a neighbouring cell is unloaded.
  // At most twelve near trees per full cell, below the old fifteen-tree bound.
  for(int32 Slot=FMath::CeilToInt(Start/10);Slot*10<End;++Slot){
   FRandomStream Placement(29117+Track*71000+Slot*137);
   const double D=Slot*10+Placement.FRandRange(1.f,8.f);
   if(D>=End)continue;
   const double Plot=BaseChainage(Track,D);
   if(AtWater(Plot,25))continue;
   const double Grove=.5+.32*FMath::Sin(Plot*.024+Side*1.7)+.18*FMath::Sin(Plot*.0071-Side);
   const double Cover=(Airport?.55:Industrial?.70:1.)*(.20+.72*Grove);
   if(Placement.FRand()>Cover)continue;
   Rail::Sample S;if(!Sample(D,S))continue;
   // Tree trunks stand outside the road, avoiding the earlier row of trees
   // physically planted through the asphalt or footpath. Crowns overhang the
   // verge, while the root line varies within the same clear strip of land.
   const double RootSetback=Terminal?Placement.FRandRange(80.f,85.f):NearStation?Placement.FRandRange(57.5f,62.f):Placement.FRandRange(20.9f,24.3f);
   const double Y=Side*RootSetback,H=Canopy.FRandRange(5.5f,9.5f);
   // Palms form occasional neighbourhood groups rather than a rigid every-Nth
   // tree pattern; a broadleaf-dominant street remains visually coherent.
   const bool Palm=Canopy.FRand()<(.10+.18*(.5+.5*FMath::Sin(Plot*.0081+Side)));
   const double Lean=Canopy.FRandRange(-.45f,.45f),Yaw=Canopy.FRandRange(0.f,6.283185f);
   const FVector Root(0,Y,-.65),Crown(Lean,Y,H-.6);
   if(Palm){
    for(int32 K=0;K<5;++K){
     const double A=K/5.,B=(K+1)/5.;
     Limb(S,Root+FVector(Lean*A*A,0,H*A),Root+FVector(Lean*B*B,0,H*B),.30-K*.018);
    }
    for(int32 Frond=0;Frond<9;++Frond){
     const double A=Yaw+Frond*6.283185/9;
     const FVector Dir(FMath::Cos(A),FMath::Sin(A),0);
     const FVector Bend=Crown+Dir*1.55+FVector(0,0,.75),Tip=Crown+Dir*3.1-FVector(0,0,.80);
     Limb(S,Crown,Bend,.055);Limb(S,Bend,Tip,.035);
     for(int32 K=0;K<4;++K){
      const double T=(K+.5)/4.;
      const FVector P=Crown+Dir*(T*3.2)+FVector(0,0,1.3*FMath::Sin(T*3.14159)-T*.9);
      Add(17+Frond%2,S,P,FVector(1.15,.45*(1-T)+.1,.34),FRotator(25-45*T,FMath::RadiansToDegrees(A),0));
     }
    }
    Add(17,S,Crown+FVector(0,0,.15),FVector(1.,1.,.8),FRotator(0,FMath::RadiansToDegrees(Yaw),0));
   }else{
    // Alternate spreading shade-tree and narrower upright habits. Crown
    // height/spread vary together instead of every tree sharing one ball.
    const bool Upright=Canopy.FRand()<.32f;
    const double CrownWidth=Upright?.68:1.0,CrownHeight=Upright?1.30:.85;
    Limb(S,Root,FVector(Lean*.5,Y,H*.6-.6),Canopy.FRandRange(.30f,.48f));
    // Overlapping inner shoots fill the centre and bring the crown down over
    // the branch forks. Thin tip-only clusters exposed a candelabra silhouette.
    for(int32 Level=0;Level<2;++Level)
     Add(17+Level,S,FVector(Lean*.7,Y,H*(Level?.88:.68)),FVector(3.1*CrownWidth,2.8*CrownWidth,3.0*CrownHeight),FRotator(Level*12,Level*73,Level*-9));
    const int32 Arms=Canopy.RandRange(5,7);
    for(int32 Arm=0;Arm<Arms;++Arm){
     const double A=Yaw+Arm*2.39996,Spread=Canopy.FRandRange(1.35f,2.5f)*CrownWidth;
     const FVector Tip(Lean+FMath::Cos(A)*Spread,Y+FMath::Sin(A)*Spread,H+Canopy.FRandRange(-1.8f,.1f));
     const FVector Fork(Lean*.3,Y,H*(.45+.025*(Arm%3))-.35),Bend=Fork+(Tip-Fork)*.57+FVector(-FMath::Sin(A)*.17,FMath::Cos(A)*.17,.20);
     Limb(S,Fork,Bend,.19);Limb(S,Bend,Tip,.11);
     for(int32 L=0;L<3;++L){
      const double LA=A+L*2.1;
      Add(17+(Arm+L)%2,S,Tip+FVector(FMath::Cos(LA)*.55,FMath::Sin(LA)*.55,L*.26-.35),FVector(Canopy.FRandRange(1.9f,2.8f)*CrownWidth,Canopy.FRandRange(1.7f,2.5f)*CrownWidth,Canopy.FRandRange(1.9f,2.6f)*CrownHeight),FRotator(Canopy.FRandRange(-16.f,16.f),FMath::RadiansToDegrees(LA),Canopy.FRandRange(-20.f,20.f)));
     }
    }
   }
   // A staggered far group adds depth behind the architecture without
   // colliding with front-row buildings. It uses the same bounded HISM cells.
   if(!Terminal&&Canopy.FRand()<.85f){
    const double GroveY=Canopy.FRandRange(106.f,145.f);
    const int32 GroveTrees=Canopy.RandRange(1,3);
    for(int32 Tree=0;Tree<GroveTrees;++Tree){
     const double FY=Side*(GroveY+Canopy.FRandRange(-5.f,10.f)),FH=Canopy.FRandRange(4.5f,10.f);
     const double FX=Canopy.FRandRange(-11.f,12.f),Width=Canopy.FRandRange(2.5f,3.5f);
     Limb(S,FVector(FX,FY,-.7),FVector(FX+.3,FY,FH*.82),.25);
     for(int32 K=0;K<5;++K){
      const double A=K*2.4+Tree;
      Add(17+K%2,S,FVector(FX+FMath::Cos(A)*.9,FY+FMath::Sin(A)*.9,FH*(K<2?.70:.93)+FMath::Sin(A)*.4),FVector(Width,Width*.85,3.0),FRotator(K*9,K*71+Tree*23,0));
     }
     if(Tree%2==0)Add(28,S,FVector(FX+2,FY,-.665-FoliageBottom*1.3),FVector(2.8,2.3,1.3),FRotator(0,Tree*61,0));
    }
   }
  }
  // Chennai never shows an empty horizon from a suburban line: beyond the trackside rows the
  // land is packed with low-rise houses, coconut palms and rain-tree/neem canopy. A compact
  // house (pastel walls, flat roof, parapet, chajja-shaded windows, stair room and black
  // polyethylene tank) fills a third row and a continuous far band out to the 176m soil edge.
  // Independent streams: nothing above reshuffles. Facades face the railway; backs stay plain.
  auto CompactHouse=[&](const Rail::Sample& S,FRandomStream& R,double X,double Y,double W,double Depth,int32 Floors,bool Far){
   static constexpr int32 Pastels[]={34,35,36,37,38,39,34,38,5,14};
   const int32 Paint=Pastels[R.RandRange(0,9)];
   const double H=Floors*3.05,Base=-.55,Front=Y-Depth*.5;
   auto Box=[&](int32 F,double BX,double BY,double Z,double SX,double SY,double SZ){Add(F,S,FVector(X+BX,Side*BY,Base+Z),FVector(SX,SY,SZ));};
   Box(Paint,0,Y,H*.5,W,Depth,H);
   Box(8,0,Y,H+.1,W+.2,Depth+.2,.2);
   for(double E:{-1.,1.}){Box(Paint,E*(W*.5-.08),Y,H+.5,.16,Depth,.8);Box(Paint,0,Y+E*(Depth*.5-.08),H+.5,W,.16,.8);}
   const int32 Bays=FMath::Max(1,FMath::FloorToInt(W/(Far?3.2:2.7)));
   for(int32 Floor=0;Floor<Floors;++Floor)for(int32 Bay=0;Bay<Bays;++Bay){
    const double BX=-W*.5+(Bay+.5)*W/Bays,Z=Floor*3.05;
    Box(9,BX,Front-.04,Z+1.55,1.3,.08,1.3);
    if(!Far||Floor<2)Box(5,BX,Front-.32,Z+2.36,1.7,.6,.09); // chajja sunshade
   }
   if(R.FRand()<.8f){
    const double TX=R.FRandRange(-.3f,.3f)*W,TY=Y+R.FRandRange(-.2f,.2f)*Depth;
    Add(20,S,FVector(X+TX,Side*TY,Base+H+.85),FVector(1.25,1.25,1.1));
    Add(20,S,FVector(X+TX,Side*TY,Base+H+1.45),FVector(1.3,1.3,.08));
   }
   if(!Far||R.FRand()<.6f)Box(Paint,R.FRandRange(-.25f,.25f)*W,Y+Depth*.15,H+1.1,2.6,2.7,2.2);
  };
  auto Palm=[&](const Rail::Sample& S,FRandomStream& R,double X,double Y,double H){
   const double Lean=R.FRandRange(-.6f,.6f),Yaw=R.FRandRange(0.f,6.283185f);
   const FVector Root(X,Y,-.65),Crown(X+Lean,Y,H);
   Limb(S,Root,FVector(X+Lean*.35,Y,H*.55),.24);Limb(S,FVector(X+Lean*.35,Y,H*.55),Crown,.20);
   for(int32 Frond=0;Frond<7;++Frond){
    const double A=Yaw+Frond*6.283185/7;const FVector Dir(FMath::Cos(A),FMath::Sin(A),0);
    for(int32 K=0;K<2;++K){
     const double T=K?.78:.35;
     Add(17+Frond%2,S,Crown+Dir*(T*3.1)+FVector(0,0,.9*FMath::Sin(T*3.14159)-T*1.1),FVector(1.5,.42,.32),FRotator(20-50*T,FMath::RadiansToDegrees(A),0));
    }
   }
  };
  if(!Terminal){
   FRandomStream Fabric(88317+Track*67000+CellIndex*211);
   // Third row: 62-100m, mostly residential; open airport land keeps its gaps.
   for(double D=Start+Fabric.FRandRange(4.f,12.f);D<End-6;D+=Fabric.FRandRange(13.f,21.f)){
    const double Plot=BaseChainage(Track,D);
    if(AtWater(Plot,60)||Fabric.FRand()>(Airport?.25:Industrial?.55:.85))continue;
    Rail::Sample S;if(!Sample(D,S))continue;
    const double W=Fabric.FRandRange(7.f,12.f),Depth=Fabric.FRandRange(8.f,12.f);
    CompactHouse(S,Fabric,Fabric.FRandRange(-2.f,2.f),Fabric.FRandRange(64.f,96.f),W,Depth,Fabric.RandRange(1,3+(District>.55?1:0)),false);
    if(Fabric.FRand()<.35f)Palm(S,Fabric,Fabric.FRandRange(-5.f,5.f),Side*Fabric.FRandRange(58.f,100.f),Fabric.FRandRange(9.f,14.f));
   }
   // Far band: 112-172m, closed skyline of taller blocks, palms above canopy and tree mass.
   for(double D=Start+Fabric.FRandRange(2.f,8.f);D<End-4;D+=Fabric.FRandRange(9.f,15.f)){
    const double Plot=BaseChainage(Track,D);
    if(AtWater(Plot,80))continue;
    Rail::Sample S;if(!Sample(D,S))continue;
    if(Fabric.FRand()<(Airport?.15:Industrial?.5:.7)){
     const int32 Floors=Airport?Fabric.RandRange(1,2):Fabric.RandRange(2,District>.5?6:4);
     CompactHouse(S,Fabric,Fabric.FRandRange(-1.5f,1.5f),Fabric.FRandRange(118.f,162.f),Fabric.FRandRange(9.f,16.f),Fabric.FRandRange(9.f,14.f),Floors,true);
    }
    const int32 Crowns=Fabric.RandRange(1,3);
    for(int32 Crown=0;Crown<Crowns;++Crown){
     const double CY=Side*Fabric.FRandRange(108.f,172.f),CH=Fabric.FRandRange(6.f,12.f),CW=Fabric.FRandRange(5.f,9.f);
     const double CX=Fabric.FRandRange(-5.f,5.f);
     Limb(S,FVector(CX,CY,-.65),FVector(CX,CY,CH-CW*.15),.28);
     Add(17+Crown%2,S,FVector(CX,CY,CH),FVector(CW,CW*.85,CW*.55),FRotator(0,Fabric.FRandRange(0.f,360.f),0));
     Add(18-Crown%2,S,FVector(CX+CW*.2,CY,CH-CW*.22),FVector(CW*.8,CW*.75,CW*.45),FRotator(0,Fabric.FRandRange(0.f,360.f),0));
    }
    if(Fabric.FRand()<.45f)Palm(S,Fabric,Fabric.FRandRange(-4.f,4.f),Side*Fabric.FRandRange(110.f,168.f),Fabric.FRandRange(11.f,17.f));
   }
  }
  // Independent dry grass patches and occasional woody scrub occupy the
  // neglected verge. Grass blades grow from ground level instead of scaling
  // tree crowns into flat green pancakes. The cable route and drain stay clear.
  if(!NearStation&&!Terminal)for(int32 Slot=FMath::CeilToInt(Start/6);Slot*6<End;++Slot){
   FRandomStream Verge(41837+Track*61000+Slot*193);
   const double D=Slot*6+Verge.FRandRange(.5f,4.5f);
   if(D>=End)continue;
   const double Plot=BaseChainage(Track,D);
   if(AtWater(Plot,25))continue;
   const double Patch=.5+.5*FMath::Sin(Plot*.041+Side*2.3);
   if(Verge.FRand()>.25+.65*Patch)continue;
   Rail::Sample S;if(!Sample(D,S))continue;
   const double PatchY=Verge.FRandRange(4.5f,6.8f);
   const int32 Tufts=Verge.RandRange(3,6);
   for(int32 Clump=0;Clump<Tufts;++Clump){
    const double Height=Verge.FRandRange(.16f,.48f);
    const double Y=Side*(PatchY+Verge.FRandRange(-.35f,.35f));
    const int32 Family=GrassTuft&&GrassMaterial?32:28;
    Add(Family,S,FVector(Clump*.52+Verge.FRandRange(-.25f,.25f),Y,Family==32?-.67:-.675-FoliageBottom*Height),
     FVector(Verge.FRandRange(.40f,.85f),Verge.FRandRange(.35f,.80f),Height),FRotator(0,Verge.FRandRange(0.f,360.f),0));
   }
   if(Slot%4==0){
    const double Height=Verge.FRandRange(.45f,.82f);
    Add(28,S,FVector(0,Side*PatchY,-.675-FoliageBottom*Height),FVector(1.35,1.05,Height),FRotator(0,Verge.FRandRange(0.f,360.f),0));
   }
   // Sparse spill beyond the toe breaks the geometrically perfect ballast
   // boundary without changing its slope or scattering objects on the track.
   for(int32 Stone=0;Stone<4;++Stone){
    const double Size=Verge.FRandRange(.03f,.07f);
    Add(2,S,FVector(Verge.FRandRange(-.8f,1.1f),Side*Verge.FRandRange(2.36f,2.75f),-.655),
     FVector(Size,Size*.8,Size*.5),FRotator(0,Verge.FRandRange(0.f,180.f),20));
   }
  }
  // Bounded background growth occupies only the unused outer land behind
  // both building rows. Nothing is placed through private courtyards, roads,
  // station precincts or river approaches. The broad surface remains one
  // continuous textured soil layer: no raised vegetation-coloured slabs.
  // Two global 60m candidates per cell cap this at 30 tufts and six shrubs.
  if(!Terminal)for(int32 Slot=FMath::CeilToInt(Start/60);Slot*60<End;++Slot){
   FRandomStream Backland(59281+Track*81000+Slot*317);
   const double D=Slot*60+Backland.FRandRange(12.f,44.f);
   if(D>=End||AtWater(BaseChainage(Track,D),45))continue;
   Rail::Sample S;if(!Sample(D,S))continue;
   const double PatchY=Backland.FRandRange(151.f,165.f);
   const int32 Tufts=Backland.RandRange(10,15);
   for(int32 Tuft=0;Tuft<Tufts;++Tuft){
    const double X=Backland.FRandRange(-10.f,10.f),Y=Side*(PatchY+Backland.FRandRange(-4.f,4.f));
    const double Height=Backland.FRandRange(.28f,.68f);
    const int32 Family=GrassTuft&&GrassMaterial?32:28;
    Add(Family,S,FVector(X,Y,Family==32?-.67:-.675-FoliageBottom*Height),
     FVector(Backland.FRandRange(1.2f,2.4f),Backland.FRandRange(1.0f,1.9f),Height),FRotator(0,Backland.FRandRange(0.f,360.f),0));
   }
   const int32 Shrubs=Backland.RandRange(2,3);
   for(int32 Shrub=0;Shrub<Shrubs;++Shrub){
    const double X=Backland.FRandRange(-8.f,8.f),Y=Side*(PatchY+Backland.FRandRange(-4.f,4.f));
    const double Height=Backland.FRandRange(.9f,1.7f),Width=Backland.FRandRange(2.4f,4.2f);
    Add(28,S,FVector(X,Y,-.675-FoliageBottom*Height),FVector(Width,Width*Backland.FRandRange(.65f,.95f),Height),FRotator(0,Backland.FRandRange(0.f,360.f),0));
   }
  }
 }
 const int32 MeshIndex[]={0,1,2,2,2,2,2,3,2,2,2,2,2,2,2,2,2,4,4,5,5,2,2,2,2,2,2,2,4,2,2,2,4,2,2,2,2,2,2,2};
 const int32 MaterialIndex[]={-1,-1,0,1,2,3,4,-1,5,6,7,8,9,10,11,12,13,14,15,16,8,17,18,19,20,21,5,20,14,21,0,-1,-1,2,22,23,24,25,26,27};
 for(int32 Family=0;Family<FamilyCount;++Family){
  if(Instances[Family].IsEmpty())continue;
  auto* Part=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);
  Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Meshes[MeshIndex[Family]]);
  if(MaterialIndex[Family]>=0)Part->SetMaterial(0,Materials[MaterialIndex[Family]]);
  if(Family==0&&TrackFinishes.Num()==4)for(int32 Slot=0;Slot<Part->GetNumMaterials();++Slot){
   if(const auto* Original=Part->GetMaterial(Slot)){
    const FString Name=Original->GetName();const int32 Finish=Name==TEXT("RailHead")?0:Name==TEXT("RailRust")?1:Name==TEXT("Concrete")?2:-1;
    if(Finish>=0&&TrackFinishes[Finish])Part->SetMaterial(Slot,TrackFinishes[Finish]);
   }
  }
  if(Family==2&&BallastStone){Part->SetStaticMesh(BallastStone);if(TrackFinishes.Num()==4&&TrackFinishes[3])Part->SetMaterial(0,TrackFinishes[3]);}
  if(Family==26&&TrackFinishes.Num()==4&&TrackFinishes[2])Part->SetMaterial(0,TrackFinishes[2]);
  if(Family==19&&BranchMesh){Part->SetStaticMesh(BranchMesh);if(BranchMaterial)Part->SetMaterial(0,BranchMaterial);}
  if(Family==31&&TerrainStrip){Part->SetStaticMesh(TerrainStrip);if(TerrainMaterial)Part->SetMaterial(0,TerrainMaterial);}
  if(Family==32&&GrassTuft){Part->SetStaticMesh(GrassTuft);if(GrassMaterial)Part->SetMaterial(0,GrassMaterial);}
  if(Family==21&&WaterMaterial)Part->SetMaterial(0,WaterMaterial);
  if((Family==17||Family==18||Family==28)&&LeafCluster&&LeafPalette.Num()==12){
   Part->SetStaticMesh(LeafCluster);
   const int32 Kind=Family==17?0:Family==18?1:2;
   const int32 Shade=FMath::Clamp(FMath::RoundToInt(3*FMath::Abs(FMath::Sin(CellIndex*1.71f+Family*.93f))),0,3);
   Part->SetMaterial(0,LeafPalette[Kind*4+Shade]);
  }
  if(GroundPalette.Num()==6){
   const int32 GroundIndex=Family==4||Family==33?0:Family==25?1:Family==24?2:Family==27?3:Family==29?4:Family==30?5:-1;
   if(GroundIndex>=0)Part->SetMaterial(0,GroundPalette[GroundIndex]);
  }
  if(Family==33&&OuterGroundMaterial)Part->SetMaterial(0,OuterGroundMaterial);
  Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);
  Part->SetCastShadow(Family!=2&&Family!=4&&Family!=32&&Family!=33);
  Part->SetCullDistances(Family==2?7000:Family==32?18000:70000,Family==2?11000:Family==32?30000:100000);
  Part->bAutoRebuildTreeOnInstanceChanges=false;Part->RegisterComponent();Part->AddInstances(Instances[Family],false,true);
  Part->BuildTreeIfOutdated(true,true);Part->bAutoRebuildTreeOnInstanceChanges=true;
  Components.Add(Part);Cell.Parts.Add(Part);Cell.Instances+=Instances[Family].Num();if(Family==2)Cell.Stones+=Instances[Family].Num();
 }
 ++RebuildCount;
}
bool ARailTambaramCorridorView::PresentSignals(const Rail::OperationsWorld& World,int32 ChunkId){
 const auto* Train=World.Spatial(Rail::TrainId(2));if(!Train)return false;
 // Every lineside signal is infrastructure from the route plan; its aspect is evaluated from the
 // authoritative operations state each frame. Instances are rebuilt only when an aspect changes.
 const auto& Plan=Rail::SignalPlanFor(World);const auto Aspects=Rail::EvaluateSignals(Plan,World);
 TArray<int32> Sites;TArray<uint8> Shown;TArray<FTransform> Poses;
 for(const auto& Site:Plan.sites){
  Rail::Sample S;if(!Train->Network().SamplePosition(Site.position,S).ok||!Owns(ChunkId,S.position.x))continue;
  Sites.Add(int32(Site.index));Shown.Add(uint8(Aspects[Site.index])|(Site.role==Rail::SignalRole::Automatic?8:0));
  Poses.Add(FTransform(Facing(S.tangent),Cm(S.position)));
 }
 if(SignalHisms.Num()!=9){
  auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
  auto* Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
  auto* Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  auto* Lens=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Egmore/M_SignalLens.M_SignalLens"));
  if(!Cube||!Cylinder||!Sphere||!Lens||Materials.Num()<9)return false;
  // Lit lens colours are emissive; dark lenses keep a faint glass sheen.
  const FLinearColor LensColours[]={FLinearColor(9.f,.25f,.12f),FLinearColor(9.f,4.6f,.15f),FLinearColor(.15f,8.f,2.6f),FLinearColor(.012f,.012f,.012f)};
  for(const auto& Colour:LensColours){auto* M=UMaterialInstanceDynamic::Create(Lens,this);M->SetVectorParameterValue(TEXT("Color"),Colour);SignalLamps.Add(M);}
  // 0 post (galvanised), 1 head and 2 hoods (black), 3 'A' plate (cream), 4 glyph (black), 5-8 lenses.
  UStaticMesh* Mesh[]={Cylinder,Cube,Cube,Cylinder,Cube,Sphere,Sphere,Sphere,Sphere};
  UMaterialInterface* Finish[]={Materials[1],Materials[8],Materials[8],Materials[3],Materials[8],SignalLamps[0],SignalLamps[1],SignalLamps[2],SignalLamps[3]};
  for(int32 I=0;I<9;++I){
   auto* Part=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);
   Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Mesh[I]);Part->SetMaterial(0,Finish[I]);
   Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetCastShadow(I<3);Part->SetCullDistances(150000,180000);
   Part->RegisterComponent();SignalHisms.Add(Part);
  }
  SignalSitesShown.Reset();
 }
 if(Sites==SignalSitesShown&&Shown==SignalAspectsShown)return true;
 for(const auto& Part:SignalHisms)Part->ClearInstances();
 TArray<FTransform> Parts[9];
 // Site frame: X along the direction of travel, Y to the right; IR signals stand left of the line.
 // Head: four lenses, top to bottom Yellow, Green, Yellow, Red, facing the approaching driver.
 constexpr double PostY=-285,HeadZ=470,Pitch=27;
 const double LensZ[]={HeadZ+1.5*Pitch,HeadZ+.5*Pitch,HeadZ-.5*Pitch,HeadZ-1.5*Pitch};
 for(int32 I=0;I<Sites.Num();++I){
  const FTransform& P=Poses[I];auto Put=[&](int32 Family,FVector L,FVector Scale,FRotator R=FRotator::ZeroRotator){Parts[Family].Add(FTransform(R,L,Scale)*P);};
  Put(0,FVector(0,PostY,260),FVector(.15,.15,5.2));
  Put(1,FVector(0,PostY,HeadZ),FVector(.30,.40,1.24));
  for(double Z:LensZ)Put(2,FVector(-24,PostY,Z+10),FVector(.20,.30,.025));
  const auto Aspect=Rail::LinesideAspect(Shown[I]&7);
  const bool Lit[]={Aspect==Rail::LinesideAspect::Attention,Aspect==Rail::LinesideAspect::Proceed,
   Aspect==Rail::LinesideAspect::Caution||Aspect==Rail::LinesideAspect::Attention,Aspect==Rail::LinesideAspect::Stop};
  const int32 LitFamily[]={6,7,6,5};
  for(int32 L=0;L<4;++L)Put(Lit[L]?LitFamily[L]:8,FVector(-16,PostY,LensZ[L]),FVector(.05,.19,.19));
  if(Shown[I]&8){ // automatic stop signal: circular 'A' marker below the head
   Put(3,FVector(-14,PostY,HeadZ-105),FVector(.34,.34,.02),FRotator(90,0,0));
   for(double Side:{-1.,1.})Put(4,FVector(-16.5,PostY+Side*5,HeadZ-105),FVector(.012,.025,.22),FRotator(0,0,Side*-18));
   Put(4,FVector(-16.5,PostY,HeadZ-109),FVector(.012,.09,.022));
  }
 }
 for(int32 Family=0;Family<9;++Family)if(Parts[Family].Num())SignalHisms[Family]->AddInstances(Parts[Family],false,true);
 SignalSitesShown=Sites;SignalAspectsShown=Shown;return true;
}
void ARailTambaramCorridorView::Present(const Rail::OperationsWorld& World,int32 ChunkId){
 if(!Rail::ScenarioContent::IsTambaram(World.ContentId())){if(Cells.Num())Release();return;}
 if(ChunkId!=1&&ChunkId!=2){Diagnostic=TEXT("Invalid chunk ownership");bReady=false;return;}
 if(!LoadAssets()){bReady=false;return;}
 if(!PresentSignals(World,ChunkId)){Diagnostic=TEXT("Signal presentation unavailable");bReady=false;return;}
 const auto* Player=World.Spatial(Rail::TrainId(2));if(!Player){bReady=false;return;}
 const auto& Graph=Player->Network();
 if(OwnerChunk&&OwnerChunk!=ChunkId)Release();OwnerChunk=ChunkId;
 TSet<int32> Wanted,Essential;
 for(uint64 TrainId:{1ull,2ull})if(const auto* Train=World.Spatial(Rail::TrainId(TrainId))){
  const double Head=Train->Current().headStationMetres+(TrainId==1?Rail::TambaramRoute::TrafficStationOffset():0),Rear=Head-Train->Current().properties.lengthMetres;
  for(int32 Track=0;Track<4;++Track){
   const double Length=Rail::RouteLength(Graph,Rail::TambaramRoute::Path(Track));
   const int32 First=FMath::Max(0,FMath::FloorToInt((Rear-360)/CellMetres));
   const int32 Last=FMath::Min(FMath::CeilToInt(Length/CellMetres)-1,FMath::FloorToInt((Head+720)/CellMetres));
   for(int32 I=First;I<=Last;++I){
    const int32 Key=Track*100000+I;Wanted.Add(Key);
    if(I*CellMetres<=Head+360&&(I+1)*CellMetres>=Rear-160)Essential.Add(Key);
   }
  }
 }
 if(Wanted.Num()>MaxCells){Diagnostic=TEXT("Corridor cell bound exceeded; preserving previous presentation");bReady=false;return;}
 TArray<int32> Keys;Cells.GetKeys(Keys);for(int32 Key:Keys)if(!Wanted.Contains(Key))RemoveCell(Key);
 // Bound cold loads as well as travel prefetch. The caller keeps simulation
 // paused until every essential cell exists; no train runs into missing track.
 TArray<int32> Missing;for(int32 Key:Wanted)if(!Cells.Contains(Key))Missing.Add(Key);
 Missing.Sort([&](int32 A,int32 B){const bool EA=Essential.Contains(A),EB=Essential.Contains(B);return EA!=EB?EA: A<B;});
 int32 BuildBudget=2;
 for(int32 Key:Missing){
  if(BuildBudget--<=0)break;
  FCell Cell;BuildCell(Graph,Key/100000,Key%100000,ChunkId,Cell);Cells.Add(Key,MoveTemp(Cell));
 }
 ActiveCells=Cells.Num();InstanceCount=StoneInstances=0;
 for(const auto& Pair:Cells){InstanceCount+=Pair.Value.Instances;StoneInstances+=Pair.Value.Stones;}
 bReady=true;for(int32 Key:Essential)if(!Cells.Contains(Key)){bReady=false;break;}
 Diagnostic=bReady?TEXT("Ready: bounded graph-sampled corridor cells"):TEXT("Building essential corridor cells; service safely paused");
}
