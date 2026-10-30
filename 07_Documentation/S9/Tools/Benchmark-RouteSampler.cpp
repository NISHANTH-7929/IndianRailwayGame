#include "RailTambaramRoute.h"
#include "RailRouteSampling.h"
#include <chrono>
#include <iostream>
int main(){
 const auto graph=Rail::TambaramRoute::MakeGraph();
 volatile double checksum=0;
 std::cout<<"{\"scope\":\"CPU route sampling microbenchmark; not rendered frame timing\",\"runs\":[";
 for(int run=0;run<6;++run){
  const bool cached=run%2;const auto start=std::chrono::steady_clock::now();
  for(int track=0;track<4;++track){
   const auto path=Rail::TambaramRoute::Path(track);const double length=Rail::RouteLength(graph,path);
   for(double cell=0;cell<length;cell+=120){
    const Rail::PreparedRouteSampler sampler(graph,path);
    for(double d=cell;d<std::min(cell+120,length);d+=1.0/12){
     Rail::Sample s;const bool ok=cached?sampler.SampleAt(d,s):Rail::SampleRoute(graph,path,d,s).ok;
     if(!ok)return 1;checksum+=s.position.x+s.position.y+s.tangent.x;
    }
   }
  }
  const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
  if(run)std::cout<<",";std::cout<<"{\"cached\":"<<(cached?"true":"false")<<",\"milliseconds\":"<<ms<<"}";
 }
 std::cout<<"],\"checksum\":"<<checksum<<"}";
}
