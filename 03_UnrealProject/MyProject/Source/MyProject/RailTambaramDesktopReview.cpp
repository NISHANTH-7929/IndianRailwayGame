#include "CoreMinimal.h"
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
#include "RailSimulationSubsystem.h"
#include "RailTambaramCorridorView.h"
#include "RailTambaramTurnoutView.h"
#include "RailTambaramStationView.h"
#include "RailChunkView.h"
#include "RailWheelPresentation.h"
#include "Simulation/RailWorldSnapshot.h"
#include "Simulation/RailTambaramRoute.h"
#include "RailLocomotiveView.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraActor.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Containers/Ticker.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/App.h"
#include "UnrealClient.h"
#include "ShaderCompiler.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"


namespace {
struct FTambaramReview {
 FString Output,Pending;double Started=FPlatformTime::Seconds(),LastCapture=0;
 int32 Captures=0;bool Full=false,Boundary=false,Cycled=false,Resumed=false,CyclePending=false,ReloadVerified=false;
 FString BeforeCycle;
 bool SceneryOnly=false,CabOnly=false,TrackOnly=false,TrackCrossover=false;
 FString SceneryState;
 int32 CaptureWarmup=0,PendingCameraMode=0;
 double CabWideFov=0;
 bool CheckTrackAssets(UWorld* W){
  // Inspect the materials actually assigned to populated render components.
  // A successful LoadObject alone would miss an override/family mapping bug.
  const FString Root=TEXT("/Game/IndianRailway/S9/TrackEnvironmentV1/");
  const auto AssetPath=[&Root](const TCHAR* Folder,const TCHAR* Name){return Root+Folder+TEXT("/")+Name+TEXT(".")+Name;};
  const FString Bed=AssetPath(TEXT("Meshes"),TEXT("SM_Ballast6m_RealismV3"));
  const FString Stone=AssetPath(TEXT("Meshes"),TEXT("SM_AngularBallastStoneV1"));
  const FString Track=AssetPath(TEXT("Meshes"),TEXT("SM_Track6m_RealismV2"));
  const FString Ballast=AssetPath(TEXT("MaterialsV4"),TEXT("M_TrackBallast")),StoneFinish=AssetPath(TEXT("MaterialsV4"),TEXT("M_TrackStone"));
  const FString Ground=AssetPath(TEXT("MaterialsV4"),TEXT("M_DryGround"));
  const FString NatureRoot=TEXT("/Game/IndianRailway/S9/CorridorNatureV1/");
  const TCHAR* NatureMeshes[]={TEXT("SM_CorridorLeafClusterV2"),TEXT("SM_CorridorVerge12m"),TEXT("SM_CorridorGrassPatchV1"),TEXT("SM_CorridorBranchV1")};
  const TCHAR* NatureMaterials[]={TEXT("M_CorridorLeaf"),TEXT("M_CorridorNatureGround"),TEXT("M_CorridorDryGrass"),TEXT("M_CorridorBark")};
  int32 NatureAssigned[4]={0,0,0,0};bool NatureLODs=true,NatureCollision=true;
  const TCHAR* FinishNames[]={TEXT("M_RailHead"),TEXT("M_RailRust"),TEXT("M_SleeperConcrete"),TEXT("M_Fastener")};
  int32 Beds=0,Stones=0,Grounds=0,Tracks=0,Turnouts=0;
  int32 OrdinaryFinish[3]={0,0,0},TurnoutFinish[4]={0,0,0,0};
  TMap<FString,int32> MeshCounts,MaterialCounts;
  TSet<FString> CheckedMaterials;bool TextureBindingsPassed=true;
  bool BedUVChecked=false;double MaximumBedLateralUVError=0;
  int32 VegetatedGroundInstances=0;
  auto UsedTextureJson=MakeShared<FJsonObject>();
  TArray<TSharedPtr<FJsonValue>> Components;
  for(TActorIterator<AActor> It(W);It;++It){
   const bool Corridor=It->IsA<ARailTambaramCorridorView>(),Turnout=It->IsA<ARailTambaramTurnoutView>();
   if(!Corridor&&!Turnout)continue;
   TInlineComponentArray<UStaticMeshComponent*> Parts;It->GetComponents(Parts);
   for(auto* Part:Parts){
    const UStaticMesh* Mesh=Part->GetStaticMesh();if(!Mesh||!Part->IsRegistered()||!Part->IsVisible())continue;
    const auto* Instanced=Cast<UInstancedStaticMeshComponent>(Part);
    const int32 Count=Instanced?Instanced->GetInstanceCount():1;if(Count==0)continue;
    const FString MeshPath=Mesh->GetPathName();MeshCounts.FindOrAdd(MeshPath)+=Count;
    if(MeshPath==Bed&&!BedUVChecked){
     const auto* Data=Mesh->GetRenderData();
     if(Data&&!Data->LODResources.IsEmpty()){
      const auto& Buffers=Data->LODResources[0].VertexBuffers;
      if(Buffers.PositionVertexBuffer.GetNumVertices()>0&&Buffers.StaticMeshVertexBuffer.GetNumTexCoords()>0){
       BedUVChecked=true;
       for(uint32 V=0;V<Buffers.PositionVertexBuffer.GetNumVertices();++V){
        const double Y=Buffers.PositionVertexBuffer.VertexPosition(V).Y*.01;
        const double Decoded=1.-Buffers.StaticMeshVertexBuffer.GetVertexUV(V,0).Y;
        MaximumBedLateralUVError=FMath::Max(MaximumBedLateralUVError,FMath::Abs(Y-Decoded));
       }
      }
     }
    }
    TArray<TSharedPtr<FJsonValue>> Slots;TSet<FString> Assigned;
    for(int32 Slot=0;Slot<Part->GetNumMaterials();++Slot){
     auto* Interface=Part->GetMaterial(Slot);auto* Material=Interface?Interface->GetMaterial():nullptr;
     const FString Path=Material?Material->GetPathName():TEXT("null");Assigned.Add(Path);MaterialCounts.FindOrAdd(Path)+=Count;
     if(Path==Ground&&Interface){float Coverage=0;if(Interface->GetScalarParameterValue(FMaterialParameterInfo(TEXT("VegetationCoverage")),Coverage)&&Coverage>0)VegetatedGroundInstances+=Count;}
     if(Material&&Path.StartsWith(Root+TEXT("MaterialsV4/"))&&!CheckedMaterials.Contains(Path)){
      CheckedMaterials.Add(Path);TArray<UTexture*> Used;Material->GetUsedTextures(Used);
      TArray<TSharedPtr<FJsonValue>> TexturePaths;TextureBindingsPassed&=!Used.IsEmpty();
      for(auto* Texture:Used){
       const auto* T=Cast<UTexture2D>(Texture);
       TextureBindingsPassed&=T&&T->GetPathName().StartsWith(Root+TEXT("TexturesV2/"))&&T->GetNumMips()>1;
       TexturePaths.Add(MakeShared<FJsonValueString>(Texture?Texture->GetPathName():TEXT("null")));
      }
      UsedTextureJson->SetArrayField(Path,TexturePaths);
     }
     auto Entry=MakeShared<FJsonObject>();Entry->SetNumberField(TEXT("slot"),Slot);Entry->SetStringField(TEXT("interface"),Interface?Interface->GetPathName():TEXT("null"));Entry->SetStringField(TEXT("baseMaterial"),Path);Slots.Add(MakeShared<FJsonValueObject>(Entry));
    }
    auto Entry=MakeShared<FJsonObject>();Entry->SetStringField(TEXT("actor"),It->GetName());Entry->SetStringField(TEXT("component"),Part->GetName());Entry->SetStringField(TEXT("mesh"),MeshPath);Entry->SetNumberField(TEXT("instances"),Count);Entry->SetArrayField(TEXT("materials"),Slots);Components.Add(MakeShared<FJsonValueObject>(Entry));
    if(Corridor){
     for(int32 I=0;I<4;++I)if(MeshPath==NatureRoot+TEXT("Meshes/")+NatureMeshes[I]+TEXT(".")+NatureMeshes[I]){
      const FString Expected=NatureRoot+TEXT("Materials/")+NatureMaterials[I]+TEXT(".")+NatureMaterials[I];
      if(Assigned.Contains(Expected))NatureAssigned[I]+=Count;
      NatureLODs&=Mesh->GetNumLODs()==3;
      NatureCollision&=Part->GetCollisionEnabled()==ECollisionEnabled::NoCollision;
     }
     if(MeshPath==Bed&&Assigned.Contains(Ballast))Beds+=Count;
     if(MeshPath==Stone&&Assigned.Contains(StoneFinish))Stones+=Count;
     if(Assigned.Contains(Ground))Grounds+=Count;
     if(MeshPath==Track){Tracks+=Count;for(int32 I=0;I<3;++I)if(Assigned.Contains(AssetPath(TEXT("MaterialsV4"),FinishNames[I])))OrdinaryFinish[I]+=Count;}
    }else{
     Turnouts+=Count;for(int32 I=0;I<4;++I)if(Assigned.Contains(AssetPath(TEXT("MaterialsV4"),FinishNames[I])))TurnoutFinish[I]+=Count;
    }
   }
  }
  bool Passed=Beds>0&&Stones>0&&Grounds>0&&Tracks>0&&TextureBindingsPassed&&BedUVChecked&&MaximumBedLateralUVError<.003&&VegetatedGroundInstances>0;
  bool NaturePassed=NatureLODs&&NatureCollision;
  for(int32 Count:NatureAssigned)NaturePassed&=Count>0;
  Passed&=NaturePassed;
  // Every ordinary track instance must have all three shared finishes.
  for(int32 I=0;I<3;++I)Passed&=OrdinaryFinish[I]==Tracks;
  const bool CheckTurnout=TrackCrossover||Turnouts>0;
  if(CheckTurnout)for(int32 I=0;I<4;++I)Passed&=TurnoutFinish[I]>0;
  auto Json=MakeShared<FJsonObject>();Json->SetBoolField(TEXT("passed"),Passed);Json->SetBoolField(TEXT("actualPopulatedComponents"),true);Json->SetBoolField(TEXT("turnoutRequired"),TrackCrossover);Json->SetBoolField(TEXT("turnoutAssessed"),CheckTurnout);Json->SetNumberField(TEXT("bedInstancesWithNewMaterial"),Beds);Json->SetNumberField(TEXT("stoneInstancesWithNewMaterial"),Stones);Json->SetNumberField(TEXT("groundInstancesWithNewMaterial"),Grounds);Json->SetNumberField(TEXT("ordinaryTrackInstances"),Tracks);Json->SetNumberField(TEXT("turnoutInstances"),Turnouts);
  auto Meshes=MakeShared<FJsonObject>(),Materials=MakeShared<FJsonObject>(),Ordinary=MakeShared<FJsonObject>(),Junction=MakeShared<FJsonObject>();
  for(const auto& Entry:MeshCounts)Meshes->SetNumberField(Entry.Key,Entry.Value);
  for(const auto& Entry:MaterialCounts)Materials->SetNumberField(Entry.Key,Entry.Value);
  for(int32 I=0;I<4;++I){Junction->SetNumberField(FinishNames[I],TurnoutFinish[I]);if(I<3)Ordinary->SetNumberField(FinishNames[I],OrdinaryFinish[I]);}
  Json->SetObjectField(TEXT("meshInstanceCounts"),Meshes);Json->SetObjectField(TEXT("materialSlotInstanceCounts"),Materials);Json->SetObjectField(TEXT("ordinaryFinishInstances"),Ordinary);Json->SetObjectField(TEXT("turnoutFinishInstances"),Junction);Json->SetArrayField(TEXT("components"),Components);
  Json->SetBoolField(TEXT("compiledMaterialTextureBindingsPassed"),TextureBindingsPassed);Json->SetObjectField(TEXT("usedTextures"),UsedTextureJson);
  Json->SetBoolField(TEXT("bedImportedUVChecked"),BedUVChecked);Json->SetNumberField(TEXT("maximumBedLateralUVErrorMetres"),MaximumBedLateralUVError);Json->SetNumberField(TEXT("vegetatedGroundInstances"),VegetatedGroundInstances);
  auto Nature=MakeShared<FJsonObject>();for(int32 I=0;I<4;++I)Nature->SetNumberField(NatureMeshes[I],NatureAssigned[I]);
  Json->SetObjectField(TEXT("natureAssignedInstances"),Nature);Json->SetBoolField(TEXT("natureAssetsPassed"),NaturePassed);Json->SetBoolField(TEXT("natureThreeLODs"),NatureLODs);Json->SetBoolField(TEXT("natureCollisionDisabled"),NatureCollision);
  FString Encoded;FJsonSerializer::Serialize(Json,TJsonWriterFactory<>::Create(&Encoded));
  return FFileHelper::SaveStringToFile(Encoded,*(Output/TEXT("track-assets.json")))&&Passed;
 }
 bool ConfigureCabView(ARailLocomotiveView* Loco,int32 Mode){
  if(!Loco||!Loco->IsReady())return false;
  TSharedPtr<FJsonObject> State;bool Lamp=false;
  if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Loco->TelemetryJson()),State)||!State.IsValid()||!State->TryGetBoolField(TEXT("cabLight"),Lamp))return false;
  if(Lamp!=(Mode>=19))Loco->ToggleSwitch(3);
  // Returning through exterior resets the look offset and zoom without
  // changing any driving command. Apply the lamp before UpdateCamera runs.
  if(Loco->IsCabCamera())Loco->ToggleCamera();
  Loco->ToggleCamera();
  if(!Loco->IsCabCamera())return false;
  if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Loco->TelemetryJson()),State)||!State.IsValid()||!State->TryGetNumberField(TEXT("cameraFov"),CabWideFov))return false;
  if(Mode==18||Mode==20)Loco->ToggleInstrumentView();
  return true;
 }
 bool CabFrameValid(const FString& Telemetry)const{
  TSharedPtr<FJsonObject> State;bool Cab=false,Lamp=false,Visible=false,Zoom=false;double Fov=0;
  return FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Telemetry),State)&&State.IsValid()
   &&State->TryGetBoolField(TEXT("cabCamera"),Cab)&&Cab
   &&State->TryGetBoolField(TEXT("cabLight"),Lamp)&&Lamp==(PendingCameraMode>=19)
   &&State->TryGetBoolField(TEXT("cabLightVisible"),Visible)&&Visible==Lamp
   &&State->TryGetBoolField(TEXT("instrumentView"),Zoom)&&Zoom==(PendingCameraMode==18||PendingCameraMode==20)
   &&State->TryGetNumberField(TEXT("cameraFov"),Fov)&&FMath::Abs(Fov-(Zoom?55.:CabWideFov))<.01;
 }
 double CycleStarted=0;
 int32 TerminalCaptures=0;
 int32 NextStation=1,SceneryCaptures=0,BridgeCaptures=0;
 double NextScenery=1800;
 bool ApproachCaptured=false,BoundaryAfter=false;
 bool JunctionCaptured=false;
 int32 PassingStage=0;
 bool PassingSeen=false,PassingDirectionValid=true,PassingMoving=true;
 bool PassingRecording=false,PassingExporting=false,PassingAudioReady=false;
 double PassingMinimum=1e30,PassingSigned=1e30,PassingDistance=1e30;
 double PassingExportStarted=0,PassingAudioStableSince=0;
 int64 PassingAudioBytes=0;
 double PreviousFixedDelta=0;float PreviousMaxFps=0;bool PreviousFixedStep=false,PassingCadenceChanged=false;
 double PassingRecordStart=0,PassingRecordStartDistance=0;uint64 PassingRecordStartTick=0;
 FString PassingSamples;
 void RestorePassingCadence(){
  if(!PassingCadenceChanged)return;
  FApp::SetFixedDeltaTime(PreviousFixedDelta);FApp::SetUseFixedTimeStep(PreviousFixedStep);
  if(GEngine)GEngine->SetMaxFPS(PreviousMaxFps);PassingCadenceChanged=false;
 }
 void ObservePassing(UWorld* W,const Rail::OperationsWorld& World,const Rail::Simulation& Player){
  if(!Full)return;
  if(PassingCadenceChanged){
   // Explicit wall pacing avoids fixed-step benchmarks ignoring t.MaxFPS,
   // without injecting a long real delta into the bounded railway clock.
   // Anchor deadlines to recorded simulation ticks. Re-anchoring every frame
   // accumulated OS sleep overshoot into seconds of recording drift. Late
   // frames do not skip/resize simulation steps or change acceptance tolerances.
   const double Target=PassingRecordStart+double(World.Current().tick-PassingRecordStartTick)/30.;
   const double Delay=PassingRecording?Target-FPlatformTime::Seconds():0;
   if(Delay>0)FPlatformProcess::Sleep(float(Delay));
  }
  const auto* Opposing=World.Spatial(Rail::TrainId(3));
  if(!Opposing||Opposing->Current().vehicles.empty()||Player.Current().vehicles.empty())return;
  const auto& P=Player.Current().vehicles.front();const auto& O=Opposing->Current().vehicles.front();
  const FVector Delta(O.bodyCentre.x-P.bodyCentre.x,O.bodyCentre.y-P.bodyCentre.y,O.bodyCentre.z-P.bodyCentre.z);
  const FVector PF(P.bodyForward.x,P.bodyForward.y,P.bodyForward.z),OF(O.bodyForward.x,O.bodyForward.y,O.bodyForward.z);
  PassingSigned=FVector::DotProduct(Delta,PF);PassingDistance=Delta.Size();
  if(FMath::Abs(PassingSigned)<1200){
   PassingSeen=true;PassingMinimum=FMath::Min(PassingMinimum,PassingDistance);
   const auto* PH=World.Handling(Rail::TrainId(2));const auto* OH=World.Handling(Rail::TrainId(3));
   const double PS=PH?PH->speedMps:0,OS=OH?OH->speedMps:0,Direction=FVector::DotProduct(PF,OF);
   PassingDirectionValid&=Direction<-.8;PassingMoving&=FMath::Abs(OS)>.5;
   PassingSamples+=FString::Printf(TEXT("{\"tick\":%llu,\"wallSeconds\":%.6f,\"signedMetres\":%.6f,\"distanceMetres\":%.6f,\"forwardDot\":%.6f,\"playerSpeedMps\":%.6f,\"opposingSpeedMps\":%.6f}\n"),World.Current().tick,FPlatformTime::Seconds()-Started,PassingSigned,PassingDistance,Direction,PS,OS);
   if(Resumed&&!PassingRecording&&!PassingExporting&&!PassingAudioReady&&FParse::Param(FCommandLine::Get(),TEXT("TambaramRecordAudio"))){
    PreviousFixedDelta=FApp::GetFixedDeltaTime();PreviousFixedStep=FApp::UseFixedTimeStep();PreviousMaxFps=GEngine->GetMaxFPS();
    // Keep deterministic 30 Hz steps, explicitly paced against wall time.
    FApp::SetFixedDeltaTime(1./30);FApp::SetUseFixedTimeStep(true);GEngine->SetMaxFPS(0);PassingCadenceChanged=true;
    UAudioMixerBlueprintLibrary::StartRecordingOutput(W,180);PassingRecording=true;
    PassingRecordStart=FPlatformTime::Seconds();PassingRecordStartTick=World.Current().tick;PassingRecordStartDistance=PassingSigned;
   }
  }
  if(PassingRecording&&PassingSigned < -1200){
   FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"wallSeconds\":%.6f,\"simulationSeconds\":%.6f,\"startSignedMetres\":%.6f,\"endSignedMetres\":%.6f,\"afterStreamingReload\":true}"),FPlatformTime::Seconds()-PassingRecordStart,double(World.Current().tick-PassingRecordStartTick)/30.,PassingRecordStartDistance,PassingSigned),*(Output/TEXT("passing-recording-window.json")));
   UAudioMixerBlueprintLibrary::StopRecordingOutput(W,EAudioRecordingExportType::WavFile,TEXT("passing-service-audio"),Output);
   PassingRecording=false;PassingExporting=true;PassingExportStarted=FPlatformTime::Seconds();
  }
 }
 double MaximumBogieErrorCm=0;
 bool BogiePresentationPassed=true;
 double MaximumWheelErrorCm=0,MaximumWheelAngleError=0;
 bool WheelPresentationPassed=true;
 void CheckBogies(UWorld* W,const Rail::Simulation& Train){
  TArray<FVector> Front,Rear;
  for(TActorIterator<ARailChunkView> It(W);It;++It)if(It->CoachParts.Num()==7){
   for(int32 B=0;B<2;++B){auto* Part=It->CoachParts[B+1].Get();if(!Part)continue;
    for(int32 I=0;I<Part->GetInstanceCount();++I){FTransform Pose;if(Part->GetInstanceTransform(I,Pose,true))(B==1?Front:Rear).Add(Pose.GetLocation());}
   }
  }
  for(const auto& V:Train.Current().vehicles)if(V.definition==Rail::DefinitionId(Rail::FormationContent::LhbDefinitionId))
   for(int32 B=0;B<2;++B){const auto Expected=B?V.frontBogie:V.rearBogie;const FVector P(Expected.x*100,Expected.y*100,Expected.z*100);double Best=1e30;
    for(const auto& Actual:B?Front:Rear)Best=FMath::Min(Best,FVector::Distance(P,Actual));
    MaximumBogieErrorCm=FMath::Max(MaximumBogieErrorCm,Best);BogiePresentationPassed&=Best<2.0;
   }
 }
 void CheckWheels(UWorld* W,const Rail::Simulation& Train){
  // Inspect rendered HISM transforms, not only logical bogie centres.
  TArray<FTransform> Wheels;
  for(TActorIterator<ARailChunkView> It(W);It;++It)if(It->CoachParts.Num()==7)
   for(int32 A=3;A<7;++A)if(auto* Part=It->CoachParts[A].Get())
    for(int32 I=0;I<Part->GetInstanceCount();++I){FTransform Pose;if(Part->GetInstanceTransform(I,Pose,true))Wheels.Add(Pose);}
  for(const auto& V:Train.Current().vehicles)if(V.definition==Rail::DefinitionId(Rail::FormationContent::LhbDefinitionId))
   for(int32 B=0;B<2;++B){
    Rail::Sample Sample;const double Offset=B?Rail::FormationContent::BogieOffset:-Rail::FormationContent::BogieOffset;
    if(!Train.History().SampleAt(Train.Network(),V.centreStationMetres+Offset,Sample).ok){WheelPresentationPassed=false;continue;}
    const FQuat Q=FRotationMatrix::MakeFromXZ(FVector(Sample.tangent.x,Sample.tangent.y,Sample.tangent.z),FVector::UpVector).ToQuat();
    const auto Bogie=B?V.frontBogie:V.rearBogie;const FVector Centre(Bogie.x*100,Bogie.y*100,Bogie.z*100);
    const FQuat ExpectedRotation=Q*FQuat(FVector(0,1,0),RailWheelPresentation::Angle(Train.Current().headStationMetres,.4575));
    for(double X:{-128.,128.}){
     const FVector Expected=Centre+Q.RotateVector(FVector(X,0,45.75));double Best=1e30,Angle=PI;
     for(const auto& Actual:Wheels){const double Error=FVector::Distance(Expected,Actual.GetLocation());if(Error<Best){Best=Error;Angle=ExpectedRotation.AngularDistance(Actual.GetRotation());}}
     MaximumWheelErrorCm=FMath::Max(MaximumWheelErrorCm,Best);MaximumWheelAngleError=FMath::Max(MaximumWheelAngleError,Angle);
     WheelPresentationPassed&=Best<.1&&Angle<.002;
    }
   }
 }
 TArray<TWeakObjectPtr<ARailChunkView>> OldChunks;
 uint64 LastTick=0;double LastProgress=0;
 TWeakObjectPtr<ACameraActor> Exterior;
 FVector FollowLocation=FVector::ZeroVector,FollowTarget=FVector::ZeroVector;
 bool FollowTraffic=false,FollowEnabled=false;int32 FollowVehicle=0;
 TWeakObjectPtr<URailSimulationSubsystem> Sim;
 bool Finish(bool Passed,const FString& Reason){
  RestorePassingCadence();
  const bool PassingPassed=!Full||(PassingStage==3&&PassingSeen&&PassingDirectionValid&&PassingMoving&&PassingMinimum<50);
  Passed&=PassingPassed&&(!Full||!FParse::Param(FCommandLine::Get(),TEXT("TambaramRecordAudio"))||PassingAudioReady);
  FFileHelper::SaveStringToFile(PassingSamples,*(Output/TEXT("passing-service-samples.jsonl")));
  FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"synthetic\":false,\"trainId\":3,\"captures\":%d,\"minimumHeadDistanceMetres\":%.6f,\"oppositeDirection\":%s,\"moving\":%s,\"audioExportReady\":%s}"),PassingPassed?TEXT("true"):TEXT("false"),PassingStage,PassingMinimum,PassingDirectionValid?TEXT("true"):TEXT("false"),PassingMoving?TEXT("true"):TEXT("false"),PassingAudioReady?TEXT("true"):TEXT("false")),*(Output/TEXT("passing-service.json")));
  FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"maximumBogiePositionErrorCm\":%.6f,\"toleranceCm\":2}"),BogiePresentationPassed?TEXT("true"):TEXT("false"),MaximumBogieErrorCm),*(Output/TEXT("bogie-presentation.json")));
  FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"maximumWheelPositionErrorCm\":%.6f,\"maximumPhaseErrorRadians\":%.6f,\"positionToleranceCm\":0.1,\"phaseToleranceRadians\":0.002,\"physicalContactSolver\":false}"),WheelPresentationPassed?TEXT("true"):TEXT("false"),MaximumWheelErrorCm,MaximumWheelAngleError),*(Output/TEXT("wheel-presentation.json")));
  Passed&=BogiePresentationPassed&&WheelPresentationPassed;
  const bool Interrupted=Sim.IsValid()&&Sim->ReadOnlyService().Current().phase==Rail::ServicePhase::Running&&(Sim->IsServicePaused()||Sim->IsPresentationBlocked());
  // Explicit static review: never count a paused picture as a moving journey.
  // The ordinary/full journey guard remains strict.
  const bool StaticUnchanged=SceneryOnly&&Sim.IsValid()&&Sim->IsServicePaused()&&!Sim->IsPresentationBlocked()&&!SceneryState.IsEmpty()&&SceneryState==UTF8_TO_TCHAR(Sim->ReadOnlyOperations().Json().c_str());
  Passed&=SceneryOnly?StaticUnchanged:!Interrupted;
  if(SceneryOnly)FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"staticVisualReview\":true,\"reviewKind\":\"%s\",\"liveJourneyQualification\":false,\"stateUnchanged\":%s}"),CabOnly?TEXT("cab"):TrackOnly?TEXT("track"):TEXT("scenery"),StaticUnchanged?TEXT("true"):TEXT("false")),*(Output/TEXT("review-scope.json")));
  if(SceneryOnly)FFileHelper::SaveStringToFile(TEXT("{\"assessed\":false,\"reason\":\"Static views do not verify moving traffic\"}"),*(Output/TEXT("passing-service.json")));
  const FString FinalReason=SceneryOnly?(Passed?(CabOnly?TEXT("Isolated static cab, instrument zoom and lamp review; not a live traversal"):TrackOnly?TEXT("Isolated static track detail and assigned material review; not a live traversal"):TEXT("Isolated static scenery and camera review; not a live traversal")):Reason):(Interrupted?TEXT("Review interrupted by a paused or blocked running service"):Reason);
  FString State=Sim.IsValid()?UTF8_TO_TCHAR(Sim->ReadOnlyService().Json().c_str()):TEXT("null");
  const FString Json=FString::Printf(TEXT("{\"passed\":%s,\"fullJourneyRequested\":%s,\"boundarySeen\":%s,\"cycleRequested\":%s,\"captures\":%d,\"reason\":\"%s\",\"service\":%s,\"androidTested\":false,\"realTimePerformanceClaimed\":false}"),Passed?TEXT("true"):TEXT("false"),Full?TEXT("true"):TEXT("false"),Boundary?TEXT("true"):TEXT("false"),Cycled?TEXT("true"):TEXT("false"),Captures,*FinalReason,*State);
  FFileHelper::SaveStringToFile(Json,*(Output/TEXT("result.json")));FPlatformMisc::RequestExit(false);return false;
 }
 bool Tick(float){
  const double Now=FPlatformTime::Seconds();
  if(Now-Started>(Full?5400:240))return Finish(false,TEXT("Review timeout"));
  if(!Sim.IsValid()){
   for(const auto& Context:GEngine->GetWorldContexts())if(auto* W=Context.World())if(W->IsGameWorld()&&W->HasBegunPlay()){
    auto* S=W->GetSubsystem<URailSimulationSubsystem>();if(S&&S->IsServiceMode()&&S->ReadOnlyService().Definition().terminalStop){Sim=S;LastProgress=Now;break;}
   }return true;
  }
  auto* S=Sim.Get();auto* W=S->GetWorld();const auto& Service=S->ReadOnlyService();const auto& World=S->ReadOnlyOperations();
  if(PassingExporting){
   const double AudioNow=FPlatformTime::Seconds();const int64 Bytes=IFileManager::Get().FileSize(*(Output/TEXT("passing-service-audio.wav")));
   if(Bytes>44){if(Bytes!=PassingAudioBytes){PassingAudioBytes=Bytes;PassingAudioStableSince=AudioNow;}else if(AudioNow-PassingAudioStableSince>1){PassingAudioReady=true;PassingExporting=false;RestorePassingCadence();}}
   if(PassingExporting&&AudioNow-PassingExportStarted>30)return Finish(false,TEXT("Opposing service audio export failed or incomplete"));
  }
  if(World.Faulted())return Finish(false,TEXT("Logical world fault"));
  const auto Tick=World.Current().tick;if(Tick!=LastTick){LastTick=Tick;LastProgress=Now;}
  if(Now-LastProgress>150)return Finish(false,TEXT("No logical progress for 150 seconds"));
  bool Ready=false;for(TActorIterator<ARailTambaramCorridorView> It(W);It;++It)Ready|=It->IsReady();
  if(!Ready||!S->HasLocomotivePresentation()||S->IsPresentationBlocked())return true;
  if(SceneryOnly&&SceneryState.IsEmpty()){
   FString ExpectedState;
   if(!FFileHelper::LoadFileToString(ExpectedState,*(Output+TEXT("-logical.json")))||ExpectedState!=UTF8_TO_TCHAR(World.Json().c_str()))return Finish(false,TEXT("Recovered static review fixture differs from exported state"));
   if(Service.Current().phase!=Rail::ServicePhase::Running||Service.Current().accumulator>4./30.+1e-9)return Finish(false,TEXT("Static review requires a valid recovered running checkpoint"));
   if(!S->IsServicePaused())S->ServiceAction(2);
   SceneryState=UTF8_TO_TCHAR(World.Json().c_str());
  }
  const auto* Train=World.Spatial(Service.Definition().player);if(!Train)return true;
  const double Head=Train->Current().headStationMetres;
  ObservePassing(W,World,*Train);
  // Keep exterior review cameras moving throughout the journey, not fixed at
  // the previous screenshot location while the train drives out of view.
  if(Exterior.IsValid()&&FollowEnabled&&Service.Current().phase!=Rail::ServicePhase::Result){
   const auto* Follow=FollowTraffic?World.Spatial(Service.Definition().traffic):Train;
   if(Follow&&Follow->Current().vehicles.size()>static_cast<size_t>(FollowVehicle)){
    const auto& V=Follow->Current().vehicles[FollowVehicle];
    const FVector P(V.bodyCentre.x*100,V.bodyCentre.y*100,V.bodyCentre.z*100);
    const FQuat Q=FRotationMatrix::MakeFromXZ(FVector(V.bodyForward.x,V.bodyForward.y,V.bodyForward.z),FVector::UpVector).ToQuat();
    const FVector L=P+Q.RotateVector(FollowLocation),T=P+Q.RotateVector(FollowTarget);
    Exterior->SetActorLocation(L);Exterior->SetActorRotation((T-L).Rotation());
   }
  }
  bool Spanning=false;double Min=1e30,Max=-1e30;
  for(const auto& V:Train->Current().vehicles){Min=FMath::Min(Min,FMath::Min(V.frontReference.x,V.rearReference.x));Max=FMath::Max(Max,FMath::Max(V.frontReference.x,V.rearReference.x));}
  Spanning=Min<0&&Max>=0;Boundary|=Spanning;
  if(!Pending.IsEmpty()){
   if(CaptureWarmup>0){
    if(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())return true;
    if(--CaptureWarmup==0){
     CheckBogies(W,*Train);
     CheckWheels(W,*Train);
     const auto* Loco=S->PlayerLocomotive();const FString CabTelemetry=Loco?Loco->TelemetryJson():TEXT("null");
     FFileHelper::SaveStringToFile(CabTelemetry,*(Output/FString::Printf(TEXT("%02d-locomotive.json"),Captures-1)));
     if((CabOnly||(TrackOnly&&PendingCameraMode==17))&&!CabFrameValid(CabTelemetry))return Finish(false,TEXT("Cab camera, instrument zoom or lamp differs from requested view"));
     const FString Frame=FString::Printf(TEXT("{\"label\":\"%s\",\"headMetres\":%.6f,\"cameraMode\":%d,\"tick\":%llu,\"cameraWarmupFrames\":%d}"),*FPaths::GetBaseFilename(Pending),Head,PendingCameraMode,World.Current().tick,(CabOnly||TrackOnly)?30:8);
     FFileHelper::SaveStringToFile(Frame,*(Output/FString::Printf(TEXT("%02d-frame.json"),Captures-1)));
     FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(World.Json().c_str()),*(Output/FString::Printf(TEXT("%02d-logical.json"),Captures-1)));
     FScreenshotRequest::RequestScreenshot(Pending,!Full&&!SceneryOnly,false);
    }
    return true;
   }
   if(IFileManager::Get().FileSize(*Pending)<=0){if(Now-LastCapture>20)return Finish(false,TEXT("Screenshot missing"));return true;}
   Pending.Empty();
   if(CyclePending){
    BeforeCycle=UTF8_TO_TCHAR(World.Json().c_str());
    for(TActorIterator<ARailChunkView> It(W);It;++It)OldChunks.Add(*It);
    S->SaveS6Checkpoint();S->CycleS6Chunks();Cycled=true;CyclePending=false;CycleStarted=Now;
    return true;
   }
  }
  if(Cycled&&!Resumed){
   if(Now-CycleStarted<6||S->IsPresentationBlocked())return true;
   bool Replaced=true;for(const auto& Old:OldChunks)if(Old.IsValid())
    for(TActorIterator<ARailChunkView> It(W);It;++It)if(*It==Old.Get())Replaced=false;
   ReloadVerified=Replaced&&BeforeCycle==UTF8_TO_TCHAR(World.Json().c_str());
   FFileHelper::SaveStringToFile(ReloadVerified?TEXT("{\"passed\":true,\"stateUnchanged\":true,\"actorsReplaced\":true}"):TEXT("{\"passed\":false}"),*(Output/TEXT("streaming-reload.json")));
   if(!ReloadVerified)return Finish(false,TEXT("Streaming reload changed state or retained active chunk actors"));
   S->ServiceAction(0);Resumed=true;
  }
  if(Service.Current().phase==Rail::ServicePhase::Result){
   if(PassingExporting)return true;
   if(!Service.Current().success)return Finish(false,TEXT("Service failed"));
   if(Full&&TerminalCaptures<2){
    if(Now-LastCapture<3)return true;
    auto* PC=W->GetFirstPlayerController();if(!PC)return true;
    if(TerminalCaptures==0){
     if(auto* Loco=S->PlayerLocomotive())if(Loco->IsCabCamera())Loco->ToggleCamera();
     if(!Exterior.IsValid())Exterior=W->SpawnActor<ACameraActor>();
     const auto& V=Train->Current().vehicles.front();const FVector P(V.bodyCentre.x*100,V.bodyCentre.y*100,V.bodyCentre.z*100);
     const FVector Forward(V.bodyForward.x,V.bodyForward.y,0),Right(-Forward.Y,Forward.X,0);
     const FVector Location=P-Forward*6500+Right*5500+FVector(0,0,2400);
     Exterior->SetActorLocation(Location);Exterior->SetActorRotation((P-Forward*5000-Location).Rotation());PC->SetViewTarget(Exterior.Get());
    }else if(auto* Loco=S->PlayerLocomotive()){if(!Loco->IsCabCamera())Loco->ToggleCamera();PC->SetViewTarget(Loco);}
    Pending=Output/FString::Printf(TEXT("%02d-terminal-%d.png"),Captures++,TerminalCaptures++);LastCapture=Now;
    PendingCameraMode=TerminalCaptures==1?1:0;CaptureWarmup=8;
    if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();return true;
   }
   if(!Service.Current().settled){S->ServiceAction(0);if(!S->ReadOnlyService().Current().settled)return Finish(false,TEXT("Terminal settlement failed; review saves retained"));return true;}
   return Finish(Full&&Boundary&&ReloadVerified&&NextStation==14&&SceneryCaptures>=7&&BridgeCaptures==2&&BoundaryAfter&&JunctionCaptured,TEXT("Terminal result settled with station, bridge and corridor captures"));
  }
  if(!Full&&Captures>=4){
   if((TrackOnly||SceneryOnly)&&!CheckTrackAssets(W))return Finish(false,TEXT("Required track/nature meshes or assigned materials missing from populated components"));
   return Finish(true,TEXT("Origin presentation smoke review only"));
  }
  const bool StationDue=Full&&NextStation<14&&Head>Rail::TambaramRoute::StationMetres[NextStation]-120;
  const double BridgeStarts[]={3675.88,9606.31};
  const bool BridgeDue=Full&&BridgeCaptures<2&&Head>BridgeStarts[BridgeCaptures]+25;
  const auto* Traffic=World.Spatial(Service.Definition().traffic);
  const bool JunctionDue=Full&&!JunctionCaptured&&Traffic&&(Traffic->Current().head.edge==Rail::EdgeId(90000)||Traffic->Current().head.edge==Rail::EdgeId(90001));
  const bool SceneryDue=Full&&Head>NextScenery&&NextScenery<Service.Definition().berthHead-600;
  const bool ApproachDue=Full&&!ApproachCaptured&&Head>Service.Definition().berthHead-300;
  const bool PassingDue=Full&&((PassingStage==0&&PassingSigned>80&&PassingSigned<500)||(PassingStage==1&&FMath::Abs(PassingSigned)<40)||(PassingStage==2&&PassingSigned < -400));
  const bool Need=Captures<4||PassingDue||JunctionDue||(Full&&Spanning&&!Cycled)||(Resumed&&!BoundaryAfter)||BridgeDue||StationDue||SceneryDue||ApproachDue;
  // Accelerated journeys can leave a station camera's streamed corridor during
  // a long wall-clock pause. Distance gates choose views; rendered frames warm up.
  if(!Need||(!PassingDue&&Now-LastCapture<(Full?.5:5)))return true;
  auto* PC=W->GetFirstPlayerController();if(!PC)return true;
  FString Label;int32 CameraMode=1;
  if(Captures<4){Label=TEXT("origin");CameraMode=(Captures==0||Captures==3)?0:Captures;}
  else if(PassingDue){const TCHAR* Labels[]={TEXT("opposing-approach"),TEXT("opposing-pass"),TEXT("opposing-departed")};Label=Labels[PassingStage++];CameraMode=12;}
  else if(JunctionDue){Label=TEXT("ai-crossover");CameraMode=6;JunctionCaptured=true;}
  else if(Spanning&&!Cycled){Label=TEXT("boundary-before");CameraMode=3;}
  else if(Resumed&&!BoundaryAfter){Label=TEXT("boundary-after-lookback");CameraMode=3;BoundaryAfter=true;}
  else if(BridgeDue){Label=FString::Printf(TEXT("bridge-%02d"),++BridgeCaptures);CameraMode=11;}
  else if(StationDue){Label=FString::Printf(TEXT("station-%02d"),NextStation++);CameraMode=5;}
  else if(SceneryDue){Label=FString::Printf(TEXT("corridor-%05d"),int32(NextScenery));CameraMode=SceneryCaptures%3==0?4:SceneryCaptures%3==1?3:1;NextScenery+=3000;++SceneryCaptures;}
  else {Label=TEXT("tambaram-approach");CameraMode=0;ApproachCaptured=true;}
  if(FParse::Param(FCommandLine::Get(),TEXT("TambaramWheelReview"))&&Label==TEXT("origin")){const int32 Modes[]={7,4,10,4};CameraMode=Modes[Captures];Label=TEXT("wheel-detail");}
  if(FParse::Param(FCommandLine::Get(),TEXT("TambaramArtReview"))&&Label==TEXT("origin")){const int32 Modes[]={8,9,8,1};CameraMode=Modes[Captures];Label=TEXT("art-detail");}
  if(SceneryOnly){const int32 Modes[]={13,14,15,16};CameraMode=Modes[Captures];Label=TEXT("scenery");}
  if(CabOnly){
   const TCHAR* Labels[]={TEXT("cab-wide-lamp-off"),TEXT("cab-instruments-lamp-off"),TEXT("cab-wide-lamp-on"),TEXT("cab-instruments-lamp-on")};
   CameraMode=17+Captures;Label=Labels[Captures];
  }
  if(TrackOnly){
   const int32 Modes[]={17,15,21,TrackCrossover?9:22};
   const TCHAR* Labels[]={TEXT("track-from-cab"),TEXT("track-forward"),TEXT("track-sleepers-gravel"),TrackCrossover?TEXT("track-crossover"):TEXT("track-shoulder-ground")};
   CameraMode=Modes[Captures];Label=Labels[Captures];
  }
  if(CabOnly||(TrackOnly&&CameraMode==17)){
   FollowEnabled=false;auto* Loco=S->PlayerLocomotive();
   if(!ConfigureCabView(Loco,CameraMode))return Finish(false,TEXT("Could not configure the actual cab camera for review"));
   PC->SetViewTarget(Loco);
  }else if(CameraMode==15||CameraMode==16){
   FollowEnabled=false;
   if(auto* Loco=S->PlayerLocomotive()){
    if(!Loco->IsCabCamera())Loco->ToggleCamera();
    Loco->CycleJourneyCamera();if(CameraMode==16)Loco->CycleJourneyCamera();PC->SetViewTarget(Loco);
   }
  }else if(CameraMode==0){
   FollowEnabled=false;
   if(auto* Loco=S->PlayerLocomotive())if(Loco->IsReady()){
    if(!Loco->IsCabCamera())Loco->ToggleCamera();PC->SetViewTarget(Loco);
   }
  }else{
   if(auto* Loco=S->PlayerLocomotive())if(Loco->IsCabCamera())Loco->ToggleCamera();
   if(!Exterior.IsValid())Exterior=W->SpawnActor<ACameraActor>();
   const auto& V=(CameraMode==6&&Traffic?Traffic:Train)->Current().vehicles.front();const FVector P(V.bodyCentre.x*100,V.bodyCentre.y*100,V.bodyCentre.z*100);
   const FVector Forward(V.bodyForward.x,V.bodyForward.y,V.bodyForward.z),Right(-Forward.Y,Forward.X,0);
   FVector Location=P+Right*(CameraMode==1?-2100:4500)-Forward*2500+FVector(0,0,1400),Target=P+FVector(0,0,180);
   if(CameraMode==13||CameraMode==14){const double Side=CameraMode==13?-1:1;Location=P+Right*Side*900+FVector(0,0,680);Target=P+Forward*1800+Right*Side*5000+FVector(0,0,220);}
   if(CameraMode==3){Location=P-Forward*1200+Right*750+FVector(0,0,420);Target=P-Forward*11000+FVector(0,0,100);}
   if(CameraMode==12){Location=P-Forward*2200+Right*4200+FVector(0,0,1200);Target=P+Forward*(PassingStage==1?15000:PassingStage==3?-15000:0)+FVector(0,0,180);}
   if(CameraMode==4&&Train->Current().vehicles.size()>3){const auto& Coach=Train->Current().vehicles[3];Target=FVector(Coach.frontBogie.x*100,Coach.frontBogie.y*100,Coach.frontBogie.z*100)+FVector(0,0,75);Location=Target-Right*580-Forward*280+FVector(0,0,70);}
   if(CameraMode==10&&Train->Current().vehicles.size()>3){const auto& Coach=Train->Current().vehicles[3];Target=FVector(Coach.bodyCentre.x*100,Coach.bodyCentre.y*100,Coach.bodyCentre.z*100)+FVector(0,0,150);Location=Target-Right*2000-Forward*1100+FVector(0,0,250);}
   if(CameraMode==7){Target=P+Forward*580+FVector(0,0,60);Location=Target-Right*480-Forward*190+FVector(0,0,60);}
   if(CameraMode==8){const auto Board=Rail::TambaramRoute::SampleRoute(1,Rail::TambaramRoute::StationMetres[0]-60);const FVector F(Board.tangent.x,Board.tangent.y,0),R(-F.Y,F.X,0);Target=FVector(Board.position.x*100,Board.position.y*100,Board.position.z*100)-R*580+FVector(0,0,240);Location=Target+R*(Captures==2?650:-650)+FVector(0,0,30);}
   if(CameraMode==9){const auto Toe=Rail::TambaramRoute::CrossoverGeometry(false).At(12);const FVector F(Toe.tangent.x,Toe.tangent.y,0),R(-F.Y,F.X,0);Target=FVector(Toe.position.x*100,Toe.position.y*100,Toe.position.z*100);Location=Target+(TrackOnly?R*1500:R*-850)-F*(TrackOnly?1100:900)+FVector(0,0,TrackOnly?1600:500);}
   if(CameraMode==21||CameraMode==22){
    // Adjacent ordinary track, close to the recovered player and within its
    // streamed cells. This avoids the locomotive occluding sleepers/shoulders.
    const auto Detail=Rail::TambaramRoute::SampleRoute(0,Head+(CameraMode==21?18.:35.));
    const FVector Centre(Detail.position.x*100,Detail.position.y*100,Detail.position.z*100);
    const FVector F(Detail.tangent.x,Detail.tangent.y,Detail.tangent.z),R(-F.Y,F.X,0);
    if(CameraMode==21){Target=Centre+F*200+FVector(0,0,5);Location=Centre-F*500+R*260+FVector(0,0,170);}
    else{Target=Centre+F*450-R*175+FVector(0,0,-35);Location=Centre-F*150-R*280+FVector(0,0,80);}
   }
   if(CameraMode==11){
    const auto Bridge=Rail::TambaramRoute::SampleRoute(1,BridgeStarts[BridgeCaptures-1]+40);
    const FVector F(Bridge.tangent.x,Bridge.tangent.y,0),R(-F.Y,F.X,0);
    Target=FVector(Bridge.position.x*100,Bridge.position.y*100,Bridge.position.z*100);
    Location=Target-R*12500-F*14000+FVector(0,0,5500);
   }
   if(CameraMode==5){
    Rail::Sample Station;
    if(Rail::SampleRoute(Train->Network(),Rail::TambaramRoute::Path(1),Rail::TambaramRoute::StationMetres[NextStation-1],Station).ok){
     const FVector Centre(Station.position.x*100,Station.position.y*100,Station.position.z*100);
     const FVector F(Station.tangent.x,Station.tangent.y,Station.tangent.z),R(-F.Y,F.X,0);
     Location=Centre+R*5500-F*15000+FVector(0,0,2600);Target=Centre+R*1300;
    }
   }
   FollowTraffic=CameraMode==6;FollowVehicle=(CameraMode==4||CameraMode==10)?3:0;FollowEnabled=CameraMode!=5&&CameraMode!=8&&CameraMode!=9&&CameraMode!=11&&CameraMode!=21&&CameraMode!=22;
   const auto& Anchor=(FollowTraffic?Traffic:Train)->Current().vehicles[FollowVehicle];
   const FVector AnchorP(Anchor.bodyCentre.x*100,Anchor.bodyCentre.y*100,Anchor.bodyCentre.z*100);
   const FQuat AnchorQ=FRotationMatrix::MakeFromXZ(FVector(Anchor.bodyForward.x,Anchor.bodyForward.y,Anchor.bodyForward.z),FVector::UpVector).ToQuat();
   FollowLocation=AnchorQ.UnrotateVector(Location-AnchorP);FollowTarget=AnchorQ.UnrotateVector(Target-AnchorP);
   Exterior->SetActorLocation(Location);Exterior->SetActorRotation((Target-Location).Rotation());PC->SetViewTarget(Exterior.Get());
  }
  Pending=Output/FString::Printf(TEXT("%02d-%s.png"),Captures++,*Label);LastCapture=Now;
  CheckBogies(W,*Train);
  const FString Frame=FString::Printf(TEXT("{\"label\":\"%s\",\"headMetres\":%.6f,\"cameraMode\":%d,\"tick\":%llu}"),*Label,Head,CameraMode,World.Current().tick);
  FFileHelper::SaveStringToFile(Frame,*(Output/FString::Printf(TEXT("%02d-frame.json"),Captures-1)));
  FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(World.Json().c_str()),*(Output/FString::Printf(TEXT("%02d-logical.json"),Captures-1)));
  PendingCameraMode=CameraMode;CaptureWarmup=(CabOnly||TrackOnly)?30:8;
  if(PC->PlayerCameraManager)PC->PlayerCameraManager->SetGameCameraCutThisFrame();
  if(Spanning&&!Cycled&&!SceneryOnly)CyclePending=true;
  return true;
 }
};
struct FWheelMotionReview {
 FString Output,Before,Pending;Rail::SnapshotRecord BeforeRecord;double Started=FPlatformTime::Seconds();
 TWeakObjectPtr<URailSimulationSubsystem> Owner;TWeakObjectPtr<ACameraActor> Camera;
 Rail::OperationsWorld Railway;FTambaramReview Audit;
 int32 Stage=0,Frames=0,Warmup=0;bool Ready=false;double ReverseStart=0;
 bool Recording=false,Exporting=false;double ExportStarted=0,LastAudioStep=0,EmergencyTailUntil=0;
 bool Finish(bool Pass,const TCHAR* Reason){
  Pass&=Audit.BogiePresentationPassed&&Audit.WheelPresentationPassed;
  Rail::SnapshotRecord AfterRecord;
  const bool Unchanged=Owner.IsValid()&&Rail::WorldSnapshotCodec::Capture(Owner->ReadOnlyOperations(),1,AfterRecord).ok&&BeforeRecord.Bytes()==AfterRecord.Bytes();Pass&=Unchanged;
  FFileHelper::SaveStringToFile(FString::Printf(TEXT("{\"passed\":%s,\"captures\":%d,\"forwardReverseBraking\":true,\"ownerStateUnchanged\":%s,\"maximumWheelPositionErrorCm\":%.6f,\"maximumPhaseErrorRadians\":%.6f,\"reason\":\"%s\"}"),Pass?TEXT("true"):TEXT("false"),Frames,Unchanged?TEXT("true"):TEXT("false"),Audit.MaximumWheelErrorCm,Audit.MaximumWheelAngleError,Reason),*(Output/TEXT("result.json")));
  if(Recording&&Owner.IsValid()){
   UAudioMixerBlueprintLibrary::StopRecordingOutput(Owner->GetWorld(),EAudioRecordingExportType::WavFile,TEXT("motion-audio"),Output);
   Recording=false;Exporting=true;ExportStarted=FPlatformTime::Seconds();return true;
  }
  FPlatformMisc::RequestExit(false);return false;
 }
 void Command(Rail::CommandKind K,double V){if(!Railway.Command(Rail::TrainId(2),K,V).ok)Stage=-1;}
 void Shot(const TCHAR* Label,int32 Distance=0){
  const auto& V=Railway.Spatial(Rail::TrainId(2))->Current().vehicles[3];
  const FVector P(V.frontBogie.x*100,V.frontBogie.y*100,V.frontBogie.z*100),F(V.bodyForward.x,V.bodyForward.y,0),R(-F.Y,F.X,0);
  const FVector Target=P+FVector(0,0,75),Location=Target-R*(Distance?1700:580)-F*(Distance?850:280)+FVector(0,0,Distance?200:70);
  Camera->SetActorLocation(Location);Camera->SetActorRotation((Target-Location).Rotation());
  Pending=Output/FString::Printf(TEXT("%02d-%s.png"),Frames++,Label);Warmup=12;
 }
 bool Tick(float){
  if(Exporting){if(IFileManager::Get().FileSize(*(Output/TEXT("motion-audio.wav")))>44||FPlatformTime::Seconds()-ExportStarted>15){FPlatformMisc::RequestExit(false);return false;}return true;}
  if(FPlatformTime::Seconds()-Started>600)return Finish(false,TEXT("Motion review timeout"));
  if(!Owner.IsValid())for(const auto& Context:GEngine->GetWorldContexts())if(auto* W=Context.World())if(W->IsGameWorld()&&W->HasBegunPlay())if(auto* S=W->GetSubsystem<URailSimulationSubsystem>())if(S->IsServiceMode())Owner=S;
  if(!Owner.IsValid())return true;auto* S=Owner.Get();auto* W=S->GetWorld();
  if(!Ready){
   const auto* Train=S->ReadOnlyOperations().Spatial(Rail::TrainId(2));
   if(!Train||Train->Current().headStationMetres<850||!S->HasLocomotivePresentation())return true;
   Before=UTF8_TO_TCHAR(S->ReadOnlyOperations().Json().c_str());if(!Rail::WorldSnapshotCodec::Capture(S->ReadOnlyOperations(),1,BeforeRecord).ok)return Finish(false,TEXT("Owner snapshot invalid"));Railway=S->ReadOnlyOperations();S->FreezeForWheelMotionReview();Ready=true;
   if(FParse::Param(FCommandLine::Get(),TEXT("TambaramRecordAudio"))){UAudioMixerBlueprintLibrary::StartRecordingOutput(W,180);Recording=true;LastAudioStep=FPlatformTime::Seconds();}
   Camera=W->SpawnActor<ACameraActor>();W->GetFirstPlayerController()->SetViewTarget(Camera.Get());Command(Rail::CommandKind::Throttle,0);Command(Rail::CommandKind::Brake,1);
  }
  if(Stage<0)return Finish(false,TEXT("Review command rejected"));
  // FixedTimeStep can bypass the engine's frame cap. Keep recorded control
  // changes at real 30 Hz so pneumatic events are not compressed in the WAV.
  if(Recording){const double Now=FPlatformTime::Seconds();if(Now-LastAudioStep<1.0/30)return true;LastAudioStep=Now;}
  if(Pending.IsEmpty()){
   const auto Result=Railway.Update(1.0/30);if(!Result.ok)return Finish(false,TEXT("Disposable railway fault"));
  }
  const auto* Train=Railway.Spatial(Rail::TrainId(2));const auto* H=Railway.Handling(Rail::TrainId(2));
  const auto* AudioEdge=Train->Network().Find(Train->Current().head.edge);
  S->PresentReviewAudio(*H,AudioEdge?AudioEdge->geometry.curvaturePerMetre:0,1.0f/30);
  for(TActorIterator<ARailChunkView> It(W);It;++It)It->Present(Railway,false,2,false);
  if(auto* L=S->PlayerLocomotive())L->Present(*Train,*H,1.0f/30,false);
  Audit.CheckBogies(W,*Train);Audit.CheckWheels(W,*Train);
  if(!Pending.IsEmpty()){
   if(Warmup>0){if(GShaderCompilingManager&&GShaderCompilingManager->IsCompiling())return true;
    if(--Warmup==0){FFileHelper::SaveStringToFile(UTF8_TO_TCHAR(Railway.Json().c_str()),*(Output/FString::Printf(TEXT("%02d-logical.json"),Frames-1)));FScreenshotRequest::RequestScreenshot(Pending,false,false);}return true;}
   if(IFileManager::Get().FileSize(*Pending)<=0)return true;Pending.Empty();
  }
  if(Stage==0&&H->speedMps==0){Shot(TEXT("stopped-detail"));Stage=1;Command(Rail::CommandKind::Brake,0);Command(Rail::CommandKind::Throttle,.2);}
  else if(Stage==1&&H->speedMps>=3){Shot(TEXT("forward-close"));Stage=2;}
  else if(Stage==2){Shot(TEXT("forward-distance"),1);Stage=3;Command(Rail::CommandKind::Throttle,0);Command(Rail::CommandKind::Brake,.65);}
  else if(Stage==3&&H->appliedBrake>.45&&H->speedMps>.1){Shot(TEXT("service-braking"));Stage=4;}
  else if(Stage==4&&H->speedMps==0){Shot(TEXT("stopped-before-reverse"));Stage=5;ReverseStart=Train->Current().headStationMetres;Command(Rail::CommandKind::Direction,-1);Command(Rail::CommandKind::Brake,0);Command(Rail::CommandKind::Throttle,.2);}
  else if(Stage==5&&H->speedMps<=-3){Shot(TEXT("reverse-close"));Stage=6;}
  else if(Stage==6){Shot(TEXT("reverse-distance"),1);Stage=7;Command(Rail::CommandKind::Throttle,0);Command(Rail::CommandKind::Emergency,1);}
  else if(Stage==7&&H->speedMps==0){Shot(TEXT("emergency-stopped"));Stage=8;EmergencyTailUntil=FPlatformTime::Seconds()+6;}
  else if(Stage==8&&(!Recording||FPlatformTime::Seconds()>=EmergencyTailUntil))return Finish(Frames==8&&Train->Current().headStationMetres<ReverseStart,TEXT("Forward, graduated service braking, reverse, emergency stop and displayed phase verified"));
  return true;
 }
};
void BeginTambaramReview(){
 FString Out;if(!FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram"))||!FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview"))||!FParse::Value(FCommandLine::Get(),TEXT("TambaramEvidence="),Out)||FPaths::IsRelative(Out))return;
 if(IFileManager::Get().DirectoryExists(*Out)||!IFileManager::Get().MakeDirectory(*Out,true))return;
 if(FParse::Param(FCommandLine::Get(),TEXT("TambaramMotionReview"))){auto Motion=MakeShared<FWheelMotionReview>();Motion->Output=Out;FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Motion](float Dt){return Motion->Tick(Dt);}));return;}
 auto Review=MakeShared<FTambaramReview>();Review->Output=Out;Review->Full=FParse::Param(FCommandLine::Get(),TEXT("TambaramFullJourney"));
 Review->CabOnly=FParse::Param(FCommandLine::Get(),TEXT("TambaramCabReview"));
 Review->TrackOnly=FParse::Param(FCommandLine::Get(),TEXT("TambaramTrackReview"));
 Review->TrackCrossover=Review->TrackOnly&&FParse::Param(FCommandLine::Get(),TEXT("TambaramTrackCrossover"));
 Review->SceneryOnly=Review->CabOnly||Review->TrackOnly||FParse::Param(FCommandLine::Get(),TEXT("TambaramSceneryReview"));if(Review->SceneryOnly)Review->Full=false;
 FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([Review](float Dt){return Review->Tick(Dt);}));
}
FAutoConsoleCommand TambaramReviewCommand(TEXT("Rail.TambaramDesktopReview"),TEXT("Isolated route desktop review; normal simulation movement only"),FConsoleCommandDelegate::CreateStatic(&BeginTambaramReview));
}
#endif
