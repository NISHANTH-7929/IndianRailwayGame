#include "RailEgmoreSceneView.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "EngineUtils.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Simulation/RailEgmoreLayout.h"
#include "Simulation/RailScenarioContent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace {
bool ReadVector(const FJsonObject& Object,const TCHAR* Key,FVector& Out) {
 const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;
 if(!Object.TryGetArrayField(Key,Values)||Values->Num()!=3)return false;
 double V[3];for(int32 I=0;I<3;++I)if(!(*Values)[I]->TryGetNumber(V[I])||!FMath::IsFinite(V[I]))return false;
 Out=FVector(V[0],V[1],V[2]);return true;
}
}
ARailEgmoreSceneView::ARailEgmoreSceneView() {
 PrimaryActorTick.bCanEverTick=true;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 RootComponent->SetMobility(EComponentMobility::Static);
}
void ARailEgmoreSceneView::OnConstruction(const FTransform& Transform) {
 Super::OnConstruction(Transform);RebuildScene();
}
void ARailEgmoreSceneView::BeginPlay() {
 Super::BeginPlay();if(!RebuildScene())return;
 if(GetWorld()->GetMapName().Contains(TEXT("L_EgmoreScene02_Study"))){
  ReviewCamera=GetWorld()->SpawnActor<ACameraActor>();
  auto& Exposure=ReviewCamera->GetCameraComponent()->PostProcessSettings;
  Exposure.bOverride_AutoExposureMethod=true;Exposure.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
  Exposure.bOverride_AutoExposureBias=true;Exposure.AutoExposureBias=.5f;
  Exposure.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Exposure.AutoExposureApplyPhysicalCameraExposure=false;
  ReviewCamera->GetCameraComponent()->PostProcessBlendWeight=1;
  ReviewCamera->GetCameraComponent()->bConstrainAspectRatio=false;
  FParse::Value(FCommandLine::Get(),TEXT("S8EgmoreView="),ReviewView);
  ReviewView=FMath::Clamp(ReviewView,0,5);ApplyReviewCamera();
  if(auto* PC=GetWorld()->GetFirstPlayerController()){PC->SetViewTarget(ReviewCamera);PC->SetVirtualJoystickVisibility(false);}
 }
}
bool ARailEgmoreSceneView::RebuildScene() {
 SceneReady=false;SignalsReady=false;
 auto Fail=[this](const TCHAR* Reason){ImportDiagnostic=Reason;return false;};
 TMap<FString,UStaticMesh*> Meshes;TMap<FString,TArray<FTransform>> Transforms;TSet<FString> StableIds;
 for(int32 Family=0;Family<2;++Family){
 const FString Stem=Family==0?TEXT("EgmoreScene02"):TEXT("EgmoreInterior01");
 FString Text;TSharedPtr<FJsonObject> Doc;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("IndianRailway/Production/stations")/(Stem+TEXT(".json"))))||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Doc)||!Doc.IsValid())return Fail(TEXT("Scene02 import manifest unavailable"));
 FString Identity,Version;
 if(!Doc->TryGetStringField(TEXT("assetFamilyId"),Identity)||Identity!=(Family==0?TEXT("Station.Egmore.Scene.02"):TEXT("Station.Egmore.Interior.01"))||
    !Doc->TryGetStringField(TEXT("version"),Version)||Version!=(Family==0?TEXT("2.0.0"):TEXT("2.1.0")))return Fail(TEXT("Unsupported Scene02 art identity; existing presentation retained"));
 const TArray<TSharedPtr<FJsonValue>>* Definitions=nullptr;const TArray<TSharedPtr<FJsonValue>>* Placements=nullptr;
 if(!Doc->TryGetArrayField(TEXT("importedModules"),Definitions)||Definitions->IsEmpty()||Definitions->Num()>64||
    !Doc->TryGetArrayField(TEXT("placements"),Placements)||Placements->IsEmpty()||Placements->Num()>2048)return Fail(TEXT("Scene02 manifest count outside bounds"));
 for(const auto& Value:*Definitions) {
  const TSharedPtr<FJsonObject>* Row=nullptr;FString Name,Asset;
  if(!Value->TryGetObject(Row)||!(*Row)->TryGetStringField(TEXT("module"),Name)||Name.IsEmpty()||Meshes.Contains(Name)||
     !(*Row)->TryGetStringField(TEXT("asset"),Asset)||!Asset.StartsWith(TEXT("/Game/IndianRailway/S8/Stations/")+Stem+TEXT("/Meshes/")))return Fail(TEXT("Invalid Scene02 module"));
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);if(!Mesh)return Fail(TEXT("Scene02 mesh unavailable"));Meshes.Add(Name,Mesh);
 }
 for(const auto& Value:*Placements) {
  const TSharedPtr<FJsonObject>* Row=nullptr;FString Name,StableId;FVector Position,Scale;double Yaw=0;
  if(!Value->TryGetObject(Row)||!(*Row)->TryGetStringField(TEXT("module"),Name)||!Meshes.Contains(Name)||
     !(*Row)->TryGetStringField(TEXT("stableId"),StableId)||StableId.IsEmpty()||StableIds.Contains(StableId)||
     !ReadVector(**Row,TEXT("positionM"),Position)||Position.GetAbsMax()>1000||!ReadVector(**Row,TEXT("scale"),Scale)||Scale.GetMin()<=0||Scale.GetMax()>100||
     !(*Row)->TryGetNumberField(TEXT("rotationZDeg"),Yaw)||!FMath::IsFinite(Yaw))return Fail(TEXT("Invalid Scene02 placement; existing presentation retained"));
  StableIds.Add(StableId);if(IncludeServiceTrackExtension&&Name==TEXT("SignalUnbound"))continue;Transforms.FindOrAdd(Name).Add(FTransform(FRotator(0,Yaw,0),Position*100,Scale));
 }
 }
 if(IncludeServiceTrackExtension){
  // Base module track coverage is -120..120m. Extend only rails/ballast to
  // the verified logical route endpoints; no outside scenery expansion.
  for(const TCHAR* Name:{TEXT("Track6m"),TEXT("Ballast6m")}){
   if(!Meshes.Contains(Name))return Fail(TEXT("Service track module missing"));
   auto& Instances=Transforms.FindOrAdd(Name);
   for(double Y:Rail::EgmoreLayout::TrackY)for(int32 X=-600;X<600;X+=6)
    if(X< -120||X>=120)Instances.Add(FTransform(FVector(X*100,Y*100,0)));
  }
 }
 // Only replace disposable presentation after every source reference validates.
 for(const auto& Component:Modules)if(Component){RemoveInstanceComponent(Component);Component->DestroyComponent();}Modules.Empty();
 PlacementCount=0;
 for(const auto& Pair:Transforms) {
  auto* Component=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
  AddInstanceComponent(Component);Component->SetupAttachment(RootComponent);Component->SetMobility(EComponentMobility::Static);
  Component->SetStaticMesh(Meshes[Pair.Key]);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);Component->SetGenerateOverlapEvents(false);
  // Authored decimation breaks thin roof/truss silhouettes. Retain these small
  // structural modules until replacement LODs preserve their topology.
  if(Pair.Key.StartsWith(TEXT("Bridge"))||Pair.Key==TEXT("OHEPortal")||Pair.Key==TEXT("EgmoreCanopy6m"))Component->SetForcedLodModel(1);
  Component->SetCastShadow(true);Component->RegisterComponent();
  Component->AddInstances(Pair.Value,false,false,false);PlacementCount+=Pair.Value.Num();Modules.Add(Component);
 }
 SceneReady=true;
 ImportDiagnostic=FString::Printf(TEXT("Scene02: %d modules / %d placements; service extension=%d"),Modules.Num(),PlacementCount,IncludeServiceTrackExtension);
 return true;
}

