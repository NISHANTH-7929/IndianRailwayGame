#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailOperations.h"
#include "RailTambaramTurnoutView.generated.h"
class UHierarchicalInstancedStaticMeshComponent;
class UStaticMesh;
class UMaterialInterface;

// Read-only, bounded turnout presentation. Never changes route or point state.
UCLASS()
class MYPROJECT_API ARailTambaramTurnoutView : public AActor {
 GENERATED_BODY()
public:
 ARailTambaramTurnoutView();
 void Present(const Rail::OperationsWorld& World,int32 ChunkId);
 void Release();
 bool IsReady() const { return bReady; }
 static bool ReplacesPlainTrack(int32 Track,double Chainage);
 int32 InstanceCount=0;
 FString Diagnostic;
private:
 UPROPERTY(Transient) TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Parts;
 UPROPERTY(Transient) TObjectPtr<UStaticMesh> Cube;
 UPROPERTY(Transient) TArray<TObjectPtr<UMaterialInterface>> Materials;
 bool bReady=false,bBuilt=false;
 int32 OwnerChunk=0,LastPoint1=-1,LastPoint2=-1;
 bool LoadAssets();
 void Build(int32 ChunkId,int32 Point1,int32 Point2);
};
