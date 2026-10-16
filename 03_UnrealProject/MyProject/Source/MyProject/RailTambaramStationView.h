#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailOperations.h"
#include "RailTambaramStationView.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UTextRenderComponent;
// Disposable station scenery, owned by the streamed chunk, never simulation state.
UCLASS()
class MYPROJECT_API ARailTambaramStationView : public AActor {
 GENERATED_BODY()
public:
 ARailTambaramStationView();
 bool Present(const Rail::OperationsWorld& World,int32 ChunkId);
 bool IsReady() const { return Ready; }
 FString TelemetryJson() const;
 int32 RebuildCount=0;
private:
 UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Parts;
 UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> Labels;
 int32 BuiltChunk=0,Placements=0;
 uint32 BuiltStations=0;
 bool Ready=false;
 FString Diagnostic;
};
