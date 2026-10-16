#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Dom/JsonObject.h"
#include "RailProductionSubsystem.generated.h"

// Presentation catalog only. No world/physics/service mutation API is exposed.
// Build-time schema validation is authoritative for authored catalog structure.
UCLASS()
class MYPROJECT_API URailProductionSubsystem : public UGameInstanceSubsystem {
 GENERATED_BODY()
public:
 virtual void Initialize(FSubsystemCollectionBase& Collection) override;
 static bool LoadFoundation(TMap<FString,TSharedPtr<FJsonObject>>& Documents,FString& Diagnostic);
 static bool ValidateFoundation(const TMap<FString,TSharedPtr<FJsonObject>>& Documents,FString& Diagnostic);
 static bool ValidateSaveMetadata(const FJsonObject& Saved,const FJsonObject& Manifest,FString& Diagnostic);
 static bool RunFoundationChecks(TArray<FString>& Failures);
 static TSharedPtr<FJsonObject> Resolve(const FJsonObject& Manifest,const FString& Collection,const FString& Key,const FString& Id);
};
