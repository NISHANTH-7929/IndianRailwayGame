#include "RailSimulationSubsystem.h"
#include "RailServiceView.h"
#include "RailControlPresentation.h"
#include "Simulation/RailTambaramDriving.h"
#include "Simulation/RailJourneyPresentation.h"
#include "Simulation/RailDrivingAdvice.h"
#include "Simulation/RailScenarioContent.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundWave.h"
#include "DrawDebugHelpers.h"
void URailSimulationSubsystem::PresentService(){
#if PLATFORM_ANDROID
 const bool KeepAwake=Service.Current().phase==Rail::ServicePhase::Running&&!S6Paused;
 if(KeepAwake!=ServiceKeepsScreenAwake){
  UKismetSystemLibrary::ControlScreensaver(!KeepAwake);
  ServiceKeepsScreenAwake=KeepAwake;
 }
#endif
 const auto Tick=Operations.Current().tick,Run=Service.Current().run;
 if(Run!=ServiceDispatchRun||Tick<ServiceDispatchTick){ServiceDispatchWatchdog.Reset();ServiceDispatchTick=0;ServiceDispatchNotice.Empty();ServiceDispatchRun=Run;}
 // Once per simulation second; a pause cannot age the diagnostic clock.
 if(Tick>=ServiceDispatchTick+30){
  ServiceDispatchTick=Tick;ServiceDispatchNotice.Empty();
  for(const auto& Hold:ServiceDispatchWatchdog.Observe(Operations))if(Hold.train==Service.Definition().player&&(Hold.prolonged||Hold.cyclic)){
   ServiceDispatchNotice=Hold.cyclic?TEXT("Cyclic route dependency: remain stopped; dispatcher action required."):TEXT("Prolonged route wait: no recent train movement. Remain stopped; authority is unchanged.");break;
  }
 }
 if(ServiceView)ServiceView->Present(Service.Definition(),Operations,HasLocomotivePresentation());
 if(S6Paused)FlushDebugStrings(GetWorld());
 UpdateServiceAudio();
}
FString URailSimulationSubsystem::ServicePrimaryLabel() const {
 const auto& S=Service.Current();
 switch(S.phase){case Rail::ServicePhase::Selection:return FString::Printf(TEXT("SELECT %s"),UTF8_TO_TCHAR(Service.Definition().station.c_str()));
 case Rail::ServicePhase::Briefing:return S.formationConfirmed?TEXT("START SERVICE"):TEXT("CONFIRM FORMATION");
 case Rail::ServicePhase::Running:return S.accumulator>4.0/30.0+1e-9?TEXT("RESTORE LAST CHECKPOINT"):TEXT("RESUME SERVICE");
 default:return S.success&&!S.settled?TEXT("SAVE REPUTATION + UNLOCK"):S.success?TEXT("REPLAY SERVICE"):TEXT("RETRY SERVICE");}
}
FString URailSimulationSubsystem::ServicePanelText() const {
 const auto& D=Service.Definition();const auto& S=Service.Current();FString Text;
 if(S6RecoveryBlocked)return TEXT("SAFE RECOVERY PAUSE\nNo compatible checkpoint could be recovered. Existing records were retained.\n")+S6Recovery;
 if(S.phase==Rail::ServicePhase::Selection){
  Text=FString::Printf(TEXT("%s\nAbout %.0f minutes | %s\nWait for the express, serve %s, then stop at Finish.\nReputation: %llu | Mastery badge: %s\nProvisional meshes and sounds. Fully offline."),UTF8_TO_TCHAR(D.name.c_str()),D.expectedDurationSeconds/60,UTF8_TO_TCHAR(D.formation.c_str()),UTF8_TO_TCHAR(D.station.c_str()),Service.Profile().reputation,Service.Profile().unlocked?TEXT("Unlocked"):TEXT("Locked"));
  if(D.terminalStop)Text=FString::Printf(TEXT("%s\nAbout %.0f minutes | %s\nDrive to %s and remain stopped through the terminal dwell.\nReputation: %llu | Mastery badge: %s"),UTF8_TO_TCHAR(D.name.c_str()),D.expectedDurationSeconds/60,UTF8_TO_TCHAR(D.formation.c_str()),UTF8_TO_TCHAR(D.station.c_str()),Service.Profile().reputation,Service.Profile().unlocked?TEXT("Unlocked"):TEXT("Locked"));
 }else if(S.phase==Rail::ServicePhase::Briefing){
  const auto* Train=Operations.Spatial(D.player);
  Text=FString::Printf(TEXT("CONFIRM YOUR SERVICE\n%s | %.2f m | Compatible %.0f m platform\nMode: %s. Limit: %.1f km/h. Forward-only itinerary.\n"),UTF8_TO_TCHAR(D.formation.c_str()),Train?Train->Current().properties.lengthMetres:0.0,D.platformTo-D.platformFrom,S.mode==Rail::DrivingMode::Assisted?TEXT("Assisted braking"):TEXT("Standard Loco Pilot"),D.speedLimit*3.6);
  for(const auto& Step:D.tutorial)Text+=FString(UTF8_TO_TCHAR(Step.c_str()))+TEXT("\n");
  Text+=TEXT("To move: release Brake, then apply Power. Pause saves your service.");
  if(Rail::ScenarioContent::IsTambaram(D.contentId)){
   Text+=Rail::ScenarioContent::IsLiveJourney(D.contentId)?TEXT("\nTraffic: wait for the priority express, then cross onto your authorised line. Three opposing services run on separate protected tracks."):TEXT("\nTraffic: a crossing express at Egmore and one northbound train on the adjacent line. Your authorised route runs straight through the junction.");
   Text+=S.mode==Rail::DrivingMode::Assisted?TEXT("\nAssisted mode controls cruise speed and braking after you apply power. Choose Standard below to drive power and brakes yourself."):TEXT("\nStandard: you control acceleration, coasting and braking. Observe signals and stop at Tambaram; no automatic station braking.");
  }
 }else if(S.phase==Rail::ServicePhase::Running){
  Text=S.accumulator>4.0/30.0+1e-9?TEXT("SERVICE PAUSED: FRAME OVERLOAD\nRestore the last verified checkpoint to continue.\nProgress since that checkpoint will be lost."):TEXT("SERVICE PAUSED\nYour train, traffic, station and objectives are retained.\nResume when ready, or Save and exit.");
 }else{
  const FString BerthSummary=S.objectives[4]==1?FString::Printf(TEXT("Berth error: %+.2f m (tolerance %.1f m)"),S.berthError,D.berthTolerance):TEXT("Berth: no valid station stop recorded");
  Text=FString::Printf(TEXT("%s | Grade %s | %.1f / 100\n%s\n%s\nTraffic delay: %.1f s — excluded from lateness\nPlayer lateness: %.1f s | Overspeed: %.1f s\nReputation: %llu | %s\n%s"),S.success?TEXT("SERVICE COMPLETE"):TEXT("SERVICE NOT COMPLETED"),UTF8_TO_TCHAR(S.grade.c_str()),S.score,UTF8_TO_TCHAR(S.reason.c_str()),*BerthSummary,double(S.trafficTicks)/30,S.playerDelaySeconds,double(S.overspeedTicks)/30,Service.Profile().reputation,Service.Profile().unlocked?TEXT("Mastery badge unlocked"):TEXT("Mastery badge locked"),S.success?(S.settled?TEXT("Reward saved. Replay is ready."):TEXT("Save your +10 reputation and mastery badge below.")):TEXT("Retry starts a new attempt; earned reputation is retained."));
 }
 if(S.phase==Rail::ServicePhase::Result){const auto Points=Service.ScoreComponents();Text+=FString::Printf(TEXT("\nPoints: completion %.1f | safety %.1f | stop %.1f | speed %.1f | timing %.1f"),Points[0],Points[1],Points[2],Points[3],Points[4]);}
 if(Rail::ScenarioContent::IsTambaram(D.contentId)&&(S.phase==Rail::ServicePhase::Selection||S.phase==Rail::ServicePhase::Briefing))Text+=TEXT("\nRepresentative WAP-7/LHB service; not an exact dated timetable or stock allocation.");
 Text+=TEXT("\n")+LocomotiveDisplay();if(!ServiceNotice.IsEmpty())Text+=TEXT("\n")+ServiceNotice;return Text;
}
FString URailSimulationSubsystem::ServiceHudText() const {
 const auto& D=Service.Definition();const auto& S=Service.Current();const auto* H=Operations.Handling(D.player);const auto* Train=Operations.Spatial(D.player);if(!H||!Train)return TEXT("Loading service");
 const auto& A=Operations.Current().authorities.at(D.player);const auto& Signal=Operations.Current().signals.at(Rail::SignalId(2));
 const auto& Boundary=Operations.DefinitionsView().signals.at(Rail::SignalId(2)).boundary;
 bool SignalBehind=false;
 for(const auto& Occurrence:Train->History().occurrences){
  if(Occurrence.directed.edge!=Boundary.edge)continue;
  const double Offset=Occurrence.directed.direction==1?Boundary.distanceMetres:
   Occurrence.endMetres-Occurrence.beginMetres-Boundary.distanceMetres;
  SignalBehind=Train->Current().headStationMetres>Occurrence.beginMetres+Offset;
  break;
 }
 const double Target=S.departureAuthorized?D.finishHead:D.berthHead;
 const auto Controls=Rail::ControlPresentation::From(*H);
 const double RouteLimit=Rail::ScenarioContent::IsTambaram(D.contentId)?Rail::TambaramDriving::ApplicableLimit(Train->Current().headStationMetres,Train->Current().properties.lengthMetres,D.speedLimit):D.speedLimit;
 FString Text=FString::Printf(TEXT("%s | %s | SPEED %.1f / %.1f km/h\nSignal S2: %s | %s\n%s: %.1f m ahead | %s | Dwell %.1f / %.1f s\nApplied Power %.0f%% / Brake %.0f%% | %s | %s"),UTF8_TO_TCHAR(D.name.c_str()),S.mode==Rail::DrivingMode::Assisted?TEXT("ASSISTED"):TEXT("STANDARD"),FMath::Abs(H->speedMps)*3.6,RouteLimit*3.6,SignalBehind&&A.routeGranted?TEXT("BEHIND TRAIN; ROUTE HELD"):Signal.aspect==Rail::SignalAspect::Proceed?TEXT("PROCEED"):TEXT("STOP"),UTF8_TO_TCHAR(Service.HoldReason(Operations).c_str()),S.departureAuthorized?TEXT("Finish marker"):TEXT("Station marker"),Target-Train->Current().headStationMetres,UTF8_TO_TCHAR(Rail::StationPhaseName(S.station)),double(S.dwellTicks)/30,double(D.dwellTicks)/30,Controls.power*100,Controls.brake*100,Controls.emergency?TEXT("EMERGENCY"):Controls.direction>0?TEXT("FWD"):Controls.direction<0?TEXT("REV"):TEXT("NEUTRAL"),S6Paused?TEXT("PAUSED"):S6Blocked?TEXT("LOADING: safely paused"):TEXT("RUNNING"));
 if(Rail::ScenarioContent::IsTambaram(D.contentId)){
  const double Head=Train->Current().headStationMetres;
  const auto Journey=Rail::JourneyPresentation::At(Head,Train->Current().properties.lengthMetres);
  const FString Distance=FMath::Abs(Journey.distance)>=1000?FString::Printf(TEXT("%.1f km"),FMath::Abs(Journey.distance)/1000):FString::Printf(TEXT("%.0f m"),FMath::Abs(Journey.distance));
  const FString Location=FString::Printf(TEXT("%s %s | %s"),Journey.alongside?TEXT("At"):TEXT("Next:"),UTF8_TO_TCHAR(Rail::TambaramRoute::StationNames[Journey.station]),Journey.terminal?TEXT("Scheduled stop"):Journey.station==0?TEXT("Departure station"):TEXT("Passing station - no stop scheduled"));
  const FString Authority=A.routeGranted?TEXT("Route authorised - observe lineside signals"):FString(UTF8_TO_TCHAR(Service.HoldReason(Operations).c_str()));
  Text=FString::Printf(TEXT("%.1f km/h | Limit %.0f | %s%s\n%s\n%s | %s %s\nJourney %02llu:%02llu | %.0f%% | Power %.0f%% | Brake %.0f%%"),FMath::Abs(H->speedMps)*3.6,RouteLimit*3.6,S.mode==Rail::DrivingMode::Assisted?TEXT("Assisted"):TEXT("Standard"),Controls.emergency?TEXT(" | EMERGENCY BRAKE"):TEXT(""),*Authority,*Location,*Distance,Journey.distance>=0?TEXT("ahead"):TEXT("behind cab"),S.ticks/1800,(S.ticks/30)%60,Journey.progress*100,Controls.power*100,Controls.brake*100);
  if(Journey.terminal)Text+=FString::Printf(TEXT("\nTambaram berth: %+.0f m | Dwell %.1f / %.1f s"),D.berthHead-Head,double(S.dwellTicks)/30,double(D.dwellTicks)/30);
  const double Approach=Rail::TambaramDriving::PermittedSpeed(Head,Train->Current().properties.lengthMetres,D.speedLimit)*3.6;
  if(Approach+1<RouteLimit*3.6)Text+=FString::Printf(TEXT("\nRestriction ahead - reduce towards %.0f km/h"),Approach);
  if(S.mode==Rail::DrivingMode::Standard){
   switch(Rail::StandardDrivingAdvice(H->speedMps,RouteLimit,Approach/3.6,D.berthHead-Head,D.berthTolerance,S.station==Rail::StationPhase::Dwelling)){
    case Rail::DrivingAdvice::Overspeed:Text+=TEXT("\nOVERSPEED - cut power and apply brake");break;
    case Rail::DrivingAdvice::Restriction:Text+=TEXT("\nREDUCE SPEED - brake for the approaching restriction");break;
    case Rail::DrivingAdvice::BrakeForBerth:Text+=TEXT("\nTAMBARAM APPROACH - cut power and begin braking; judge your stopping distance");break;
    case Rail::DrivingAdvice::PositionAtBerth:Text+=TEXT("\nBERTH MARKER - creep into position, then hold the brake");break;
    case Rail::DrivingAdvice::HoldDwell:Text+=TEXT("\nDWELL - remain stopped with the brake applied");break;
    default:break;
   }
  }
  // Derive encounter information from the live railway, never from a timer or
  // a decorative spawn. The message cannot grant or change a route.
  Rail::Sample PlayerHead;
  if(Train->Network().SamplePosition(Train->Current().head,PlayerHead).ok){
   for(uint64 Id=3;Id<=Rail::ScenarioContent::TrainCount(D.contentId);++Id)if(const auto* Other=Operations.Spatial(Rail::TrainId(Id))){
    Rail::Sample OtherHead;
    if(Other->Network().SamplePosition(Other->Current().head,OtherHead).ok){
     const auto Delta=OtherHead.position-PlayerHead.position;
     const double Along=Delta.x*PlayerHead.tangent.x+Delta.y*PlayerHead.tangent.y;
     const double DistanceToTrain=Delta.Length();
     const TCHAR* Side=(-PlayerHead.tangent.y*Delta.x+PlayerHead.tangent.x*Delta.y)>=0?TEXT("right"):TEXT("left");
     if(DistanceToTrain<1400&&Along>0)Text+=FString::Printf(TEXT("\nOncoming train on your %s: %.0f m ahead | CAMERA for side views"),Side,DistanceToTrain);
     else if(DistanceToTrain<350&&Along+Other->Current().properties.lengthMetres>=-Train->Current().properties.lengthMetres)Text+=FString::Printf(TEXT("\nTrain passing on your %s - maintain your route"),Side);
    }
   }
  }
  double Junction=0;for(size_t I=0;I<Rail::TambaramRoute::CrossoverSegment;++I)Junction+=Rail::TambaramRoute::GeometryFor(1,I).lengthMetres;
  if(Head<Junction+180){
   if(!S.trafficCleared)Text+=TEXT("\nJunction: wait for the priority express to clear");
   else Text+=Rail::ScenarioContent::IsLiveJourney(D.contentId)?TEXT("\nJunction: protected crossover, 30 km/h | CAMERA: forward route view"):TEXT("\nJunction: points set for your straight route after the express");
  }
  if(S6Blocked)Text+=TEXT("\nLoading scenery - service safely paused");
 }
 if(!A.routeGranted&&!ServiceDispatchNotice.IsEmpty())Text+=TEXT("\n")+ServiceDispatchNotice;
 if(!ServiceNotice.IsEmpty())Text+=TEXT("\n")+ServiceNotice;
 if(ServiceDebug){
  const auto Points=Service.ScoreComponents();Text+=FString::Printf(TEXT("\nPhase %s | Berth %.2f +/- %.2f m | points %.1f / %.1f / %.1f / %.1f / %.1f"),UTF8_TO_TCHAR(Rail::ServicePhaseName(S.phase)),D.berthHead,D.berthTolerance,Points[0],Points[1],Points[2],Points[3],Points[4]);
  Text+=FString::Printf(TEXT("\nDEBUG %s run%llu | T%llu / AI T%llu | tick%llu | authority %.2f..%.2f | generation%llu\n"),UTF8_TO_TCHAR(D.id.c_str()),S.run,D.player.value,D.traffic.value,Operations.Current().tick,A.minimumRear,A.maximumHead,S6Generation);
  for(size_t I=0;I<D.objectives.size();++I)Text+=FString::Printf(TEXT("%s: %u; "),UTF8_TO_TCHAR(D.objectives[I].title.c_str()),S.objectives[size_t(D.objectives[I].kind)]);
  Text+=FString::Printf(TEXT("\nSettlement %s/%llu: %s | Reputation %llu | %s\n%s"),UTF8_TO_TCHAR(D.id.c_str()),S.run,S.settled?TEXT("SAVED"):TEXT("not settled"),Service.Profile().reputation,*S6SaveStatus,*S6Recovery);
 }return Text;
}



