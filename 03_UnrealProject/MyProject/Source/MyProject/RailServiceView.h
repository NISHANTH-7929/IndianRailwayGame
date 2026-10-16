#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailService.h"
#include "RailServiceView.generated.h"
class UInstancedStaticMeshComponent;
UCLASS()
class MYPROJECT_API ARailServiceView : public AActor {
 GENERATED_BODY()
public:
 ARailServiceView();
 void Configure(const Rail::ServiceDefinition& Definition);
 void Present(const Rail::ServiceDefinition& Definition,const Rail::OperationsWorld& World,bool HideProxyCab=false);
private:
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Platform;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Cab;
};
