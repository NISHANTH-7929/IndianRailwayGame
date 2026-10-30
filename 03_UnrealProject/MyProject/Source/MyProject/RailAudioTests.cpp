#include "Misc/AutomationTest.h"
#include "Sound/SoundWave.h"
#include "RailSimulationSubsystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundClass.h"
#include "Sound/SoundSubmix.h"
#include "SubmixEffects/AudioMixerSubmixEffectDynamicsProcessor.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailAudioBankTest,"IndianRailway.S8.AudioBank",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailAudioBankTest::RunTest(const FString& Parameters){
 const TCHAR* Names[]={TEXT("Rail"),TEXT("Station"),TEXT("Horn"),TEXT("Brake"),TEXT("Motor"),TEXT("Coast"),TEXT("Squeal"),TEXT("BrakeRelease"),TEXT("Emergency")};
 for(int32 I=0;I<9;++I){
  const FString Path=FString::Printf(TEXT("/Game/IndianRailway/%sSW_%s.SW_%s"),I<4?TEXT("S8/Audio/"):TEXT("S9/JourneyAudioV3/"),Names[I],Names[I]);
  auto* Sound=LoadObject<USoundWave>(nullptr,*Path);
  if(!TestNotNull(*Path,Sound))continue;
  TestEqual(TEXT("Mono bank"),Sound->NumChannels,1);
  TestEqual(TEXT("Only continuous textures loop"),bool(Sound->bLooping),I<2||(I>=4&&I<=6));
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
  TestEqual(TEXT("Exactly ten resident sounds"),System->ServiceSounds.Num(),10);
  TestNotNull(TEXT("Dedicated protected audio bus"),System->ServiceAudioSubmix.Get());
  TestEqual(TEXT("Limiter owned by service bus"),System->ServiceAudioSubmix->SubmixEffectChain.Num(),1);
  auto* Limiter=Cast<USubmixEffectDynamicsProcessorPreset>(System->ServiceAudioSubmix->SubmixEffectChain[0]);
  if(TestNotNull(TEXT("Dynamics preset"),Limiter)){
   TestEqual(TEXT("Makeup retained across effect initialization"),Limiter->Settings.InputGainDb,18.f);
   TestEqual(TEXT("Peak limiter mode retained"),Limiter->Settings.DynamicsProcessorType,ESubmixEffectDynamicsProcessorType::Limiter);
   TestEqual(TEXT("Limiter has overlap headroom"),Limiter->Settings.ThresholdDb,-6.f);
   TestEqual(TEXT("Limiter has final output headroom"),Limiter->Settings.OutputGainDb,-3.f);
  }
  TestTrue(TEXT("Voices routed through service class"),System->ServiceMotorAudio->SoundClassOverride==System->ServiceAudioClass);
  TestTrue(TEXT("Class routes to protected bus"),System->ServiceAudioClass->Properties.DefaultSubmix==System->ServiceAudioSubmix);
  TestTrue(TEXT("Traffic horn spatialized"),System->TrafficHornAudio->bAllowSpatialization);
  TestTrue(TEXT("Opposing horn spatialized"),System->OpposingHornAudio->bAllowSpatialization);
  TestTrue(TEXT("Opposing horn shares protected bus"),System->OpposingHornAudio->SoundClassOverride==System->ServiceAudioClass);
  TestTrue(TEXT("Traffic horn shares protected bus"),System->TrafficHornAudio->SoundClassOverride==System->ServiceAudioClass);
  System->TrafficHornDynamics.hornPlayed=true;
  System->OpposingHornDynamics.hornPlayed=true;
  // Change only in-memory preferences; automated checks must not write user config.
  System->ServiceAudioPreferencesLoaded=true;
  System->ServiceAudioVolumes[0]=0;
  Rail::LongitudinalState Audible;Audible.massKg=400000;Audible.speedMps=20;Audible.appliedThrottle=1;Audible.tractionN=200000;
  System->ApplyServiceAudio(Audible,0,.1f,1,false);
  TestEqual(TEXT("Master zero mutes motor"),System->ServiceMotorAudio->VolumeMultiplier,0.f);
  TestEqual(TEXT("Master zero mutes rail"),System->ServiceMovementAudio->VolumeMultiplier,0.f);
  TestEqual(TEXT("Master zero mutes alerts"),System->ServiceCueAudio->VolumeMultiplier,0.f);
  System->ServiceAudioVolumes[0]=1;System->ServiceAudioVolumes[1]=0;
  System->ApplyServiceAudio(Audible,0,.1f,1,false);
  TestEqual(TEXT("Engine category mutes traction"),System->ServiceMotorAudio->VolumeMultiplier,0.f);
  TestTrue(TEXT("Engine category retains effects"),System->ServiceMovementAudio->VolumeMultiplier>0);
  System->ServiceAudioVolumes[1]=1;System->ServiceAudioVolumes[2]=0;
  System->ApplyServiceAudio(Audible,0,.1f,1,false);
  TestTrue(TEXT("Effects category retains engine"),System->ServiceMotorAudio->VolumeMultiplier>0);
  TestEqual(TEXT("Effects zero mutes rolling"),System->ServiceMovementAudio->VolumeMultiplier,0.f);
  TestEqual(TEXT("Effects zero mutes emergency"),System->ServiceEmergencyAudio->VolumeMultiplier,0.f);
  System->ServiceAudioVolumes[2]=1;
  for(int32 Reason=0;Reason<3;++Reason){
   System->S6Paused=Reason==0;System->S6Blocked=Reason==1;System->AppBackground.Store(Reason==2);
   System->UpdateServiceAudio();TestFalse(TEXT("Pause/block/background stops running state"),System->ServiceAudioWasRunning);
   for(auto* Audio:{System->ServiceMovementAudio.Get(),System->ServiceStationAudio.Get(),System->ServiceHornAudio.Get(),System->ServiceBrakeAudio.Get(),System->ServiceCueAudio.Get(),
       System->ServiceMotorAudio.Get(),System->ServiceCoastAudio.Get(),System->ServiceSquealAudio.Get(),System->ServiceBrakeReleaseAudio.Get(),System->ServiceEmergencyAudio.Get()}){
    if(TestNotNull(TEXT("Owned voice"),Audio))TestFalse(TEXT("Suspended voice stopped"),Audio->IsPlaying());
   }
   TestEqual(TEXT("Rail gain resets"),System->ServiceRailGain,0.f);
   TestEqual(TEXT("Station gain resets"),System->ServiceStationGain,0.f);
   TestTrue(TEXT("Pause preserves traffic encounter latch"),System->TrafficHornDynamics.hornPlayed);
   TestFalse(TEXT("Traffic horn stops on suspension"),System->TrafficHornAudio->IsPlaying());
   TestFalse(TEXT("Traffic rolling stops on suspension"),System->TrafficRollingAudio->IsPlaying());
   TestFalse(TEXT("Traffic motor stops on suspension"),System->TrafficMotorAudio->IsPlaying());
   TestTrue(TEXT("Opposing encounter retained during suspension"),System->OpposingHornDynamics.hornPlayed);
   for(auto* Voice:{System->OpposingHornAudio.Get(),System->OpposingRollingAudio.Get(),System->OpposingMotorAudio.Get()})TestFalse(TEXT("Opposing voice stops on suspension"),Voice->IsPlaying());
   System->S6Paused=false;System->S6Blocked=false;System->AppBackground.Store(false);System->UpdateServiceAudio();
   TestTrue(TEXT("Resume audio state"),System->ServiceAudioWasRunning);
   TestEqual(TEXT("Resume reuses bank"),System->ServiceSounds.Num(),10);
  }
  TWeakObjectPtr<UAudioComponent> RetiredMotor=System->ServiceMotorAudio;
  System->DestroyServiceAudio();
  TestFalse(TEXT("Teardown releases bank"),System->ServiceAudioLoaded);
  TestEqual(TEXT("Teardown clears sound references"),System->ServiceSounds.Num(),0);
  TestNull(TEXT("Teardown clears traffic horn"),System->TrafficHornAudio.Get());
  TestNull(TEXT("Teardown clears opposing horn"),System->OpposingHornAudio.Get());
  TestNull(TEXT("Teardown clears opposing rolling"),System->OpposingRollingAudio.Get());
  TestNull(TEXT("Teardown clears opposing motor"),System->OpposingMotorAudio.Get());
  TestTrue(TEXT("Teardown destroys registered component"),!RetiredMotor.IsValid()||!RetiredMotor->IsRegistered());
  System->DestroyServiceAudio(); // Idempotent during world teardown.
 }else AddError(TEXT("No simulation subsystem"));
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);return !HasAnyErrors();
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailAudioDynamicsTest,"IndianRailway.S8.AudioDynamics",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailAudioDynamicsTest::RunTest(const FString& Parameters){
 Rail::LongitudinalState H;H.massKg=400000;
 Rail::ServiceAudioDynamics Audio;
 auto Mix=Audio.Update(H,0,.033);
 TestEqual(TEXT("No moving texture at rest"),Mix.rail+Mix.motor+Mix.coast+Mix.squeal,0.0);
 TestFalse(TEXT("No startup transient"),Mix.brakeApply||Mix.brakeRelease||Mix.emergency);
 H.speedMps=20;H.appliedThrottle=.8;H.tractionN=180000;
 for(int I=0;I<60;++I)Mix=Audio.Update(H,0,1.0/30);
 const auto Powered=Mix;
 TestTrue(TEXT("Motion and real traction audible"),Mix.rail>.1&&Mix.motor>.05);
 TestEqual(TEXT("Straight unbraked has no squeal"),Mix.squeal,0.0);
 H.appliedThrottle=0;H.tractionN=0;
 for(int I=0;I<60;++I)Mix=Audio.Update(H,0,1.0/30);
 TestTrue(TEXT("Coast removes power and raises running air"),Mix.motor<.001&&Mix.coast>Powered.coast);
 for(int I=0;I<60;++I)Mix=Audio.Update(H,.004,1.0/30);
 TestTrue(TEXT("Actual curved edge activates contact approximation"),Mix.squeal>.01);
 Rail::ServiceAudioDynamics Reverse;H.speedMps=-20;
 for(int I=0;I<60;++I)Mix=Reverse.Update(H,0,1.0/30);
 TestTrue(TEXT("Reverse running still audible"),Mix.rail>.1);
 H.speedMps=0;H.appliedBrake=0;Audio.Reset();Audio.Update(H,0,.033);
 int Apply=0,Release=0,Emergency=0;
 for(int I=1;I<=30;++I){H.appliedBrake=double(I)/30;Mix=Audio.Update(H,0,1.0/30);Apply+=Mix.brakeApply;}
 for(int I=0;I<30;++I){Mix=Audio.Update(H,0,1.0/30);Apply+=Mix.brakeApply;}
 for(int I=29;I>=0;--I){H.appliedBrake=double(I)/30;Mix=Audio.Update(H,0,1.0/30);Release+=Mix.brakeRelease;}
 TestEqual(TEXT("Continuous brake application emits once"),Apply,1);
 TestEqual(TEXT("Continuous brake release emits once"),Release,1);
 Rail::ServiceAudioDynamics HighFps;H.appliedBrake=1;HighFps.Update(H,0,1.0/120);
 int SlowRelease=0;
 for(int I=1;I<=2160;++I){H.appliedBrake=1.-double(I)/2160;SlowRelease+=HighFps.Update(H,0,1.0/120).brakeRelease;}
 TestEqual(TEXT("18-second release emits once at 120 FPS"),SlowRelease,1);
 H.command.emergency=true;H.appliedBrake=0;
 Mix=Audio.Update(H,0,.033);TestFalse(TEXT("Emergency waits for application"),Mix.emergency);
 for(int I=1;I<=60;++I){H.appliedBrake=FMath::Min(double(I)/15,1.0);Mix=Audio.Update(H,0,1.0/30);Emergency+=Mix.emergency;}
 TestEqual(TEXT("Emergency latch emits once"),Emergency,1);
 Audio.Reset();Mix=Audio.Update(H,0,.033);
 TestFalse(TEXT("Restore held emergency silently"),Mix.emergency||Mix.brakeApply||Mix.brakeRelease);
 TestEqual(TEXT("Restore gain begins with no stationary rolling"),Mix.rail,0.0);
 H.command.emergency=false;H.appliedBrake=0;Audio.Update(H,0,.1);
 H.command.emergency=true;H.appliedBrake=.1;Mix=Audio.Update(H,0,.1);
 TestTrue(TEXT("New emergency after reset audible"),Mix.emergency);
 Rail::ServiceAudioDynamics Slow,Fast;H.command.emergency=false;H.appliedBrake=0;H.speedMps=20;
 Rail::ServiceAudioMix A,B;
 for(int I=0;I<30;++I)A=Slow.Update(H,0,1.0/30);
 for(int I=0;I<120;++I)B=Fast.Update(H,0,1.0/120);
 TestTrue(TEXT("Fade independent of rendering cadence"),FMath::Abs(A.rail-B.rail)<1e-8);
 H.speedMps=3;for(int I=0;I<120;++I)B=Fast.Update(H,0,1.0/30);
 TestTrue(TEXT("Deceleration lowers wheel rate and running gain"),B.rail<A.rail&&B.railPitch<A.railPitch);
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
