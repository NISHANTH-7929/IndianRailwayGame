#include "RailChunkView.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

// Optional presentation only. Chunk actor owns all components and hard refs.
// Placement samples the existing graph; no collision or railway state is authored.
bool ARailChunkView::LoadStation(const Rail::Graph& Graph){
 if(!FParse::Param(FCommandLine::Get(),TEXT("S8Station")))return true;
 if(!StationParts.IsEmpty())return true;
 FString Text;TSharedPtr<FJsonObject> Doc;const TArray<TSharedPtr<FJsonValue>>* Parts=nullptr;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("IndianRailway/Production/stations/Kit01.json")))||
 !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Doc)||!Doc||!Doc->TryGetArrayField(TEXT("parts"),Parts))return false;
 TMap<FString,UStaticMesh*> Meshes;
 for(const auto& Value:*Parts){const auto Row=Value->AsObject();FString Name,Path;
  if(!Row||!Row->TryGetStringField(TEXT("name"),Name)||!Row->TryGetStringField(TEXT("asset"),Path)||!Path.StartsWith(TEXT("/Game/IndianRailway/S8/Stations/Kit01/Meshes/")))return false;
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);if(!Mesh||Meshes.Contains(Name))return false;Meshes.Add(Name,Mesh);
 }
 TMap<FString,TArray<FTransform>> Instances;
 auto Add=[&](const FString& Name,double X,double Y,double Z,double Yaw=0.0,FVector Scale=FVector::OneVector,double Pitch=0.0){
  if((ChunkId==1)!=(X<0))return;
  Instances.FindOrAdd(Name).Add(FTransform(FRotator(Pitch,Yaw,0),FVector(X*100,Y*100,Z*100),Scale));
 };
 // Rail heads sit at z=0, the logical rail plane. Chord segments keep curvature.
 for(const auto& Edge:Graph.edges){
  const int32 Count=FMath::CeilToInt(Edge.lengthMetres);
  for(int32 I=0;I<Count;++I){const double A=Edge.lengthMetres*I/Count,B=Edge.lengthMetres*(I+1)/Count;
   const auto P=Edge.geometry.At(A).position,Q=Edge.geometry.At(B).position;const auto D=Q-P;
   const double Yaw=FMath::RadiansToDegrees(FMath::Atan2(D.y,D.x));
   const double Pitch=FMath::RadiansToDegrees(FMath::Atan2(D.z,FMath::Sqrt(D.x*D.x+D.y*D.y)));
   Add(TEXT("Rail3m"),P.x,P.y,P.z-.1825,Yaw,FVector(D.Length()/3,1,1),Pitch);
   Add(TEXT("Ballast3m"),P.x,P.y,P.z-.18,Yaw,FVector(D.Length()/3,1,1),Pitch);
   Add(TEXT("ContactWire3m"),P.x,P.y,P.z+5.5,Yaw,FVector(D.Length()/3,1,1),Pitch);
  }
  for(double S=0;S<Edge.lengthMetres;S+=.60){const auto P=Edge.geometry.At(S);Add(TEXT("Sleeper"),P.position.x,P.position.y,P.position.z-.18,FMath::RadiansToDegrees(FMath::Atan2(P.tangent.y,P.tangent.x)));}
 }
 // Exact S7 platform extent 95..145; 0.84 m surface, 1.675 m coping offset.
 for(int32 I=0;I<10;++I)Add(TEXT("Platform5m"),95+I*5,-6.675,0);
 for(int32 I=0;I<10;++I)for(double Y:{-11.675,-16.675})Add(TEXT("Apron5m"),95+I*5,Y,0);
 Add(TEXT("BridgePier"),132.76,-8,.4);Add(TEXT("BridgePier"),132.76,5,.4);
 const bool VariantB=FParse::Param(FCommandLine::Get(),TEXT("S8StationB"));
 for(int32 I=0;I<6;++I)Add(TEXT("Canopy6m"),101+I*6,-6.675,.84);
 for(double X:{102.,112.,125.,137.})Add(TEXT("Bench"),X,-6.2,.85);
 for(double X:{98.,142.}){Add(TEXT("StationBoard"),X,-6.4,.85);Add(TEXT("LampPost"),X,-7.0,.85);}
 Add(TEXT("VendorKiosk"),VariantB?106:132,-11.0,.85);
 Add(TEXT("StationFacade6m"),VariantB?130:111,-13,.84,180);
 Add(TEXT("WaitingShelter"),VariantB?137:100,-11,.85);
 for(int32 I=0;I<16;++I)Add(TEXT("Boundary3m"),95+I*3,-17,0);
 for(int32 I=0;I<4;++I)Add(TEXT("Railing3m"),98+I*3,-7.6,.85);
 // Two flights rise to 7.89 m, clear of the complete train envelope.
 Add(TEXT("StairFlight"),120,-11,.85);Add(TEXT("StairFlight"),126.38,-11,4.37);
 for(int32 I=0;I<3;++I)Add(TEXT("Footbridge6m"),132.76,-11+I*6,7.89,90);
 Add(TEXT("StairFlight"),139.14,7,4.37,180);Add(TEXT("StairFlight"),145.52,7,.85,180);
 // Fixture presentation anchors use edge arc distance, like the contact wire.
 // Keep masts vertical; grade affects the foundation height, not mast pitch.
 // Edge IDs belong to this optional fixture layout, never inferred from actors.
 const Rail::TrackPosition MastAnchors[]={
  {Rail::EdgeId(1),40,1},{Rail::EdgeId(1),90,1},{Rail::EdgeId(1),140,1},{Rail::EdgeId(1),190,1},
  {Rail::EdgeId(2),0,1},{Rail::EdgeId(2),50,1},{Rail::EdgeId(2),90,1},{Rail::EdgeId(2),150,1},
  {Rail::EdgeId(7),40,1},{Rail::EdgeId(7),90,1},{Rail::EdgeId(7),140,1},{Rail::EdgeId(7),190,1}};
 for(const auto& Anchor:MastAnchors){
  Rail::Sample Sample;if(!Graph.SamplePosition(Anchor,Sample).ok)return false;
  const double Yaw=FMath::Atan2(Sample.tangent.y,Sample.tangent.x);
  Add(TEXT("OHEMast"),Sample.position.x+3.3*FMath::Sin(Yaw),Sample.position.y-3.3*FMath::Cos(Yaw),Sample.position.z,FMath::RadiansToDegrees(Yaw));
 }
 Add(TEXT("OHEPortal12m"),80,-4,0,90);
 Add(TEXT("ElectricalCabinet"),88,-4,0);Add(TEXT("ElectricalCabinet"),148,-4,0);
 Add(TEXT("WhistleBoard"),60,-3,0,90);Add(TEXT("TerminationBoard"),170,-3,0,90);
 // Non-operational sign samples remain in source/review, not false track rules.
 for(int32 X=-5;X<8;++X)for(int32 Y=-1;Y<3;++Y)Add(TEXT("Ground50m"),X*50,Y*50,X>=4?(X*50-160)*.02:0,0,FVector::OneVector,X>=4?FMath::RadiansToDegrees(FMath::Atan(.02)):0);
 for(const auto& Pair:Instances)if(!Meshes.Contains(Pair.Key))return false;
 for(const auto& Pair:Instances){auto* Component=NewObject<UInstancedStaticMeshComponent>(this);
  AddInstanceComponent(Component);Component->SetupAttachment(RootComponent);Component->SetStaticMesh(Meshes[Pair.Key]);
  Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetGenerateOverlapEvents(false);
  Component->RegisterComponent();Component->AddInstances(Pair.Value,false);StationParts.Add(Component);
 }
 UE_LOG(LogTemp,Display,TEXT("S8_STATION_READY chunk=%d components=%d variant=%s"),ChunkId,StationParts.Num(),VariantB?TEXT("B"):TEXT("A"));return true;
}
