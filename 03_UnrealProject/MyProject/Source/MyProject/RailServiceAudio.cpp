#include "RailSimulationSubsystem.h"
#include "Simulation/RailJourneyPresentation.h"
#include "Simulation/RailScenarioContent.h"
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
 ServiceMovementAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Rail.SW_Rail"),0);
 ServiceStationAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Station.SW_Station"),0);
 ServiceHornAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Horn.SW_Horn"),.65f);
 ServiceBrakeAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Brake.SW_Brake"),.35f);
 ServiceCueAudio=Create(TEXT("/Game/IndianRailway/S7/SW_StationCue.SW_StationCue"),.18f);
 ServiceMotorAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_Motor.SW_Motor"),0);
 ServiceCoastAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_Coast.SW_Coast"),0);
 ServiceSquealAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_Squeal.SW_Squeal"),0);
 ServiceBrakeReleaseAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_BrakeRelease.SW_BrakeRelease"),.3f);
 ServiceEmergencyAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_Emergency.SW_Emergency"),.45f);
 TrafficHornAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Horn.SW_Horn"),0);
 TrafficRollingAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Rail.SW_Rail"),0);
 TrafficMotorAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_Motor.SW_Motor"),0);
 OpposingHornAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Horn.SW_Horn"),0);
 OpposingRollingAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/SW_Rail.SW_Rail"),0);
 OpposingMotorAudio=Create(TEXT("/Game/IndianRailway/S8/Audio/RealismV2/SW_Motor.SW_Motor"),0);
 FSoundAttenuationSettings Spatial;
 Spatial.bSpatialize=true;Spatial.bAttenuate=false;Spatial.bAttenuateWithLPF=false;
 for(auto* Component:{TrafficHornAudio.Get(),TrafficRollingAudio.Get(),TrafficMotorAudio.Get(),OpposingHornAudio.Get(),OpposingRollingAudio.Get(),OpposingMotorAudio.Get()})if(Component){
  Component->bAllowSpatialization=true;Component->AdjustAttenuation(Spatial);
 }
}

void URailSimulationSubsystem::StopServiceAudio(){
 for(auto* Audio:{OpposingHornAudio.Get(),OpposingRollingAudio.Get(),OpposingMotorAudio.Get()})if(IsValid(Audio))Audio->Stop();
 OpposingHornDynamics.Suspend();OpposingRollingDynamics.Suspend();
 for(auto* Audio:{TrafficHornAudio.Get(),TrafficRollingAudio.Get(),TrafficMotorAudio.Get()})if(IsValid(Audio))Audio->Stop();
 TrafficHornDynamics.Suspend();TrafficRollingDynamics.Suspend();
 for(auto* Audio:{ServiceMovementAudio.Get(),ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),ServiceCueAudio.Get(),
     ServiceMotorAudio.Get(),ServiceCoastAudio.Get(),ServiceSquealAudio.Get(),ServiceBrakeReleaseAudio.Get(),ServiceEmergencyAudio.Get()})if(IsValid(Audio))Audio->Stop();
 ServiceAudioWasRunning=false;ServiceRailGain=0;ServiceStationGain=0;ServiceRailPitch=1;
 ServiceAudioDynamics.Reset();
}

void URailSimulationSubsystem::DestroyServiceAudio(){
 StopServiceAudio();
 for(auto* Audio:{OpposingHornAudio.Get(),OpposingRollingAudio.Get(),OpposingMotorAudio.Get()})if(IsValid(Audio))Audio->DestroyComponent();
 OpposingHornAudio=nullptr;OpposingRollingAudio=nullptr;OpposingMotorAudio=nullptr;
 OpposingHornDynamics={};OpposingRollingDynamics={};
 for(auto* Audio:{TrafficHornAudio.Get(),TrafficRollingAudio.Get(),TrafficMotorAudio.Get()})if(IsValid(Audio))Audio->DestroyComponent();
 TrafficHornAudio=nullptr;TrafficRollingAudio=nullptr;TrafficMotorAudio=nullptr;
 TrafficHornDynamics={};TrafficRollingDynamics={};
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
 TrafficReviewSeconds=0;TrafficReviewLastLoggedSecond=-1;
#endif
 for(auto* Audio:{ServiceMovementAudio.Get(),ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),ServiceCueAudio.Get(),
     ServiceMotorAudio.Get(),ServiceCoastAudio.Get(),ServiceSquealAudio.Get(),ServiceBrakeReleaseAudio.Get(),ServiceEmergencyAudio.Get()})if(IsValid(Audio))Audio->DestroyComponent();
 ServiceMovementAudio=nullptr;ServiceStationAudio=nullptr;ServiceHornAudio=nullptr;ServiceBrakeAudio=nullptr;ServiceCueAudio=nullptr;
 ServiceMotorAudio=nullptr;ServiceCoastAudio=nullptr;ServiceSquealAudio=nullptr;ServiceBrakeReleaseAudio=nullptr;ServiceEmergencyAudio=nullptr;
 ServiceSounds.Reset();ServiceAudioLoaded=false;
 if(GetWorld())if(auto Device=GetWorld()->GetAudioDevice()){
  if(ServiceAudioClass)Device->UnregisterSoundClass(ServiceAudioClass);
  if(ServiceAudioSubmix)Device->UnregisterSoundSubmix(ServiceAudioSubmix,false);
 }
 ServiceAudioClass=nullptr;ServiceAudioSubmix=nullptr;
}

