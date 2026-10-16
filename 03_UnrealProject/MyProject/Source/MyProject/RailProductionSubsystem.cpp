#include "RailProductionSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/FileManager.h"
#include "Simulation/RailContent.h"
#include "Simulation/RailService.h"
#include "Misc/AutomationTest.h"

namespace {
bool Fail(FString& Diagnostic,const FString& Message){Diagnostic=Message;return false;}
FString Canonical(const TSharedPtr<FJsonValue>& Value){FString Text;auto Writer=TJsonWriterFactory<>::Create(&Text);FJsonSerializer::Serialize(Value,TEXT(""),Writer);return Text;}
bool String(const FJsonObject& Object,const TCHAR* Key,const FString& Expected){FString Value;return Object.TryGetStringField(Key,Value)&&Value==Expected;}
bool Version(const FString& Value){TArray<FString> Parts;Value.ParseIntoArray(Parts,TEXT("."),false);if(Parts.Num()!=3)return false;for(const auto& Part:Parts){if(Part.IsEmpty()||(Part.Len()>1&&Part[0]=='0'))return false;for(TCHAR C:Part)if(C<'0'||C>'9')return false;}return true;}
bool Number(const FJsonObject& Object,const TCHAR* Key,double Expected){double Value=0;return Object.TryGetNumberField(Key,Value)&&Value==Expected;}
bool Array(const FJsonObject& Object,const TCHAR* Key,const TArray<TSharedPtr<FJsonValue>>*& Values){return Object.TryGetArrayField(Key,Values);}
}

TSharedPtr<FJsonObject> URailProductionSubsystem::Resolve(const FJsonObject& Manifest,const FString& Collection,const FString& Key,const FString& Id){
 const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!Manifest.TryGetArrayField(Collection,Rows))return nullptr;
 TSharedPtr<FJsonObject> Found;
 for(const auto& Row:*Rows){const TSharedPtr<FJsonObject>* Object=nullptr;if(!Row->TryGetObject(Object))return nullptr;FString Value;if((*Object)->TryGetStringField(Key,Value)&&Value==Id){if(Found)return nullptr;Found=*Object;}}
 return Found;
}

bool URailProductionSubsystem::LoadFoundation(TMap<FString,TSharedPtr<FJsonObject>>& Documents,FString& Diagnostic){
 TMap<FString,TSharedPtr<FJsonObject>> Candidate;
 for(const TCHAR* Name:{TEXT("manifest"),TEXT("graphics"),TEXT("localization"),TEXT("workload"),TEXT("profile")}){
  FString Text;const FString Path=FPaths::ProjectContentDir()/TEXT("IndianRailway/Production")/(FString(Name)+TEXT(".json"));
  if(!FFileHelper::LoadFileToString(Text,*Path))return Fail(Diagnostic,TEXT("Missing packaged foundation data: ")+Path);
  TSharedPtr<FJsonObject> Object;if(!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Object)||!Object)return Fail(Diagnostic,TEXT("Invalid JSON: ")+Path);
  Candidate.Add(Name,Object);
 }
 if(!ValidateFoundation(Candidate,Diagnostic))return false;
 Documents=MoveTemp(Candidate);return true;
}

