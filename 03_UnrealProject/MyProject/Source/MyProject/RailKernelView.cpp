#include "RailKernelView.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "UObject/ConstructorHelpers.h"
namespace { FVector Cm(Rail::Vec3 V){return FVector(V.x*100,V.y*100,V.z*100);} }
ARailKernelView::ARailKernelView(){
 PrimaryActorTick.bCanEverTick=false;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Bodies=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bodies"));Bodies->SetupAttachment(RootComponent);
 Bogies=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bogies"));Bogies->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Bodies->SetStaticMesh(Cube.Object);Bogies->SetStaticMesh(Cube.Object);
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> BodyMaterial(TEXT("/Game/IndianRailway/S2/M_S2_Body.M_S2_Body"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> BogieMaterial(TEXT("/Game/IndianRailway/S2/M_S2_Bogie.M_S2_Bogie"));
 Bodies->SetMaterial(0,BodyMaterial.Object);Bogies->SetMaterial(0,BogieMaterial.Object);
 Bodies->SetCollisionEnabled(ECollisionEnabled::NoCollision);Bogies->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));Camera->SetupAttachment(RootComponent);
 Camera->SetRelativeLocation(FVector(6500,-32000,37000));Camera->SetRelativeRotation((FVector(6500,3000,0)-Camera->GetRelativeLocation()).Rotation());Camera->FieldOfView=65;
}
void ARailKernelView::Present(const Rail::Graph& Graph,const Rail::PathHistory& History,const Rail::Snapshot& S,const FString& Status){
 auto* W=GetWorld();
 for(const auto& E:Graph.edges){
  bool Active=false;for(const auto& O:History.occurrences)Active|=O.directed.edge==E.id;
  const FColor Color=Active?FColor::Cyan:FColor(80,80,100);
  const int Steps=FMath::CeilToInt(E.lengthMetres/4);FVector Prev=Cm(E.geometry.At(0).position);
  for(int i=1;i<=Steps;++i){const FVector Next=Cm(E.geometry.At(E.lengthMetres*i/Steps).position);DrawDebugLine(W,Prev,Next,Color,false,0,0,20);Prev=Next;}
  auto Mid=E.geometry.At(E.lengthMetres*.5);DrawDebugDirectionalArrow(W,Cm(Mid.position),Cm(Mid.position+Mid.tangent*12),250,FColor::Green,false,0,0,30);
  DrawDebugString(W,Cm(Mid.position)+FVector(0,0,250),FString::Printf(TEXT("E%llu  %.1fm"),E.id.value,E.lengthMetres),nullptr,FColor::White,0,true,1.2f);
 }
 for(const auto& N:Graph.nodes)DrawDebugSphere(W,Cm(N.position),180,8,FColor::Yellow,false,0,0,20);
 const int Count=int(S.vehicles.size());
 if(Bodies->GetInstanceCount()!=Count){Bodies->ClearInstances();Bogies->ClearInstances();for(int i=0;i<Count;++i){Bodies->AddInstance(FTransform::Identity);Bogies->AddInstance(FTransform::Identity);Bogies->AddInstance(FTransform::Identity);}}
 FString Vehicles;
 for(int i=0;i<Count;++i){const auto& V=S.vehicles[size_t(i)];const FVector Forward(V.bodyForward.x,V.bodyForward.y,V.bodyForward.z);const FQuat Rotation=FRotationMatrix::MakeFromXZ(Forward,FVector::UpVector).ToQuat();
  const double Length=V.lengthMetres;
  Bodies->UpdateInstanceTransform(i,FTransform(Rotation,Cm(V.bodyCentre)+FVector(0,0,220),FVector(Length,3,2.5)),true,false,true);
  Bogies->UpdateInstanceTransform(i*2,FTransform(Rotation,Cm(V.frontBogie)+FVector(0,0,70),FVector(2,3.3,.8)),true,false,true);
  Bogies->UpdateInstanceTransform(i*2+1,FTransform(Rotation,Cm(V.rearBogie)+FVector(0,0,70),FVector(2,3.3,.8)),true,false,true);
  DrawDebugLine(W,Cm(V.frontReference)+FVector(0,0,150),Cm(V.rearReference)+FVector(0,0,150),FColor::Orange,false,0,0,15);
  DrawDebugString(W,Cm(V.bodyCentre)+FVector(0,0,550),FString::Printf(TEXT("V%llu"),V.id.value),nullptr,FColor::Yellow,0,true,1.3f);
  Vehicles+=FString::Printf(TEXT("V%llu:E%llu %.1fm d%+d | "),V.id.value,V.trackCentre.edge.value,V.trackCentre.distanceMetres,V.trackCentre.direction);
 }
 Bodies->MarkRenderStateDirty();Bogies->MarkRenderStateDirty();
 FString Path;for(const auto& O:History.occurrences)Path+=FString::Printf(TEXT(" E%llu(%+d)/T%llu"),O.directed.edge.value,O.directed.direction,O.transitionFromPrevious.value);
 if(GEngine&&!(Status.StartsWith(TEXT("S4"))||Status.StartsWith(TEXT("S5")))){GEngine->AddOnScreenDebugMessage(2201,0,FColor::White,FString::Printf(TEXT("%s | Train %llu | tick %llu | head E%llu s %.2fm dir %+d | speed %.1fm/s\nFormation: %llu vehicles / %.0fkg / %.1fm / %llu powered\n%s\nHistory:%s\n%s"),Status.StartsWith(TEXT("S3"))?TEXT("S3 HANDLING"):TEXT("S2 KERNEL"),S.train.value,S.tick,S.head.edge.value,S.head.distanceMetres,S.head.direction,S.commandedSpeedMps,uint64(S.properties.vehicleCount),S.properties.massKg,S.properties.lengthMetres,uint64(S.properties.poweredCount),*Vehicles,*Path,*Status),false,FVector2D(1.1,1.1));}
}