void URailSimulationSubsystem::PlayServiceHorn(){
 if(S6Paused||S6Blocked||AppBackground.Load()||Service.Current().phase!=Rail::ServicePhase::Running)return;
 InitializeServiceAudio();
 // One owned voice prevents rapid taps from stacking horn recordings.
 if(IsValid(ServiceHornAudio)&&!ServiceHornAudio->IsPlaying()){
  const auto AudioPerspective=ServiceAudioDynamics.Perspective(IsLocomotiveCabCamera(),0);
  ServiceHornAudio->SetVolumeMultiplier(float(AudioPerspective.hornGain)*GetServiceAudioVolume(0)*GetServiceAudioVolume(2));
  ServiceHornAudio->SetLowPassFilterEnabled(true);
  ServiceHornAudio->SetLowPassFilterFrequency(float(AudioPerspective.lowPassHz));
  ServiceHornAudio->Play();UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO horn"));
 }
}

void URailSimulationSubsystem::UpdateServiceAudio(){
 const auto& State=Service.Current();const auto& Definition=Service.Definition();
 const auto* H=Operations.Handling(Definition.player);const auto* Train=Operations.Spatial(Definition.player);
 const bool Running=H&&Train&&IsValid(GetWorld())&&!S6Paused&&!S6Blocked&&!AppBackground.Load()&&State.phase==Rail::ServicePhase::Running;
 if(!Running){StopServiceAudio();ServiceLastCue=State.cue;return;}
 InitializeServiceAudio();
 if(!ServiceAudioWasRunning){ServiceLastCue=State.cue;ServiceAudioWasRunning=true;}
 if(State.cue!=ServiceLastCue){ServiceLastCue=State.cue;if(ServiceCueAudio){ServiceCueAudio->SetVolumeMultiplier(.18f*GetServiceAudioVolume(0)*GetServiceAudioVolume(2));ServiceCueAudio->Play();}}
 const float Delta=FMath::Clamp(GetWorld()->GetDeltaSeconds(),0.f,.1f);
 const bool InCab=IsLocomotiveCabCamera();
 const auto* Edge=Train->Network().Find(Train->Current().head.edge);
 const float Distance=float(FMath::Abs(Train->Current().headStationMetres-(Definition.platformFrom+Definition.platformTo)*.5));
 const float Near=Rail::ScenarioContent::IsTambaram(Definition.contentId)?float(Rail::JourneyPresentation::StationAmbience(Train->Current().headStationMetres)):FMath::Clamp(1.f-Distance/120.f,0.f,1.f);
 ApplyServiceAudio(*H,Edge?Edge->geometry.curvaturePerMetre:0,Delta,Near,InCab);
 UpdateTrafficAudio(Delta,InCab);
}

