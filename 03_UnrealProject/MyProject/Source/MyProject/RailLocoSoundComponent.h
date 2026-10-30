#pragma once
#include "CoreMinimal.h"
#include "Components/SynthComponent.h"
#include "RailLocoSound.h"
#include "RailLocoSoundComponent.generated.h"

// Audio-thread host for the procedural train sound (RailLocoSound.h). The game thread queues only
// the newest simulation state and wheel strikes; the synth state is touched on the render thread.
UCLASS(ClassGroup=Synth)
class URailLocoSoundComponent : public USynthComponent {
 GENERATED_BODY()
public:
 URailLocoSoundComponent(const FObjectInitializer& Initializer);
 // Before the first Start: a distinct seed per train, and which layers this voice renders.
 void Configure(uint32 Seed,uint32 Layers);
 // Queued for the audio thread; ignored while stopped so the command queue cannot grow.
 void Push(const Rail::LocoSoundInput& Input,TArray<Rail::WheelStrike>&& Strikes);
 Rail::LocoSoundInput LastInput; // game-thread copy for tests and review logs
 uint32 Layers=Rail::SoundAll;
 int32 StrikesQueued=0;
protected:
 virtual bool Init(int32& SampleRate) override;
 virtual int32 OnGenerateAudio(float* OutAudio,int32 NumSamples) override;
private:
 Rail::LocoSoundSynth Synth;
};
