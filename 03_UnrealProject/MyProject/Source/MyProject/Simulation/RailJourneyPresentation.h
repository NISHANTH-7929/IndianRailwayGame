#pragma once
#include "RailTambaramRoute.h"
#include <algorithm>
#include <cmath>

namespace Rail {
// Derived route information only: never advances a service or grants authority.
struct JourneyPresentation {
 size_t station=0;
 double distance=0,progress=0;
 bool alongside=false,terminal=false;
 static double StationAmbience(double head){
  double nearest=1e30;
  for(double marker:TambaramRoute::StationMetres)nearest=std::min(nearest,std::abs(head-marker));
  // Continuous spatial fade, including intermediate stations; no repeated cue triggers.
  const double x=std::clamp(1.0-nearest/300.0,0.0,1.0);
  return x*x*(3.0-2.0*x);
 }
 static JourneyPresentation At(double head,double length){
  JourneyPresentation out;
  for(size_t i=0;i<TambaramRoute::StationCount;++i){
   out.station=i;
   // Keep the name while the whole formation passes the station reference.
   if(head-length<=TambaramRoute::StationMetres[i]+90)break;
  }
  const double marker=TambaramRoute::StationMetres[out.station];
  out.distance=marker-head;
  out.alongside=head>=marker-250&&head-length<=marker+90;
  out.terminal=out.station+1==TambaramRoute::StationCount;
  out.progress=std::clamp((head-TambaramRoute::StartHead)/(TambaramRoute::TambaramBerthHead-TambaramRoute::StartHead),0.0,1.0);
  return out;
 }
};
}
