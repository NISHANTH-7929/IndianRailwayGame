#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailHandling.h"
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
 void ToggleInstrumentView();
 void ToggleSwitch(int32 Switch);
 bool IsReady() const {return !Parts.IsEmpty();}
 bool IsCabCamera() const {return CabCamera;}
 FString ClassName() const {return PresentationClass;}
 FString Display() const;
 FString TelemetryJson() const;
 int32 PartCount() const {return Parts.Num();}
 double LastLoadMilliseconds=0;
private:
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
 bool PointerDown=false,PointerDragged=false;
 FVector2D PointerStart=FVector2D::ZeroVector,PointerPrevious=FVector2D::ZeroVector;
 FRotator LookOffset=FRotator::ZeroRotator;
 double InstrumentClock=0;
 double LastSpeed=0,LastPower=0,LastBrake=0;
 double LastAppliedPower=0,LastAppliedBrake=0,DriverFov=95;
 bool LastEmergency=false,InstrumentView=false;
 int32 LastDirection=0,CabTransitions=0,InteractionCount=0;
 bool CabCamera=false,Headlights=true,PantographUp=true,Wipers=false,CabLighting=false;
 double WiperTime=0,WheelRadiusCm=54.6;
 void UpdateCamera();
};
