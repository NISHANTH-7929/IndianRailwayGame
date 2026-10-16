#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Simulation/RailKernel.h"
#include "Simulation/RailHandling.h"
#include "Simulation/RailOperations.h"
#include "Simulation/RailService.h"
#include "Simulation/RailDispatchDiagnostics.h"
#include "RailServiceAudioModel.h"
#include "RailSimulationSubsystem.generated.h"
class ARailKernelView;
class URailDrivingWidget;
class ARailOperationsView;
class ULevelStreamingDynamic;
class ACameraActor;
class ARailChunkView;
class UStaticMesh;
class URailServiceWidget;
class ARailServiceView;
class UAudioComponent;
class USoundWave;
class USoundClass;
class USoundSubmix;
class ARailLocomotiveView;
class ARailEgmoreSceneView;
UCLASS()
class MYPROJECT_API URailSimulationSubsystem : public UTickableWorldSubsystem {
 GENERATED_BODY()
public:
 virtual void OnWorldBeginPlay(UWorld& InWorld) override;
 virtual void Deinitialize() override;
 virtual void Tick(float DeltaTime) override;
 virtual TStatId GetStatId() const override {RETURN_QUICK_DECLARE_CYCLE_STAT(URailSimulationSubsystem,STATGROUP_Tickables);}
 virtual bool IsTickable() const override {return Active&&!IsTemplate();}
 const Rail::Simulation& ReadOnlySimulation() const {return Simulation;}
 const Rail::LongitudinalState& ReadOnlyHandling() const {return Handling.Current();}
 Rail::Result SubmitPlayerCommand(Rail::CommandKind Kind,double Value);
 void ToggleHandlingPause();
 void ResetHandlingFixture();
 FString HandlingDisplay() const;
 bool IsAIMode() const {return AIMode;}
 bool IsContinuityMode() const {return ContinuityMode;}
 bool IsServiceMode() const {return ServiceMode;}
 bool IsServicePaused() const {return S6Paused;}
 bool IsPresentationBlocked() const {return S6Blocked;}
 ARailLocomotiveView* PlayerLocomotive() const;
 const Rail::ServiceRuntime& ReadOnlyService() const {return Service;}
 const Rail::OperationsWorld& ReadOnlyOperations() const {return Operations;}
 void ServiceAction(int32 Action);
 float GetServiceAudioVolume(int32 Channel) const;
 void SetServiceAudioVolume(int32 Channel,float Value);
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
 // Isolated render-review flag only: freeze this owner without altering its
 // checkpoint state while a disposable railway drives presentation components.
 void FreezeForWheelMotionReview();
 void PresentReviewAudio(const Rail::LongitudinalState& State,double Curvature,float Delta);
#endif
 void LocomotiveAction(int32 Action);
 void CabControl(const FString& Control);
 FString LocomotiveDisplay() const;
 bool HasLocomotivePresentation() const;
 bool IsLocomotiveCabCamera() const;
 FString ServicePanelText() const;
 FString ServiceHudText() const;
 FString ServicePrimaryLabel() const;
 void SaveS6Checkpoint();
 void ToggleS6LookBack();
 void CycleS6Chunks();
 bool IsOperationsMode() const {return OperationsMode;}
 void SelectTrain(uint64 Id);
 void RequestPlayerRoute(uint64 Id);
 void RequestPlayerPoint();
 void CancelPlayerRoute();
 void RecreateOperationsView();
private:
 friend class FRailAudioLifecycleTest;
 UPROPERTY() TObjectPtr<ARailLocomotiveView> LocomotiveView;
 UPROPERTY() TObjectPtr<ARailLocomotiveView> EgmoreTrafficLocomotive;
 UPROPERTY() TObjectPtr<ARailLocomotiveView> OpposingLocomotive;
 UPROPERTY() TObjectPtr<ARailEgmoreSceneView> EgmoreScene;
 double S8ReviewSeconds=0;
 int32 S8ReviewPhase=-1;
 void PresentService();
 Rail::DispatchWatchdog ServiceDispatchWatchdog;
 FString ServiceDispatchNotice;
 uint64 ServiceDispatchTick=0,ServiceDispatchRun=0;
 Rail::ServiceRuntime Service;
 bool ServiceMode=false,ServiceDebug=false;
 uint64 ServiceLastSaveTick=0,ServiceLastCue=0;
 FString ServiceNotice;
 UPROPERTY() TObjectPtr<URailServiceWidget> ServiceWidget;
 UPROPERTY() TObjectPtr<ARailServiceView> ServiceView;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceMovementAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceStationAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceHornAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceBrakeAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceCueAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceMotorAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceCoastAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceSquealAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceBrakeReleaseAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> ServiceEmergencyAudio;
 Rail::ServiceAudioDynamics ServiceAudioDynamics;
 UPROPERTY() TObjectPtr<UAudioComponent> TrafficHornAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> TrafficRollingAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> TrafficMotorAudio;
 Rail::PassingAudioDynamics TrafficHornDynamics,TrafficRollingDynamics;
 UPROPERTY() TObjectPtr<UAudioComponent> OpposingHornAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> OpposingRollingAudio;
 UPROPERTY() TObjectPtr<UAudioComponent> OpposingMotorAudio;
 Rail::PassingAudioDynamics OpposingHornDynamics,OpposingRollingDynamics;
 void UpdateTrafficAudio(float Delta,bool InCab);
 void ApplyTrafficAudio(const FVector& Horn,const FVector& Running,const FVector& Listener,double Speed,double Traction,float Delta,bool InCab,bool Opposing=false);
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
 double TrafficReviewSeconds=0;
 int32 TrafficReviewLastLoggedSecond=-1;
#endif
 UPROPERTY() TArray<TObjectPtr<USoundWave>> ServiceSounds;
 UPROPERTY() TObjectPtr<USoundClass> ServiceAudioClass;
 UPROPERTY() TObjectPtr<USoundSubmix> ServiceAudioSubmix;
 mutable bool ServiceAudioPreferencesLoaded=false;
 mutable float ServiceAudioVolumes[3]={1.f,1.f,1.f};
 bool ServiceAudioLoaded=false,ServiceAudioWasRunning=false,ServiceBrakeEngaged=false;
 float ServiceRailGain=0,ServiceStationGain=0,ServiceRailPitch=1;
 void InitializeServiceAudio();
 void UpdateServiceAudio();
 void ApplyServiceAudio(const Rail::LongitudinalState& State,double Curvature,float Delta,float Near,bool InCab);
 void StopServiceAudio();
 void DestroyServiceAudio();
 void PlayServiceHorn();
 void InitializeS6();
 void TickS6(float DeltaTime);
 void WriteS6Evidence();
 void SaveS6(const FString& Reason);
 void SuspendS6();
 FString S6Display() const;
 bool ContinuityMode=false,S6Paused=true,S6Blocked=true,S6LookBack=false,S6Milestone=false;
 bool S6RecoveryBlocked=false;
 double S6ReloadAfter=0,S6LastEvidence=0,S6SaveMilliseconds=0;
 uint64 S6Generation=0;
 FString S6Recovery=TEXT("Not restored"),S6SaveStatus=TEXT("No checkpoint"),S6StreamStatus;
 UPROPERTY() TArray<TObjectPtr<ULevelStreamingDynamic>> S6Levels;
 UPROPERTY() TObjectPtr<ACameraActor> S6Camera;
 double S6LoadStarted[2]={0,0};
 double S6LoadMilliseconds[2]={0,0};
 bool S6Ready[2]={false,false};
 uint32 S6Loads=0,S6Unloads=0;
 TArray<TWeakObjectPtr<ARailChunkView>> S6RetiredViews;
 TArray<TWeakObjectPtr<UStaticMesh>> S6RetiredMeshes;
 void CreateView();
 void WriteHandlingEvidence();
 void TickOperations(float DeltaTime);
 void WriteOperationsEvidence();
 FString OperationsDisplay() const;
 void SuspendFixture(){if(ContinuityMode){SuspendS6();return;}CancelInputRequested.Store(true);AppBackground.Store(true);}
 void ResumeFixture(){SkipNextDelta.Store(true);AppBackground.Store(false);}
 TAtomic<bool> AppBackground{false},SkipNextDelta{false};
 TAtomic<bool> CancelInputRequested{false};
 Rail::Simulation Simulation;
 Rail::HandlingModel Handling;
 Rail::OperationsWorld Operations;
 Rail::TrainId SelectedTrain{1};
 UPROPERTY() TObjectPtr<ARailOperationsView> OperationsView;
 UPROPERTY() TObjectPtr<ARailKernelView> View;
 UPROPERTY() TObjectPtr<URailDrivingWidget> DrivingWidget;
 bool HandlingMode=false;
 bool OperationsMode=false,AIMode=false;
 uint64 CommandSequence=0,LastEvidenceTick=0;
 bool Active=false,Started=false,Recreated=false,ChangedChoice=false;
 double Warmup=0;
 FString Status;
};

