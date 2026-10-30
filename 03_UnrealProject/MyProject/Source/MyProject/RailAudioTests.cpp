#include "Misc/AutomationTest.h"
#include "Sound/SoundWave.h"
#include "RailSimulationSubsystem.h"
#include "RailLocoSoundComponent.h"
#include "RailLocoSound.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailAudioBankTest,"IndianRailway.S8.AudioBank",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailAudioBankTest::RunTest(const FString& Parameters){
 // Recordings only: the trains' continuous sound is synthesised from the simulation (RailLocoSound.h).
 const struct {const TCHAR* Path;bool Loop;} Bank[]={
  {TEXT("/Game/IndianRailway/S10/AudioV1/SW_StationChatter.SW_StationChatter"),true},{TEXT("/Game/IndianRailway/S10/AudioV1/SW_Wap7HornLoop.SW_Wap7HornLoop"),true},
  {TEXT("/Game/IndianRailway/S10/AudioV1/SW_TrafficHorn.SW_TrafficHorn"),false},{TEXT("/Game/IndianRailway/S8/Audio/SW_Brake.SW_Brake"),false}};
 for(const auto& Entry:Bank){
  auto* Sound=LoadObject<USoundWave>(nullptr,Entry.Path);
  if(!TestNotNull(Entry.Path,Sound))continue;
  TestEqual(TEXT("Mono bank"),Sound->NumChannels,1);
  TestEqual(TEXT("Only continuous textures loop"),bool(Sound->bLooping),Entry.Loop);
  TestTrue(TEXT("Finite short mobile sample"),FMath::IsFinite(Sound->Duration)&&Sound->Duration>1&&Sound->Duration<40);
  TestEqual(TEXT("No hidden asset gain boost"),Sound->Volume,1.f);
 }
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailAudioLifecycleTest,"IndianRailway.S8.AudioLifecycle",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailAudioLifecycleTest::RunTest(const FString& Parameters){
 const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!TestNotNull(TEXT("Test world"),World))return false;
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* System=World->GetSubsystem<URailSimulationSubsystem>();
 if(System){
  System->PlayServiceHorn();TestFalse(TEXT("Paused horn does not load voices"),System->ServiceAudioLoaded);
  TestTrue(TEXT("Fixture initializes"),System->Operations.InitializeFixture().ok);
  System->Service.Select();System->Service.ConfirmFormation();TestTrue(TEXT("Service starts"),System->Service.Start(System->Operations).ok);
  System->S6Paused=false;System->S6Blocked=false;System->UpdateServiceAudio();
  TestTrue(TEXT("Running audio state"),System->ServiceAudioWasRunning);
  TestEqual(TEXT("Exactly five resident recordings"),System->ServiceSounds.Num(),5);
  TestNotNull(TEXT("Player train voice"),System->ServiceLocoSound.Get());
  TestTrue(TEXT("Distinct traffic and opposing voices"),System->TrafficLocoSound&&System->OpposingLocoSound&&System->TrafficLocoSound!=System->OpposingLocoSound&&System->TrafficRollingSound&&System->OpposingRollingSound);
  TestTrue(TEXT("Player voice renders every layer, traffic split loco/rolling"),System->ServiceLocoSound->Layers==Rail::SoundAll&&!(System->TrafficLocoSound->Layers&Rail::SoundRunning)&&System->TrafficRollingSound->Layers==Rail::SoundRunning);
  TestNotNull(TEXT("Dedicated protected audio bus"),System->ServiceAudioSubmix.Get());
  TestEqual(TEXT("Limiter owned by service bus"),System->ServiceAudioSubmix->SubmixEffectChain.Num(),1);
  auto* Limiter=Cast<USubmixEffectDynamicsProcessorPreset>(System->ServiceAudioSubmix->SubmixEffectChain[0]);
  if(TestNotNull(TEXT("Dynamics preset"),Limiter)){
   TestEqual(TEXT("Makeup retained across effect initialization"),Limiter->Settings.InputGainDb,18.f);
   TestEqual(TEXT("Peak limiter mode retained"),Limiter->Settings.DynamicsProcessorType,ESubmixEffectDynamicsProcessorType::Limiter);
   TestEqual(TEXT("Limiter has overlap headroom"),Limiter->Settings.ThresholdDb,-6.f);
   TestEqual(TEXT("Limiter has final output headroom"),Limiter->Settings.OutputGainDb,-3.f);
  }
  TestTrue(TEXT("Voices routed through service class"),System->ServiceLocoSound->SoundClass==System->ServiceAudioClass&&System->ServiceHornAudio->SoundClassOverride==System->ServiceAudioClass);
  TestTrue(TEXT("Class routes to protected bus"),System->ServiceAudioClass->Properties.DefaultSubmix==System->ServiceAudioSubmix);
  TestTrue(TEXT("Traffic horn spatialized"),System->TrafficHornAudio->bAllowSpatialization);
  TestTrue(TEXT("Opposing horn spatialized"),System->OpposingHornAudio->bAllowSpatialization);
  TestTrue(TEXT("Traffic voices spatialized, player voice not"),System->TrafficLocoSound->bAllowSpatialization&&System->OpposingRollingSound->bAllowSpatialization&&!System->ServiceLocoSound->bAllowSpatialization);
  TestTrue(TEXT("Opposing horn shares protected bus"),System->OpposingHornAudio->SoundClassOverride==System->ServiceAudioClass);
  TestTrue(TEXT("Traffic horn shares protected bus"),System->TrafficHornAudio->SoundClassOverride==System->ServiceAudioClass);
  System->TrafficHornDynamics.hornPlayed=true;
  System->OpposingHornDynamics.hornPlayed=true;
  // Change only in-memory preferences; automated checks must not write user config.
  System->ServiceAudioPreferencesLoaded=true;
  System->ServiceAudioVolumes[0]=0;
  Rail::LongitudinalState Audible;Audible.massKg=400000;Audible.speedMps=20;Audible.appliedThrottle=1;Audible.tractionN=200000;
  System->ApplyServiceAudio(Audible,0,.1f,1,false);
  TestFalse(TEXT("Master zero silences the train voice"),System->ServiceLocoSound->IsPlaying());
  TestEqual(TEXT("Master zero mutes alerts"),System->ServiceCueAudio->VolumeMultiplier,0.f);
  TestEqual(TEXT("Master zero mutes horn"),System->ServiceHornAudio->VolumeMultiplier,0.f);
  System->ServiceAudioVolumes[0]=1;System->ServiceAudioVolumes[1]=0;
  System->ApplyServiceAudio(Audible,0,.1f,1,false);
  TestEqual(TEXT("Engine category mutes traction"),System->ServiceLocoSound->LastInput.engineGain,0.0);
  TestTrue(TEXT("Engine category retains effects"),System->ServiceLocoSound->LastInput.effectsGain>0);
  System->ServiceAudioVolumes[1]=1;System->ServiceAudioVolumes[2]=0;
  System->ApplyServiceAudio(Audible,0,.1f,1,false);
  TestTrue(TEXT("Effects category retains engine"),System->ServiceLocoSound->LastInput.engineGain>0);
  TestEqual(TEXT("Effects zero mutes rolling and air"),System->ServiceLocoSound->LastInput.effectsGain,0.0);
  TestTrue(TEXT("Realised effort reaches the voice"),FMath::Abs(System->ServiceLocoSound->LastInput.effort-200000/Rail::Wap7Drive::StartingTractionN)<1e-9);
  System->ServiceAudioVolumes[2]=1;
  for(int32 Reason=0;Reason<3;++Reason){
   System->S6Paused=Reason==0;System->S6Blocked=Reason==1;System->AppBackground.Store(Reason==2);
   System->UpdateServiceAudio();TestFalse(TEXT("Pause/block/background stops running state"),System->ServiceAudioWasRunning);
   for(auto* Audio:{System->ServiceStationAudio.Get(),System->ServiceHornAudio.Get(),System->ServiceBrakeAudio.Get(),System->ServiceCueAudio.Get()}){
    if(TestNotNull(TEXT("Owned voice"),Audio))TestFalse(TEXT("Suspended voice stopped"),Audio->IsPlaying());
   }
   TestFalse(TEXT("Suspended train voice stopped"),System->ServiceLocoSound->IsPlaying());
   TestFalse(TEXT("Suspension releases the horn switch"),System->ServiceHornHeld);
   TestEqual(TEXT("Station gain resets"),System->ServiceStationGain,0.f);
   TestTrue(TEXT("Pause preserves traffic encounter latch"),System->TrafficHornDynamics.hornPlayed);
   TestFalse(TEXT("Traffic horn stops on suspension"),System->TrafficHornAudio->IsPlaying());
   TestFalse(TEXT("Traffic rolling stops on suspension"),System->TrafficRollingSound->IsPlaying());
   TestFalse(TEXT("Traffic loco stops on suspension"),System->TrafficLocoSound->IsPlaying());
   TestTrue(TEXT("Opposing encounter retained during suspension"),System->OpposingHornDynamics.hornPlayed);
   TestFalse(TEXT("Opposing horn stops on suspension"),System->OpposingHornAudio->IsPlaying());
   for(auto* Voice:{System->OpposingLocoSound.Get(),System->OpposingRollingSound.Get()})TestFalse(TEXT("Opposing voice stops on suspension"),Voice->IsPlaying());
   System->S6Paused=false;System->S6Blocked=false;System->AppBackground.Store(false);System->UpdateServiceAudio();
   TestTrue(TEXT("Resume audio state"),System->ServiceAudioWasRunning);
   TestEqual(TEXT("Resume reuses bank"),System->ServiceSounds.Num(),5);
  }
  TWeakObjectPtr<URailLocoSoundComponent> RetiredMotor=System->ServiceLocoSound;
  System->DestroyServiceAudio();
  TestFalse(TEXT("Teardown releases bank"),System->ServiceAudioLoaded);
  TestEqual(TEXT("Teardown clears sound references"),System->ServiceSounds.Num(),0);
  TestNull(TEXT("Teardown clears traffic horn"),System->TrafficHornAudio.Get());
  TestNull(TEXT("Teardown clears opposing horn"),System->OpposingHornAudio.Get());
  TestNull(TEXT("Teardown clears opposing rolling"),System->OpposingRollingSound.Get());
  TestNull(TEXT("Teardown clears opposing loco"),System->OpposingLocoSound.Get());
  TestNull(TEXT("Teardown clears player voice"),System->ServiceLocoSound.Get());
  TestTrue(TEXT("Teardown destroys registered component"),!RetiredMotor.IsValid()||!RetiredMotor->IsRegistered());
  System->DestroyServiceAudio(); // Idempotent during world teardown.
 }else AddError(TEXT("No simulation subsystem"));
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailAudioDynamicsTest,"IndianRailway.S8.AudioDynamics",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailAudioDynamicsTest::RunTest(const FString& Parameters){
 Rail::LongitudinalState H;H.massKg=400000;
 Rail::ServiceAudioDynamics Audio;
 auto Mix=Audio.Update(H,.033);
 TestEqual(TEXT("No air at rest"),Mix.applyFlow+Mix.releaseFlow+Mix.ventFlow,0.0);
 TestFalse(TEXT("No startup transient"),Mix.brakeApply||Mix.brakeRelease||Mix.emergency);
 int Apply=0,Release=0,Emergency=0;double PeakApply=0,PeakRelease=0;
 for(int I=1;I<=120;++I){H.appliedBrake=FMath::Min(double(I)/120,1.0);Mix=Audio.Update(H,1.0/30);Apply+=Mix.brakeApply;PeakApply=FMath::Max(PeakApply,Mix.applyFlow);}
 TestTrue(TEXT("A full-service fill exhausts the A9 at full flow"),PeakApply>.9&&Mix.releaseFlow<1e-6);
 for(int I=0;I<30;++I){Mix=Audio.Update(H,1.0/30);Apply+=Mix.brakeApply;}
 TestTrue(TEXT("Exhaust stops when the cylinder is steady"),Mix.applyFlow<.01);
 for(int I=539;I>=0;--I){
  H.appliedBrake=double(I)/540;Mix=Audio.Update(H,1.0/30);Release+=Mix.brakeRelease;PeakRelease=FMath::Max(PeakRelease,Mix.releaseFlow);
  if(I==135)TestTrue(TEXT("Release exhaust falls with cylinder pressure"),Mix.releaseFlow<PeakRelease*.8&&Mix.releaseFlow>.3);
 }
 TestTrue(TEXT("The 18 s graduated release exhausts throughout"),PeakRelease>.9&&Mix.applyFlow<1e-6);
 TestEqual(TEXT("Continuous brake application emits once"),Apply,1);
 TestEqual(TEXT("Continuous brake release emits once"),Release,1);
 Rail::ServiceAudioDynamics HighFps;H.appliedBrake=1;HighFps.Update(H,1.0/120);
 int SlowRelease=0;
 for(int I=1;I<=2160;++I){H.appliedBrake=1.-double(I)/2160;SlowRelease+=HighFps.Update(H,1.0/120).brakeRelease;}
 TestEqual(TEXT("18-second release emits once at 120 FPS"),SlowRelease,1);
 H.command.emergency=true;H.appliedBrake=0;
 Mix=Audio.Update(H,.033);TestFalse(TEXT("Emergency waits for application"),Mix.emergency);
 TestEqual(TEXT("Emergency vents the brake pipe at once"),Mix.ventFlow,1.0);
 for(int I=1;I<=60;++I){H.appliedBrake=FMath::Min(double(I)/15,1.0);Mix=Audio.Update(H,1.0/30);Emergency+=Mix.emergency;}
 TestEqual(TEXT("Emergency latch emits once"),Emergency,1);
 TestTrue(TEXT("Emergency vent dies away as the pipe empties"),Mix.ventFlow<.6&&Mix.ventFlow>.2&&Mix.applyFlow<1e-6);
 Audio.Reset();Mix=Audio.Update(H,.033);
 TestFalse(TEXT("Restore held emergency silently"),Mix.emergency||Mix.brakeApply||Mix.brakeRelease);
 TestEqual(TEXT("Restore does not replay the vent"),Audio.Update(H,.033).ventFlow,0.0);
 H.command.emergency=false;H.appliedBrake=0;Audio.Update(H,.1);
 H.command.emergency=true;H.appliedBrake=.1;Mix=Audio.Update(H,.1);
 TestTrue(TEXT("New emergency after reset audible"),Mix.emergency);
 Rail::ServiceAudioDynamics Slow,Fast;H.command.emergency=false;H.appliedBrake=0;
 Rail::ServiceAudioMix A,B;Slow.Update(H,1.0/30);Fast.Update(H,1.0/120);
 for(int I=1;I<=30;++I){H.appliedBrake=I/120.0;A=Slow.Update(H,1.0/30);}
 for(int I=1;I<=120;++I){H.appliedBrake=I/480.0;B=Fast.Update(H,1.0/120);}
 TestTrue(TEXT("Air flow independent of rendering cadence"),FMath::Abs(A.applyFlow-B.applyFlow)<.02);
 Rail::ServiceAudioDynamics CabSlow,CabFast;
 auto Outside=CabSlow.Perspective(false,0);
 CabFast.Perspective(false,0);
 auto FirstCab=CabSlow.Perspective(true,1.0/30);
 TestTrue(TEXT("Camera cut fades gain instead of stepping"),FirstCab.outsideGain<Outside.outsideGain&&FirstCab.outsideGain>.65);
 TestTrue(TEXT("Cab entry progressively filters external sound"),FirstCab.lowPassHz<Outside.lowPassHz&&FirstCab.lowPassHz>3200);
 Rail::ServiceAudioPerspective VS,VF;
 for(int I=1;I<30;++I)VS=CabSlow.Perspective(true,1.0/30);
 for(int I=0;I<120;++I)VF=CabFast.Perspective(true,1.0/120);
 TestTrue(TEXT("Camera mix is frame-rate independent"),FMath::Abs(VS.lowPassHz-VF.lowPassHz)<1e-7&&FMath::Abs(VS.outsideGain-VF.outsideGain)<1e-8);
 TestTrue(TEXT("Cab retains more motor than external running sound"),VF.motorGain>VF.outsideGain);
 const auto MidExit=CabFast.Perspective(false,1.0/30);
 TestTrue(TEXT("Cab exit progressively restores gain and bandwidth"),MidExit.outsideGain>VF.outsideGain&&MidExit.lowPassHz>VF.lowPassHz&&MidExit.lowPassHz<20000);
 CabFast.Reset();const auto RestoredCab=CabFast.Perspective(true,0);
 TestTrue(TEXT("Resume inside cab starts with cab balance"),FMath::Abs(RestoredCab.outsideGain-.65)<1e-8&&FMath::Abs(RestoredCab.lowPassHz-3200)<1e-7);
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailLocoSoundTest,"IndianRailway.S10.LocoSound",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailLocoSoundTest::RunTest(const FString& Parameters){
 // Goertzel power at one frequency, normalised to the block length.
 auto Power=[](const TArray<float>& X,double Hz,double Rate){const double W=2*PI*Hz/Rate,C=2*FMath::Cos(W);double S1=0,S2=0;for(float V:X){const double S0=V+C*S1-S2;S2=S1;S1=S0;}return (S1*S1+S2*S2-C*S1*S2)/double(X.Num())/double(X.Num());};
 auto Run=[](Rail::LocoSoundSynth& Synth,const Rail::LocoSoundInput& In,double Seconds){
  TArray<float> Out;Out.SetNumZeroed(int32(Seconds*48000));
  for(int32 At=0;At<Out.Num();At+=1600){Rail::LocoSoundInput Step=In;Step.odometer+=In.speedMps*At/48000.;Synth.Submit(Step);Synth.Render(Out.GetData()+At,FMath::Min(1600,Out.Num()-At));}
  return Out;
 };
 auto Tail=[](const TArray<float>& X,int32 From){return TArray<float>(X.GetData()+From,X.Num()-From);};
 const double Speed=71/3.6;
 TestTrue(TEXT("Drive train: 71 km/h is the 1283 rpm continuous rating"),FMath::Abs(Rail::Wap7Drive::MotorRps(Speed)*60-1283)<8);
 Rail::LocoSoundInput Powering;Powering.speedMps=Speed;Powering.effort=.7;Powering.cab=1;
 Rail::LocoSoundSynth A(5),B(5),Idle(5),Other(6);
 const auto X=Run(A,Powering,2),Y=Run(B,Powering,2);
 TestTrue(TEXT("Deterministic per seed"),X==Y);
 TestTrue(TEXT("Another train's seed sounds different"),Run(Other,Powering,2)!=X);
 bool Finite=true;float Peak=0;for(float V:X){Finite&=FMath::IsFinite(V);Peak=FMath::Max(Peak,FMath::Abs(V));}
 TestTrue(TEXT("Bounded finite output"),Finite&&Peak<1&&Peak>.01f);
 const double Mesh=A.MeshHz();
 TestTrue(TEXT("Gear mesh follows wheel speed (20-tooth pinion)"),FMath::Abs(Mesh-Rail::Wap7Drive::MotorRps(Speed)*20)<1e-6);
 Rail::LocoSoundInput Coasting=Powering;Coasting.effort=0;
 const auto Z=Run(Idle,Coasting,2);const auto Loaded=Tail(X,48000),Coast=Tail(Z,48000);
 TestTrue(TEXT("Converter tones need torque (square-wave 5th harmonic)"),Power(Loaded,5*A.StatorHz(),48000)>20*Power(Coast,5*Idle.StatorHz(),48000));
 TestTrue(TEXT("Gear mesh louder under torque"),Power(Loaded,Mesh,48000)>2*Power(Coast,Mesh,48000));
 TestEqual(TEXT("Square-wave operation above base speed"),Rail::Wap7Drive::ModulationFor(A.StatorHz()).pulses,1);
 TestEqual(TEXT("Asynchronous PWM when starting"),Rail::Wap7Drive::ModulationFor(Rail::Wap7Drive::StatorHz(2,1)).pulses,0);
 // Wheel strikes fire once each, in order, when the odometer reaches them.
 Rail::LocoSoundSynth W(9);Rail::LocoSoundInput Run20;Run20.speedMps=20;Run20.cab=1;W.Submit(Run20);
 for(int K=0;K<6;++K){W.Schedule({uint64_t(100+K),5+1.9*K,1,Rail::StrikeKind::Joint});W.Schedule({uint64_t(100+K),5+1.9*K,1,Rail::StrikeKind::Joint});}
 TestEqual(TEXT("Duplicate strikes are ignored"),W.Pending(),6);
 TArray<float> Buffer;Buffer.SetNumZeroed(4800);int32 FirstAt=-1;
 for(int32 Block=0;Block<10;++Block){Rail::LocoSoundInput Step=Run20;Step.odometer=Block*.1*20;W.Submit(Step);W.Render(Buffer.GetData(),4800);if(FirstAt<0&&W.StrikesFired()>0)FirstAt=Block;}
 TestEqual(TEXT("Every axle strikes once"),W.StrikesFired(),6);
 TestEqual(TEXT("First strike at 5 m (0.25 s at 20 m/s)"),FirstAt,2);
 W.Schedule({100,W.Odometer()+1,1,Rail::StrikeKind::Joint});TestEqual(TEXT("A sounded strike is not repeated"),W.Pending(),0);
 // Energised standstill: transformer hum and blowers; de-energised: silence.
 Rail::LocoSoundSynth Still(3),Dead(3);Rail::LocoSoundInput Stand;Stand.cab=1;
 const auto Hum=Tail(Run(Still,Stand,1.5),24000);
 TestTrue(TEXT("Energised standstill: 100 Hz transformer hum"),Power(Hum,100,48000)>50*Power(Hum,137,48000));
 Stand.energized=false;const auto Off=Tail(Run(Dead,Stand,1.5),24000);float OffPeak=0;for(float V:Off)OffPeak=FMath::Max(OffPeak,FMath::Abs(V));
 TestTrue(TEXT("De-energised standstill is silent"),OffPeak<1e-4f);
 Rail::TrainOdometer Odo;Odo.Advance(2,100);const double Base=Odo.metres;Odo.Advance(2,103.5);
 TestTrue(TEXT("Odometer follows the head"),FMath::Abs(Odo.metres-Base-3.5)<1e-9);
 Odo.Advance(2,900);TestTrue(TEXT("A jump moves past the synth snap distance"),Odo.metres-Base>50);
 return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailPassingAudioTest,"IndianRailway.S8.PassingAudio",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailPassingAudioTest::RunTest(const FString& Parameters){
 Rail::PassingAudioDynamics Audio;Rail::PassingAudioMix Mix;int Horns=0;
 double Far=0,Near=0,ApproachPitch=1,DepartPitch=1;
 // Actual relative trajectory test: 40 m/s pass on an adjacent track, 5 m offset.
 for(int I=0;I<=1000;++I){
  const double X=1000-I*2.;Mix=Audio.Update(std::sqrt(X*X+25),.05,true);Horns+=Mix.horn;
  if(I==100)Far=Mix.gain;if(I==490){Near=Mix.gain;ApproachPitch=Mix.pitch;}if(I==650)DepartPitch=Mix.pitch;
 }
 TestEqual(TEXT("One horn per continuous encounter"),Horns,1);
 TestTrue(TEXT("Approach grows and departure fades"),Near>Far*5&&Mix.gain<Near*.1);
 TestTrue(TEXT("Doppler rises approaching falls receding"),ApproachPitch>1&&DepartPitch<1);
 TestTrue(TEXT("Pitch safely bounded"),ApproachPitch<=1.18&&DepartPitch>=.85);
 Audio.Suspend();TestTrue(TEXT("Suspension retains fired encounter"),Audio.hornPlayed);
 TestFalse(TEXT("Resume does not replay horn"),Audio.Update(300,.05,true).horn);
 TestFalse(TEXT("Still approaching after resume does not replay"),Audio.Update(298,.05,true).horn);
 Audio.Update(1300,.05,true);TestFalse(TEXT("Far field rearms encounter"),Audio.hornPlayed);
 Audio.Update(551,.05,true);TestTrue(TEXT("New approach can horn"),Audio.Update(549,.05,true).horn);
 Rail::PassingAudioDynamics Static;Static.Update(100,.05,false);
 TestFalse(TEXT("Stationary traffic does not horn"),Static.Update(99,.05,false).horn);
 Rail::PassingAudioDynamics Cut;Cut.Update(1000,.05,true);
 TestFalse(TEXT("Camera teleport cannot emit approach horn"),Cut.Update(100,.05,true).horn);
 TestTrue(TEXT("Camera teleport avoids Doppler spike"),FMath::Abs(Cut.pitch-1)<1e-8);
 Rail::PassingAudioDynamics Slow,Fast;Rail::PassingAudioMix A,B;
 for(int I=0;I<30;++I)A=Slow.Update(80,1./30,true);
 for(int I=0;I<120;++I)B=Fast.Update(80,1./120,true);
 TestTrue(TEXT("Distance gain smoothing frame-rate independent"),FMath::Abs(A.gain-B.gain)<1e-8);
 return !HasAnyErrors();
}
#endif
