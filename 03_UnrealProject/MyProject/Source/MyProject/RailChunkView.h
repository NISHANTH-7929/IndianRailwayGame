#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailOperations.h"
#include "RailChunkView.generated.h"
class UInstancedStaticMeshComponent;
class ARailTambaramCorridorView;
class ARailTambaramStationView;
class ARailTambaramTurnoutView;
// Owned by a streamed ULevel. No simulation state or authority lives here.
UCLASS()
class MYPROJECT_API ARailChunkView : public AActor {
 GENERATED_BODY()
public:
 ARailChunkView();
 UPROPERTY(EditAnywhere,Category="S6") int32 ChunkId=1;
 UPROPERTY(VisibleAnywhere,Category="S6") TObjectPtr<UInstancedStaticMeshComponent> Bodies;
 UPROPERTY(VisibleAnywhere,Category="S6") TObjectPtr<UInstancedStaticMeshComponent> Scenery;
 void Present(const Rail::OperationsWorld& World,bool DebugLabels=true,uint64 HiddenLocomotiveTrain=0,bool RefreshScenery=true);
 uint64 PresentedTick=0;
 int32 PresentedVehicles=0;
 UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> CoachParts;
 int32 PresentedLhbVehicles=0;
 bool LoadCoach();
 UPROPERTY() TArray<TObjectPtr<UInstancedStaticMeshComponent>> StationParts;
 bool LoadStation(const Rail::Graph& Graph);
 UPROPERTY() TObjectPtr<ARailTambaramCorridorView> TambaramCorridor;
 UPROPERTY() TObjectPtr<ARailTambaramStationView> TambaramStation;
 UPROPERTY() TObjectPtr<ARailTambaramTurnoutView> TambaramTurnout;
 bool TambaramReady=false;
protected:
 virtual void BeginPlay() override;
};