bool URailProductionSubsystem::ValidateFoundation(const TMap<FString,TSharedPtr<FJsonObject>>& Documents,FString& Diagnostic){
 for(const TCHAR* Name:{TEXT("manifest"),TEXT("graphics"),TEXT("localization"),TEXT("workload"),TEXT("profile")}){
  const auto* Value=Documents.Find(Name);if(!Value||!Value->IsValid()||!Number(**Value,TEXT("schemaVersion"),1))return Fail(Diagnostic,FString(Name)+TEXT(": unsupported/missing schema"));
 }
 const auto& Manifest=*Documents[TEXT("manifest")];
 FString ContentVersion;if(!Manifest.TryGetStringField(TEXT("contentVersion"),ContentVersion)||!Version(ContentVersion))return Fail(Diagnostic,TEXT("manifest: invalid content version"));
 if(!String(Manifest,TEXT("contentId"),TEXT("Content.Foundation.01"))||!String(Manifest,TEXT("simulationContentId"),UTF8_TO_TCHAR(Rail::ContentManifest::ContentId)))return Fail(Diagnostic,TEXT("manifest: unsupported content identity"));
 const TArray<TSharedPtr<FJsonValue>>* Chunks=nullptr;
 if(!Array(Manifest,TEXT("chunks"),Chunks)||Chunks->Num()!=static_cast<int32>(Rail::ContentManifest::Chunks().size()))return Fail(Diagnostic,TEXT("manifest: S6 chunk count mismatch"));
 TSet<int32> Seen;
 for(const auto& Value:*Chunks){const TSharedPtr<FJsonObject>* Row=nullptr;double Id=0;if(!Value->TryGetObject(Row)||!(*Row)->TryGetNumberField(TEXT("id"),Id)||Id<1||Id>2||Seen.Contains(static_cast<int32>(Id)))return Fail(Diagnostic,TEXT("manifest: invalid/duplicate chunk"));
  const auto& Expected=Rail::ContentManifest::Chunks()[static_cast<size_t>(Id)-1];
  if(Id!=Expected.id||!String(**Row,TEXT("level"),UTF8_TO_TCHAR(Expected.level))||!String(**Row,TEXT("rebindKey"),UTF8_TO_TCHAR(Expected.rebindKey)))return Fail(Diagnostic,TEXT("manifest: chunk differs from S6"));Seen.Add(static_cast<int32>(Id));
 }
 const TSharedPtr<FJsonObject>* Tiers=nullptr;
 if(!Documents[TEXT("graphics")]->TryGetObjectField(TEXT("tiers"),Tiers))return Fail(Diagnostic,TEXT("graphics: missing tiers"));
 for(const TCHAR* Tier:{TEXT("Low"),TEXT("Medium"),TEXT("High")}){const TSharedPtr<FJsonObject>* Row=nullptr;if(!(*Tiers)->TryGetObjectField(Tier,Row))return Fail(Diagnostic,TEXT("graphics: missing tier"));}
 const TArray<TSharedPtr<FJsonValue>>* Strings=nullptr;
 if(!Array(*Documents[TEXT("localization")],TEXT("strings"),Strings)||Strings->IsEmpty())return Fail(Diagnostic,TEXT("localization: missing strings"));
 TSet<FString> Keys;
 for(const auto& Value:*Strings){const TSharedPtr<FJsonObject>* Row=nullptr;FString Key,En,Hi;if(!Value->TryGetObject(Row)||!(*Row)->TryGetStringField(TEXT("key"),Key)||Key.IsEmpty()||Keys.Contains(Key)||!(*Row)->TryGetStringField(TEXT("en"),En)||En.IsEmpty()||!(*Row)->TryGetStringField(TEXT("hi"),Hi)||Hi.IsEmpty())return Fail(Diagnostic,TEXT("localization: invalid sample"));Keys.Add(Key);}
 if(!Resolve(Manifest,TEXT("vehicleBindings"),TEXT("presentationId"),TEXT("Vehicle.Proxy.01"))||!Resolve(Manifest,TEXT("stationBindings"),TEXT("stationId"),TEXT("Station.Shantipur.01"))||!Resolve(Manifest,TEXT("audioProfiles"),TEXT("profileId"),TEXT("Audio.Foundation.01")))return Fail(Diagnostic,TEXT("manifest: missing foundation bindings"));
 const TSharedPtr<FJsonObject>* Save=nullptr;
 if(!Manifest.TryGetObjectField(TEXT("saveCompatibility"),Save)||!Number(**Save,TEXT("snapshotSchema"),Rail::SnapshotRecord::Schema)||!String(**Save,TEXT("simulationContentId"),UTF8_TO_TCHAR(Rail::ContentManifest::ContentId))||!String(**Save,TEXT("serviceVersion"),UTF8_TO_TCHAR(Rail::ServiceDefinition().version.c_str())))return Fail(Diagnostic,TEXT("saveCompatibility: differs from S6/S7"));
 Diagnostic=TEXT("Foundation configuration loaded; no production acceptance claim");return true;
}

bool URailProductionSubsystem::ValidateSaveMetadata(const FJsonObject& Saved,const FJsonObject& Manifest,FString& Diagnostic){
 const TSharedPtr<FJsonObject>* Current=nullptr;if(!Manifest.TryGetObjectField(TEXT("saveCompatibility"),Current))return Fail(Diagnostic,TEXT("Missing compatibility contract"));
 for(const TCHAR* Key:{TEXT("snapshotSchema"),TEXT("simulationContentId"),TEXT("serviceId"),TEXT("serviceVersion"),TEXT("serviceConfigurationPolicy"),TEXT("vehicleDefinitions"),TEXT("routeContract")}){
  const auto A=Saved.TryGetField(Key),B=(*Current)->TryGetField(Key);if(!A||!B||Canonical(A)!=Canonical(B))return Fail(Diagnostic,FString(TEXT("Incompatible "))+Key+TEXT("; retain checkpoint"));
 }
 FString Id,ExpectedId,OldVersion,CurrentVersion;
 if(!Saved.TryGetStringField(TEXT("contentId"),Id)||!Manifest.TryGetStringField(TEXT("contentId"),ExpectedId)||Id!=ExpectedId)return Fail(Diagnostic,TEXT("Incompatible catalog ID"));
 if(!Saved.TryGetStringField(TEXT("contentVersion"),OldVersion)||!Manifest.TryGetStringField(TEXT("contentVersion"),CurrentVersion))return Fail(Diagnostic,TEXT("Missing content version"));
 if(OldVersion!=CurrentVersion){const TArray<TSharedPtr<FJsonValue>>* Previous=nullptr;bool Found=false;if(Array(Manifest,TEXT("compatiblePreviousVersions"),Previous))for(const auto& V:*Previous){FString Text;if(V->TryGetString(Text)&&Text==OldVersion)Found=true;}if(!Found)return Fail(Diagnostic,TEXT("No explicit content compatibility declaration"));}
 Diagnostic=TEXT("Metadata compatible; S6/S7 integrity and exact service-header validation still required");return true;
}

