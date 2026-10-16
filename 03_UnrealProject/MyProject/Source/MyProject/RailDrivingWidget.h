#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RailDrivingWidget.generated.h"
class UTextBlock;
// Stateless touch adapter: every driving action goes through the subsystem command queue.
UCLASS()
class MYPROJECT_API URailDrivingWidget : public UUserWidget {
 GENERATED_BODY()
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
private:
 UPROPERTY() TObjectPtr<UTextBlock> Telemetry;
 UFUNCTION() void Throttle0();
 UFUNCTION() void Throttle25();
 UFUNCTION() void Throttle50();
 UFUNCTION() void Throttle100();
 UFUNCTION() void Brake0();
 UFUNCTION() void Brake50();
 UFUNCTION() void Brake100();
 UFUNCTION() void Reverse();
 UFUNCTION() void Neutral();
 UFUNCTION() void Forward();
 UFUNCTION() void Emergency();
 UFUNCTION() void ResetEmergency();
 UFUNCTION() void Pause();
 UFUNCTION() void ResetTest();
 UFUNCTION() void SelectA();
 UFUNCTION() void SelectB();
 UFUNCTION() void RouteA();
 UFUNCTION() void RouteB();
 UFUNCTION() void Point();
 UFUNCTION() void CancelRoute();
 UFUNCTION() void RecreateView();
 UFUNCTION() void Checkpoint();
 UFUNCTION() void LookBack();
 UFUNCTION() void CabCamera();
 UFUNCTION() void LocoClass();
 UFUNCTION() void Lamps();
 UFUNCTION() void Pantograph();
 UFUNCTION() void Wipers();
 UFUNCTION() void CabLamp();
};

