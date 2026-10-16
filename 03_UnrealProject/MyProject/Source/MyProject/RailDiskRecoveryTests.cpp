#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Guid.h"
#include "HAL/FileManager.h"
#include "Simulation/RailEgmoreService.h"
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailDiskRecoveryTest,"IndianRailway.S8.IsolatedDiskRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailDiskRecoveryTest::RunTest(const FString& Parameters){
 const FString Root=FPaths::ProjectSavedDir()/TEXT("Automation/S8DiskRecovery")/FGuid::NewGuid().ToString();
 AddInfo(TEXT("Isolated evidence retained at ")+Root);
 for(int Family=0;Family<3;++Family){
  Rail::ServiceDefinition D;
  if(Family==1){D.contentId=Rail::FormationContent::LhbId;D.berthHead=Rail::FormationContent::ServiceBerthHead;D.id="S8-Shantipur-LHB-01";D.version="IR-S8-LHB-service-1";D.formation="WAP-7 presentation + LHB AC 3-tier";}
  if(Family==2)D=Rail::MakeEgmoreService();
  const FString Dir=Root/FString::FromInt(Family);IFileManager::Get().MakeDirectory(*Dir,true);
  auto File=[&](unsigned Slot){return Dir/FString::Printf(TEXT("checkpoint-%u.irs"),Slot);};
  bool Interrupt=false;
  Rail::CheckpointIO IO{[&](unsigned Slot){std::vector<uint8_t> Out;if(!IFileManager::Get().FileExists(*File(Slot)))return Out;TArray<uint8> Bytes;if(FFileHelper::LoadFileToArray(Bytes,*File(Slot))&&!Bytes.IsEmpty())Out.assign(Bytes.GetData(),Bytes.GetData()+Bytes.Num());return Out;},[&](unsigned Slot,const std::vector<uint8_t>& Bytes){const int32 Size=Interrupt?int32(Bytes.size()/2):int32(Bytes.size());TArray<uint8> Out;Out.Append(Bytes.data(),Size);if(!FFileHelper::SaveArrayToFile(Out,*File(Slot)))return Rail::Result::Fail("disk write failed");return Interrupt?Rail::Result::Fail("injected partial write to isolated slot"):Rail::Result{};}};
  Rail::ServiceRuntime Service(D);Rail::OperationsWorld World;
  if(!TestTrue(TEXT("Content initializes"),World.InitializeContent(D.contentId).ok))continue;
  Service.Select();Service.ConfirmFormation();
  if(!TestTrue(TEXT("Service starts"),Service.Start(World).ok))continue;
  Rail::CheckpointCommit First,Second;
  if(!TestTrue(TEXT("First physical disk commit"),Service.Save(World,IO,First).ok))continue;
  Service.Input(World,Rail::CommandKind::Brake,0);Service.Input(World,Rail::CommandKind::Throttle,.25);
  for(int Tick=0;Tick<120;++Tick)if(!Service.Update(World,1.0/30).ok){AddError(TEXT("Update failed"));break;}
  if(!TestTrue(TEXT("Second physical disk commit"),Service.Save(World,IO,Second).ok))continue;
  TestTrue(TEXT("Commits alternate slots"),First.slot!=Second.slot);
  Rail::ServiceRuntime Restored(D);Rail::OperationsWorld Recovered;Rail::RecoverySelection Selected;
  if(TestTrue(TEXT("Fresh instance cold disk recovery"),Restored.Recover(IO,Recovered,Selected).ok)){
   TestEqual(TEXT("Newest generation selected"),Selected.record.Generation(),Second.generation);
   TestTrue(TEXT("Full service and world state identical"),Restored.Json()==Service.Json()&&Recovered.Json()==World.Json());
   bool Same=true;for(int Tick=0;Tick<60&&Same;++Tick)Same=Restored.Update(Recovered,1.0/30).ok&&Service.Update(World,1.0/30).ok&&Restored.Json()==Service.Json()&&Recovered.Json()==World.Json();
   TestTrue(TEXT("Recovered deterministic continuation"),Same);
  }
  const auto Good=IO.read(Second.slot);Interrupt=true;Rail::CheckpointCommit Failed;
  TestFalse(TEXT("Partial disk commit rejected"),Service.Save(World,IO,Failed).ok);Interrupt=false;
  TestTrue(TEXT("Last valid disk slot untouched"),IO.read(Second.slot)==Good);
  Rail::ServiceRuntime Fallback(D);Rail::OperationsWorld FallbackWorld;
  TestTrue(TEXT("Partial slot ignored on cold recovery"),Fallback.Recover(IO,FallbackWorld,Selected).ok&&Selected.record.Generation()==Second.generation);
  auto Other=D;Other.version="incompatible-test-only";Rail::ServiceRuntime Incompatible(Other);Rail::OperationsWorld OtherWorld;
  const auto Slot0=IO.read(0),Slot1=IO.read(1);
  TestFalse(TEXT("Incompatible content rejected"),Incompatible.Recover(IO,OtherWorld,Selected).ok);
  TestTrue(TEXT("Rejected recovery retains both physical files"),IO.read(0)==Slot0&&IO.read(1)==Slot1);
 }
 return !HasAnyErrors();
}
#endif
