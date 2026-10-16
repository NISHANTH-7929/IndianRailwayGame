#include "RailOperationsView.h"
#include "DrawDebugHelpers.h"
namespace {FVector Cm(Rail::Vec3 V){return FVector(V.x*100,V.y*100,V.z*100);}}
void ARailOperationsView::Present(const Rail::Graph& Graph,const Rail::OperationsDefinitions& Definitions,const Rail::OperationsSnapshot& Snapshot){
 Display=Snapshot;
 for(const auto& B:Definitions.blocks){auto State=Display.blocks.find(B.first);if(State==Display.blocks.end())continue;
  const FColor Color=!State->second.occupants.empty()?FColor::Orange:State->second.reservation.Active()?FColor::Purple:FColor(80,100,100);
  for(const auto& C:B.second.coverage){const auto* E=Graph.Find(C.edge);if(!E)continue;const int N=FMath::CeilToInt((C.to-C.from)/4);
   FVector Prev=Cm(E->geometry.At(C.from).position)+FVector(0,0,70);
   for(int i=1;i<=N;++i){const FVector Next=Cm(E->geometry.At(C.from+(C.to-C.from)*i/N).position)+FVector(0,0,70);DrawDebugLine(GetWorld(),Prev,Next,Color,false,0,0,45);Prev=Next;}
  }
 }
 for(const auto& S:Definitions.signals){Rail::Sample Sample;if(!Graph.SamplePosition(S.second.boundary,Sample).ok)continue;auto State=Display.signals.find(S.first);
  const bool Proceed=State!=Display.signals.end()&&State->second.aspect==Rail::SignalAspect::Proceed;const auto Location=Cm(Sample.position)+FVector(0,-650,1000);
  DrawDebugSphere(GetWorld(),Location,230,12,Proceed?FColor::Green:FColor::Red,false,0,0,55);
  DrawDebugString(GetWorld(),Location+FVector(0,0,450),FString::Printf(TEXT("S%llu %s"),S.first.value,Proceed?TEXT("PROCEED"):TEXT("STOP")),nullptr,FColor::White,0,true,1.2f);
 }
 for(const auto& P:Display.points){const Rail::EdgeId Edge(P.first.value==1?1:7);const auto* E=Graph.Find(Edge);if(!E)continue;
  const auto Location=Cm(E->geometry.At(P.first.value==1?240:0).position)+FVector(0,800,1500);
  DrawDebugString(GetWorld(),Location,FString::Printf(TEXT("P%llu pos%d R%llu"),P.first.value,P.second.position,P.second.lock.route.value),nullptr,P.second.lock.Active()?FColor::Purple:FColor::White,0,true,1.2f);
 }
}
FString ARailOperationsView::PresentationDigest() const {
 FString Result=FString::Printf(TEXT("tick=%llu valid=%d"),Display.tick,Display.valid);
 for(const auto& S:Display.signals)Result+=FString::Printf(TEXT(" S%llu:%d:R%llu:T%llu"),S.first.value,int(S.second.aspect),S.second.route.value,S.second.train.value);
 for(const auto& P:Display.points)Result+=FString::Printf(TEXT(" P%llu:%d:R%llu:T%llu:G%llu"),P.first.value,P.second.position,P.second.lock.route.value,P.second.lock.train.value,P.second.lock.generation);
 return Result;
}
