#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailOperations.h"
#include "RailTambaramCorridorView.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Disposable streamed presentation only. All distances resolve on the simulation graph.
UCLASS()
class MYPROJECT_API ARailTambaramCorridorView : public AActor {
 GENERATED_BODY()
public:
 ARailTambaramCorridorView();
 void Present(const Rail::OperationsWorld& World,int32 ChunkId);
 bool IsReady() const { return bReady; }
 void Release();
 int32 ActiveCells=0,InstanceCount=0,StoneInstances=0;
 uint64 RebuildCount=0,ReleasedCells=0;
 FString Diagnostic;
private:
 struct FCell { TArray<UHierarchicalInstancedStaticMeshComponent*> Parts; int32 Instances=0,Stones=0; };
 TMap<int32,FCell> Cells;
 UPROPERTY(Transient) TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Components;
 UPROPERTY(Transient) TArray<TObjectPtr<UStaticMesh>> Meshes;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> Materials;
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> LeafCluster;
 UPROPERTY(Transient) TObjectPtr<UMaterialInterface> LeafMaterial;
 UPROPERTY(Transient) TObjectPtr<UMaterialInterface> GroundMaterial;
 UPROPERTY(Transient) TObjectPtr<UMaterialInterface> WaterMaterial;
 UPROPERTY(Transient) TArray<TObjectPtr<UStaticMeshComponent>> SignalParts;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInstanceDynamic>> SignalLamps;
 TArray<uint64> SignalIds;
 bool PresentSignals(const Rail::OperationsWorld& World,int32 ChunkId);
 bool bReady=false,bAssetsAttempted=false;
 int32 OwnerChunk=0;
 bool LoadAssets();
 void BuildCell(const Rail::Graph& Graph,int32 Track,int32 CellIndex,int32 ChunkId,FCell& Cell);
 void RemoveCell(int32 Key);
};
