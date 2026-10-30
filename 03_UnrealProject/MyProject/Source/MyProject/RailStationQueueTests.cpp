#include "RailTambaramStationView.h"
#include "Simulation/RailTambaramService.h"
#include "Simulation/RailWorldSnapshot.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#if WITH_EDITOR && WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRailStationQueueTest,"IndianRailway.S9.StationBuildQueue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FRailStationQueueTest::RunTest(const FString&){
 Rail::OperationsWorld Logical;Rail::ServiceRuntime Service(Rail::MakeLiveTambaramService());
 auto R=Service.Select();if(R.ok)R=Service.ConfirmFormation();if(R.ok)R=Service.Start(Logical);
 if(!TestTrue(TEXT("Live service initializes"),R.ok))return false;
 Rail::SnapshotRecord Before,After;Service.Capture(Logical,1,Before);
 const auto Init=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&Init);
 if(!World){AddError(TEXT("No station test world"));return false;}
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(World);
 auto* View=World->SpawnActor<ARailTambaramStationView>();
 if(View){
  auto Counts=[&](){
   TArray<UHierarchicalInstancedStaticMeshComponent*> Components;View->GetComponents(Components);
   int32 Instances=0;for(auto* Part:Components){Instances+=Part->GetInstanceCount();TestTrue(TEXT("Scenery collision disabled"),Part->GetCollisionEnabled()==ECollisionEnabled::NoCollision);}
   return FIntPoint(Components.Num(),Instances);
  };
  FIntPoint OriginCounts=FIntPoint::ZeroValue;
  // Switch to an empty distant chunk and back to exercise retirement and restart.
  for(int32 Chunk:{1,2,1}){
   TestFalse(TEXT("Planning does not claim readiness"),View->Present(Logical,Chunk));
   TestFalse(TEXT("Readiness stays false during build"),View->IsReady());
   bool Complete=false;int32 Frames=0;
   for(;Frames<500&&!Complete;++Frames){
    const auto A=Counts();Complete=View->Present(Logical,Chunk);const auto B=Counts();
    TestTrue(TEXT("At most two components created per update"),B.X-A.X<=2);
    TestTrue(TEXT("At most two components retired per update"),A.X-B.X<=2);
    TestTrue(TEXT("At most 128 instances added per update"),B.Y-A.Y<=128);
    TestEqual(TEXT("Return value agrees with readiness"),Complete,View->IsReady());
   }
   TestTrue(TEXT("Bounded queue finishes"),Complete);
   const auto Built=Counts();
   if(Chunk==1){
    TestTrue(TEXT("Origin scenery constructed"),Built.X>0&&Built.Y>128);
    TestTrue(TEXT("Origin requires multiple updates"),Frames>1);
    if(OriginCounts.X)TestTrue(TEXT("Rebuild preserves component and instance counts"),Built==OriginCounts);
    OriginCounts=Built;
   }else TestTrue(TEXT("Distant empty chunk has no leftover components"),Built==FIntPoint::ZeroValue);
   TestTrue(TEXT("Ready presentation is idempotent"),View->Present(Logical,Chunk));
   TestTrue(TEXT("Repeated presentation does not add components"),Counts()==Built);
  }
  View->Destroy();
  // Dropping a streamed chunk halfway through a queue must also be safe.
  View=World->SpawnActor<ARailTambaramStationView>();
  View->Present(Logical,1);View->Present(Logical,1);
  TestTrue(TEXT("Partially built actor is disposable"),View->Destroy());
 }else AddError(TEXT("Station actor creation failed"));
 Service.Capture(Logical,1,After);
 TestTrue(TEXT("Presentation cannot change saved railway state"),Before.Bytes()==After.Bytes());
 World->DestroyWorld(false);GEngine->DestroyWorldContext(World);
 return !HasAnyErrors();
}
#endif
