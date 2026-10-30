#pragma once
#include "RailEgmoreLayout.h"
#include "RailTambaramRoute.h"
#include "RailOperationsTypes.h"

namespace Rail {
// One configuration boundary shared by initialization, service and recovery.
struct ScenarioContent {
 static bool IsEgmore(const std::string& Id){return Id==EgmoreLayout::ReservedContentId;}
 static bool IsLiveJourney(const std::string& Id){return Id=="IR-S9-MSTBM-LIVE-1";}
 static bool IsTambaram(const std::string& Id){return Id==TambaramRoute::ContentId||IsLiveJourney(Id);}
 static size_t TrafficTrack(uint64_t Id){return Id==3?1:Id==4?2:3;}
 static size_t TrafficStart(uint64_t Id){
  if(Id==3)return TambaramRoute::SegmentCount-1;
  size_t I=Id==4?60:90;while(I+1<TambaramRoute::SegmentCount&&TambaramRoute::GeometryFor(TrafficTrack(Id),I).lengthMetres<300)++I;return I;
 }
 static std::vector<DirectedEdge> PlayerPath(const std::string& Id){
  if(!IsLiveJourney(Id))return TambaramRoute::Path(1);
  std::vector<DirectedEdge> P;for(size_t I=0;I<6;++I)P.push_back({TambaramRoute::EdgeFor(1,I),1});
  P.push_back({EdgeId(90000),1});P.push_back({EdgeId(90001),1});
  for(size_t I=7;I<TambaramRoute::SegmentCount;++I)P.push_back({TambaramRoute::EdgeFor(0,I),1});return P;
 }
 static bool IsS8Route(const std::string& Id){return IsEgmore(Id)||IsTambaram(Id);}
 static bool Supports(const std::string& Id){return FormationContent::Supports(Id)||IsS8Route(Id);}
 static bool UsesLhb(const std::string& Id){return Id==FormationContent::LhbId||IsS8Route(Id);}
 static Result Resolve(const std::string& Id,Fixture& F,Formation& Player,Formation& Traffic){
  if(IsTambaram(Id))return TambaramRoute::ResolveSpatial(F,Player,Traffic);
  if(!IsEgmore(Id))return FormationContent::Resolve(Id,F,Player,Traffic);
  auto R=EgmoreLayout::ResolveSpatial(F,Player);Traffic=Player;return R;
 }
 static unsigned TrainCount(const std::string& Id){return IsLiveJourney(Id)?5:IsTambaram(Id)?3:2;}
 static double Start(const std::string& Id,uint64_t Train){if(IsTambaram(Id))return Train==1?110.0:TambaramRoute::StartHead;return Train==1?200:IsEgmore(Id)?EgmoreLayout::StationMetres(EgmoreLayout::StartHeadX):100;}
 static OperationsDefinitions LiveOperations(){
  OperationsDefinitions D;const auto G=TambaramRoute::MakeGraph();
  auto B=[](size_t T,size_t I){return BlockId(10000+TambaramRoute::EdgeFor(T,I).value);};
  for(size_t T=0;T<4;++T)for(size_t I=0;I<TambaramRoute::SegmentCount;++I){
   const auto E=TambaramRoute::EdgeFor(T,I);const double L=G.Find(E)->lengthMetres;
   uint64_t Owner=(T==1&&(I==4||I==10))?1:(T==0&&I==14)?2:0;
   for(uint64_t Id=3;Id<=5;++Id)if(T==TrafficTrack(Id)&&(I==14||I==TrafficStart(Id)))Owner=Id;
   if(T==1&&I==0){D.blocks.emplace(BlockId(3),BlockDefinition{BlockId(3),{{E,0,250}},{},TrainId(2)});D.blocks.emplace(B(T,I),BlockDefinition{B(T,I),{{E,250,L}},{},{}});}
   else D.blocks.emplace(B(T,I),BlockDefinition{B(T,I),{{E,0,L}},{},TrainId(Owner),T==1&&I==4});
  }
  // Merge unbroken plain-line sections; keep holding areas and throat blocks
  // separate. This bounds occupancy scans without weakening route ownership.
  for(size_t T=0;T<4;++T){BlockId Previous;
   for(size_t I=15;I<TambaramRoute::SegmentCount;++I){const auto Id=B(T,I);auto It=D.blocks.find(Id);
    if(It->second.exclusiveTo.value){Previous={};continue;}
    if(!Previous.value){Previous=Id;continue;}
    auto& Into=D.blocks.at(Previous).coverage;Into.insert(Into.end(),It->second.coverage.begin(),It->second.coverage.end());D.blocks.erase(It);
   }
  }
  for(uint64_t E:{90000ull,90001ull})D.blocks.emplace(BlockId(E),BlockDefinition{BlockId(E),{{EdgeId(E),0,G.Find(EdgeId(E))->lengthMetres}},{},{}});
  const std::set<BlockId> Fouling{B(1,5),B(1,6),B(0,6),B(0,7),BlockId(90000),BlockId(90001)};
  for(auto A:Fouling)for(auto C:Fouling)if(A!=C)D.blocks.at(A).conflicts.insert(C);
  D.points.emplace(PointId(1),PointDefinition{PointId(1),{{0,{TambaramRoute::TransitionFor(1,5,false)}},{1,{TransitionId(90000)}}},Fouling});
  D.points.emplace(PointId(2),PointDefinition{PointId(2),{{0,{TambaramRoute::TransitionFor(0,6,false)}},{1,{TransitionId(90004)}}},Fouling});
  auto Add=[&](uint64_t Id,uint64_t Train,std::vector<DirectedEdge> P,TrackPosition From,TrackPosition To,BlockId Origin,BlockId Destination,std::map<PointId,int> Points){
   std::set<BlockId> Blocks;
   for(auto E:P)for(const auto& Pair:D.blocks)for(const auto& Range:Pair.second.coverage)if(Range.edge==E.edge&&Pair.first!=Origin&&Pair.first!=Destination)Blocks.insert(Pair.first);
   D.routes.emplace(RouteId(Id),RouteDefinition{RouteId(Id),TrainId(Train),From,To,P,Blocks,Points,Origin,Destination,SignalId(Id)});
  };
  std::vector<DirectedEdge> AI;for(size_t I=4;I<=10;++I)AI.push_back({TambaramRoute::EdgeFor(1,I),1});
  Add(1,1,AI,{AI.front().edge,G.Find(AI.front().edge)->lengthMetres,1},{AI.back().edge,G.Find(AI.back().edge)->lengthMetres-10,1},B(1,4),B(1,10),{{PointId(1),0}});
  auto P=PlayerPath("IR-S9-MSTBM-LIVE-1");
  std::vector<DirectedEdge> Throat(P.begin(),P.begin()+16),Onward(P.begin()+15,P.end());
  Add(2,2,Throat,{P.front().edge,250,1},{Throat.back().edge,500,1},BlockId(3),B(0,14),{{PointId(1),1},{PointId(2),1}});
  Add(3,2,Onward,{Onward.front().edge,500,1},{Onward.back().edge,G.Find(Onward.back().edge)->lengthMetres-10,1},B(0,14),{},{});
  for(uint64_t Id=3;Id<=5;++Id){std::vector<DirectedEdge> Path;const auto T=TrafficTrack(Id),Start=TrafficStart(Id);
   for(size_t I=Start+1;I-->14;)Path.push_back({TambaramRoute::EdgeFor(T,I),-1});
   Add(Id+1,Id,Path,{Path.front().edge,G.Find(Path.front().edge)->lengthMetres-250,-1},{Path.back().edge,10,-1},B(T,Start),B(T,14),{});
  }
  for(const auto& Route:D.routes)D.signals.emplace(Route.second.signal,SignalDefinition{Route.second.signal,Route.second.origin,Route.first});return D;
 }
 static OperationsDefinitions OperationsFor(const std::string& Id){
  if(IsLiveJourney(Id))return LiveOperations();
  if(IsTambaram(Id)){
   OperationsDefinitions D;const Graph G=TambaramRoute::MakeGraph();
   // Per-edge blocks keep the short throat release independent of onward track.
   for(size_t T=0;T<4;++T)for(size_t I=0;I<TambaramRoute::SegmentCount;++I){
    const auto E=TambaramRoute::EdgeFor(T,I);const double L=G.Find(E)->lengthMetres;
    if(T>=2||(T==0&&(I==0||I==11||I==15))||(T==1&&I==15)){
     const size_t Last=T==0?(I==0?5:I==11?13:TambaramRoute::SegmentCount-2):TambaramRoute::SegmentCount-1;std::vector<TrackRange> Coverage;
     for(size_t J=I;J<=Last;++J){const auto Edge=TambaramRoute::EdgeFor(T,J);Coverage.push_back({Edge,0,G.Find(Edge)->lengthMetres});}
     D.blocks.emplace(BlockId(10000+E.value),BlockDefinition{BlockId(10000+E.value),std::move(Coverage),{},{}});I=Last;continue;
    }
    if(T==1&&I==0){D.blocks.emplace(BlockId(3),BlockDefinition{BlockId(3),{{E,0,250}},{},TrainId(2)});D.blocks.emplace(BlockId(10000+E.value),BlockDefinition{BlockId(10000+E.value),{{E,250,L}},{},{}});}
    else {const uint64_t Owner=(T==1&&I==4)||(T==0&&I==10)?1:(T==1&&I==14)?2:(T==0&&(I==14||I+1==TambaramRoute::SegmentCount))?3:0;
     D.blocks.emplace(BlockId(10000+E.value),BlockDefinition{BlockId(10000+E.value),{{E,0,L}},{},TrainId(Owner),T==1&&I==4});}
   }
   auto Block=[](size_t T,size_t I){return BlockId(10000+TambaramRoute::EdgeFor(T,I).value);};
   for(uint64_t E:{90000ull,90001ull})D.blocks.emplace(BlockId(E),BlockDefinition{BlockId(E),{{EdgeId(E),0,G.Find(EdgeId(E))->lengthMetres}},{},{}});
   // Whole turnout area is mutually fouling, including both plain running rails.
   const std::set<BlockId> Fouling{Block(1,5),Block(1,6),Block(0,6),Block(0,7),BlockId(90000),BlockId(90001)};
   for(auto A:Fouling)for(auto B:Fouling)if(A!=B)D.blocks.at(A).conflicts.insert(B);
   D.points.emplace(PointId(1),PointDefinition{PointId(1),{{0,{TambaramRoute::TransitionFor(1,5,false)}},{1,{TransitionId(90000)}}},Fouling});
   D.points.emplace(PointId(2),PointDefinition{PointId(2),{{0,{TambaramRoute::TransitionFor(0,6,false)}},{1,{TransitionId(90004)}}},Fouling});
   std::vector<DirectedEdge> AI{{TambaramRoute::EdgeFor(1,4),1},{TambaramRoute::EdgeFor(1,5),1},{EdgeId(90000),1},{EdgeId(90001),1}};
   for(size_t I=7;I<=10;++I)AI.push_back({TambaramRoute::EdgeFor(0,I),1});
   std::set<BlockId> AIBlocks{Block(1,5),BlockId(90000),BlockId(90001),Block(0,7),Block(0,8),Block(0,9)};
   D.routes.emplace(RouteId(1),RouteDefinition{RouteId(1),TrainId(1),{AI.front().edge,G.Find(AI.front().edge)->lengthMetres,1},{AI.back().edge,G.Find(AI.back().edge)->lengthMetres-10,1},AI,AIBlocks,{{PointId(1),1},{PointId(2),1}},Block(1,4),Block(0,10),SignalId(1)});
   auto Player=TambaramRoute::Path(1);std::vector<DirectedEdge> Throat(Player.begin(),Player.begin()+15),Onward(Player.begin()+14,Player.end());
   std::set<BlockId> ThroatBlocks,OnwardBlocks{Block(1,15)};for(size_t I=0;I<14;++I)ThroatBlocks.insert(Block(1,I));
   D.routes.emplace(RouteId(2),RouteDefinition{RouteId(2),TrainId(2),{Player.front().edge,250,1},{Throat.back().edge,500,1},Throat,ThroatBlocks,{{PointId(1),0}},BlockId(3),Block(1,14),SignalId(2)});
   D.routes.emplace(RouteId(3),RouteDefinition{RouteId(3),TrainId(2),{Onward.front().edge,500,1},{Onward.back().edge,G.Find(Onward.back().edge)->lengthMetres-10,1},Onward,OnwardBlocks,{},Block(1,14),{},SignalId(3)});
   // Representative northbound service on the actual reverse-directed adjacent
   // graph. It stops wholly in segment 14, clear of the shared Egmore throat.
   // No teleport, duplicate track geometry or privileged occupancy bypass.
   std::vector<DirectedEdge> Opposing;
   for(size_t I=TambaramRoute::SegmentCount;I-->14;)Opposing.push_back({TambaramRoute::EdgeFor(0,I),-1});
   const auto South=Opposing.front().edge,North=Opposing.back().edge;
   D.routes.emplace(RouteId(4),RouteDefinition{RouteId(4),TrainId(3),{South,G.Find(South)->lengthMetres-250,-1},{North,10,-1},Opposing,{Block(0,15)},{},Block(0,TambaramRoute::SegmentCount-1),Block(0,14),SignalId(4)});
   for(const auto& Pair:D.routes)D.signals.emplace(Pair.second.signal,SignalDefinition{Pair.second.signal,Pair.second.origin,Pair.first});
   return D;
  }
  if(!IsEgmore(Id))return MakeOperationsDefinitions();
  OperationsDefinitions D;
  auto B=[&](uint64_t Id,uint64_t Edge,double From,double To,uint64_t Train=0){D.blocks.emplace(BlockId(Id),BlockDefinition{BlockId(Id),{{EdgeId(Edge),From,To}},{},TrainId(Train)});};
  B(1,1,0,250,1);B(2,1,250,900);B(5,1,900,1200,1);
  B(3,2,0,600,2);B(4,2,600,1200);B(6,3,0,1200);B(7,4,0,1200);
  D.routes.emplace(RouteId(1),RouteDefinition{RouteId(1),TrainId(1),{EdgeId(1),250,1},{EdgeId(1),1190,1},{{EdgeId(1),1}},{BlockId(2)},{},BlockId(1),BlockId(5),SignalId(1)});
  D.routes.emplace(RouteId(2),RouteDefinition{RouteId(2),TrainId(2),{EdgeId(2),600,1},{EdgeId(2),1190,1},{{EdgeId(2),1}},{BlockId(4)},{},BlockId(3),{},SignalId(2)});
  for(const auto& Pair:D.routes)D.signals.emplace(Pair.second.signal,SignalDefinition{Pair.second.signal,Pair.second.origin,Pair.first});
  return D;
 }
};
}