bool ARailEgmoreSceneView::IsServiceReady() const {
 return SceneReady&&IncludeServiceTrackExtension&&Modules.Num()==39&&PlacementCount==2594&&SignalsReady;
}
FString ARailEgmoreSceneView::TelemetryJson() const {
 return FString::Printf(TEXT("{\"ready\":%s,\"sceneRebuilt\":%s,\"trackExtension\":%s,\"modules\":%d,\"placements\":%d,\"signals\":%s}"),IsServiceReady()?TEXT("true"):TEXT("false"),SceneReady?TEXT("true"):TEXT("false"),IncludeServiceTrackExtension?TEXT("true"):TEXT("false"),Modules.Num(),PlacementCount,*SignalEvidence);
}
bool ARailEgmoreSceneView::PresentSignals(const Rail::OperationsWorld& World){
 SignalsReady=false;
 if(!SceneReady||!IncludeServiceTrackExtension||!Rail::ScenarioContent::IsEgmore(World.ContentId()))return false;
 const auto* Train=World.Spatial(Rail::TrainId(2));if(!Train)return false;
 const auto& Definitions=World.DefinitionsView().signals;
 if(Definitions.size()!=2)return false;
 // Resolve by stable signal ID and graph boundary, never by map iteration index.
 TArray<FTransform> Poses;TArray<uint64> Ids;TArray<bool> Proceed;
 for(uint64 Id:{1ull,2ull}){
  const auto Def=Definitions.find(Rail::SignalId(Id));const auto State=World.Current().signals.find(Rail::SignalId(Id));Rail::Sample Sample;
  if(Def==Definitions.end()||State==World.Current().signals.end()||!Train->Network().SamplePosition(Def->second.boundary,Sample).ok)return false;
  const FVector Forward(Sample.tangent.x,Sample.tangent.y,0);
  const FVector Right(-Forward.Y,Forward.X,0);
  const FVector Position=FVector(Sample.position.x,Sample.position.y,Sample.position.z)*100-Right*270+FVector(0,0,Id==2?84:0);
  Poses.Add(FTransform(FRotator(0,Forward.Rotation().Yaw+90,0),Position));Ids.Add(Id);Proceed.Add(State->second.aspect==Rail::SignalAspect::Proceed);
 }
 if(SignalIds!=Ids||SignalParts.Num()!=8||SignalLamps.Num()!=6){
  auto* Housing=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/Stations/EgmoreScene02/Meshes/SM_SignalUnbound.SM_SignalUnbound"));
  auto* Lens=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));
  auto* Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Egmore/M_SignalLens.M_SignalLens"));
  if(!Housing||!Lens||!Material)return false;
  for(const auto& Part:SignalParts)if(Part){RemoveInstanceComponent(Part.Get());Part->DestroyComponent();}
  SignalParts.Empty();SignalLamps.Empty();SignalIds=Ids;
  for(int32 I=0;I<2;++I)for(int32 P=0;P<4;++P){
   auto* Part=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);Part->SetMobility(EComponentMobility::Movable);
   Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);Part->SetStaticMesh(P==0?Housing:Lens);Part->SetCastShadow(P==0);Part->RegisterComponent();
   if(P){auto* Lamp=UMaterialInstanceDynamic::Create(Material,this);Part->SetMaterial(0,Lamp);SignalLamps.Add(Lamp);}
   SignalParts.Add(Part);
  }
 }
 SignalEvidence=TEXT("[");
 for(int32 I=0;I<2;++I){
  SignalParts[I*4]->SetWorldTransform(Poses[I]);
  for(int32 L=0;L<3;++L){
   // Source lenses face local +Y after the verified Blender-to-UE conversion.
   SignalParts[I*4+L+1]->SetWorldTransform(FTransform(Poses[I].GetRotation(),Poses[I].TransformPosition(FVector(0,19,330+45*L)),FVector(.25,.025,.25)));
   const FLinearColor Color=L==0&&!Proceed[I]?FLinearColor(6,0,0):L==2&&Proceed[I]?FLinearColor(0,6,0):FLinearColor(.005f,.005f,.005f);
   SignalLamps[I*3+L]->SetVectorParameterValue(TEXT("Color"),Color);
  }
  const FVector P=Poses[I].GetLocation()/100;
  SignalEvidence+=FString::Printf(TEXT("%s{\"id\":%llu,\"aspect\":\"%s\",\"positionM\":[%.3f,%.3f,%.3f]}"),I?TEXT(","):TEXT(""),Ids[I],Proceed[I]?TEXT("proceed"):TEXT("stop"),P.X,P.Y,P.Z);
 }
 SignalEvidence+=TEXT("]");SignalsReady=true;return true;
}

