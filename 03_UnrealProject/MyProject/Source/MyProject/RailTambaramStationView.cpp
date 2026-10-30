#include "RailTambaramStationView.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/RailScenarioContent.h"
#include "Simulation/RailRouteSampling.h"
#include "Simulation/RailTambaramRoute.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(RailStations,true);

ARailTambaramStationView::ARailTambaramStationView(){
 PrimaryActorTick.bCanEverTick=false;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}
bool ARailTambaramStationView::Present(const Rail::OperationsWorld& World,int32 ChunkId){
 CSV_SCOPED_TIMING_STAT(RailStations,PresentMs);
 if(!Rail::ScenarioContent::IsTambaram(World.ContentId())||(ChunkId!=1&&ChunkId!=2))return false;
 if(Building)return AdvanceBuild();
 const auto* Train=World.Spatial(Rail::TrainId(2));if(!Train)return false;
 const auto& Graph=Train->Network();const auto Path=Rail::TambaramRoute::Path(1);
 const Rail::PreparedRouteSampler Sampler(Graph,Path);
 // Only nearby station precincts exist. Hysteresis avoids rebuilding at a
 // streaming threshold; at most four stations per chunk are resident.
 TArray<TPair<double,int32>> Nearby;
 for(int32 I=0;I<static_cast<int32>(Rail::TambaramRoute::StationCount);++I){
  const double D=Rail::TambaramRoute::StationMetres[I];Rail::Sample Centre;
  if(!Sampler.SampleAt(D,Centre)||(ChunkId==1?Centre.position.x>=0:Centre.position.x<0))continue;
  double Distance=1.e30;
  for(uint64 Id:{1ull,2ull})if(const auto* T=World.Spatial(Rail::TrainId(Id))){
   const double Head=T->Current().headStationMetres+(Id==1?Rail::TambaramRoute::TrafficStationOffset():0),Rear=Head-T->Current().properties.lengthMetres;
   Distance=FMath::Min(Distance,FMath::Max(0.0,FMath::Max(Rear-D,D-Head)));
  }
  if(Distance<((BuiltStations&(1u<<I))?1500.0:1300.0))Nearby.Emplace(Distance,I);
 }
 Nearby.Sort([](const auto& A,const auto& B){return A.Key<B.Key;});
 uint32 Wanted=0;for(int32 I=0;I<FMath::Min(4,Nearby.Num());++I)Wanted|=1u<<Nearby[I].Value;
 if(Ready&&BuiltChunk==ChunkId&&BuiltStations==Wanted)return true;
 CSV_SCOPED_TIMING_STAT(RailStations,RebuildMs);
 CSV_CUSTOM_STAT(RailStations,Rebuilds,1,ECsvCustomStatOp::Accumulate);
 Ready=false;
 TMap<FString,FString> Meshes;TMap<FString,TArray<FTransform>> Instances;
 auto Fail=[this](const TCHAR* Reason){Diagnostic=Reason;return false;};
 // Parse paths now; load only the next bounded group during AdvanceBuild.
 TArray<TSharedPtr<FJsonObject>> Manifests;
 for(const TCHAR* Stem:{TEXT("EgmoreScene02"),TEXT("EgmoreInterior01")}){
  FString Text;TSharedPtr<FJsonObject> Doc;
  if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("IndianRailway/Production/stations")/(FString(Stem)+TEXT(".json"))))||
     !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Doc)||!Doc.IsValid())return Fail(TEXT("Station source manifest missing"));
  const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
  if(!Doc->TryGetArrayField(TEXT("importedModules"),Rows))return Fail(TEXT("Station modules absent"));
  for(const auto& Value:*Rows){const auto& Row=Value->AsObject();if(!Row.IsValid())return Fail(TEXT("Malformed station module"));
   const FString Name=Row->GetStringField(TEXT("module")),Asset=Row->GetStringField(TEXT("asset"));
   if(!Asset.StartsWith(TEXT("/Game/IndianRailway/S8/Stations/")))return Fail(TEXT("Station asset outside approved family"));
   Meshes.Add(Name,Asset);
  }Manifests.Add(Doc);
 }
 auto Pose=[&](double Station,double Lateral,double Z,double Yaw,FVector Scale,FTransform& Out){
  Rail::Sample Sample;if(!Sampler.SampleAt(Station,Sample))return false;
  const FVector Forward(Sample.tangent.x,Sample.tangent.y,Sample.tangent.z),Right(-Forward.Y,Forward.X,0);
  const FVector Position=FVector(Sample.position.x,Sample.position.y,Sample.position.z)+Right*Lateral+FVector(0,0,Z);
  Out=FTransform(FRotator(0,Forward.Rotation().Yaw+Yaw,0),Position*100,Scale);return true;
 };
 auto Add=[&](const FString& Name,double Station,double Y,double Z=0,double Yaw=0,FVector Scale=FVector::OneVector){
  FTransform Transform;if(!Meshes.Contains(Name)||!Pose(Station,Y,Z,Yaw,Scale,Transform))return false;
  Instances.FindOrAdd(Name).Add(Transform);return true;
 };
 if(Wanted&1u){
  // The original player start (-120m) corresponds to station200m. Preserve
  // local platform clearances; route track/OHE are provided by corridor view.
  const TSet<FString> CorridorOwned={TEXT("Track6m"),TEXT("Ballast6m"),TEXT("BallastScatter6m"),TEXT("Wire18m"),TEXT("OHEPortal"),TEXT("SignalUnbound"),TEXT("UrbanA"),TEXT("UrbanB")};
  for(const auto& Doc:Manifests){const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
   if(!Doc->TryGetArrayField(TEXT("placements"),Rows))return Fail(TEXT("Station placements absent"));
   for(const auto& Value:*Rows){const auto& Row=Value->AsObject();if(!Row.IsValid())return Fail(TEXT("Malformed station placement"));
    const FString Name=Row->GetStringField(TEXT("module"));if(CorridorOwned.Contains(Name))continue;
    const auto& P=Row->GetArrayField(TEXT("positionM"));const auto& S=Row->GetArrayField(TEXT("scale"));
    if(P.Num()!=3||S.Num()!=3)return Fail(TEXT("Malformed station transform"));
    if(!Add(Name,P[0]->AsNumber()+320,P[1]->AsNumber(),P[2]->AsNumber(),Row->GetNumberField(TEXT("rotationZDeg")),FVector(S[0]->AsNumber(),S[1]->AsNumber(),S[2]->AsNumber())))return Fail(TEXT("Origin placement outside authoritative path"));
   }
  }
 }
 TArray<FBuildGroup> Boards;
 auto Board=[&](int32 Index,double S,double Y,double Width){
  FBuildGroup Group;Group.Name=TEXT("Nameboard");
  Group.MeshPath=TEXT("/Engine/BasicShapes/Plane.Plane");
  Group.MaterialPath=FString::Printf(TEXT("/Game/IndianRailway/S8/Chennai/Nameboards/M_Nameboard_%02d.M_Nameboard_%02d"),Index,Index);
  for(double Side:{-1.,1.}){FTransform T;if(!Pose(S,Y+Side*.095,2.4,0,FVector::OneVector,T))continue;
   const FQuat Face=FQuat(FVector::UpVector,Side>0?PI:0)*FQuat(FVector::ForwardVector,PI*.5)*FQuat(FVector::UpVector,PI);
   T.SetRotation(T.GetRotation()*Face);T.SetScale3D(FVector(Width,Width*.25,1));Group.Transforms.Add(T);
  }
  Boards.Add(MoveTemp(Group));
 };
 if(Wanted&(1u<<14)){
  const double From=Rail::TambaramRoute::TambaramPlatformFrom,To=Rail::TambaramRoute::TambaramPlatformTo;
  // Three visible islands bound the four simulation tracks. This represents
  // the playable main-line group, not a fabricated ten-platform topology.
  for(double Y:{-9.675,6.675,23.025}){
   for(double S=From;S<To;S+=6){
    if(!Add(TEXT("Platform6x8m"),S,Y,0,0,FVector(FMath::Min(6.0,To-S)/6,1,1)))return Fail(TEXT("Destination platform outside route"));
    Add(TEXT("EgmoreDrain3m"),S,Y+.2,.845);
    if(S>From+36&&S<To-36)Add(TEXT("EgmoreCanopy6m"),S,Y+.56,.84,0,FVector(1,.86,1));
   }
   for(double S=From+24;S<To-12;S+=36){
    Add(TEXT("EgmorePlatformBench"),S,Y+4,.84,90);
    Add(TEXT("EgmoreLitterBins"),S+2,Y+4,.84);
    Add(TEXT("YardLamp"),S+12,Y+4,.84);
   }
   Add(TEXT("EgmoreWaterPoint"),From+70,Y+5,.84);
   // Stairs meet reused deck at rail+8.4m; no support is placed in a track.
   Add(TEXT("BridgeStairs"),From+140,Y+4,.84);
   Add(TEXT("BridgePier"),From+154,Y+3,.84);
  }
  // Authored entry modules remove the western guardrail at each stair landing.
  for(double Y=-12;Y<=36;Y+=6)Add(Y==-6?TEXT("BridgeDeckEntry-6"):Y==12?TEXT("BridgeDeckEntry12"):Y==30?TEXT("BridgeDeckEntry30"):TEXT("BridgeDeck6m"),From+154,Y,7.56);
  // Modest long, low booking hall/forecourt. No speculative grand redevelopment.
  const FString Cube=TEXT("/Engine/BasicShapes/Cube.Cube");
  for(const TCHAR* Name:{TEXT("TambaramConcourse"),TEXT("TambaramRoof"),TEXT("TambaramGlazing"),TEXT("TambaramTrim"),TEXT("TambaramSign")})Meshes.Add(Name,Cube);
  Add(TEXT("TambaramConcourse"),From+170,-23,2.2,0,FVector(54,12,4.4));
  Add(TEXT("TambaramRoof"),From+170,-23,4.55,0,FVector(57,14,.3));
  Add(TEXT("TambaramTrim"),From+170,-16.8,1,0,FVector(54,.3,.8));
  for(double S=From+147;S<From+195;S+=6){
   Add(TEXT("TambaramGlazing"),S,-16.94,2.6,0,FVector(3.8,.2,1.5));
   Add(TEXT("TambaramConcourse"),S+2.4,-15.9,2.1,0,FVector(.35,.35,4.2));
  }
  Add(TEXT("TambaramGlazing"),From+170,-16.75,1.55,0,FVector(5,.3,3.1));
  for(double S:{From+20,To-20})for(double Y:{-5.8,10.6,27.0}){
   Add(TEXT("TambaramSign"),S,Y,2.4,0,FVector(5.2,.16,1.3));
   for(double X:{-2.1,2.1})Add(TEXT("TambaramTrim"),S+X,Y,1.2,0,FVector(.1,.1,2.4));
  }
  for(double S=From+147;S<From+195;S+=6){Add(TEXT("Forecourt6m"),S,-14,0);Add(TEXT("Cabinet"),S,-16,.16);}
  for(double S=From;S<To;S+=12){Add(TEXT("RoadTile12m"),S,-35,0);Add(TEXT("Boundary6m"),S,35,0);}
  for(double S=From+12;S<To;S+=48)Add(TEXT("TreeBroadleaf"),S,39,-.5);
 }
 // Intermediate stations: representative outer platforms beside the suburban
 // pair, leaving the player/main-line corridor unobstructed. No platform number
 // is claimed from OSM; every six-metre piece follows the authoritative tangent.
 const FString StationCube=TEXT("/Engine/BasicShapes/Cube.Cube");
 for(const TCHAR* Name:{TEXT("TambaramSign"),TEXT("TambaramTrim"),TEXT("TambaramConcourse"),TEXT("TambaramRoof"),TEXT("TambaramGlazing")})Meshes.Add(Name,StationCube);
 if(Wanted&1u)for(double S:{Rail::TambaramRoute::StationMetres[0]-60,Rail::TambaramRoute::StationMetres[0]+60}){
  Add(TEXT("TambaramSign"),S,-5.8,2.4,0,FVector(5.2,.16,1.3));
  for(double X:{-2.1,2.1})Add(TEXT("TambaramTrim"),S+X,-5.8,1.2,0,FVector(.1,.1,2.4));
 }
 for(int32 I=1;I<14;++I)if(Wanted&(1u<<I)){
  const double Centre=Rail::TambaramRoute::StationMetres[I],From=Centre-144,To=Centre+144;
  for(double Y:{6.675,23.025}){
   for(double S=From;S<To;S+=6){
    if(!Add(TEXT("Platform6x8m"),S,Y))return Fail(TEXT("Intermediate platform sample failed"));
    Add(TEXT("EgmoreDrain3m"),S,Y+.25,.845);
    // Alternating open and sheltered lengths retain views of the neighbourhood.
    if(FMath::Abs(S-Centre)<72+(I%3)*12)Add(TEXT("EgmoreCanopy6m"),S,Y+.56,.84,0,FVector(1,.86,1));
   }
   for(double S=From+18;S<To;S+=36){Add(TEXT("EgmorePlatformBench"),S,Y+4,.84,90);Add(TEXT("YardLamp"),S+9,Y+4,.84);}
   Add(TEXT("EgmoreWaterPoint"),Centre+54,Y+5,.84);Add(TEXT("EgmoreLitterBins"),Centre+60,Y+4,.84);
   Add(TEXT("BridgeStairs"),Centre-30,Y+4,.84);Add(TEXT("BridgePier"),Centre-16,Y+3,.84);
   for(double S:{From+18,To-18}){
    Add(TEXT("TambaramSign"),S,Y+4,2.4,0,FVector(7,.16,1.75));
    for(double X:{-3.0,3.0})Add(TEXT("TambaramTrim"),S+X,Y+4,1.2,0,FVector(.1,.1,2.4));
   }
  }
  for(double Y=6;Y<=36;Y+=6)Add(Y==12?TEXT("BridgeDeckEntry12"):Y==30?TEXT("BridgeDeckEntry30"):TEXT("BridgeDeck6m"),Centre-16,Y,7.56);
  Add(TEXT("TambaramConcourse"),Centre+12,38,1.6,0,FVector(12+(I%3)*3,6,3.2));
  Add(TEXT("TambaramRoof"),Centre+12,38,3.3,0,FVector(14+(I%3)*3,7,.24));
  for(double X:{-3.0,1.0,5.0})Add(TEXT("TambaramGlazing"),Centre+12+X,34.94,1.8,0,FVector(1.7,.15,1.3));
  for(double S=From;S<To;S+=12){Add(TEXT("RoadTile12m"),S,47);Add(TEXT("Boundary6m"),S,33);}
  for(double S=From+12;S<To;S+=48)Add(TEXT("TreeBroadleaf"),S,55,-.5,FMath::Fmod(S,360.0),FVector(.8+(I%3)*.1));
 }
 // Queue components in stable order. Asset loading and registration happen
 // over subsequent frames, while the caller holds simulation readiness false.
 PendingGroups.Reset();
 TArray<FString> Names;Instances.GetKeys(Names);Names.Sort();
 for(const auto& Name:Names){
  FBuildGroup Group;Group.Name=Name;Group.MeshPath=Meshes[Name];
  Group.Transforms=MoveTemp(Instances[Name]);
  if(Name.StartsWith(TEXT("Tambaram"))){
   const TCHAR* Material=Name==TEXT("TambaramGlazing")?TEXT("Glass"):Name==TEXT("TambaramSign")?TEXT("Yellow"):Name==TEXT("TambaramRoof")?TEXT("Roof"):Name==TEXT("TambaramTrim")?TEXT("RedOxide"):TEXT("Cream");
   Group.MaterialPath=FString::Printf(TEXT("/Game/IndianRailway/S8/Stations/Kit01/Materials/%s.%s"),Material,Material);
  }
  PendingGroups.Add(MoveTemp(Group));
 }
 if(Wanted&1u)for(double S:{Rail::TambaramRoute::StationMetres[0]-60,Rail::TambaramRoute::StationMetres[0]+60})Board(0,S,-5.8,5);
 if(Wanted&(1u<<14)){
  for(double S:{Rail::TambaramRoute::TambaramPlatformFrom+20,Rail::TambaramRoute::TambaramPlatformTo-20})for(double Y:{-5.8,10.6,27.0}){
   Board(14,S,Y,5);
  }
 }
 for(int32 I=1;I<14;++I)if(Wanted&(1u<<I))for(double S:{Rail::TambaramRoute::StationMetres[I]-126,Rail::TambaramRoute::StationMetres[I]+126})for(double Y:{10.675,27.025})Board(I,S,Y,6.8);
 PendingGroups.Append(MoveTemp(Boards));
 RetiringParts=MoveTemp(Parts);Parts.Reset();
 PendingGroup=PendingOffset=0;PendingChunk=ChunkId;PendingStations=Wanted;
 Placements=0;Building=true;Diagnostic=TEXT("Building station precincts");
 // Planning is its own frame; no synchronous mesh loads on this path.
 return false;
}
bool ARailTambaramStationView::AdvanceBuild(){
 CSV_SCOPED_TIMING_STAT(RailStations,BuildBatchMs);
 // Retire at most two components, separately from the construction budget.
 if(!RetiringParts.IsEmpty()){
  for(int32 I=0;I<2&&!RetiringParts.IsEmpty();++I){
   auto Part=RetiringParts.Pop();
   if(Part){RemoveInstanceComponent(Part);Part->DestroyComponent();}
   CSV_CUSTOM_STAT(RailStations,ComponentsRetired,1,ECsvCustomStatOp::Accumulate);
  }
  return false;
 }
 int32 Components=0,InstancesLeft=128;
 while(PendingGroup<PendingGroups.Num()&&Components<2&&InstancesLeft>0){
  const auto& Group=PendingGroups[PendingGroup];
  UHierarchicalInstancedStaticMeshComponent* Part=nullptr;
  if(PendingOffset==0){
   auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Group.MeshPath);
   auto* Material=Group.MaterialPath.IsEmpty()?nullptr:LoadObject<UMaterialInterface>(nullptr,*Group.MaterialPath);
   if(!Mesh||(!Group.MaterialPath.IsEmpty()&&!Material)){
    Diagnostic=TEXT("Station asset missing; waiting for essential presentation");return false;
   }
   Part=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
   AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);
   Part->SetStaticMesh(Mesh);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);
   if(Material)Part->SetMaterial(0,Material);
   if(Group.Name==TEXT("TreeBroadleaf"))if(auto* Tint=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Chennai/M_SceneryTint.M_SceneryTint"))){
    const auto& Slots=Mesh->GetStaticMaterials();
    for(int32 I=0;I<Slots.Num();++I){
     const FString Name=Slots[I].MaterialSlotName.ToString();
     auto* Colour=UMaterialInstanceDynamic::Create(Tint,this);
     Colour->SetVectorParameterValue(TEXT("Color"),Name.Contains(TEXT("Bark"))?FLinearColor(.12f,.075f,.035f):Name.Contains(TEXT("Light"))?FLinearColor(.16f,.27f,.045f):FLinearColor(.075f,.19f,.035f));
     Part->SetMaterial(I,Colour);
    }
   }
   Part->SetCullDistances(0,Group.Name==TEXT("Nameboard")?90000:180000);Part->SetCastShadow(true);
   Part->RegisterComponent();Parts.Add(Part);++Components;
   CSV_CUSTOM_STAT(RailStations,ComponentsBuilt,1,ECsvCustomStatOp::Accumulate);
  }else Part=Parts.Last();
  const int32 Count=FMath::Min(InstancesLeft,Group.Transforms.Num()-PendingOffset);
  TArray<FTransform> Batch;Batch.Append(Group.Transforms.GetData()+PendingOffset,Count);
  Part->AddInstances(Batch,false,true,false);
  PendingOffset+=Count;Placements+=Count;InstancesLeft-=Count;
  CSV_CUSTOM_STAT(RailStations,InstancesBuilt,Count,ECsvCustomStatOp::Accumulate);
  if(PendingOffset==Group.Transforms.Num()){++PendingGroup;PendingOffset=0;}
 }
 if(PendingGroup<PendingGroups.Num())return false;
 PendingGroups.Reset();PendingGroup=PendingOffset=0;Building=false;
 BuiltChunk=PendingChunk;BuiltStations=PendingStations;++RebuildCount;Ready=true;
 Diagnostic=TEXT("Bounded graph-bound station precincts; representative platform layouts");
 return true;
}
FString ARailTambaramStationView::TelemetryJson() const {
 return FString::Printf(TEXT("{\"ready\":%s,\"chunk\":%d,\"instances\":%d,\"moduleGroups\":%d,\"labels\":%d,\"stationMask\":%u,\"rebuilds\":%d,\"building\":%s,\"pendingGroups\":%d,\"retiringGroups\":%d,\"surveyReplica\":false}"),Ready?TEXT("true"):TEXT("false"),BuiltChunk,Placements,Parts.Num(),Labels.Num(),BuiltStations,RebuildCount,Building?TEXT("true"):TEXT("false"),PendingGroups.Num()-PendingGroup,RetiringParts.Num());
}
