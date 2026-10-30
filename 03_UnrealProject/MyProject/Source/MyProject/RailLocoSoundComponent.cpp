#include "RailLocoSoundComponent.h"

URailLocoSoundComponent::URailLocoSoundComponent(const FObjectInitializer& Initializer):Super(Initializer){
 NumChannels=1;bAutoActivate=false;bAutoDestroy=false;
}
void URailLocoSoundComponent::Configure(uint32 Seed,uint32 InLayers){
 Layers=InLayers;Synth=Rail::LocoSoundSynth(Seed,InLayers);
}
void URailLocoSoundComponent::Push(const Rail::LocoSoundInput& Input,TArray<Rail::WheelStrike>&& Strikes){
 LastInput=Input;
 if(!IsPlaying())return;
 StrikesQueued+=Strikes.Num();
 SynthCommand([this,Input,Strikes=MoveTemp(Strikes)](){Synth.Submit(Input);for(const auto& S:Strikes)Synth.Schedule(S);});
}
bool URailLocoSoundComponent::Init(int32& SampleRate){
 Synth.Prepare(SampleRate);return true;
}
int32 URailLocoSoundComponent::OnGenerateAudio(float* OutAudio,int32 NumSamples){
 Synth.Render(OutAudio,NumSamples);return NumSamples;
}
