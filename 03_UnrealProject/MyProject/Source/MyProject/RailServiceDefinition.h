#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Simulation/RailService.h"
#include "RailServiceDefinition.generated.h"
UCLASS(BlueprintType)
class MYPROJECT_API URailServiceDefinition : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,Category="Service") FString ServiceId=TEXT("S7-Shantipur-01");
 UPROPERTY(EditAnywhere,Category="Service") FString ServiceName=TEXT("Shantipur shuttle");
 UPROPERTY(EditAnywhere,Category="Service") FString ContentVersion=TEXT("IR-S7-service-1");
 UPROPERTY(EditAnywhere,Category="Service") FString Formation=TEXT("Powered locomotive proxy + one coach");
 UPROPERTY(EditAnywhere,Category="Service") FString StationName=TEXT("Shantipur");
 UPROPERTY(EditAnywhere,Category="Service") int32 PlayerTrain=2;
 UPROPERTY(EditAnywhere,Category="Service") int32 TrafficTrain=1;
 UPROPERTY(EditAnywhere,Category="Service") int32 PlayerRoute=2;
 UPROPERTY(EditAnywhere,Category="Service") int32 TrafficRoute=1;
 UPROPERTY(EditAnywhere,Category="Service") double StartHead=100;
 UPROPERTY(EditAnywhere,Category="Station") double PlatformFrom=95;
 UPROPERTY(EditAnywhere,Category="Station") double PlatformTo=145;
 UPROPERTY(EditAnywhere,Category="Station") double BerthHead=140;
 UPROPERTY(EditAnywhere,Category="Station") double BerthTolerance=2;
 UPROPERTY(EditAnywhere,Category="Station") double FinishHead=152;
 UPROPERTY(EditAnywhere,Category="Station") int32 DwellTicks=300;
 UPROPERTY(EditAnywhere,Category="Service") double SpeedLimit=3;
 UPROPERTY(EditAnywhere,Category="Service") double AssistedCruise=2;
 UPROPERTY(EditAnywhere,Category="Timing") double ArrivalWindowSeconds=75;
 UPROPERTY(EditAnywhere,Category="Timing") double DepartureWindowSeconds=95;
 UPROPERTY(EditAnywhere,Category="Timing") double ExpectedDurationSeconds=240;
 UPROPERTY(EditAnywhere,Category="Timing") int32 RouteRetryTicks=30;
 UPROPERTY(EditAnywhere,Category="Result") int32 ReputationReward=10;
 UPROPERTY(EditAnywhere,Category="Result") FString Unlock=TEXT("Shantipur service mastery badge");
 UPROPERTY(EditAnywhere,Category="Result") TArray<double> ScoreWeights={50,20,15,10,5};
 UPROPERTY(EditAnywhere,Category="Objectives") TArray<FString> ObjectiveTitles;
 UPROPERTY(EditAnywhere,Category="Tutorial") TArray<FString> TutorialSteps;
 URailServiceDefinition(){Rail::ServiceDefinition Default;for(const auto& O:Default.objectives)ObjectiveTitles.Add(UTF8_TO_TCHAR(O.title.c_str()));for(const auto& T:Default.tutorial)TutorialSteps.Add(UTF8_TO_TCHAR(T.c_str()));}
 Rail::ServiceDefinition Record() const {
  Rail::ServiceDefinition D;D.id=TCHAR_TO_UTF8(*ServiceId);D.name=TCHAR_TO_UTF8(*ServiceName);D.version=TCHAR_TO_UTF8(*ContentVersion);D.formation=TCHAR_TO_UTF8(*Formation);D.station=TCHAR_TO_UTF8(*StationName);
  D.player=Rail::TrainId(PlayerTrain>0?uint64(PlayerTrain):0);D.traffic=Rail::TrainId(TrafficTrain>0?uint64(TrafficTrain):0);D.playerRoute=Rail::RouteId(PlayerRoute>0?uint64(PlayerRoute):0);D.trafficRoute=Rail::RouteId(TrafficRoute>0?uint64(TrafficRoute):0);
  D.startHead=StartHead;D.platformFrom=PlatformFrom;D.platformTo=PlatformTo;D.berthHead=BerthHead;D.berthTolerance=BerthTolerance;D.finishHead=FinishHead;D.dwellTicks=DwellTicks>0?uint64(DwellTicks):0;
  D.speedLimit=SpeedLimit;D.assistedCruise=AssistedCruise;D.arrivalWindowSeconds=ArrivalWindowSeconds;D.departureWindowSeconds=DepartureWindowSeconds;D.expectedDurationSeconds=ExpectedDurationSeconds;D.routeRetryTicks=RouteRetryTicks>0?uint64(RouteRetryTicks):0;D.reputationReward=ReputationReward>0?uint64(ReputationReward):0;D.unlock=TCHAR_TO_UTF8(*Unlock);
  if(ScoreWeights.Num()==5){D.completionPoints=ScoreWeights[0];D.safetyPoints=ScoreWeights[1];D.accuracyPoints=ScoreWeights[2];D.speedPoints=ScoreWeights[3];D.punctualityPoints=ScoreWeights[4];}else D.id.clear();
  if(ObjectiveTitles.Num()==9)for(int32 I=0;I<9;++I)D.objectives[I].title=TCHAR_TO_UTF8(*ObjectiveTitles[I]);else D.id.clear();
  if(TutorialSteps.Num()==4)for(int32 I=0;I<4;++I)D.tutorial[I]=TCHAR_TO_UTF8(*TutorialSteps[I]);else D.id.clear();return D;
 }
};
