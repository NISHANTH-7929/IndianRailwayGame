#include "RailSimulationSubsystem.h"
#include "RailLocoSoundComponent.h"
#include "Simulation/RailJourneyPresentation.h"
#include "Simulation/RailScenarioContent.h"
#include "Simulation/RailFormationContent.h"
#include "Simulation/RailTrackJoints.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Sound/SoundWave.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/ConfigCacheIni.h"
#include "AudioDevice.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#include "GameFramework/PlayerController.h"
#include "Sound/SoundAttenuation.h"

namespace {
// Wheel strikes due within the next ~0.6 s: every axle of the train at every real joint on its
// path (insulated joints at signals, switch and crossing nose at turnouts), heard from the cab.
void CollectWheelStrikes(const Rail::OperationsWorld& World,Rail::TrainId Train,double Odometer,double Speed,TArray<Rail::WheelStrike>& Out){
 const auto* S=World.Spatial(Train);if(!S||Speed<.3||S->Current().commandedSpeedMps<0)return;
 const auto& Snap=S->Current();const double Head=Snap.headStationMetres,Window=Speed*.6+3;
 const auto Joints=Rail::JointsBetween(World,Train,Head-Snap.properties.lengthMetres-1,Head+Window);
 if(Joints.empty())return;
 uint64 Axle=0;
 for(const auto& Vehicle:Snap.vehicles){
  const bool Loco=Vehicle.definition==Rail::DefinitionId(Rail::FormationContent::Wap7DefinitionId);
  const double Bogie=Loco?Rail::FormationContent::Wap7BogieOffset:Rail::FormationContent::BogieOffset;
  const double Pitch=Loco?Rail::Wap7AxlePitch:Rail::FormationContent::BogieWheelbase;
  for(double Side:{1.0,-1.0})for(int K=0;K<(Loco?3:2);++K){
   const double Station=Vehicle.centreStationMetres+Side*Bogie+Pitch*(Loco?K-1:K-.5);
   const double Gain=Rail::StrikeGain(Head-Station,Loco);++Axle;
   for(const auto& J:Joints){const double Ahead=J.station-Station;if(Ahead<=0||Ahead>=Window)continue;
    const auto Kind=J.kind==Rail::JointKind::CrossingNose?Rail::StrikeKind::CrossingNose:J.kind==Rail::JointKind::Switch?Rail::StrikeKind::Switch:Rail::StrikeKind::Joint;
    Out.Add({J.key*64+Axle,Odometer+Ahead,Gain,Kind});}
  }
 }
}
}

float URailSimulationSubsystem::GetServiceAudioVolume(int32 Channel) const {
 if(!ServiceAudioPreferencesLoaded){
  ServiceAudioPreferencesLoaded=true;
  // Review renders always use defaults and must not touch personal preferences.
  if(!FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview"))&&GConfig){
   const TCHAR* Keys[]={TEXT("Master"),TEXT("Engine"),TEXT("Effects")};
   for(int32 I=0;I<3;++I){float V=1;GConfig->GetFloat(TEXT("Rail.Audio"),Keys[I],V,GGameUserSettingsIni);ServiceAudioVolumes[I]=FMath::IsFinite(V)?FMath::Clamp(V,0.f,1.f):1.f;}
  }
 }
 return Channel>=0&&Channel<3?ServiceAudioVolumes[Channel]:0.f;
}
void URailSimulationSubsystem::SetServiceAudioVolume(int32 Channel,float Value){
 if(Channel<0||Channel>=3||!FMath::IsFinite(Value))return;
 GetServiceAudioVolume(Channel);
 ServiceAudioVolumes[Channel]=FMath::Clamp(Value,0.f,1.f);
 if(GConfig&&!FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview"))){
  const TCHAR* Keys[]={TEXT("Master"),TEXT("Engine"),TEXT("Effects")};
  GConfig->SetFloat(TEXT("Rail.Audio"),Keys[Channel],ServiceAudioVolumes[Channel],GGameUserSettingsIni);
  GConfig->Flush(false,GGameUserSettingsIni);
 }
}

