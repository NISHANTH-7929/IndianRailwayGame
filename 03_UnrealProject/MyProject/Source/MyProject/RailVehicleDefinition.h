#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Simulation/RailKernel.h"
#include "RailVehicleDefinition.generated.h"
UCLASS(BlueprintType)
class MYPROJECT_API URailVehicleDefinition : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere, Category="Rail|Identity") int64 StableId=0;
 UPROPERTY(EditAnywhere, Category="Rail|Identity") int32 Version=1;
 UPROPERTY(EditAnywhere, Category="Rail|SI") double LengthMetres=20;
 UPROPERTY(EditAnywhere, Category="Rail|SI") double MassKg=80000;
 UPROPERTY(EditAnywhere, Category="Rail|SI") double FrontBogieOffsetMetres=7;
 UPROPERTY(EditAnywhere, Category="Rail|SI") double RearBogieOffsetMetres=-7;
 UPROPERTY(EditAnywhere, Category="Rail") bool Powered=false;
 Rail::VehicleDefinition Record() const {return {Rail::DefinitionId(StableId>0?uint64(StableId):0),Version>0?uint32(Version):0,LengthMetres,MassKg,FrontBogieOffsetMetres,RearBogieOffsetMetres,Powered};}
};
