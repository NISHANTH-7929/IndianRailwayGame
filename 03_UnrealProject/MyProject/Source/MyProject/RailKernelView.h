#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailKernel.h"
#include "RailKernelView.generated.h"
class UInstancedStaticMeshComponent;
class UCameraComponent;
UCLASS()
class MYPROJECT_API ARailKernelView : public AActor {
 GENERATED_BODY()
public:
 ARailKernelView();
 void Present(const Rail::Graph& Graph,const Rail::PathHistory& History,const Rail::Snapshot& Snapshot,const FString& Status);
private:
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bodies;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Bogies;
 UPROPERTY() TObjectPtr<UCameraComponent> Camera;
};
