#include "Misc/AutomationTest.h"
#include "RailLocomotiveView.h"
#include "RailSimulationSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailCabStabilizationTest,"IndianRailway.S8.CabStabilization",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailCabStabilizationTest::RunTest(const FString& Parameters){
 const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!TestNotNull(TEXT("Test world"),World))return false;
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* View=World->SpawnActor<ARailLocomotiveView>();FString Diagnostic;
 if(TestTrue(TEXT("Existing WAP7 loads without changing assembly"),View->LoadPresentation(TEXT("WAP7"),Diagnostic))){
  auto Read=[&](){TSharedPtr<FJsonObject> J;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(View->TelemetryJson()),J);return J;};
  // Inspect the components actually presented to the renderer: successful
  // assembly loading alone can silently retain the original fallback assets.
  TArray<UStaticMeshComponent*> MeshComponents;View->GetComponents<UStaticMeshComponent>(MeshComponents);
  // The editor adds a camera-proxy mesh which is never part of the game cab.
  MeshComponents.RemoveAll([](const UStaticMeshComponent* Component){return Component&&Component->IsEditorOnly();});
  int32 CoreCount=0;
  const FString CandidateCore=TEXT("/Game/IndianRailway/S9/CabRealismV1/Meshes/SM_WAP7_Cab_StaticCore_HardwareV1.SM_WAP7_Cab_StaticCore_HardwareV1");
  const TCHAR* RequiredMaterials[]={TEXT("DeskEnamel_StaticAO"),TEXT("CabIvory_StaticAO"),TEXT("PanelCharcoal_StaticAO"),TEXT("Rubber_StaticAO"),TEXT("Steel_StaticAO"),TEXT("InstrumentBezel"),TEXT("WindowGlass"),TEXT("InstrumentGlass")};
  TSet<FString> ObservedMaterials;
  for(auto* Component:MeshComponents){
   const UStaticMesh* Mesh=Component?Component->GetStaticMesh().Get():nullptr;if(!Mesh)continue;
   if(Mesh->GetName().StartsWith(TEXT("SM_WAP7_Cab_StaticCore"))){
    ++CoreCount;TestEqual(TEXT("Rendered cab uses hardware candidate instead of fallback"),Mesh->GetPathName(),CandidateCore);
   }
   if(!Mesh->GetName().StartsWith(TEXT("SM_WAP7_Cab_")))continue;
   for(int32 Slot=0;Slot<Component->GetNumMaterials();++Slot)if(const auto* Material=Component->GetMaterial(Slot)){
    for(const TCHAR* Required:RequiredMaterials)if(Material->GetName()==Required){
     const FString Expected=FString::Printf(TEXT("/Game/IndianRailway/S9/CabRealismV1/Materials/%s.%s"),Required,Required);
     TestEqual(FString::Printf(TEXT("Rendered %s uses candidate material instead of fallback"),Required),Material->GetPathName(),Expected);
     ObservedMaterials.Add(Required);
    }
   }
  }
  TestEqual(TEXT("Hardware replaces exactly one cab core component"),CoreCount,1);
  for(const TCHAR* Required:RequiredMaterials)TestTrue(FString::Printf(TEXT("Rendered cab contains required %s surface"),Required),ObservedMaterials.Contains(Required));
  FString ManifestText;TSharedPtr<FJsonObject> Manifest;const TArray<TSharedPtr<FJsonValue>>* ManifestParts=nullptr;
  if(TestTrue(TEXT("Original assembly remains readable for component budget"),FFileHelper::LoadFileToString(ManifestText,*(FPaths::ProjectContentDir()/TEXT("IndianRailway/Production/locomotives/WAP7.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(ManifestText),Manifest)&&Manifest.IsValid()&&Manifest->TryGetArrayField(TEXT("parts"),ManifestParts))){
   TestEqual(TEXT("Cab realism adds no components beyond original assembly and six wheelsets"),MeshComponents.Num(),ManifestParts->Num()+6);
   TestEqual(TEXT("All presented mesh components belong to the recorded assembly"),View->PartCount(),MeshComponents.Num());
  }
  // This standalone fixture is deliberately not in service mode: handles use
  // H.command here, while a running service uses its requestedPower/Brake intent.
  // Neither path should use the emergency-adjusted actuators for handle poses.
  if(auto* Subsystem=World->GetSubsystem<URailSimulationSubsystem>())TestFalse(TEXT("Standalone pose fixture is not a service"),Subsystem->IsServiceMode());
  auto CheckPose=[&](const TSharedPtr<FJsonObject>& State,const TCHAR* Role,const FVector& Axis,double Degrees){
   if(!TestTrue(TEXT("Pose telemetry is valid"),State.IsValid()))return;
   const TArray<TSharedPtr<FJsonValue>>* Poses=nullptr;
   if(!TestTrue(TEXT("Pose telemetry has control array"),State->TryGetArrayField(TEXT("controlPoses"),Poses)))return;
   for(const auto& Pose:*Poses){
    const auto O=Pose->AsObject();if(!O.IsValid()||O->GetStringField(TEXT("role"))!=Role)continue;
    const TArray<TSharedPtr<FJsonValue>>* R=nullptr;
    if(!TestTrue(FString::Printf(TEXT("%s rotation has three components"),Role),O->TryGetArrayField(TEXT("rotation"),R)&&R->Num()==3))return;
    const FQuat Actual=FRotator((*R)[0]->AsNumber(),(*R)[1]->AsNumber(),(*R)[2]->AsNumber()).Quaternion();
    const FQuat Expected(Axis,FMath::DegreesToRadians(Degrees));
    TestTrue(FString::Printf(TEXT("%s actual component pose at %.2f degrees"),Role,Degrees),FMath::RadiansToDegrees(Actual.AngularDistance(Expected))<.03);
    return;
   }
   AddError(FString::Printf(TEXT("Missing %s actual component pose"),Role));
  };
  Rail::Simulation S;auto Fixture=Rail::MakeFixture();TestTrue(TEXT("Placement fixture initializes"),S.Initialize(Fixture.graph,Fixture.definitions,Fixture.Long(),{Rail::EdgeId(1),1},200,Fixture.SelectUpper()).ok);
  Rail::LongitudinalState H;H.command.direction=1;H.command.throttle=.75;H.command.brake=0;H.command.emergency=true;H.appliedThrottle=0;H.appliedBrake=1;H.speedMps=5;
  View->ToggleCamera();View->Present(S,H,.2f,false);auto J=Read();
  TestTrue(TEXT("Cab selected"),J->GetBoolField(TEXT("cabCamera")));
  TestEqual(TEXT("Applied emergency brake presented"),J->GetNumberField(TEXT("appliedBrake")),1.0);
  TestEqual(TEXT("Emergency power cutout presented"),J->GetNumberField(TEXT("appliedPower")),0.0);
  TestTrue(TEXT("Emergency indication follows handling latch"),J->GetBoolField(TEXT("emergency")));
  TestEqual(TEXT("Speed gauge source is actual speed"),J->GetNumberField(TEXT("speedKmh")),18.0);
  CheckPose(J,TEXT("gauge_speed"),FVector(-1,0,0),-104);
  CheckPose(J,TEXT("throttle"),FVector(0,1,0),12);
  CheckPose(J,TEXT("train_brake"),FVector(0,0,1),-30);
  CheckPose(J,TEXT("reverser"),FVector(0,1,0),18);
  // The authored emergency button has [0,0] rotation: verify its indication,
  // but do not invent lever motion for this fixed push-button presentation.
  CheckPose(J,TEXT("switch_emergency"),FVector(0,1,0),0);
  struct FPoseCase {double Power,Brake,SpeedMps;int Direction;};
  const FPoseCase Cases[]={{0,0,0,-1},{.25,.5,25,0},{.5,.75,-25,1},{1,1,60,-1}};
  H.command.emergency=false;
  for(const auto& C:Cases){
   H.command.throttle=C.Power;H.command.brake=C.Brake;H.command.direction=C.Direction;H.speedMps=C.SpeedMps;
   // Deliberately distinct from handle intent, including release/application lag.
   H.appliedThrottle=.1;H.appliedBrake=.3;
   View->Present(S,H,.2f,false);J=Read();
   CheckPose(J,TEXT("throttle"),FVector(0,1,0),-24+48*C.Power);
   CheckPose(J,TEXT("train_brake"),FVector(0,0,1),-30+70*C.Brake);
   CheckPose(J,TEXT("reverser"),FVector(0,1,0),18*C.Direction);
   CheckPose(J,TEXT("gauge_speed"),FVector(-1,0,0),-130+260*FMath::Clamp(FMath::Abs(C.SpeedMps)*3.6/180,0.0,1.0));
   TestEqual(TEXT("Actual power remains distinct from handle intent"),J->GetNumberField(TEXT("appliedPower")),.1);
   TestEqual(TEXT("Actual brake remains distinct from handle intent"),J->GetNumberField(TEXT("appliedBrake")),.3);
   TestFalse(TEXT("Reset clears emergency indication"),J->GetBoolField(TEXT("emergency")));
   CheckPose(J,TEXT("switch_emergency"),FVector(0,1,0),0);
  }
  const double NormalFov=J->GetNumberField(TEXT("cameraFov"));
  View->ToggleInstrumentView();J=Read();TestTrue(TEXT("Instrument view selected"),J->GetBoolField(TEXT("instrumentView")));TestEqual(TEXT("Label view FOV"),J->GetNumberField(TEXT("cameraFov")),55.0);
  View->ToggleInstrumentView();J=Read();TestEqual(TEXT("Normal FOV restored exactly"),J->GetNumberField(TEXT("cameraFov")),NormalFov);
  View->ToggleSwitch(3);View->Present(S,H,.2f,false);J=Read();TestTrue(TEXT("Cab lamp visible inside"),J->GetBoolField(TEXT("cabLightVisible")));
  View->ToggleInstrumentView();TestTrue(TEXT("Zoom active before leaving cab"),Read()->GetBoolField(TEXT("instrumentView")));
  View->ToggleCamera();View->Present(S,H,.2f,false);J=Read();TestFalse(TEXT("Cab lamp hidden outside"),J->GetBoolField(TEXT("cabLightVisible")));TestFalse(TEXT("Outside resets active zoom"),J->GetBoolField(TEXT("instrumentView")));TestEqual(TEXT("Outside restores normal FOV"),J->GetNumberField(TEXT("cameraFov")),NormalFov);
  View->ToggleInstrumentView();TestFalse(TEXT("Label action outside cannot change camera"),Read()->GetBoolField(TEXT("instrumentView")));
  View->ToggleCamera();View->Present(S,H,.2f,false);J=Read();TestTrue(TEXT("Cab lamp selection retained on return"),J->GetBoolField(TEXT("cabLightVisible")));TestFalse(TEXT("Return starts in normal driver view"),J->GetBoolField(TEXT("instrumentView")));TestEqual(TEXT("Return keeps normal FOV"),J->GetNumberField(TEXT("cameraFov")),NormalFov);
  View->ToggleSwitch(3);View->Present(S,H,.2f,false);J=Read();TestFalse(TEXT("Cab lamp can be switched off"),J->GetBoolField(TEXT("cabLightVisible")));TestFalse(TEXT("Cab lamp off selection is retained"),J->GetBoolField(TEXT("cabLight")));
  for(const TCHAR* Name:{TEXT("Forward route"),TEXT("Right side"),TEXT("Left side"),TEXT("Cab")}){
   View->CycleJourneyCamera();View->Present(S,H,.2f,false);
   TestEqual(TEXT("Journey camera cycles through route and both train sides"),View->CameraName(),FString(Name));
   TestEqual(TEXT("Only cab view shows cab controls"),View->IsCabCamera(),FString(Name)==TEXT("Cab"));
  }

 }
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return !HasAnyErrors();
}
#endif