bool URailProductionSubsystem::RunFoundationChecks(TArray<FString>& Failures){
 TMap<FString,TSharedPtr<FJsonObject>> Documents;FString Diagnostic;
 if(!LoadFoundation(Documents,Diagnostic)){Failures.Add(Diagnostic);return false;}
 auto Record=[&](bool Passed,const TCHAR* Name){if(!Passed)Failures.Add(Name);};
 auto Manifest=Documents[TEXT("manifest")];
 const FString Original=Manifest->GetStringField(TEXT("contentVersion"));
 Manifest->SetStringField(TEXT("contentVersion"),TEXT("invalid"));Record(!ValidateFoundation(Documents,Diagnostic),TEXT("invalid version rejected"));Manifest->SetStringField(TEXT("contentVersion"),Original);
 auto Vehicle=Resolve(*Manifest,TEXT("vehicleBindings"),TEXT("presentationId"),TEXT("Vehicle.Proxy.01"));
 Record(Vehicle.IsValid()&&Number(*Vehicle,TEXT("definitionId"),1),TEXT("vehicle binding"));
 auto Station=Resolve(*Manifest,TEXT("stationBindings"),TEXT("stationId"),TEXT("Station.Shantipur.01"));Record(Station.IsValid()&&String(*Station,TEXT("serviceId"),TEXT("S7-Shantipur-01")),TEXT("station binding"));
 auto Audio=Resolve(*Manifest,TEXT("audioProfiles"),TEXT("profileId"),TEXT("Audio.Foundation.01"));Record(Audio.IsValid()&&Audio->HasTypedField<EJson::Object>(TEXT("layers")),TEXT("audio binding"));
 auto Saved=MakeShared<FJsonObject>(*Manifest->GetObjectField(TEXT("saveCompatibility")));Saved->SetStringField(TEXT("contentId"),Manifest->GetStringField(TEXT("contentId")));Saved->SetStringField(TEXT("contentVersion"),Original);
 Record(ValidateSaveMetadata(*Saved,*Manifest,Diagnostic),TEXT("matching save metadata"));Saved->SetNumberField(TEXT("snapshotSchema"),999);Record(!ValidateSaveMetadata(*Saved,*Manifest,Diagnostic),TEXT("future schema rejected"));
 // Exercise the actual legacy codec without touching the user's checkpoint slots.
 Rail::OperationsWorld World;Rail::ServiceRuntime Service;Rail::SnapshotRecord Snapshot;
 auto Result=Service.Select();if(Result.ok)Result=Service.ConfirmFormation();if(Result.ok)Result=Service.Start(World);if(Result.ok)Result=Service.Capture(World,1,Snapshot);
 Record(Result.ok,TEXT("S7 in-memory capture"));
 Manifest->SetStringField(TEXT("contentVersion"),TEXT("0.2.0"));
 Rail::OperationsWorld Restored;Rail::ServiceRuntime RestoredService;
 Record(Result.ok&&Rail::ServiceRuntime::Restore(Snapshot,Rail::ServiceDefinition(),Restored,RestoredService).ok,TEXT("presentation replacement preserves S7 save"));
 Manifest->SetStringField(TEXT("contentVersion"),Original);
 return Failures.IsEmpty();
}

void URailProductionSubsystem::Initialize(FSubsystemCollectionBase& Collection){
 Super::Initialize(Collection);
 TArray<FString> Failures;const bool Passed=RunFoundationChecks(Failures);
 auto Report=MakeShared<FJsonObject>();Report->SetStringField(TEXT("scope"),TEXT("S8 foundation configuration; not representative workload"));Report->SetBoolField(TEXT("passed"),Passed);Report->SetStringField(TEXT("utc"),FDateTime::UtcNow().ToIso8601());
 TArray<TSharedPtr<FJsonValue>> Rows;for(const auto& Failure:Failures)Rows.Add(MakeShared<FJsonValueString>(Failure));Report->SetArrayField(TEXT("failures"),Rows);
 FString Text;FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Text));const FString Directory=FPaths::ProjectSavedDir()/TEXT("S8");IFileManager::Get().MakeDirectory(*Directory,true);FFileHelper::SaveStringToFile(Text,*(Directory/TEXT("foundation-runtime.json")));
 UE_LOG(LogTemp,Display,TEXT("S8 foundation configuration: %s"),Passed?TEXT("PASS"):TEXT("FAIL"));
}

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailProductionTest,"IndianRailway.S8.Foundation",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailProductionTest::RunTest(const FString& Parameters){TArray<FString> Failures;const bool Passed=URailProductionSubsystem::RunFoundationChecks(Failures);for(const auto& Failure:Failures)AddError(Failure);TestTrue(TEXT("Packaged catalog and legacy compatibility"),Passed);return Passed;}
#endif
