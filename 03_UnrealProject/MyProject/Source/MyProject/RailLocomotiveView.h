#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailHandling.h"
#include "Simulation/RailHudInputRegions.h"
#include "RailLocomotiveView.generated.h"
class UStaticMeshComponent;
class UCameraComponent;
class UTextRenderComponent;
class UPointLightComponent;
class UMaterialInstanceDynamic;

// Disposable presentation: the simulation remains the sole authority for movement and controls.
UCLASS()
class MYPROJECT_API ARailLocomotiveView : public AActor {
 GENERATED_BODY()
public:
 ARailLocomotiveView();
 bool LoadPresentation(const FString& ClassId,FString& Diagnostic);
 void Present(const Rail::Simulation& Spatial,const Rail::LongitudinalState& Handling,float DeltaTime,bool Paused);
 void ToggleCamera();
 void CycleJourneyCamera();
 FString CameraName() const;
 void ToggleInstrumentView();
 void ToggleSwitch(int32 Switch);
 // Enable only for the edge-control driving HUD. BlockAll covers its drawer,
 // paused menus and other overlays; the fractions live in RailHudInputRegions.h.
 void SetDrivingHudInput(bool Enabled,bool BlockAll=false);
 bool IsReady() const {return !Parts.IsEmpty();}
 bool IsCabCamera() const {return CabCamera;}
 FString ClassName() const {return PresentationClass;}
 FString Display() const;
 FString TelemetryJson() const;
 int32 PartCount() const {return Parts.Num();}
 double LastLoadMilliseconds=0;
private:
 virtual void BeginPlay() override;
 virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
 void CancelCabPointerInput();
 void OnApplicationActivationChanged(bool Active);
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Parts;
 UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> LampMaterials;
 UPROPERTY() TObjectPtr<UCameraComponent> Camera;
 UPROPERTY() TObjectPtr<UTextRenderComponent> Instruments;
 UPROPERTY() TObjectPtr<UTextRenderComponent> SpeedReadout;
 UPROPERTY() TObjectPtr<UPointLightComponent> CabLight;
 TArray<FString> Roles;
 TArray<bool> CabOnly;
 TArray<FTransform> Rest;
 TArray<FVector> Axes;
 TArray<FVector2D> Ranges;
 TArray<FVector> PantoBases,PantoElbows,PantoContacts;
 int32 Triangles=0,MaterialSlots=0;
 FString PresentationClass;
 FVector DriverLocation=FVector(736,-78,298);
 FRotator DriverRotation=FRotator::ZeroRotator;
 bool CabAvailable=true;
 Rail::HudInputRegions::PointerRouter CabPointerRouter;
 bool DrivingHudInput=false,HudBlocksAllInput=false;
 FRotator LookOffset=FRotator::ZeroRotator;
 double InstrumentClock=0;
 double LastSpeed=0,LastPower=0,LastBrake=0;
 double LastAppliedPower=0,LastAppliedBrake=0,DriverFov=95;
 bool LastEmergency=false,InstrumentView=false;
 int32 LastDirection=0,CabTransitions=0,InteractionCount=0;
 bool CabCamera=false,Headlights=true,PantographUp=true,Wipers=false,CabLighting=false;
 int32 ExteriorView=0; // left side, forward route, right side
 double WiperTime=0,WheelRadiusCm=54.6;
 void UpdateCamera();
};