void URailSimulationSubsystem::UpdateTrafficAudio(float Delta,bool InCab){
 for(bool Opposing:{false,true}){
 const auto TrafficId=Opposing?Rail::TrainId(3):Service.Definition().traffic;
 const auto* Traffic=Operations.Spatial(TrafficId);
 const auto* TrafficHandling=Operations.Handling(TrafficId);
 auto* Controller=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
 if(!Traffic||!TrafficHandling||!Controller||Traffic->Current().vehicles.empty()||Service.Definition().player==TrafficId){
  for(auto* Audio:{Opposing?OpposingHornAudio.Get():TrafficHornAudio.Get(),Opposing?OpposingRollingAudio.Get():TrafficRollingAudio.Get(),Opposing?OpposingMotorAudio.Get():TrafficMotorAudio.Get()})if(Audio)Audio->Stop();
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
 ApplyTrafficAudio(Horn,Running,Listener,FMath::Abs(TrafficHandling->speedMps),TrafficHandling->tractionN,Delta,InCab,Opposing);
 }
}

void URailSimulationSubsystem::ApplyTrafficAudio(const FVector& Horn,const FVector& Running,const FVector& Listener,double Speed,double Traction,float Delta,bool InCab,bool Opposing){
 auto* HornVoice=Opposing?OpposingHornAudio.Get():TrafficHornAudio.Get();
 auto* RollingVoice=Opposing?OpposingRollingAudio.Get():TrafficRollingAudio.Get();
 auto* MotorVoice=Opposing?OpposingMotorAudio.Get():TrafficMotorAudio.Get();
 const auto HornMix=(Opposing?OpposingHornDynamics:TrafficHornDynamics).Update(FVector::Distance(Horn,Listener)/100,Delta,Speed>.5);
 const auto RollMix=(Opposing?OpposingRollingDynamics:TrafficRollingDynamics).Update(FVector::Distance(Running,Listener)/100,Delta,Speed>.5,false);
 const auto Perspective=ServiceAudioDynamics.Perspective(InCab,0);
 const float Master=GetServiceAudioVolume(0),Effects=Master*GetServiceAudioVolume(2),Engine=Master*GetServiceAudioVolume(1);
 auto Set=[&](UAudioComponent* Audio,const FVector& Position,double Gain,double Pitch){
  if(!Audio)return;Audio->SetWorldLocation(Position);Audio->SetVolumeMultiplier(float(Gain));Audio->SetPitchMultiplier(float(Pitch));
  Audio->SetLowPassFilterEnabled(true);Audio->SetLowPassFilterFrequency(float(Perspective.lowPassHz));
 };
 Set(HornVoice,Horn,HornMix.gain*.65*Perspective.outsideGain*Effects,HornMix.pitch);
 Set(RollingVoice,Running,RollMix.gain*.40*std::sqrt(std::clamp(Speed/30.,0.,1.))*Perspective.outsideGain*Effects,RollMix.pitch*(.72+.68*std::clamp(Speed/35.,0.,1.)));
 const double Load=std::clamp(FMath::Abs(Traction)/240000.,0.,1.);
 Set(MotorVoice,Horn,HornMix.gain*.30*Load*Perspective.outsideGain*Engine,HornMix.pitch*(.65+1.1*std::clamp(Speed/32.,0.,1.)));
 for(auto* Audio:{RollingVoice,MotorVoice})if(Audio){if(Audio->VolumeMultiplier>.0001f){if(!Audio->IsPlaying())Audio->Play();}else Audio->Stop();}
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
  for(auto* Audio:{ServiceMovementAudio.Get(),ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),ServiceCueAudio.Get(),ServiceMotorAudio.Get(),ServiceCoastAudio.Get(),ServiceSquealAudio.Get(),ServiceBrakeReleaseAudio.Get(),ServiceEmergencyAudio.Get()})if(Audio)Audio->Stop();
  auto* Controller=GetWorld()?GetWorld()->GetFirstPlayerController():nullptr;
  if(!Controller)return;
  FVector Listener,Front,Right;Controller->GetAudioListenerPosition(Listener,Front,Right);
  const FVector Axis=Right.GetSafeNormal2D(),Across=FVector::CrossProduct(Axis,FVector::UpVector).GetSafeNormal();
  TrafficReviewSeconds+=FMath::Clamp(double(Delta),0.,.1);
  const double Head=700-40*TrafficReviewSeconds;
  const FVector Horn=Listener+Axis*(Head*100)+Across*500+FVector(0,0,150);
  const FVector Running=Listener+Axis*(std::clamp(0.,Head,Head+190)*100)+Across*500-FVector(0,0,120);
  ServiceAudioDynamics.Perspective(InCab,Delta);
  ApplyTrafficAudio(Horn,Running,Listener,40,180000,Delta,InCab);
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
 const float Speed=float(FMath::Abs(State.speedMps));
 const auto AudioPerspective=ServiceAudioDynamics.Perspective(InCab,Delta);
 const float Cab=float(AudioPerspective.outsideGain);
 const float Master=GetServiceAudioVolume(0),Engine=Master*GetServiceAudioVolume(1),Effects=Master*GetServiceAudioVolume(2);
 // Filter external sound consistently, including one-shots already in progress.
 // The station cue remains an unfiltered gameplay notification.
 for(auto* Audio:{ServiceMovementAudio.Get(),ServiceStationAudio.Get(),ServiceHornAudio.Get(),ServiceBrakeAudio.Get(),
     ServiceMotorAudio.Get(),ServiceCoastAudio.Get(),ServiceSquealAudio.Get(),ServiceBrakeReleaseAudio.Get(),ServiceEmergencyAudio.Get()}){
  if(IsValid(Audio)){Audio->SetLowPassFilterEnabled(true);Audio->SetLowPassFilterFrequency(float(AudioPerspective.lowPassHz));}
 }
 const auto Mix=ServiceAudioDynamics.Update(State,Curvature,Delta);
 ServiceBrakeEngaged=State.appliedBrake>.02;
 ServiceRailGain=float(Mix.rail)*Cab;ServiceRailPitch=float(Mix.railPitch);
 const float Duck=ServiceHornAudio&&ServiceHornAudio->IsPlaying()?.4f:1.f;
 ServiceStationGain=FMath::FInterpTo(ServiceStationGain,Near*Near*.3f*Cab*Duck,Delta,2.f);
 auto Loop=[](UAudioComponent* Audio,float Gain,float Pitch){
  if(!IsValid(Audio))return;Audio->SetVolumeMultiplier(Gain);Audio->SetPitchMultiplier(Pitch);
  if(Gain>.001f){if(!Audio->IsPlaying())Audio->Play();}else Audio->Stop();
 };
 Loop(ServiceMovementAudio,ServiceRailGain*Effects*1.25f,ServiceRailPitch);Loop(ServiceStationAudio,ServiceStationGain*Effects,1.f);
 Loop(ServiceMotorAudio,float(Mix.motor)*float(AudioPerspective.motorGain)*Engine*1.6f,float(Mix.motorPitch));
 Loop(ServiceCoastAudio,float(Mix.coast)*Cab*Effects,.85f+FMath::Clamp(Speed/60.f,0.f,.35f));
 Loop(ServiceSquealAudio,float(Mix.squeal)*Cab*Effects,.9f+FMath::Clamp(Speed/100.f,0.f,.2f));
 if(IsValid(ServiceCueAudio))ServiceCueAudio->SetVolumeMultiplier(.18f*Effects);
 if(IsValid(ServiceHornAudio))ServiceHornAudio->SetVolumeMultiplier(float(AudioPerspective.hornGain)*Effects);
 if(IsValid(ServiceBrakeAudio))ServiceBrakeAudio->SetVolumeMultiplier(.3f*Cab*Effects);
 if(IsValid(ServiceBrakeReleaseAudio))ServiceBrakeReleaseAudio->SetVolumeMultiplier(.26f*Cab*Effects);
 if(IsValid(ServiceEmergencyAudio))ServiceEmergencyAudio->SetVolumeMultiplier(.45f*Cab*Effects);
 auto OneShot=[Cab,Effects](UAudioComponent* Audio,float Gain){if(IsValid(Audio)){Audio->Stop();Audio->SetVolumeMultiplier(Gain*Cab*Effects);Audio->Play();}};
 if(Mix.emergency){
  if(IsValid(ServiceBrakeAudio))ServiceBrakeAudio->Stop();
  if(IsValid(ServiceBrakeReleaseAudio))ServiceBrakeReleaseAudio->Stop();
  OneShot(ServiceEmergencyAudio,.45f);UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO emergency applied"));
 }else if(Mix.brakeApply){
  if(IsValid(ServiceBrakeReleaseAudio))ServiceBrakeReleaseAudio->Stop();
  OneShot(ServiceBrakeAudio,.3f);UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO brake apply"));
 }else if(Mix.brakeRelease){
  if(IsValid(ServiceBrakeAudio))ServiceBrakeAudio->Stop();
  if(IsValid(ServiceEmergencyAudio))ServiceEmergencyAudio->Stop();
  OneShot(ServiceBrakeReleaseAudio,.26f);UE_LOG(LogTemp,Display,TEXT("RAIL_AUDIO brake release"));
 }
}
