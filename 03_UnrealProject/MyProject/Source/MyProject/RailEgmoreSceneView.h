#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Simulation/RailOperations.h"
#include "RailEgmoreSceneView.generated.h"

class UHierarchicalInstancedStaticMeshComponent;
class ACameraActor;
class UStaticMeshComponent;
class UMaterialInstanceDynamic;

// Disposable station presentation; operations remain the signal authority.
UCLASS()
class MYPROJECT_API ARailEgmoreSceneView : public AActor {
 GENERATED_BODY()
public:
 ARailEgmoreSceneView();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 UFUNCTION(CallInEditor, BlueprintCallable, Category="Railway|Egmore")
 bool RebuildScene();
 bool PresentSignals(const Rail::OperationsWorld& World);
 bool IsServiceReady() const;
 FString TelemetryJson() const;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Railway|Egmore")
 FString ImportDiagnostic;
 UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Railway|Egmore")
 int32 PlacementCount=0;
 UPROPERTY(EditAnywhere, Category="Railway|Egmore")
 bool IncludeServiceTrackExtension=false;
private:
 UPROPERTY() TArray<TObjectPtr<UHierarchicalInstancedStaticMeshComponent>> Modules;
 UPROPERTY() TObjectPtr<ACameraActor> ReviewCamera;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> SignalParts;
 UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> SignalLamps;
 TArray<uint64> SignalIds;
 FString SignalEvidence=TEXT("[]");
 bool SceneReady=false,SignalsReady=false;
 int32 ReviewView=0;
 float ReviewElapsed=0;
 void ApplyReviewCamera();
};