void URailSimulationSubsystem::InitializeServiceAudio(){
 if(ServiceAudioLoaded||!IsValid(GetWorld()))return;
 ServiceAudioLoaded=true;
 // Dedicated transient service bus: never alter shared source assets or the
 // global master mix. Measured quiet sources need makeup gain before limiting.
 ServiceAudioSubmix=NewObject<USoundSubmix>(this);
 auto* Limiter=NewObject<USubmixEffectDynamicsProcessorPreset>(ServiceAudioSubmix);
 FSubmixEffectDynamicsProcessorSettings Settings;
 Settings.DynamicsProcessorType=ESubmixEffectDynamicsProcessorType::Limiter;
 Settings.PeakMode=ESubmixEffectDynamicsPeakMode::Peak;
 Settings.LinkMode=ESubmixEffectDynamicsChannelLinkMode::Peak;
 Settings.InputGainDb=18.f;Settings.ThresholdDb=-6.f;Settings.OutputGainDb=-3.f;Settings.KneeBandwidthDb=0.f;
 Settings.LookAheadMsec=10.f;Settings.AttackTimeMsec=1.f;Settings.ReleaseTimeMsec=100.f;
 // Init copies the serialized Settings into the render-thread settings. Set
 // both: SetSettings alone is overwritten when a newly created preset initializes.
 Settings.bAnalogMode=false;Limiter->Settings=Settings;Limiter->SetSettings(Settings);
 ServiceAudioSubmix->SubmixEffectChain.Add(Limiter);
 ServiceAudioClass=NewObject<USoundClass>(this);
 ServiceAudioClass->Properties.bApplyEffects=false;
 ServiceAudioClass->Properties.DefaultSubmix=ServiceAudioSubmix;
 if(auto Device=GetWorld()->GetAudioDevice()){
  Device->RegisterSoundSubmix(ServiceAudioSubmix,true);
  Device->RegisterSoundClass(ServiceAudioClass);
 }
 auto Create=[&](const TCHAR* Path,float Gain)->UAudioComponent*{
  auto* Sound=LoadObject<USoundWave>(nullptr,Path);
  if(!Sound){UE_LOG(LogTemp,Error,TEXT("RAIL_AUDIO missing %s"),Path);return nullptr;}
  ServiceSounds.AddUnique(Sound);
  auto* Component=NewObject<UAudioComponent>(this);
  Component->bAutoActivate=false;Component->bAutoDestroy=false;
  Component->bAllowSpatialization=false;Component->bIsUISound=false;
  Component->SoundClassOverride=ServiceAudioClass;
  Component->SetSound(Sound);Component->SetVolumeMultiplier(Gain);
  Component->RegisterComponentWithWorld(GetWorld());
  UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO loaded %s duration=%.3f looping=%d"),Path,Sound->Duration,Sound->bLooping);
  return Component;
 };
 FSoundAttenuationSettings Spatial;
 Spatial.bSpatialize=true;Spatial.bAttenuate=false;Spatial.bAttenuateWithLPF=false;
 auto Synth=[&](uint32 Seed,uint32 Layers,bool Spatialized)->URailLocoSoundComponent*{
  auto* Component=NewObject<URailLocoSoundComponent>(this);
  Component->Configure(Seed,Layers);Component->SoundClass=ServiceAudioClass;Component->bIsUISound=false;
  Component->bAllowSpatialization=Spatialized;
  if(Spatialized){Component->bOverrideAttenuation=true;Component->AttenuationOverrides=Spatial;}
  Component->RegisterComponentWithWorld(GetWorld());
  return Component;
 };
 ServiceStationAudio=Create(TEXT("/Game/IndianRailway/S10/AudioV1/SW_StationChatter.SW_StationChatter"),0);
 ServiceHornAudio=Create(TEXT("/Game/IndianRailway/S10/AudioV1/SW_Wap7HornLoop.SW_Wap7HornLoop"),.65f);
 ServiceBrakeAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Brake.SW_Brake"),.35f);
 ServiceCueAudio=Create(TEXT("/Game/IndianRailway/S7/SW_StationCue.SW_StationCue"),.18f);
 ServiceLocoSound=Synth(11,Rail::SoundAll,false);
 TrafficHornAudio=Create(TEXT("/Game/IndianRailway/S10/AudioV1/SW_TrafficHorn.SW_TrafficHorn"),0);
 OpposingHornAudio=Create(TEXT("/Game/IndianRailway/S10/AudioV1/SW_TrafficHorn.SW_TrafficHorn"),0);
 TrafficLocoSound=Synth(31,Rail::SoundDrive|Rail::SoundAuxiliaries|Rail::SoundAir,true);
 TrafficRollingSound=Synth(37,Rail::SoundRunning,true);
 OpposingLocoSound=Synth(47,Rail::SoundDrive|Rail::SoundAuxiliaries|Rail::SoundAir,true);
 OpposingRollingSound=Synth(53,Rail::SoundRunning,true);
 for(auto* Component:{TrafficHornAudio.Get(),OpposingHornAudio.Get()})if(Component){
  Component->bAllowSpatialization=true;Component->AdjustAttenuation(Spatial);
 }
}

