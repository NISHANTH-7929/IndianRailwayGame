#pragma once
#include "RailKernel.h"
#include <cmath>

namespace Rail {
// Immutable presentation snapshot: validate the ordered route once, then reuse
// its geometry for dense scenery placement. Never used for movement authority.
class PreparedRouteSampler {
 struct Segment { Geometry geometry; double length; int direction; };
 std::vector<Segment> segments_;
public:
 PreparedRouteSampler(const Graph& graph,const std::vector<DirectedEdge>& path){
  for(size_t i=0;i<path.size();++i){
   const auto d=path[i];const auto* e=graph.Find(d.edge);
   if(!e||!std::isfinite(e->lengthMetres)||e->lengthMetres<=0||
      (d.direction!=1&&d.direction!=-1)||(d.direction==1?!e->allowForward:!e->allowReverse)||
      (i&&!graph.Connection(path[i-1],d))){segments_.clear();return;}
   segments_.push_back({e->geometry,e->lengthMetres,d.direction});
  }
 }
 bool SampleAt(double station,Sample& out) const {
  if(!std::isfinite(station)||station<0)return false;
  for(size_t i=0;i<segments_.size();++i){
   const auto& s=segments_[i];
   if(station<s.length||(i+1==segments_.size()&&station<=s.length)){
    out=s.geometry.At(s.direction==1?station:s.length-station);
    out.tangent=out.tangent*double(s.direction);return true;
   }
   station-=s.length;
  }
  return false;
 }
};
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
