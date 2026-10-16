#pragma once
#include "RailFormationContent.h"
#include "RailRouteSampling.h"
#include "RailTambaramAlignment.generated.h"
#include <array>
#include <cmath>
#include <string>
#include <vector>
namespace Rail {
// Source-backed representative corridor; see alignment-audit.json limitations.
struct TambaramRoute {
 static constexpr const char* ContentId="IR-S8-MSTBM-6";
 static constexpr const char* OriginStationId="Station.ChennaiEgmore.01";
 static constexpr const char* DestinationStationId="Station.Tambaram.01";
 static constexpr unsigned Version=6;
 static constexpr size_t StationCount=15;
 static constexpr size_t SegmentCount=TambaramAlignment::Pieces.size();
 static constexpr std::array<const char*,StationCount> StationNames{{
  "Chennai Egmore","Chennai Chetpet","Nungambakkam","Kodambakkam","Mambalam",
  "Saidapet","Guindy","St. Thomas Mount","Palavanthangal","Meenambakkam",
  "Tirusulam","Pallavaram","Chrompet","Tambaram Sanatorium","Tambaram"}};
 static constexpr auto StationMetres=TambaramAlignment::Stations;
 // Established Egmore art cross-section, not separately surveyed track centres.
 static constexpr std::array<double,4> TrackOffsets{{5.0,0.0,16.35,21.35}};
 static constexpr double StartHead=200.0;
 static constexpr double TambaramPlatformFrom=StationMetres.back()-250.0;
 static constexpr double TambaramPlatformTo=StationMetres.back()+90.0;
 static constexpr double TambaramBerthHead=StationMetres.back();
 static constexpr double FinishHead=TambaramAlignment::Length-10.0;
 static EdgeId EdgeFor(size_t Track,size_t Segment){
  if(Segment==0&&Track<2)return EdgeId(Track+1);
  return EdgeId(1000+Track*1000+Segment);
 }
 static NodeId NodeFor(size_t Track,size_t Index){return NodeId(1+Track*(SegmentCount+1)+Index);}
 static TransitionId TransitionFor(size_t Track,size_t Seam,bool Reverse){return TransitionId(1+Track*2*(SegmentCount-1)+2*Seam+(Reverse?1:0));}
 static constexpr size_t CrossoverSegment=6;
 // Presentation window approximation only; authoritative placement uses history.
 static double TrafficStationOffset(){double S=0;for(size_t I=0;I<4;++I)S+=GeometryFor(1,I).lengthMetres;return S;}
 static Geometry CrossoverGeometry(bool Second){
  const auto Base=GeometryFor(1,CrossoverSegment);const double X=Base.lengthMetres,Y=TrackOffsets[0];
  const double Radius=(X*X+Y*Y)/(4*Y),Angle=2*std::atan2(Y,X);
  Geometry A{Base.origin,Base.yawRadians,1/Radius,0,Radius*Angle};
  if(!Second)return A;const auto End=A.At(A.lengthMetres);
  return {End.position,Base.yawRadians+Angle,-1/Radius,0,Radius*Angle};
 }
 static Geometry GeometryFor(size_t Track,size_t Segment){
  const auto& P=TambaramAlignment::Pieces[Segment];const double Offset=TrackOffsets[Track];
  // Exact parallel circular arcs have different radii AND true lengths.
  const double Scale=1.0-P.curvature*Offset;
  return {{P.x-std::sin(P.yaw)*Offset,P.y+std::cos(P.yaw)*Offset,0},P.yaw,P.curvature/Scale,0,P.length*Scale};
 }
 static Graph MakeGraph(){
  Graph G;
  for(size_t T=0;T<TrackOffsets.size();++T){
   for(size_t I=0;I<SegmentCount;++I){const auto A=GeometryFor(T,I);
    G.nodes.push_back({NodeFor(T,I),A.origin});
    G.edges.push_back({EdgeFor(T,I),NodeFor(T,I),NodeFor(T,I+1),A.lengthMetres,A,true,true});
   }
   const auto Last=GeometryFor(T,SegmentCount-1);G.nodes.push_back({NodeFor(T,SegmentCount),Last.At(Last.lengthMetres).position});
   for(size_t I=0;I+1<SegmentCount;++I){const DirectedEdge A{EdgeFor(T,I),1},B{EdgeFor(T,I+1),1};
    G.transitions.push_back({TransitionFor(T,I,false),A,B});G.transitions.push_back({TransitionFor(T,I,true),B.Reversed(),A.Reversed()});}
  }
  const auto A=CrossoverGeometry(false),B=CrossoverGeometry(true);
  G.nodes.push_back({NodeId(90000),B.origin});
  G.edges.push_back({EdgeId(90000),NodeFor(1,6),NodeId(90000),A.lengthMetres,A,true,true});
  G.edges.push_back({EdgeId(90001),NodeId(90000),NodeFor(0,7),B.lengthMetres,B,true,true});
  auto Connect=[&](uint64_t Id,DirectedEdge From,DirectedEdge To){G.transitions.push_back({TransitionId(Id),From,To});G.transitions.push_back({TransitionId(Id+1),To.Reversed(),From.Reversed()});};
  Connect(90000,{EdgeFor(1,5),1},{EdgeId(90000),1});
  Connect(90002,{EdgeId(90000),1},{EdgeId(90001),1});
  Connect(90004,{EdgeId(90001),1},{EdgeFor(0,7),1});
  return G;
 }
 static std::vector<DirectedEdge> Path(size_t Track){std::vector<DirectedEdge> P;for(size_t I=0;I<SegmentCount;++I)P.push_back({EdgeFor(Track,I),1});return P;}
 static double RouteLength(size_t Track){double L=0;for(size_t I=0;I<SegmentCount;++I)L+=GeometryFor(Track,I).lengthMetres;return L;}
 static Result SampleAt(const Graph& G,size_t Track,double Station,Sample& Out){return Rail::SampleRoute(G,Path(Track),Station,Out);}
 static Sample SampleRoute(size_t Track,double Station){
  for(size_t I=0;I<SegmentCount;++I){auto A=GeometryFor(Track,I);if(Station<=A.lengthMetres||I+1==SegmentCount)return A.At(Station);Station-=A.lengthMetres;}return {};
 }
 static TrackPosition TrackPositionAt(size_t Track,double Station){for(size_t I=0;I<SegmentCount;++I){auto A=GeometryFor(Track,I);if(Station<=A.lengthMetres||I+1==SegmentCount)return {EdgeFor(Track,I),Station,1};Station-=A.lengthMetres;}return {};}
 static Result ResolveSpatial(Fixture& F,Formation& Player,Formation& Traffic){Formation Short,Long;auto R=FormationContent::Resolve(FormationContent::LhbId,F,Short,Long);if(!R.ok)return R;F.definitions.at(DefinitionId(FormationContent::Wap7DefinitionId)).massKg=123000;F.definitions.at(DefinitionId(FormationContent::Wap7DefinitionId)).version=2;F.graph=MakeGraph();Player=Long;Traffic=Short;return F.graph.Validate();}
};
}