void URailSimulationSubsystem::StopServiceAudio(){
 for(auto* Audio:{OpposingHornAudio.Get(),TrafficHornAudio.Get()})if(IsValid(Audio))Audio->Stop();
 for(auto* Sound:{OpposingLocoSound.Get(),OpposingRollingSound.Get(),TrafficLocoSound.Get(),TrafficRollingSound.Get(),ServiceLocoSound.Get()})if(IsValid(Sound))Sound->Stop();
 OpposingHornDynamics.Suspend();OpposingRollingDynamics.Suspend();
 TrafficHornDynamics.Suspend();TrafficRollingDynamics.Suspend();
 for(auto* Audio:{ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),ServiceCueAudio.Get()})if(IsValid(Audio))Audio->Stop();
 ServiceAudioWasRunning=false;ServiceStationGain=0;ServiceHornHeld=false;ServiceHornUntil=0;
 ServiceAudioDynamics.Reset();ServiceStrikes.Reset();
}

void URailSimulationSubsystem::DestroyServiceAudio(){
 StopServiceAudio();
 for(auto* Audio:{OpposingHornAudio.Get(),TrafficHornAudio.Get(),ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),ServiceCueAudio.Get()})if(IsValid(Audio))Audio->DestroyComponent();
 for(auto* Sound:{OpposingLocoSound.Get(),OpposingRollingSound.Get(),TrafficLocoSound.Get(),TrafficRollingSound.Get(),ServiceLocoSound.Get()})if(IsValid(Sound))Sound->DestroyComponent();
 OpposingHornAudio=nullptr;OpposingLocoSound=nullptr;OpposingRollingSound=nullptr;
 OpposingHornDynamics={};OpposingRollingDynamics={};
 TrafficHornAudio=nullptr;TrafficLocoSound=nullptr;TrafficRollingSound=nullptr;
 TrafficHornDynamics={};TrafficRollingDynamics={};
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
 TrafficReviewSeconds=0;TrafficReviewLastLoggedSecond=-1;
#endif
 ServiceStationAudio=nullptr;ServiceHornAudio=nullptr;ServiceBrakeAudio=nullptr;ServiceCueAudio=nullptr;ServiceLocoSound=nullptr;
 ServiceOdometer={};ServiceSounds.Reset();ServiceAudioLoaded=false;
 if(GetWorld())if(auto Device=GetWorld()->GetAudioDevice()){
  if(ServiceAudioClass)Device->UnregisterSoundClass(ServiceAudioClass);
  if(ServiceAudioSubmix)Device->UnregisterSoundSubmix(ServiceAudioSubmix,false);
 }
 ServiceAudioClass=nullptr;ServiceAudioSubmix=nullptr;
}

void URailSimulationSubsystem::ServiceHorn(bool Down){
 if(Down){PlayServiceHorn(true);return;}
 // Released: the valve closes now, but a tap still gives a short blast.
 ServiceHornHeld=false;
}

