#include "RailServiceView.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "DrawDebugHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Simulation/RailScenarioContent.h"
#include "Simulation/RailRouteSampling.h"
ARailServiceView::ARailServiceView(){
 PrimaryActorTick.bCanEverTick=false;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Platform=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Platform"));Platform->SetupAttachment(RootComponent);
 Cab=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Cab"));Cab->SetupAttachment(RootComponent);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UMaterialInterface> Material(TEXT("/Game/IndianRailway/S2/M_S2_Bogie.M_S2_Bogie"));
 for(auto* Component:{Platform.Get(),Cab.Get()}){Component->SetStaticMesh(Cube.Object);Component->SetMaterial(0,Material.Object);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);}
}
void ARailServiceView::Configure(const Rail::ServiceDefinition& D){
 Platform->ClearInstances();if(Rail::ScenarioContent::IsS8Route(D.contentId)||FParse::Param(FCommandLine::Get(),TEXT("S8Station")))return;const double Centre=(D.platformFrom+D.platformTo)*50;
 Platform->AddInstance(FTransform(FRotator::ZeroRotator,FVector(Centre,-650,50),FVector(D.platformTo-D.platformFrom,5,1)));
 Platform->AddInstance(FTransform(FRotator::ZeroRotator,FVector(Centre,-900,800),FVector(22,3,.2)));
 for(double X:{Centre-950,Centre+950})Platform->AddInstance(FTransform(FRotator::ZeroRotator,FVector(X,-900,425),FVector(.2,.2,7.5)));
}
void ARailServiceView::Present(const Rail::ServiceDefinition& D,const Rail::OperationsWorld& World,bool HideProxyCab){
 Cab->SetVisibility(!HideProxyCab);
 if(Rail::ScenarioContent::IsTambaram(D.contentId)){
  // Station meshes belong to streamed presentation, not this persistent HUD view.
  // Optional debug marker samples the actual route rather than treating metres as X.
  if(FParse::Param(FCommandLine::Get(),TEXT("S8RouteDebug"))){
   const auto* Train=World.Spatial(D.player);Rail::Sample Berth;
   if(Train&&Rail::SampleRoute(Train->Network(),Rail::TambaramRoute::Path(1),D.berthHead,Berth).ok){
    const FVector Position(Berth.position.x*100,Berth.position.y*100,Berth.position.z*100+40);
    const FQuat Rotation=FRotationMatrix::MakeFromX(FVector(Berth.tangent.x,Berth.tangent.y,Berth.tangent.z)).ToQuat();
    DrawDebugBox(GetWorld(),Position,FVector(D.berthTolerance*100,170,40),Rotation,FColor::Green,false,0,0,8);
    DrawDebugString(GetWorld(),Position+FVector(0,0,500),TEXT("TAMBARAM BERTH"),nullptr,FColor::Green,0,true);
   }
  }
  return;
 }
 if(Rail::ScenarioContent::IsEgmore(D.contentId)||FParse::Param(FCommandLine::Get(),TEXT("S8Station")))return;
 DrawDebugString(GetWorld(),FVector((D.platformFrom+D.platformTo)*50,-1000,1000),UTF8_TO_TCHAR(D.station.c_str()),nullptr,FColor::White,0,true);
 DrawDebugBox(GetWorld(),FVector(D.berthHead*100,0,40),FVector(D.berthTolerance*100,300,40),FColor::Green,false,0,0,15);
 DrawDebugString(GetWorld(),FVector(D.berthHead*100,500,500),TEXT("BERTH"),nullptr,FColor::Green,0,true);
 DrawDebugString(GetWorld(),FVector(D.finishHead*100,-800,500),TEXT("FINISH"),nullptr,FColor::White,0,true);
 const auto* Train=World.Spatial(D.player);if(!Train||Train->Current().vehicles.empty())return;
 const auto& V=Train->Current().vehicles.front();const auto Rotation=FRotationMatrix::MakeFromXZ(FVector(V.bodyForward.x,V.bodyForward.y,V.bodyForward.z),FVector::UpVector).ToQuat();
 const FVector Location(V.bodyCentre.x*100,V.bodyCentre.y*100,V.bodyCentre.z*100+430);
 const FTransform Transform(Rotation,Location+Rotation.RotateVector(FVector(650,0,0)),FVector(4,2.7,2));
 if(Cab->GetInstanceCount()==0)Cab->AddInstance(Transform,true);else Cab->UpdateInstanceTransform(0,Transform,true,true,true);
}
