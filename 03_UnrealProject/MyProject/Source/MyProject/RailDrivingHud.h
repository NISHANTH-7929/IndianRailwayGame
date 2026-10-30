#pragma once
#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "RailDrivingHud.generated.h"
class URailSimulationSubsystem;
class SRailDrivingHud;

// Native driving overlay. All values come from the railway; inputs go through
// the same command validation as keyboard and physical cab controls.
UCLASS()
class MYPROJECT_API URailDrivingHud : public UWidget {
 GENERATED_BODY()
public:
 void Synchronize(URailSimulationSubsystem* Simulation, bool Driving);
 bool IsCabDrawerOpen() const;
 virtual void ReleaseSlateResources(bool ReleaseChildren) override;
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
private:
 TSharedPtr<SRailDrivingHud> DrivingSlate;
 TWeakObjectPtr<URailSimulationSubsystem> Railway;
 void Dispatch(int32 Action, double Value);
};