void URailSimulationSubsystem::PlayServiceHorn(bool Held){
 if(S6Paused||S6Blocked||AppBackground.Load()||Service.Current().phase!=Rail::ServicePhase::Running)return;
 InitializeServiceAudio();
 if(!IsValid(ServiceHornAudio)||!GetWorld())return;
 // The horn sounds while the switch is held (minimum 0.35 s); a one-shot request is a 1 s blast.
 // ServiceHornUntil < 0 marks a blast already fading out; a new press restarts the horn.
 const double Now=GetWorld()->GetRealTimeSeconds();
 const bool Sounding=ServiceHornAudio->IsPlaying()&&ServiceHornUntil>=0;
 ServiceHornHeld=Held;ServiceHornUntil=FMath::Max(Sounding?ServiceHornUntil:0.,Now+(Held?.35:1.0));
 const auto AudioPerspective=ServiceAudioDynamics.Perspective(IsLocomotiveCabCamera(),0);
 ServiceHornAudio->SetLowPassFilterEnabled(true);
 ServiceHornAudio->SetLowPassFilterFrequency(float(AudioPerspective.lowPassHz));
 ServiceHornAudio->SetVolumeMultiplier(float(AudioPerspective.hornGain)*GetServiceAudioVolume(0)*GetServiceAudioVolume(2));
 if(!Sounding){ServiceHornAudio->FadeIn(.04f,1.f);UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO horn held=%d"),Held);}
}

void URailSimulationSubsystem::UpdateServiceAudio(){
 const auto& State=Service.Current();const auto& Definition=Service.Definition();
 const auto* H=Operations.Handling(Definition.player);const auto* Train=Operations.Spatial(Definition.player);
 const bool Running=H&&Train&&IsValid(GetWorld())&&!S6Paused&&!S6Blocked&&!AppBackground.Load()&&State.phase==Rail::ServicePhase::Running;
 if(!Running){StopServiceAudio();ServiceLastCue=State.cue;return;}
 InitializeServiceAudio();
 if(!ServiceAudioWasRunning){ServiceLastCue=State.cue;ServiceAudioWasRunning=true;}
 if(State.cue!=ServiceLastCue){ServiceLastCue=State.cue;if(ServiceCueAudio){ServiceCueAudio->SetVolumeMultiplier(.18f*GetServiceAudioVolume(0)*GetServiceAudioVolume(2));ServiceCueAudio->Play();}}
 if(IsValid(ServiceHornAudio)&&ServiceHornAudio->IsPlaying()&&ServiceHornUntil>=0&&!ServiceHornHeld&&GetWorld()->GetRealTimeSeconds()>=ServiceHornUntil){
  ServiceHornAudio->FadeOut(.25f,0.f);ServiceHornUntil=-1; // the valve closes: the blast dies away
 }
 const float Delta=FMath::Clamp(GetWorld()->GetDeltaSeconds(),0.f,.1f);
 const bool InCab=IsLocomotiveCabCamera();
 const auto* Edge=Train->Network().Find(Train->Current().head.edge);
 const float Distance=float(FMath::Abs(Train->Current().headStationMetres-(Definition.platformFrom+Definition.platformTo)*.5));
 const float Near=Rail::ScenarioContent::IsTambaram(Definition.contentId)?float(Rail::JourneyPresentation::StationAmbience(Train->Current().headStationMetres)):FMath::Clamp(1.f-Distance/120.f,0.f,1.f);
 ServiceOdometer.Advance(Definition.player.value,Train->Current().headStationMetres);
 ServiceStrikes.Reset();CollectWheelStrikes(Operations,Definition.player,ServiceOdometer.metres,FMath::Abs(H->speedMps),ServiceStrikes);
 ApplyServiceAudio(*H,Edge?Edge->geometry.curvaturePerMetre:0,Delta,Near,InCab);
 UpdateTrafficAudio(Delta,InCab);
}

void URailSimulationSubsystem::UpdateTrafficAudio(float Delta,bool InCab){
 for(bool Opposing:{false,true}){
 auto TrafficId=Opposing?Rail::TrainId(3):Service.Definition().traffic;
 // Keep the two bounded traffic voices, selecting the nearest actual opposing
 // formation. Distance/Doppler envelopes reject source-switch discontinuities.
 if(Opposing&&Rail::ScenarioContent::IsLiveJourney(Operations.ContentId())){
  const auto* Player=Operations.Spatial(Service.Definition().player);double Nearest=TNumericLimits<double>::Max();
  if(Player&&!Player->Current().vehicles.empty())for(uint64 Id=3;Id<=Rail::ScenarioContent::TrainCount(Operations.ContentId());++Id){
   const auto* Other=Operations.Spatial(Rail::TrainId(Id));if(!Other||Other->Current().vehicles.empty())continue;
   const double Distance=(Other->Current().vehicles.front().bodyCentre-Player->Current().vehicles.front().bodyCentre).Length();
   if(Distance<Nearest){Nearest=Distance;TrafficId=Rail::TrainId(Id);}
  }
 }
 auto* HornVoice=Opposing?OpposingHornAudio.Get():TrafficHornAudio.Get();
 auto* LocoVoice=Opposing?OpposingLocoSound.Get():TrafficLocoSound.Get();
 auto* RollingVoice=Opposing?OpposingRollingSound.Get():TrafficRollingSound.Get();
 // A new formation must not inherit the previous horn latch or Doppler
 // history. Pausing the same formation still retains its encounter latch.
 if(Opposing&&OpposingAudioTrain!=TrafficId.value){
  OpposingAudioTrain=TrafficId.value;
  OpposingHornDynamics=Rail::PassingAudioDynamics{};
  OpposingRollingDynamics=Rail::PassingAudioDynamics{};
  if(HornVoice)HornVoice->Stop();
  for(auto* Sound:{LocoVoice,RollingVoice})if(Sound)Sound->Stop();
 }
 const auto* Traffic=Operations.Spatial(TrafficId);
 const auto* TrafficHandling=Operations.Handling(TrafficId);
 auto* Controller=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
 if(!Traffic||!TrafficHandling||!Controller||Traffic->Current().vehicles.empty()||Service.Definition().player==TrafficId){
  if(HornVoice)HornVoice->Stop();
  for(auto* Sound:{LocoVoice,RollingVoice})if(Sound)Sound->Stop();
  (Opposing?OpposingHornDynamics:TrafficHornDynamics).Suspend();(Opposing?OpposingRollingDynamics:TrafficRollingDynamics).Suspend();continue;
 }
 FVector Listener,Front,Right;Controller->GetAudioListenerPosition(Listener,Front,Right);
 const auto Cm=[](const auto& V){return FVector(V.x*100,V.y*100,V.z*100);};
 const auto& Vehicles=Traffic->Current().vehicles;
 const FVector Horn= Cm(Vehicles.front().bodyCentre)+FVector(0,0,330);
 FVector Running=Horn;double Best=TNumericLimits<double>::Max();
 // Closest point along each vehicle's bogie span keeps the passing coaches
 // audible after the locomotive has crossed the listener.
 for(const auto& Vehicle:Vehicles){
  const FVector Point=FMath::ClosestPointOnSegment(Listener,Cm(Vehicle.frontBogie)+FVector(0,0,45),Cm(Vehicle.rearBogie)+FVector(0,0,45));
  const double Distance=FVector::DistSquared(Point,Listener);
  if(Distance<Best){Best=Distance;Running=Point;}
 }
 ApplyTrafficAudio(Horn,Running,Listener,*TrafficHandling,Delta,InCab,Opposing);
 }
}

void URailSimulationSubsystem::ApplyTrafficAudio(const FVector& Horn,const FVector& Running,const FVector& Listener,const Rail::LongitudinalState& Traffic,float Delta,bool InCab,bool Opposing){
 auto* HornVoice=Opposing?OpposingHornAudio.Get():TrafficHornAudio.Get();
 auto* LocoVoice=Opposing?OpposingLocoSound.Get():TrafficLocoSound.Get();
 auto* RollingVoice=Opposing?OpposingRollingSound.Get():TrafficRollingSound.Get();
 const double Speed=FMath::Abs(Traffic.speedMps);
 const auto HornMix=(Opposing?OpposingHornDynamics:TrafficHornDynamics).Update(FVector::Distance(Horn,Listener)/100,Delta,Speed>.5);
 const auto RollMix=(Opposing?OpposingRollingDynamics:TrafficRollingDynamics).Update(FVector::Distance(Running,Listener)/100,Delta,Speed>.5,false);
 const auto Perspective=ServiceAudioDynamics.Perspective(InCab,0);
 const float Master=GetServiceAudioVolume(0),Effects=Master*GetServiceAudioVolume(2);
 // The other train is heard from outside its own body: exterior balance, filtered into the cab.
 Rail::LocoSoundInput In;In.speedMps=Traffic.speedMps;In.effort=FMath::Clamp(Traffic.tractionN/Rail::Wap7Drive::StartingTractionN,-1.0,1.0);
 In.brakeCylinder=Traffic.appliedBrake;In.cab=0;In.engineGain=GetServiceAudioVolume(1);In.effectsGain=GetServiceAudioVolume(2);
 auto Voice=[&](URailLocoSoundComponent* Sound,const FVector& Position,double Gain,double Pitch){
  if(!Sound)return;Sound->SetWorldLocation(Position);
  const float Volume=float(Gain*Perspective.outsideGain*Master);
  if(Volume>.0001f){if(!Sound->IsPlaying())Sound->Start();}else{if(Sound->IsPlaying())Sound->Stop();return;}
  Sound->SetVolumeMultiplier(Volume);Sound->SetLowPassFilterEnabled(true);Sound->SetLowPassFilterFrequency(float(Perspective.lowPassHz));
  if(auto* Audio=Sound->GetAudioComponent())Audio->SetPitchMultiplier(float(Pitch));
  Sound->Push(In,{});
 };
 Voice(LocoVoice,Horn,HornMix.gain*1.2,HornMix.pitch);
 Voice(RollingVoice,Running,RollMix.gain*1.4,RollMix.pitch);
 if(HornVoice){
  HornVoice->SetWorldLocation(Horn);HornVoice->SetVolumeMultiplier(float(HornMix.gain*.65*Perspective.outsideGain*Effects));HornVoice->SetPitchMultiplier(float(HornMix.pitch));
  HornVoice->SetLowPassFilterEnabled(true);HornVoice->SetLowPassFilterFrequency(float(Perspective.lowPassHz));
 }
 if(HornMix.horn&&HornVoice){HornVoice->Play();UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO traffic approach horn distance=%.1fm opposing=%d"),FVector::Distance(Horn,Listener)/100,Opposing);}
}

#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
void URailSimulationSubsystem::PresentReviewAudio(const Rail::LongitudinalState& State,double Curvature,float Delta){
 if(!FParse::Param(FCommandLine::Get(),TEXT("S8AssetReview"))||!FParse::Param(FCommandLine::Get(),TEXT("TambaramMotionReview")))return;
 InitializeServiceAudio();
 const bool InCab=FParse::Param(FCommandLine::Get(),TEXT("TambaramAudioCabReview"));
 if(FParse::Param(FCommandLine::Get(),TEXT("TambaramPassingAudioReview"))){
  // Editor-only synthetic acoustic fixture, never a route/simulation mutation.
  // Use precisely the runtime source mixer and the real camera audio listener.
  for(auto* Audio:{ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),ServiceCueAudio.Get()})if(Audio)Audio->Stop();
  if(ServiceLocoSound)ServiceLocoSound->Stop();
  auto* Controller=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
  if(!Controller)return;
  FVector Listener,Front,Right;Controller->GetAudioListenerPosition(Listener,Front,Right);
  const FVector Axis=Right.GetSafeNormal2D(),Across=FVector::CrossProduct(Axis,FVector::UpVector).GetSafeNormal();
  TrafficReviewSeconds+=FMath::Clamp(double(Delta),0.,.1);
  const double Head=700-40*TrafficReviewSeconds;
  const FVector Horn=Listener+Axis*(Head*100)+Across*500+FVector(0,0,150);
  const FVector Running=Listener+Axis*(std::clamp(0.,Head,Head+190)*100)+Across*500-FVector(0,0,120);
  ServiceAudioDynamics.Perspective(InCab,Delta);
  Rail::LongitudinalState Pass;Pass.speedMps=40;Pass.tractionN=180000;Pass.massKg=403000;
  ApplyTrafficAudio(Horn,Running,Listener,Pass,Delta,InCab);
  const int32 Second=FMath::FloorToInt(TrafficReviewSeconds);
  if(Second!=TrafficReviewLastLoggedSecond){
   TrafficReviewLastLoggedSecond=Second;
   UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO_SYNTHETIC_PASS t=%.3f head=%.2f hornDistance=%.2f rollingDistance=%.2f hornGain=%.5f rollingGain=%.5f hornPitch=%.5f rollingPitch=%.5f"),TrafficReviewSeconds,Head,FVector::Distance(Horn,Listener)/100,FVector::Distance(Running,Listener)/100,TrafficHornDynamics.gain,TrafficRollingDynamics.gain,TrafficHornDynamics.pitch,TrafficRollingDynamics.pitch);
  }
  return;
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("TambaramAudioStressReview"))){
  // Audio-only worst-load fixture. Never feed synthetic speed/force into the
  // railway or claim this recording as physical full-speed driving evidence.
  auto Stress=State;Stress.speedMps=100.0/3.6;Stress.appliedThrottle=1;
  Stress.tractionN=323000;Stress.massKg=403000;
  ApplyServiceAudio(Stress,.006,Delta,1,InCab);
  if(ServiceHornAudio&&!ServiceHornAudio->IsPlaying())ServiceHornAudio->Play();
 }else ApplyServiceAudio(State,Curvature,Delta,0,InCab);
}
#endif

