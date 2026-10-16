#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailOperations.h"
#include "RailOperationsView.generated.h"
// One disposable presentation actor for the whole infrastructure fixture.
UCLASS()
class MYPROJECT_API ARailOperationsView : public AActor {
 GENERATED_BODY()
public:
 ARailOperationsView(){PrimaryActorTick.bCanEverTick=false;}
 void Present(const Rail::Graph& Graph,const Rail::OperationsDefinitions& Definitions,const Rail::OperationsSnapshot& Snapshot);
 FString PresentationDigest() const;
private:
 Rail::OperationsSnapshot Display;
};
