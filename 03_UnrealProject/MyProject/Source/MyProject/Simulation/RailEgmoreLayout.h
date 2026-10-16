#pragma once
#include "RailFormationContent.h"
#include <array>

namespace Rail {
// Spatial contract admitted through ScenarioContent, never aliased to an old
// formation identity. Unreal service-map binding is a separate presentation step.
struct EgmoreLayout {
 static constexpr const char* ReservedContentId="IR-S8-EGMORE-1";
 static constexpr unsigned Version=1;
 static constexpr double OriginX=-600,EndX=600;
 static constexpr double PlatformFromX=-108,PlatformToX=108;
 static constexpr double StartHeadX=-120,BerthHeadX=100,FinishHeadX=400;
 static constexpr std::array<double,4> TrackY{{0,5,16.35,21.35}};
 static constexpr std::array<uint64_t,4> TrackEdges{{2,1,3,4}};
 static double StationMetres(double WorldX){return WorldX-OriginX;}
 static Graph MakeGraph(){
  Graph G;
  for(size_t I=0;I<TrackY.size();++I){
   const NodeId A(2*I+1),B(2*I+2);
   G.nodes.push_back({A,{OriginX,TrackY[I],0}});G.nodes.push_back({B,{EndX,TrackY[I],0}});
   G.edges.push_back({EdgeId(TrackEdges[I]),A,B,EndX-OriginX,{{OriginX,TrackY[I],0},0,0,0,EndX-OriginX},true,true});
  }
  return G;
 }
 static Result ResolveSpatial(Fixture& FixtureOut,Formation& TrainOut){
  Formation Short,Long;auto R=FormationContent::Resolve(FormationContent::LhbId,FixtureOut,Short,Long);
  if(!R.ok)return R;FixtureOut.graph=MakeGraph();TrainOut=Long;return FixtureOut.graph.Validate();
 }
};
}