void URailSimulationSubsystem::ApplyServiceAudio(const Rail::LongitudinalState& State,double Curvature,float Delta,float Near,bool InCab){
 const auto AudioPerspective=ServiceAudioDynamics.Perspective(InCab,Delta);
 const float Cab=float(AudioPerspective.outsideGain);
 const float Master=GetServiceAudioVolume(0),Effects=Master*GetServiceAudioVolume(2);
 // Recordings of the outside world are filtered by the closed cab; the train's own voice carries
 // its cab balance inside the synth, so its in-cab air and desk sounds stay unfiltered.
 for(auto* Audio:{ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get()}){
  if(IsValid(Audio)){Audio->SetLowPassFilterEnabled(true);Audio->SetLowPassFilterFrequency(float(AudioPerspective.lowPassHz));}
 }
 const auto Mix=ServiceAudioDynamics.Update(State,Delta);
 ServiceBrakeEngaged=State.appliedBrake>.02;
 const float Duck=ServiceHornAudio&&ServiceHornAudio->IsPlaying()?.4f:1.f;
 ServiceStationGain=FMath::FInterpTo(ServiceStationGain,Near*Near*.3f*Cab*Duck,Delta,2.f);
 if(IsValid(ServiceStationAudio)){ServiceStationAudio->SetVolumeMultiplier(ServiceStationGain*Effects);if(ServiceStationGain*Effects>.001f){if(!ServiceStationAudio->IsPlaying())ServiceStationAudio->Play();}else ServiceStationAudio->Stop();}
 if(IsValid(ServiceLocoSound)){
  Rail::LocoSoundInput In;
  In.speedMps=State.speedMps;In.effort=FMath::Clamp(State.tractionN/Rail::Wap7Drive::StartingTractionN,-1.0,1.0);
  In.curvature=Curvature;In.brakeCylinder=State.appliedBrake;
  In.applyFlow=Mix.applyFlow;In.releaseFlow=Mix.releaseFlow;In.ventFlow=Mix.ventFlow;
  In.odometer=ServiceOdometer.metres;In.cab=ServiceAudioDynamics.cabBlend;
  In.engineGain=GetServiceAudioVolume(1);In.effectsGain=GetServiceAudioVolume(2);
  if(Master>.0001f){if(!ServiceLocoSound->IsPlaying())ServiceLocoSound->Start();}else if(ServiceLocoSound->IsPlaying())ServiceLocoSound->Stop();
  ServiceLocoSound->SetVolumeMultiplier(Master);
  ServiceLocoSound->Push(In,MoveTemp(ServiceStrikes));ServiceStrikes.Reset();
 }
 if(IsValid(ServiceCueAudio))ServiceCueAudio->SetVolumeMultiplier(.18f*Effects);
 if(IsValid(ServiceHornAudio))ServiceHornAudio->SetVolumeMultiplier(float(AudioPerspective.hornGain)*Effects);
 // The distributor exhausts the cylinder on release: the recorded exhaust starts it, the synth
 // carries the flow for as long as the simulated cylinder keeps falling.
 if(Mix.emergency){
  if(IsValid(ServiceBrakeAudio))ServiceBrakeAudio->Stop();
  UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO emergency applied"));
 }else if(Mix.brakeApply){
  UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO brake apply"));
 }else if(Mix.brakeRelease){
  // Only a charged cylinder gives the recorded burst (scaled by its pressure); small corrections
  // are carried by the synthesised flow alone, so brake jitter never machine-guns the recording.
  if(IsValid(ServiceBrakeAudio)&&State.appliedBrake>.2&&!ServiceBrakeAudio->IsPlaying()){
   ServiceBrakeAudio->SetVolumeMultiplier(.3f*Cab*Effects*float(FMath::Min(1.0,State.appliedBrake*1.4)));ServiceBrakeAudio->Play();
  }
  UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO brake release"));
 }
}
