#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RailServiceWidget.generated.h"
class UTextBlock;
class UVerticalBox;
class UButton;
class UBorder;
class UHorizontalBox;
class UCanvasPanel;
class USlider;
UCLASS()
class MYPROJECT_API URailServiceWidget : public UUserWidget {
 GENERATED_BODY()
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry& Geometry,float DeltaTime) override;
private:
 UPROPERTY() TObjectPtr<UTextBlock> Hud;
 UPROPERTY() TObjectPtr<UTextBlock> PanelText;
 UPROPERTY() TObjectPtr<UTextBlock> PrimaryText;
 UPROPERTY() TObjectPtr<UTextBlock> SecondaryText;
 UPROPERTY() TObjectPtr<UVerticalBox> Menu;
 UPROPERTY() TObjectPtr<UVerticalBox> ServiceMenu;
 UPROPERTY() TObjectPtr<UVerticalBox> AudioControls;
 UPROPERTY() TObjectPtr<UTextBlock> AudioToggleText;
 UPROPERTY() TArray<TObjectPtr<UTextBlock>> AudioLabels;
 UPROPERTY() TArray<TObjectPtr<USlider>> AudioSliders;
 bool bAudioSettingsOpen=false;
 bool bQuietHud=false;
 UPROPERTY() TObjectPtr<UBorder> MenuBackdrop;
 UPROPERTY() TObjectPtr<UVerticalBox> Controls;
 UPROPERTY() TObjectPtr<UHorizontalBox> PowerRow;
 UPROPERTY() TObjectPtr<UHorizontalBox> DirectionRow;
 UPROPERTY() TObjectPtr<UButton> SecondaryButton;
 UPROPERTY() TObjectPtr<UCanvasPanel> EgmoreControls;
 void BuildEgmoreControls(UCanvasPanel* Canvas);
 UFUNCTION() void Primary();
 UFUNCTION() void Secondary();
 UFUNCTION() void ToggleAudioSettings();
 UFUNCTION() void ToggleHud();
 UFUNCTION() void MasterVolumeChanged(float Value);
 UFUNCTION() void EngineVolumeChanged(float Value);
 UFUNCTION() void EffectsVolumeChanged(float Value);
 UFUNCTION() void Power0();
 UFUNCTION() void Power10();
 UFUNCTION() void Power25();
 UFUNCTION() void Power50();
 UFUNCTION() void Power100();
 UFUNCTION() void Brake0();
 UFUNCTION() void Brake15();
 UFUNCTION() void Brake30();
 UFUNCTION() void Brake50();
 UFUNCTION() void Brake100();
 UFUNCTION() void Reverse();
 UFUNCTION() void Neutral();
 UFUNCTION() void Forward();
 UFUNCTION() void Horn();
 UFUNCTION() void LookBack();
 UFUNCTION() void CabCamera();
 UFUNCTION() void LocoClass();
 UFUNCTION() void Lamps();
 UFUNCTION() void Pantograph();
 UFUNCTION() void Wipers();
 UFUNCTION() void CabLamp();
 UFUNCTION() void Instruments();
 UFUNCTION() void Pause();
 UFUNCTION() void Depart();
 UFUNCTION() void Emergency();
 UFUNCTION() void ResetEmergency();
 UFUNCTION() void Debug();
 UFUNCTION() void Exit();
};

