#include "RailLocomotiveView.h"
#include "RailJourneyMaterials.h"
#include "RailWheelPresentation.h"
#include "RailControlPresentation.h"
#include "Camera/CameraComponent.h"
#include "RailSimulationSubsystem.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "StaticMeshResources.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/CoreDelegates.h"
#include "Framework/Application/SlateApplication.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProfilingDebugging/CsvProfiler.h"
#include "Engine/SkyLight.h"
#include "Components/SkyLightComponent.h"
#include "EngineUtils.h"
CSV_DEFINE_CATEGORY(RailS8,true);
namespace {
FVector Cm(Rail::Vec3 V){return FVector(V.x*100,V.y*100,V.z*100);}
bool Vector(const FJsonObject& O,const TCHAR* Key,FVector& Out){
 const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!O.TryGetArrayField(Key,A)||A->Num()!=3)return false;
 double V[3];for(int I=0;I<3;++I)if(!(*A)[I]->TryGetNumber(V[I])||!FMath::IsFinite(V[I]))return false;
 Out=FVector(V[0],V[1],V[2]);return true;
}
// Mobile shading has no sky occlusion, so an open-sky ambient floods the enclosed cab with
// blue. Only the view that entered the cab dims the captured-sky fill, and only it restores
// it; sunlight through the glazing stays physical. Scaling keeps low-light reviews intact.
constexpr float CabAmbientScale=.36f;
TWeakObjectPtr<AActor> CabAmbientOwner;
void ApplyCabAmbient(AActor* View,bool Cab){
 const bool Owner=CabAmbientOwner.Get()==View;
 if(Cab==Owner||(Cab&&CabAmbientOwner.IsValid()))return;
 bool Found=false;
 for(TActorIterator<ASkyLight> It(View->GetWorld());It;++It)if(It->ActorHasTag(TEXT("TambaramRuntimeEnvironment"))){
  auto* Light=It->GetLightComponent();Light->SetIntensity(Light->Intensity*(Cab?CabAmbientScale:1/CabAmbientScale));Found=true;
 }
 // The runtime environment may spawn after the cab opens; Present retries until it exists.
 if(Found||!Cab)CabAmbientOwner=Cab?View:nullptr;
}
}
ARailLocomotiveView::ARailLocomotiveView(){
 PrimaryActorTick.bCanEverTick=false;RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
 Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("DriverCamera"));Camera->SetupAttachment(RootComponent);Camera->SetFieldOfView(75);
 Instruments=CreateDefaultSubobject<UTextRenderComponent>(TEXT("AuthoritativeInstruments"));Instruments->SetupAttachment(RootComponent);
 Instruments->SetHorizontalAlignment(EHTA_Center);Instruments->SetWorldSize(3);Instruments->SetTextRenderColor(FColor(100,255,150));
 SpeedReadout=CreateDefaultSubobject<UTextRenderComponent>(TEXT("SpeedRecorderDigital"));SpeedReadout->SetupAttachment(RootComponent);SpeedReadout->SetRelativeLocation(FVector(906,-12,345));SpeedReadout->SetRelativeRotation(FRotator(0,180,0));SpeedReadout->SetWorldSize(2.0f);SpeedReadout->SetHorizontalAlignment(EHTA_Center);SpeedReadout->SetTextRenderColor(FColor(120,220,130));
 CabLight=CreateDefaultSubobject<UPointLightComponent>(TEXT("CabLight"));CabLight->SetupAttachment(RootComponent);CabLight->SetIntensity(1400);CabLight->SetAttenuationRadius(260);CabLight->SetCastShadows(false);CabLight->SetVisibility(false);
 if(!OriginalCabForVisualReview()){
  // Calibrated to this corridor's fixed exposure; retain dark panel contrast.
  CabLight->SetUseInverseSquaredFalloff(true);
  CabLight->SetIntensityUnits(ELightUnits::Lumens);CabLight->SetIntensity(30);
  CabLight->SetUseTemperature(true);CabLight->SetTemperature(4000);
  CabLight->SetSourceRadius(12);CabLight->SetSoftSourceRadius(16);
 }
 UpdateCamera();
}
void ARailLocomotiveView::BeginPlay(){
 Super::BeginPlay();
 FCoreDelegates::ApplicationWillDeactivateDelegate.AddUObject(this,&ARailLocomotiveView::CancelCabPointerInput);
 FCoreDelegates::ApplicationWillEnterBackgroundDelegate.AddUObject(this,&ARailLocomotiveView::CancelCabPointerInput);
 if(FSlateApplication::IsInitialized())FSlateApplication::Get().OnApplicationActivationStateChanged().AddUObject(this,&ARailLocomotiveView::OnApplicationActivationChanged);
}
void ARailLocomotiveView::EndPlay(const EEndPlayReason::Type EndPlayReason){
 FCoreDelegates::ApplicationWillDeactivateDelegate.RemoveAll(this);
 FCoreDelegates::ApplicationWillEnterBackgroundDelegate.RemoveAll(this);
 if(FSlateApplication::IsInitialized())FSlateApplication::Get().OnApplicationActivationStateChanged().RemoveAll(this);
 ApplyCabAmbient(this,false);CancelCabPointerInput();Super::EndPlay(EndPlayReason);
}
void ARailLocomotiveView::CancelCabPointerInput(){CabPointerRouter.Cancel();}
void ARailLocomotiveView::OnApplicationActivationChanged(bool Active){CancelCabPointerInput();}
void ARailLocomotiveView::SetDrivingHudInput(bool Enabled,bool BlockAll){
 if(DrivingHudInput!=Enabled||HudBlocksAllInput!=BlockAll)CancelCabPointerInput();
 DrivingHudInput=Enabled;HudBlocksAllInput=BlockAll;
}
bool ARailLocomotiveView::LoadPresentation(const FString& Id,FString& Diagnostic){
 if(Id!=TEXT("WAP7")){const auto* Sim=GetWorld()->GetSubsystem<URailSimulationSubsystem>();if(Sim&&Sim->IsServiceMode()&&Sim->ReadOnlyService().Definition().contentId==Rail::FormationContent::LhbId){Diagnostic=TEXT("This formation requires WAP-7 presentation");return false;}}
 if(auto* ReadoutMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/IndianRailway/S8/Locomotives/WAP7/Cab/Materials/ReadoutText.ReadoutText"))){Instruments->SetTextMaterial(ReadoutMaterial);SpeedReadout->SetTextMaterial(ReadoutMaterial);}

 if(Id!=TEXT("WAP7")&&Id!=TEXT("WAG9")){Diagnostic=TEXT("Unknown locomotive presentation");return false;}
 if(Id==PresentationClass&&IsReady())return true;
 const double Begin=FPlatformTime::Seconds();FString Text;TSharedPtr<FJsonObject> Doc;
 const FString Path=FPaths::ProjectContentDir()/TEXT("IndianRailway/Production/locomotives")/(Id+TEXT(".json"));
 if(!FFileHelper::LoadFileToString(Text,*Path)||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Doc)||!Doc){Diagnostic=TEXT("Locomotive assembly missing: ")+Id;return false;}
 const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;
 if(!Doc->TryGetArrayField(TEXT("parts"),Rows)||Rows->IsEmpty()||Rows->Num()>64){Diagnostic=TEXT("Assembly exceeds 64 component budget or is empty");return false;}
 TArray<bool> NewCabOnly;TArray<UStaticMesh*> Meshes;TArray<FString> NewRoles;TArray<FTransform> NewRest;TArray<FVector> NewAxes;TArray<FVector2D> NewRanges;TArray<FVector> NewBases,NewElbows,NewContacts;
 // Validate and load the complete candidate before replacing a usable presentation.
 for(const auto& Value:*Rows){const TSharedPtr<FJsonObject>* O=nullptr;FString Asset,PartRole;FVector L=FVector::ZeroVector,R=FVector::ZeroVector,S=FVector::OneVector;
  if(!Value->TryGetObject(O)||!(*O)->TryGetStringField(TEXT("asset"),Asset)||!Asset.StartsWith(TEXT("/Game/IndianRailway/S8/Locomotives/")+Id+TEXT("/"))||!(*O)->TryGetStringField(TEXT("role"),PartRole)||!Vector(**O,TEXT("location_cm"),L)||!Vector(**O,TEXT("rotation_deg"),R)||!Vector(**O,TEXT("scale"),S)||S.GetMin()<=0){Diagnostic=TEXT("Invalid assembly part");return false;}
  UStaticMesh* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);if(!Mesh){Diagnostic=TEXT("Missing cooked mesh: ")+Asset;return false;}
  if(Id==TEXT("WAP7")&&!OriginalCabForVisualReview()&&Mesh->GetName()==TEXT("SM_WAP7_Cab_StaticCore")){
   if(auto* Cab=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S9/CabRealismV1/Meshes/SM_WAP7_Cab_StaticCore_HardwareV1.SM_WAP7_Cab_StaticCore_HardwareV1")))Mesh=Cab;
  }
  if(Id==TEXT("WAP7")&&PartRole==TEXT("bogie")){
   const FString Replacement=FString::Printf(TEXT("/Game/IndianRailway/S8/RunningGearV2/SM_WAP7_Frame_%s.SM_WAP7_Frame_%s"),L.X>0?TEXT("Front"):TEXT("Rear"),L.X>0?TEXT("Front"):TEXT("Rear"));
   if(auto* Frame=LoadObject<UStaticMesh>(nullptr,*Replacement)){Mesh=Frame;L.Z-=4.79;}
  }
  bool IsCab=false;(*O)->TryGetBoolField(TEXT("cab_only"),IsCab);NewCabOnly.Add(IsCab);
  Meshes.Add(Mesh);NewRoles.Add(PartRole);NewRest.Add(FTransform(FRotator(R.X,R.Y,R.Z),L,S));
  FVector Axis(0,1,0);Vector(**O,TEXT("rotation_axis"),Axis);if(Axis.IsNearlyZero())Axis=FVector(0,1,0);NewAxes.Add(Axis.GetSafeNormal());
  FVector2D Range(0,1);const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if((*O)->TryGetArrayField(TEXT("rotation_range_deg"),A)&&A->Num()==2)Range=FVector2D((*A)[0]->AsNumber(),(*A)[1]->AsNumber());NewRanges.Add(Range);
  FVector B=FVector::ZeroVector,E=B,C=B;const TSharedPtr<FJsonObject>* Link=nullptr;
  if((*O)->TryGetObjectField(TEXT("linkage"),Link)){Vector(**Link,TEXT("base_cm"),B);Vector(**Link,TEXT("elbow_cm"),E);Vector(**Link,TEXT("contact_cm"),C);}
  NewBases.Add(B);NewElbows.Add(E);NewContacts.Add(C);
 }
 if(Id==TEXT("WAP7"))if(auto* Wheel=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/IndianRailway/S8/RunningGearV2/SM_WAP7_Wheelset.SM_WAP7_Wheelset"))){
  for(double X:{-788.39245,-582.76935,-377.14624,379.41656,585.03966,790.66277}){
   Meshes.Add(Wheel);NewRoles.Add(TEXT("wheelset"));NewCabOnly.Add(false);NewRest.Add(FTransform(FVector(X,0,54.6)));NewAxes.Add(FVector(0,1,0));NewRanges.Add(FVector2D(0,1));NewBases.Add(FVector::ZeroVector);NewElbows.Add(FVector::ZeroVector);NewContacts.Add(FVector::ZeroVector);
  }
 }
 for(const auto& Part:Parts)if(Part){RemoveInstanceComponent(Part);Part->SetStaticMesh(nullptr);Part->DestroyComponent();}Parts.Empty();LampMaterials.Empty();
 CabOnly=MoveTemp(NewCabOnly);Roles=MoveTemp(NewRoles);Rest=MoveTemp(NewRest);Axes=MoveTemp(NewAxes);Ranges=MoveTemp(NewRanges);PantoBases=MoveTemp(NewBases);PantoElbows=MoveTemp(NewElbows);PantoContacts=MoveTemp(NewContacts);Triangles=MaterialSlots=0;
 for(int32 I=0;I<Meshes.Num();++I){if(const auto* Render=Meshes[I]->GetRenderData())if(!Render->LODResources.IsEmpty())Triangles+=Render->LODResources[0].GetNumTriangles();MaterialSlots+=Meshes[I]->GetStaticMaterials().Num();auto* Part=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Part);Part->SetupAttachment(RootComponent);Part->SetMobility(EComponentMobility::Movable);Part->SetStaticMesh(Meshes[I]);if(CabOnly[I])Part->SetForcedLodModel(1);Part->SetRelativeTransform(Rest[I]);Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);Part->SetGenerateOverlapEvents(false);Part->SetCastShadow(true);Part->RegisterComponent();Parts.Add(Part);
  if(Id==TEXT("WAP7"))ApplyJourneyMaterials(Part);
  for(int32 Slot=0;Slot<Part->GetNumMaterials();++Slot)if(Part->GetMaterial(Slot)&&Part->GetMaterial(Slot)->GetName().Contains(TEXT("Lamp")))if(auto* Material=Part->CreateDynamicMaterialInstance(Slot))LampMaterials.Add(Material);
 }
 const TSharedPtr<FJsonObject>* Driver=nullptr;if(Doc->TryGetObjectField(TEXT("driver_camera"),Driver)){Vector(**Driver,TEXT("location_cm"),DriverLocation);FVector R;if(Vector(**Driver,TEXT("rotation_deg"),R))DriverRotation=FRotator(R.X,R.Y,R.Z);}
 Doc->TryGetNumberField(TEXT("wheel_radius_cm"),WheelRadiusCm);WheelRadiusCm=FMath::Clamp(WheelRadiusCm,30.0,90.0);
 if(Id==TEXT("WAP7")&&Roles.Contains(TEXT("wheelset")))WheelRadiusCm=54.6;
 Instruments->SetRelativeLocation(DriverLocation+FVector(72,0,-30));Instruments->SetRelativeRotation(FRotator(0,180,0));CabLight->SetRelativeLocation(DriverLocation+FVector(0,0,50));
 CabAvailable=true;Doc->TryGetBoolField(TEXT("cab_available"),CabAvailable);if(!CabAvailable)CabCamera=false;
 const TSharedPtr<FJsonObject>* Display=nullptr;
 if(Doc->TryGetObjectField(TEXT("instrument_display"),Display)){FVector L,R;double Size=1.8;if(Vector(**Display,TEXT("location_cm"),L))Instruments->SetRelativeLocation(L);if(Vector(**Display,TEXT("rotation_deg"),R))Instruments->SetRelativeRotation(FRotator(R.X,R.Y,R.Z));(*Display)->TryGetNumberField(TEXT("size_cm"),Size);Instruments->SetWorldSize(Size);Instruments->SetVerticalAlignment(EVRTA_TextCenter);}
 FVector LightLocation;if(Vector(*Doc,TEXT("cab_light_location_cm"),LightLocation))CabLight->SetRelativeLocation(LightLocation);
 // Cab revisions place the digital speed on their own display and set the instrument glance.
 const TSharedPtr<FJsonObject>* Readout=nullptr;if(Doc->TryGetObjectField(TEXT("speed_readout"),Readout)){FVector L,R;double Size=2;if(Vector(**Readout,TEXT("location_cm"),L))SpeedReadout->SetRelativeLocation(L);if(Vector(**Readout,TEXT("rotation_deg"),R))SpeedReadout->SetRelativeRotation(FRotator(R.X,R.Y,R.Z));(*Readout)->TryGetNumberField(TEXT("size_cm"),Size);SpeedReadout->SetWorldSize(Size);SpeedReadout->SetVerticalAlignment(EVRTA_TextCenter);}
 InstrumentPitch=Id==TEXT("WAP7")&&!OriginalCabForVisualReview()?-3:-12;Doc->TryGetNumberField(TEXT("instrument_view_pitch_deg"),InstrumentPitch);
 if(Driver){double Fov=78;(*Driver)->TryGetNumberField(TEXT("fov"),Fov);DriverFov=FMath::Clamp(Fov,55.0,95.0);}
 LookOffset=FRotator::ZeroRotator;CancelCabPointerInput();
 PresentationClass=Id;UpdateCamera();LastLoadMilliseconds=(FPlatformTime::Seconds()-Begin)*1000;
 Diagnostic=Id+TEXT(" presentation loaded; existing provisional handling retained");
 UE_LOG(LogTemp,Display,TEXT("S8_LOCOMOTIVE class=%s components=%d loadMs=%.2f"),*Id,Parts.Num(),LastLoadMilliseconds);return true;
}
void ARailLocomotiveView::UpdateCamera(){
 // View the station across the train, clear of the platform canopy.
 const bool Station=FParse::Param(FCommandLine::Get(),TEXT("S8Station"));
 const bool Corridor=FParse::Param(FCommandLine::Get(),TEXT("S8Tambaram"));
 // Keep the journey's exterior camera inside the roadside tree line, where
 // the locomotive running gear and first coach remain visible on curves.
 const FVector Exterior=ExteriorView==1?FVector(-1100,-700,1250):ExteriorView==2?FVector(1400,1250,700):Corridor?FVector(1400,-1250,700):FVector(2200,Station?2700:-2700,1150);
 const FVector Target=ExteriorView==1?FVector(4200,250,0):FVector(0,0,200);
 Camera->SetRelativeLocation(CabCamera?DriverLocation:Exterior);
 Camera->SetRelativeRotation(CabCamera?(DriverRotation+LookOffset):(Target-Camera->GetRelativeLocation()).Rotation());
 Camera->SetFieldOfView(CabCamera&&InstrumentView?55:DriverFov);
 Instruments->SetVisibility(CabCamera);SpeedReadout->SetVisibility(CabCamera);
 for(int32 I=0;I<Parts.Num();++I){const bool Visible=CabOnly[I]?CabCamera:(!CabCamera||Roles[I]!=TEXT("exterior"));Parts[I]->SetVisibility(Visible);Parts[I]->SetCastShadow(Visible);}
 CabLight->SetVisibility(CabCamera&&CabLighting);
 ApplyCabAmbient(this,CabCamera);
}
void ARailLocomotiveView::ToggleCamera(){if(CabAvailable){CancelCabPointerInput();CabCamera=!CabCamera;ExteriorView=0;InstrumentView=false;++CabTransitions;LookOffset=FRotator::ZeroRotator;UpdateCamera();}}
FString ARailLocomotiveView::CameraName() const {return CabCamera?TEXT("Cab"):ExteriorView==1?TEXT("Forward route"):ExteriorView==2?TEXT("Right side"):TEXT("Left side");}
void ARailLocomotiveView::CycleJourneyCamera(){
 if(!CabAvailable)return;
 if(CabCamera){CabCamera=false;ExteriorView=1;}
 else if(ExteriorView==1)ExteriorView=2;
 else if(ExteriorView==2)ExteriorView=0;
 else CabCamera=true;
 InstrumentView=false;CancelCabPointerInput();LookOffset=FRotator::ZeroRotator;++CabTransitions;UpdateCamera();
}
void ARailLocomotiveView::ToggleInstrumentView(){if(CabCamera){CancelCabPointerInput();InstrumentView=!InstrumentView;LookOffset=InstrumentView?FRotator(InstrumentPitch,0,0):FRotator::ZeroRotator;UpdateCamera();}}
void ARailLocomotiveView::ToggleSwitch(int32 Switch){if(Switch==0)Headlights=!Headlights;else if(Switch==1)PantographUp=!PantographUp;else if(Switch==2)Wipers=!Wipers;else if(Switch==3)CabLighting=!CabLighting;}
FString ARailLocomotiveView::Display() const {
 if(!CabAvailable)return PresentationClass+TEXT(" exterior | Cab and equipment animation in progress");
 return FString::Printf(TEXT("%s %s | Lamps %s | Panto %s | Wipers %s | Cab lamp %s\nPresentation controls only; provisional service handling"),*PresentationClass,CabCamera?TEXT("CAB"):TEXT("EXTERIOR"),Headlights?TEXT("ON"):TEXT("OFF"),PantographUp?TEXT("UP"):TEXT("DOWN"),Wipers?TEXT("ON"):TEXT("OFF"),CabLighting?TEXT("ON"):TEXT("OFF"));
}
FString ARailLocomotiveView::TelemetryJson() const {
 FString Result=FString::Printf(TEXT("{\"ready\":%s,\"class\":\"%s\",\"parts\":%d,\"trianglesLOD0\":%d,\"materialSlots\":%d,\"loadMs\":%.3f,\"cabCamera\":%s,\"cabTransitions\":%d,\"interactions\":%d,\"speedKmh\":%.3f,\"commandThrottle\":%.3f,\"commandBrake\":%.3f,\"direction\":%d"),IsReady()?TEXT("true"):TEXT("false"),*PresentationClass,Parts.Num(),Triangles,MaterialSlots,LastLoadMilliseconds,CabCamera?TEXT("true"):TEXT("false"),CabTransitions,InteractionCount,LastSpeed,LastPower,LastBrake,LastDirection);
 Result+=FString::Printf(TEXT(",\"cameraView\":\"%s\""),*CameraName());
 FString CabCore;TSet<FString> CabFinishes;int32 CabTriangles=0;
 for(int32 I=0;I<Parts.Num();++I)if(CabOnly[I]&&Parts[I]){
  if(const UStaticMesh* Mesh=Parts[I]->GetStaticMesh()){
   if(Mesh->GetName().StartsWith(TEXT("SM_WAP7_Cab_StaticCore")))CabCore=Mesh->GetPathName();
   if(const auto* Render=Mesh->GetRenderData())if(!Render->LODResources.IsEmpty())CabTriangles+=Render->LODResources[0].GetNumTriangles();
  }
  for(int32 Slot=0;Slot<Parts[I]->GetNumMaterials();++Slot)if(const auto* Material=Parts[I]->GetMaterial(Slot))if(Material->GetPathName().StartsWith(TEXT("/Game/IndianRailway/S9/CabRealismV1/Materials/")))CabFinishes.Add(Material->GetPathName());
 }
 Result+=FString::Printf(TEXT(",\"cabCoreMesh\":\"%s\",\"cabFinishMaterials\":%d,\"cabTrianglesLOD0\":%d"),*CabCore,CabFinishes.Num(),CabTriangles);
 Result+=FString::Printf(TEXT(",\"appliedPower\":%.6f,\"appliedBrake\":%.6f,\"emergency\":%s,\"cabLight\":%s,\"cabLightVisible\":%s,\"instrumentView\":%s,\"cameraFov\":%.2f"),LastAppliedPower,LastAppliedBrake,LastEmergency?TEXT("true"):TEXT("false"),CabLighting?TEXT("true"):TEXT("false"),CabLight->IsVisible()?TEXT("true"):TEXT("false"),InstrumentView?TEXT("true"):TEXT("false"),Camera->FieldOfView);
 int32 ViewWidth=0,ViewHeight=0;if(auto* PC=GetWorld()->GetFirstPlayerController())PC->GetViewportSize(ViewWidth,ViewHeight);
 Result+=FString::Printf(TEXT(",\"viewport\":[%d,%d]"),ViewWidth,ViewHeight);
 Result+=TEXT(",\"controlPoses\":[");bool First=true;
 for(int32 I=0;I<Parts.Num();++I)if(Roles[I]==TEXT("throttle")||Roles[I]==TEXT("train_brake")||Roles[I]==TEXT("reverser")||Roles[I]==TEXT("gauge_speed")||Roles[I].StartsWith(TEXT("switch_"))){
  if(!First)Result+=TEXT(",");First=false;const FRotator R=Parts[I]->GetRelativeRotation();FVector2D Screen=FVector2D::ZeroVector;
  if(auto* PC=GetWorld()->GetFirstPlayerController())PC->ProjectWorldLocationToScreen(Parts[I]->GetComponentLocation()+GetActorQuat().RotateVector((Roles[I]==TEXT("gauge_speed")||Roles[I].StartsWith(TEXT("switch_")))?FVector::ZeroVector:FVector(0,0,12)),Screen);
  Result+=FString::Printf(TEXT("{\"role\":\"%s\",\"rotation\":[%.2f,%.2f,%.2f],\"screen\":[%.1f,%.1f]}"),*Roles[I],R.Pitch,R.Yaw,R.Roll,Screen.X,Screen.Y);
 }
 return Result+TEXT("]}");
}
void ARailLocomotiveView::Present(const Rail::Simulation& Spatial,const Rail::LongitudinalState& H,float Dt,bool Paused){
 const double Begin=FPlatformTime::Seconds();Rail::Snapshot S;
 LastSpeed=FMath::Abs(H.speedMps)*3.6;LastPower=H.command.throttle;LastBrake=H.command.brake;LastDirection=H.command.direction;
 const auto Controls=Rail::ControlPresentation::From(H);LastAppliedPower=Controls.power;LastAppliedBrake=Controls.brake;LastEmergency=Controls.emergency;
 if(const auto* Sim=GetWorld()->GetSubsystem<URailSimulationSubsystem>())if(Sim->IsServiceMode()){LastPower=Sim->ReadOnlyService().Current().requestedPower;LastBrake=Sim->ReadOnlyService().Current().requestedBrake;}
 if(auto* PC=GetWorld()->GetFirstPlayerController()){
  Rail::HudInputRegions::PointerRouter::Samples Samples{};
  for(int32 I=0;I<Rail::HudInputRegions::TouchCount;++I){float X=0,Y=0;bool Down=false;PC->GetInputTouchState(static_cast<ETouchIndex::Type>(I),X,Y,Down);Samples[I]={X,Y,Down};}
  float MouseX=0,MouseY=0;const bool HasMouse=PC->GetMousePosition(MouseX,MouseY);
  Samples[Rail::HudInputRegions::MousePointer]={MouseX,MouseY,HasMouse&&PC->IsInputKeyDown(EKeys::LeftMouseButton)};
  int32 W=0,Height=0;PC->GetViewportSize(W,Height);
  const bool Focused=!FSlateApplication::IsInitialized()||FSlateApplication::Get().IsActive();
  const auto Gesture=CabPointerRouter.Update(Samples,W,Height,CabCamera&&!Paused&&Focused,DrivingHudInput,HudBlocksAllInput);
  if(Gesture.Pan){LookOffset.Yaw=FMath::Clamp(LookOffset.Yaw+Gesture.DeltaX*.07,-65.0,65.0);LookOffset.Pitch=FMath::Clamp(LookOffset.Pitch-Gesture.DeltaY*.07,-30.0,25.0);UpdateCamera();}
  if(Gesture.Tap){FVector Origin,Direction;if(PC->DeprojectScreenPositionToWorld(Gesture.TapX,Gesture.TapY,Origin,Direction)){int32 Best=INDEX_NONE;double Distance=18;
   for(int32 I=0;I<Parts.Num();++I)if(Roles[I]==TEXT("throttle")||Roles[I]==TEXT("train_brake")||Roles[I]==TEXT("reverser")||Roles[I].StartsWith(TEXT("switch_"))){const FVector Target=Parts[I]->GetComponentLocation()+GetActorQuat().RotateVector(Roles[I].StartsWith(TEXT("switch_"))?FVector::ZeroVector:FVector(0,0,12));const FVector D=Target-Origin;const double T=FVector::DotProduct(D,Direction);const double Miss=(D-Direction*T).Size();if(T>0&&Miss<Distance){Distance=Miss;Best=I;}}
   if(Best!=INDEX_NONE){++InteractionCount;GetWorld()->GetSubsystem<URailSimulationSubsystem>()->CabControl(Roles[Best]);}
  }}
 }else CancelCabPointerInput();

 ApplyCabAmbient(this,CabCamera);
 if(!Spatial.Presentation(Spatial.InterpolationAlpha(),S).ok||S.vehicles.empty())return;
 const auto& V=S.vehicles.front();const FQuat Q=FRotationMatrix::MakeFromXZ(Cm(V.bodyForward).GetSafeNormal(),FVector::UpVector).ToQuat();SetActorLocationAndRotation(Cm(V.bodyCentre),Q);
 if(!Paused)WiperTime+=FMath::Clamp(double(Dt),0.0,.1);
 const double WheelAngle=RailWheelPresentation::Angle(V.centreStationMetres,WheelRadiusCm*.01);
 for(int32 I=0;I<Parts.Num();++I){FTransform T=Rest[I];const FString& PartRole=Roles[I];
  if(PartRole.StartsWith(TEXT("wheelset")))T.SetRotation(T.GetRotation()*FQuat(Axes[I],WheelAngle));
  if(PartRole.StartsWith(TEXT("bogie"))||PartRole.StartsWith(TEXT("wheelset"))){
   const bool Front=PartRole.EndsWith(TEXT("front"))||(!PartRole.EndsWith(TEXT("rear"))&&Rest[I].GetLocation().X>0);
   const FVector Bogie=Cm(Front?V.frontBogie:V.rearBogie);const FVector Local=Q.UnrotateVector(Bogie-Cm(V.bodyCentre));
   Rail::Sample Sample;const double Offset=FVector::DotProduct(Bogie-Cm(V.bodyCentre),Q.GetForwardVector())/100;
   FQuat Turn=FQuat::Identity;if(Spatial.History().SampleAt(Spatial.Network(),V.centreStationMetres+Offset,Sample).ok)Turn=Q.Inverse()*FRotationMatrix::MakeFromXZ(Cm(Sample.tangent).GetSafeNormal(),FVector::UpVector).ToQuat();
   // Preserve authored bogie spacing (presentation can differ from provisional physics dimensions).
   const double BogieX=Front?Rail::FormationContent::Wap7BogieOffset*100:-Rail::FormationContent::Wap7BogieOffset*100;
   if(PresentationClass==TEXT("WAP7"))T.SetLocation(Local+Turn.RotateVector(FVector(Rest[I].GetLocation().X-BogieX,Rest[I].GetLocation().Y,Rest[I].GetLocation().Z)));
   else T.SetLocation(FVector(Rest[I].GetLocation().X,Local.Y,Local.Z+Rest[I].GetLocation().Z));
   T.SetRotation(Turn*T.GetRotation());
  }
  if(PartRole.StartsWith(TEXT("pantograph_"))&&!PantoContacts[I].Equals(PantoBases[I])){
   const FVector B=PantoBases[I],E=PantoElbows[I],C=PantoContacts[I];
   FVector Target=C;if(!PantographUp)Target.Z=B.Z+18;
   const double L1=FVector::Distance(B,E),L2=FVector::Distance(E,C);
   FVector D=Target-B;const double Length=FMath::Clamp(D.Size(),FMath::Abs(L1-L2)+.001,L1+L2-.001);D=D.GetSafeNormal();Target=B+D*Length;
   const double A=(L1*L1-L2*L2+Length*Length)/(2*Length),Height=FMath::Sqrt(FMath::Max(0.0,L1*L1-A*A));
   const FVector Perp(D.Z,0,-D.X);const FVector OrigD=(C-B).GetSafeNormal(),OrigPerp(OrigD.Z,0,-OrigD.X);
   const double Side=FVector::DotProduct(E-B,OrigPerp)>=0?1:-1;const FVector Elbow=B+D*A+Perp*(Height*Side);
   if(PartRole==TEXT("pantograph_lower")||PartRole==TEXT("pantograph_upper")){
    const bool Lower=PartRole==TEXT("pantograph_lower");const FVector Old=Lower?E-B:C-E,New=Lower?Elbow-B:Target-Elbow;
    const double Angle=FMath::Atan2(Old.Z,Old.X)-FMath::Atan2(New.Z,New.X);
    T.SetLocation(Lower?B:Elbow);T.SetRotation(FQuat(FVector::RightVector,Angle)*T.GetRotation());
   }else T.SetLocation(Target);
  }
  if(PartRole==TEXT("pantograph")){if(Ranges[I]!=FVector2D(0,1))T.SetRotation(T.GetRotation()*FQuat(Axes[I],FMath::DegreesToRadians(PantographUp?Ranges[I].Y:Ranges[I].X)));else {T.SetScale3D(T.GetScale3D()*FVector(1,1,PantographUp?1:.25));}}
  if(PartRole==TEXT("wiper"))T.SetRotation(T.GetRotation()*FQuat(Axes[I],FMath::DegreesToRadians(Wipers?Ranges[I].Y*.5*(1-FMath::Cos(WiperTime*3)):0)));
  double Fraction=-1;if(PartRole==TEXT("throttle"))Fraction=PresentationClass==TEXT("WAP7")?(LastPower+1)*.5:LastPower;/* WAP-7 master controller is centre-zero: BE back, TE forward */else if(PartRole==TEXT("train_brake"))Fraction=LastBrake;else if(PartRole==TEXT("reverser"))Fraction=(H.command.direction+1)*.5;
  else if(PartRole==TEXT("gauge_speed"))Fraction=FMath::Clamp(FMath::Abs(H.speedMps)*3.6/180,0.0,1.0);else if(PartRole==TEXT("gauge_brake"))Fraction=H.appliedBrake;else if(PartRole==TEXT("gauge_power"))Fraction=FMath::Clamp(FMath::Abs(H.appliedThrottle),0.0,1.0);else if(PartRole==TEXT("gauge_bp"))Fraction=Rail::AirBrake::BrakePipe(H.command.brake,H.command.emergency)/10;
  if(PartRole==TEXT("switch_lamps"))Fraction=Headlights?1:0;else if(PartRole==TEXT("switch_wipers"))Fraction=Wipers?1:0;else if(PartRole==TEXT("switch_cablight"))Fraction=CabLighting?1:0;
  if(Fraction>=0){const FVector2D R=Ranges[I]==FVector2D(0,1)?FVector2D(-35,35):Ranges[I];T.SetRotation(T.GetRotation()*FQuat(Axes[I],FMath::DegreesToRadians(FMath::Lerp(R.X,R.Y,Fraction))));}
  Parts[I]->SetRelativeTransform(T);
 }
 for(const auto& Material:LampMaterials)Material->SetScalarParameterValue(TEXT("LampIntensity"),Headlights?4:0);
 CabLight->SetVisibility(CabCamera&&CabLighting);
 InstrumentClock+=Dt;if(InstrumentClock>=.1){InstrumentClock=0;SpeedReadout->SetText(FText::FromString(FString::Printf(TEXT("%03.0f km/h"),LastSpeed)));Instruments->SetText(FText::FromString(FString::Printf(TEXT("%s %3.0fkN  BP %.1f  BC %.1f  %s"),H.tractionN<0?TEXT("BE"):TEXT("TE"),FMath::Abs(H.tractionN)/1000,Rail::AirBrake::BrakePipe(H.command.brake,H.command.emergency),H.appliedBrake*Rail::AirBrake::MaxCylinder,Controls.emergency?TEXT("EMG"):Controls.direction>0?TEXT("FWD"):Controls.direction<0?TEXT("REV"):TEXT("NEU"))));}

 CSV_CUSTOM_STAT(RailS8,LocomotiveParts,Parts.Num(),ECsvCustomStatOp::Set);CSV_CUSTOM_STAT(RailS8,LocomotivePresentMs,(FPlatformTime::Seconds()-Begin)*1000,ECsvCustomStatOp::Set);CSV_CUSTOM_STAT(RailS8,LocomotiveLoadMs,LastLoadMilliseconds,ECsvCustomStatOp::Set);
}
