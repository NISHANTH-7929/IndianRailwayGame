#pragma once
#include "RailKernel.h"
#include <cmath>

namespace Rail {
// Resolve authored path distance, never nearest-spline/world-axis projection.
inline Result PositionOnRoute(const Graph& graph,const std::vector<DirectedEdge>& path,
                              double station,TrackPosition& out){
 if(!std::isfinite(station)||station<0||path.empty())return Result::Fail("Invalid route station");
 for(size_t i=0;i<path.size();++i){
  const auto& directed=path[i];const auto* edge=graph.Find(directed.edge);
  if(!edge||(directed.direction!=1&&directed.direction!=-1)||
     (directed.direction==1?!edge->allowForward:!edge->allowReverse)||
     (i&&!graph.Connection(path[i-1],directed)))return Result::Fail("Invalid ordered route");
  if(station<edge->lengthMetres||(i+1==path.size()&&station<=edge->lengthMetres)){
   out={edge->id,directed.direction==1?station:edge->lengthMetres-station,directed.direction};return {};
  }
  station-=edge->lengthMetres;
 }
 return Result::Fail("Route station outside path");
}
inline Result SampleRoute(const Graph& graph,const std::vector<DirectedEdge>& path,
                          double station,Sample& out){
 TrackPosition p;auto result=PositionOnRoute(graph,path,station,p);
 return result.ok?graph.SamplePosition(p,out):result;
}
inline double RouteLength(const Graph& graph,const std::vector<DirectedEdge>& path){
 double length=0;for(const auto& directed:path){const auto* edge=graph.Find(directed.edge);if(!edge)return -1;length+=edge->lengthMetres;}return length;
}
}
