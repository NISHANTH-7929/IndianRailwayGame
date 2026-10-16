#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Simulation/RailHandling.h"
#include "RailHandlingDefinition.generated.h"
// Engineering fixture parameters, not measured locomotive specifications.
UCLASS(BlueprintType)
class MYPROJECT_API URailHandlingDefinition : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Category="Rail|Handling") int64 VehicleDefinitionId=0;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") int32 Version=1;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double MaxTractionN=0;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double MaxPowerWatts=0;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double TractionCutoffMps=16;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double AdhesiveMassFraction=1;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double AdhesionCoefficient=.25;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double RollingNPerKg=.015;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double AeroNPerMpsSquared=15;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double ServiceBrakeNPerKg=.8;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double EmergencyBrakeNPerKg=1.2;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double TractionResponseSeconds=.5;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double BrakeApplySeconds=1.5;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double BrakeReleaseSeconds=2;
 UPROPERTY(EditAnywhere,Category="Rail|Handling") double EmergencyApplySeconds=.5;
 Rail::HandlingDefinition Record() const {
  return {Rail::DefinitionId(VehicleDefinitionId>0?uint64(VehicleDefinitionId):0),Version>0?uint32(Version):0,
   MaxTractionN,MaxPowerWatts,TractionCutoffMps,AdhesiveMassFraction,AdhesionCoefficient,
   RollingNPerKg,AeroNPerMpsSquared,ServiceBrakeNPerKg,EmergencyBrakeNPerKg,
   TractionResponseSeconds,BrakeApplySeconds,BrakeReleaseSeconds,EmergencyApplySeconds};
 }
};
