#include "RailChunkView.h"
#include "ProfilingDebugging/CsvProfiler.h"
CSV_DEFINE_CATEGORY(RailChunkLoading,true);
#include "RailJourneyMaterials.h"
#include "RailWheelPresentation.h"
#include "RailTambaramCorridorView.h"
#include "RailTambaramStationView.h"
#include "RailTambaramTurnoutView.h"
#include "Engine/World.h"
#include "Engine/Level.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/TextureCube.h"
#include "Components/SkyLightComponent.h"
#include "Engine/Light.h"
#include "EngineUtils.h"
#include "Components/DirectionalLightComponent.h"
#include "Materials/MaterialInterface.h"
#include "Simulation/RailScenarioContent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
namespace {
FVector Cm(Rail::Vec3 V){return FVector(V.x*100,V.y*100,V.z*100);}
bool Owns(int32 Chunk,double X){return Chunk==1?X<0:X>=0;}
// Runtime-only replacement of the S6/S7 test-map environment. Never edits maps.
void PrepareTambaramEnvironment(UWorld* World,ULevel* Chunk){
 const FName Tag(TEXT("TambaramRuntimeEnvironment"));
 for(ULevel* Level:{World->PersistentLevel.Get(),Chunk})if(Level)
  for(AActor* Actor:Level->Actors)if(IsValid(Actor)&&!Actor->ActorHasTag(Tag)){
   if(auto* Mesh=Cast<AStaticMeshActor>(Actor))Mesh->SetActorHiddenInGame(true);
   if(auto* Light=Cast<ALight>(Actor))Light->GetLightComponent()->SetVisibility(false);
  }
 for(TActorIterator<ADirectionalLight> It(World);It;++It)if(It->ActorHasTag(Tag))return;
 FActorSpawnParameters Params;Params.OverrideLevel=World->PersistentLevel;
 // Chennai mid-morning (13.1 N, ~09:30): sun ~42 deg high in the south-east. Physically based
 // sky (mobile base pass supports SkyAtmosphere aerial perspective), sun tinted by atmospheric
 // transmittance, ambient captured from that sky and a humid coastal haze. Replaces the old
 // grey-cubemap fill and painted sky sphere that made every surface flat.
 auto* Atmosphere=World->SpawnActor<ASkyAtmosphere>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
 if(Atmosphere){Atmosphere->Tags.Add(Tag);auto* Air=Atmosphere->GetComponent();
  Air->SetMieScatteringScale(.013f);Air->SetMieAnisotropy(.80f);    // humid coastal aerosol, ~3x default
  Air->SetGroundAlbedo(FColor(110,98,80));Air->SetAerialPespectiveViewDistanceScale(2.2f);
  Air->SetHeightFogContribution(.6f);}
 auto* Sun=World->SpawnActor<ADirectionalLight>(FVector(0,0,2500),FRotator(-42,-35,0),Params);
 if(Sun){Sun->Tags.Add(Tag);auto* Light=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
  Light->SetMobility(EComponentMobility::Movable);Light->SetIntensity(3.6f);Light->SetAtmosphereSunLight(true);
  Light->SetAtmosphereSunLightIndex(0);Light->SetLightSourceAngle(.5357f);
  Light->SetDynamicShadowDistanceMovableLight(25000);Light->SetDynamicShadowCascades(3);}
 auto* Fill=World->SpawnActor<ASkyLight>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
 if(Fill){Fill->Tags.Add(Tag);auto* Light=Fill->GetLightComponent();Light->SetMobility(EComponentMobility::Movable);
  // Real-time capture falls back to a one-off capture where the Effects tier disables it.
  Light->SourceType=SLS_CapturedScene;Light->SetRealTimeCapture(true);
  Light->bLowerHemisphereIsBlack=true;Light->SetLowerHemisphereColor(FLinearColor(.16f,.13f,.09f)); // sun-lit laterite bounce
  Light->SetIntensity(1.f);Light->SetCastShadows(false);Light->RecaptureSky();}
 auto* Haze=World->SpawnActor<AExponentialHeightFog>(FVector(0,0,-100),FRotator::ZeroRotator,Params);
 if(Haze){Haze->Tags.Add(Tag);auto* Fog=Haze->GetComponent();Fog->SetFogDensity(.009f);Fog->SetFogHeightFalloff(.20f);
  Fog->SetStartDistance(2500);Fog->SetFogMaxOpacity(.85f);Fog->SetFogInscatteringColor(FLinearColor(.16f,.18f,.20f));
  Fog->SetDirectionalInscatteringExponent(6);Fog->SetDirectionalInscatteringStartDistance(8000);
  Fog->SetDirectionalInscatteringColor(FLinearColor(.30f,.24f,.15f));}
 // Unbound grade: humid tropical daylight is soft and warm, never the neutral grey of the
 // engine default. Bloom is a no-op where the device profile sets r.BloomQuality=0.
 auto* Grade=World->SpawnActor<APostProcessVolume>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
 if(Grade){Grade->Tags.Add(Tag);Grade->bUnbound=true;Grade->Priority=-1;auto& P=Grade->Settings;
  P.bOverride_BloomIntensity=true;P.BloomIntensity=.35f;P.bOverride_BloomThreshold=true;P.BloomThreshold=1.2f;
  P.bOverride_WhiteTemp=true;P.WhiteTemp=6900; // >6500 warms the image
  P.bOverride_ColorSaturation=true;P.ColorSaturation=FVector4(1.04,1.03,.96,1);
  P.bOverride_ColorContrast=true;P.ColorContrast=FVector4(1.04,1.04,1.04,1);
  P.bOverride_ColorGammaShadows=true;P.ColorGammaShadows=FVector4(1.0,1.0,1.03,1);
  P.bOverride_VignetteIntensity=true;P.VignetteIntensity=.3f;}
}
}
ARailChunkView::ARailChunkView(){
 PrimaryActorTick.bCanEverTick=false;
 RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Bodies=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Bodies"));Bodies->SetupAttachment(RootComponent);
 Scenery=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Scenery"));Scenery->SetupAttachment(RootComponent);
 Bodies->SetCollisionEnabled(ECollisionEnabled::NoCollision);Scenery->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 // Assets are assigned on the level instance by the fixture authoring script.
 // No ConstructorHelpers/static references pin chunk-specific assets in a CDO.
}
void ARailChunkView::BeginPlay(){
 Super::BeginPlay();Scenery->ClearInstances();
 if(FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram")))PrepareTambaramEnvironment(GetWorld(),GetLevel());
 if(FParse::Param(FCommandLine::Get(),TEXT("S8Station"))||FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram")))return;
 for(int32 I=0;I<512;++I){const double X=(ChunkId==1?-235.0:5.0)+(I%32)*7.0;
  const double Y=110.0+(I/32)*7.0;Scenery->AddInstance(FTransform(FRotator::ZeroRotator,FVector(X*100,Y*100,100),FVector(2,2,2)));}
}
bool ARailChunkView::LoadCoach(){
 if(FParse::Param(FCommandLine::Get(),TEXT("S8DisableCoach")))return false;
 if(CoachParts.Num()==7)return true;
 CSV_SCOPED_TIMING_STAT(RailChunkLoading,CoachLoadMs);
 FString Text;TSharedPtr<FJsonObject> Doc;const TArray<TSharedPtr<FJsonValue>>* Parts=nullptr;
 if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("IndianRailway/Production/coaches/LHB3A.json")))||
    !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Doc)||!Doc||!Doc->TryGetArrayField(TEXT("parts"),Parts)||Parts->Num()!=7)return false;
 FString Content;double Definition=0,Version=0,Length=0,Gap=0;
 if(!Doc->TryGetStringField(TEXT("content_id"),Content)||Content!=UTF8_TO_TCHAR(Rail::FormationContent::LhbId)||
    !Doc->TryGetNumberField(TEXT("definition_id"),Definition)||Definition!=Rail::FormationContent::LhbDefinitionId||
    !Doc->TryGetNumberField(TEXT("version"),Version)||Version!=Rail::FormationContent::LhbVersion||
    !Doc->TryGetNumberField(TEXT("coupled_length_m"),Length)||Length!=Rail::FormationContent::CoupledLength||
    !Doc->TryGetNumberField(TEXT("coupling_gap_m"),Gap)||Gap!=0)return false;
 const TCHAR* Expected[]={TEXT("Body"),TEXT("Bogie0"),TEXT("Bogie1"),TEXT("Wheel0"),TEXT("Wheel1"),TEXT("Wheel2"),TEXT("Wheel3")};
 TArray<FString> Paths;
 for(int32 I=0;I<7;++I){const auto O=(*Parts)[I]->AsObject();FString Name,Path;
  if(!O||!O->TryGetStringField(TEXT("name"),Name)||Name!=Expected[I]||!O->TryGetStringField(TEXT("asset"),Path)||!Path.StartsWith(TEXT("/Game/IndianRailway/S8/Coaches/LHB3A/")))return false;
  Paths.Add(Path);
 }
 // Readiness remains false until every independent part has been loaded.
 const int32 I=CoachParts.Num();
 {
  auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Paths[I]);if(!Mesh)return false;
  if(I<3){const FString DetailPath=I==0?TEXT("/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2/SM_LHB_BodyV2.SM_LHB_BodyV2"):TEXT("/Game/IndianRailway/S8/Coaches/LHB3A/UnderframeV2/SM_LHB_BogieV2.SM_LHB_BogieV2");if(auto* Detail=LoadObject<UStaticMesh>(nullptr,*DetailPath))Mesh=Detail;}
  if(I>=3)if(auto* Wheel=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/RunningGearV2/SM_LHB_Wheelset.SM_LHB_Wheelset")))Mesh=Wheel;
  auto* Part=NewObject<UInstancedStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Mesh);ApplyJourneyMaterials(Part);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);Part->RegisterComponent();CoachParts.Add(Part);
  CSV_CUSTOM_STAT(RailChunkLoading,CoachPartsBuilt,1,ECsvCustomStatOp::Accumulate);
 }
 if(CoachParts.Num()!=7)return false;
 UE_LOG(LogTemp,Display,TEXT("S8_LHB_CHUNK_READY chunk=%d parts=%d"),ChunkId,CoachParts.Num());return true;
}
void ARailChunkView::Present(const Rail::OperationsWorld& World,bool DebugLabels,uint64 HiddenLocomotiveTrain,bool RefreshScenery){
 auto* W=GetWorld();const auto* Player=World.Spatial(Rail::TrainId(1));if(!Player)return;
 const auto& Graph=Player->Network();
 const bool Tambaram=Rail::ScenarioContent::IsTambaram(World.ContentId());
 if(RefreshScenery){
 if(Tambaram){
  FActorSpawnParameters Params;Params.OverrideLevel=GetLevel();Params.Owner=this;
  if(!IsValid(TambaramCorridor))TambaramCorridor=W->SpawnActor<ARailTambaramCorridorView>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
  if(!IsValid(TambaramStation))TambaramStation=W->SpawnActor<ARailTambaramStationView>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
  if(!IsValid(TambaramTurnout))TambaramTurnout=W->SpawnActor<ARailTambaramTurnoutView>(FVector::ZeroVector,FRotator::ZeroRotator,Params);
  if(TambaramTurnout)TambaramTurnout->Present(World,ChunkId);
  if(TambaramCorridor)TambaramCorridor->Present(World,ChunkId);
  const bool StationReady=TambaramStation&&TambaramStation->Present(World,ChunkId);
  TambaramReady=StationReady&&TambaramCorridor&&TambaramCorridor->IsReady()&&TambaramTurnout&&TambaramTurnout->IsReady();
 }
 if(!Rail::ScenarioContent::IsS8Route(World.ContentId()))LoadStation(Graph);
 if(!Tambaram&&!FParse::Param(FCommandLine::Get(),TEXT("S8Station")))
 for(const auto& Edge:Graph.edges){const int32 Steps=FMath::CeilToInt(Edge.lengthMetres/3);
  for(int32 I=0;I<Steps;++I){const auto A=Edge.geometry.At(Edge.lengthMetres*I/Steps).position,B=Edge.geometry.At(Edge.lengthMetres*(I+1)/Steps).position;
   if(Owns(ChunkId,(A.x+B.x)*.5))DrawDebugLine(W,Cm(A),Cm(B),FColor::Cyan,false,0,0,20);}}
 }
 TArray<FTransform> Transforms;TArray<FTransform> CoachTransforms[7];PresentedLhbVehicles=0;
 const bool CoachReady=Rail::ScenarioContent::UsesLhb(World.ContentId())&&(RefreshScenery?LoadCoach():CoachParts.Num()==7);
 for(uint64 Id=1;Id<=Rail::ScenarioContent::TrainCount(World.ContentId());++Id)if(const auto* Train=World.Spatial(Rail::TrainId(Id)))for(const auto& V:Train->Current().vehicles){
  if((HiddenLocomotiveTrain==Id||(HiddenLocomotiveTrain&&Rail::ScenarioContent::IsS8Route(World.ContentId())))&&V.id==Train->Current().vehicles.front().id)continue;
  if(!Owns(ChunkId,V.bodyCentre.x))continue;
  const FQuat Rotation=FRotationMatrix::MakeFromXZ(FVector(V.bodyForward.x,V.bodyForward.y,V.bodyForward.z),FVector::UpVector).ToQuat();
  if(CoachReady&&V.definition==Rail::DefinitionId(Rail::FormationContent::LhbDefinitionId)){
   CoachTransforms[0].Add(FTransform(Rotation,Cm(V.bodyCentre)));++PresentedLhbVehicles;
   for(int32 B=0;B<2;++B){const bool Front=B==1;const FVector Position=Cm(Front?V.frontBogie:V.rearBogie);
    Rail::Sample Sample;const double Offset=Front?Rail::FormationContent::BogieOffset:-Rail::FormationContent::BogieOffset;
    FQuat BogieRotation=Rotation;
    if(Train->History().SampleAt(Train->Network(),V.centreStationMetres+Offset,Sample).ok)
     BogieRotation=FRotationMatrix::MakeFromXZ(Cm(Sample.tangent),FVector::UpVector).ToQuat();
    CoachTransforms[B+1].Add(FTransform(BogieRotation,Position));
    for(int32 A=0;A<2;++A){const double AxleOffset=A==0?-128:128;
     const double Spin=RailWheelPresentation::Angle(Train->Current().headStationMetres,Rail::FormationContent::WheelDiameter*.5);
     CoachTransforms[3+B*2+A].Add(FTransform(BogieRotation*FQuat(FVector(0,1,0),Spin),Position+BogieRotation.RotateVector(FVector(AxleOffset,0,45.75))));
    }
   }
   continue;
  }
  Transforms.Add(FTransform(Rotation,Cm(V.bodyCentre)+FVector(0,0,220),FVector(V.lengthMetres,3,2.5)));
  if(DebugLabels)DrawDebugString(W,Cm(V.bodyCentre)+FVector(0,0,450),FString::Printf(TEXT("T%llu V%llu"),Id,V.id.value),nullptr,FColor::Yellow,0,true);
 }
 if(Bodies->GetInstanceCount()!=Transforms.Num()){Bodies->ClearInstances();Bodies->AddInstances(Transforms,false,true);}
 else for(int32 I=0;I<Transforms.Num();++I)Bodies->UpdateInstanceTransform(I,Transforms[I],true,false,true);
 Bodies->MarkRenderStateDirty();PresentedVehicles=Transforms.Num();PresentedTick=World.Current().tick;
 for(int32 P=0;P<CoachParts.Num();++P){auto* Part=CoachParts[P].Get();auto& Values=CoachTransforms[P];if(Part->GetInstanceCount()!=Values.Num()){Part->ClearInstances();Part->AddInstances(Values,false,true);}else for(int32 I=0;I<Values.Num();++I)Part->UpdateInstanceTransform(I,Values[I],true,false,true);Part->MarkRenderStateDirty();}
 if(DebugLabels)for(const auto& Pair:World.DefinitionsView().signals){Rail::Sample Sample;if(!Graph.SamplePosition(Pair.second.boundary,Sample).ok||!Owns(ChunkId,Sample.position.x))continue;
  const auto Signal=World.Current().signals.find(Pair.first);if(Signal==World.Current().signals.end())continue;
  const bool Proceed=Signal->second.aspect==Rail::SignalAspect::Proceed;const auto P=Cm(Sample.position)+FVector(0,-500,500);
  DrawDebugSphere(W,P,150,8,Proceed?FColor::Green:FColor::Red,false,0,0,20);
  DrawDebugString(W,P+FVector(0,0,300),FString::Printf(TEXT("S%llu %s"),Pair.first.value,Proceed?TEXT("GO"):TEXT("STOP")),nullptr,FColor::White,0,true);
 }
 if(DebugLabels)for(const auto& Pair:World.Current().points){const double X=Pair.first.value==1?-1:160;if(!Owns(ChunkId,X))continue;
  DrawDebugString(W,FVector(X*100,1000,500),FString::Printf(TEXT("P%llu pos%d lock R%llu"),Pair.first.value,Pair.second.position,Pair.second.lock.route.value),nullptr,FColor::Orange,0,true);}
}