void ARailEgmoreSceneView::ApplyReviewCamera(){
 if(!ReviewCamera)return;
 const FVector Positions[]={FVector(9000,-460,250),FVector(3000,-560,250),FVector(4900,-560,250),FVector(6700,1000,920),FVector(9713,-65,335),FVector(11000,-900,430)};
 const FRotator Rotations[]={FRotator(3,180,0),FRotator(5,0,0),FRotator(12,0,0),FRotator(-8,180,0),FRotator(-4,0,0),FRotator(-3,160,0)};
 ReviewCamera->SetActorLocation(Positions[ReviewView]);ReviewCamera->SetActorRotation(Rotations[ReviewView]);ReviewCamera->GetCameraComponent()->SetFieldOfView(ReviewView==4?95:78);
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It){
  const bool Cab=It->ActorHasTag(TEXT("Scene02CabOnly")),Exterior=It->ActorHasTag(TEXT("Scene02Exterior"));
  if(Cab||Exterior){const bool Visible=Cab?ReviewView==4:ReviewView!=4;It->GetStaticMeshComponent()->SetVisibility(Visible);It->GetStaticMeshComponent()->SetCastShadow(Visible);}
 }
 const FString Evidence=FString::Printf(TEXT("{\"family\":\"Station.Egmore.Scene.02\",\"interior\":\"2.1.0\",\"placements\":%d,\"modules\":%d,\"view\":%d,\"staticArtReview\":true,\"gameplayBound\":false}"),PlacementCount,Modules.Num(),ReviewView);
 const FString Directory=FPaths::ProjectSavedDir()/TEXT("S8/EgmoreScene02/Review");IFileManager::Get().MakeDirectory(*Directory,true);
 FFileHelper::SaveStringToFile(Evidence,*(Directory/TEXT("telemetry.json")));
 UE_LOG(LogTemp,Display,TEXT("S8_EGMORE_SCENE02_REVIEW %s"),*Evidence);
}
void ARailEgmoreSceneView::Tick(float DeltaSeconds){
 Super::Tick(DeltaSeconds);
 if(!ReviewCamera)return;
 ReviewElapsed+=DeltaSeconds;
 if(FParse::Param(FCommandLine::Get(),TEXT("S8EgmoreCameraTour"))&&ReviewElapsed>=12){ReviewElapsed=0;ReviewView=(ReviewView+1)%6;ApplyReviewCamera();}
}

