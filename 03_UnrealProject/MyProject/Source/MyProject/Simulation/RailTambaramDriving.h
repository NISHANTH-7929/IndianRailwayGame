#pragma once
#include "RailHandling.h"
#include "RailTambaramRoute.h"
#include <algorithm>
#include <cmath>
namespace Rail {
// Scenario calibration, separate from preserved early-stage fixture handling.
struct TambaramDriving {
 static HandlingDefinitions Definitions(){
  auto H=MakeHandlingDefinitions();
  auto L=H.at(DefinitionId(1));L.id=DefinitionId(FormationContent::Wap7DefinitionId);L.version=2;
  L.maxTractionN=323000;L.maxPowerWatts=4500000;L.tractionCutoffMps=140.0/3.6;
  L.adhesionCoefficient=.28;L.tractionResponseSeconds=2;
  L.serviceBrakeNPerKg=.65;L.emergencyBrakeNPerKg=1.0;
  L.brakeApplySeconds=4;L.brakeReleaseSeconds=18;L.emergencyApplySeconds=3;
  L.rollingNPerKg=.015;L.aeroNPerMpsSquared=25;
  auto C=L;C.id=DefinitionId(FormationContent::LhbDefinitionId);C.maxTractionN=0;C.maxPowerWatts=0;C.aeroNPerMpsSquared=2.5;
  H.emplace(L.id,L);H.emplace(C.id,C);return H;
 }
 static double LocalLimit(double D,double Ceiling){
  double Limit=Ceiling;
  // Representative terminal/throat limits; no assertion of current TSRs.
  if(D<1200)Limit=std::min(Limit,30.0/3.6);
  if(D>TambaramRoute::TambaramBerthHead-900)Limit=std::min(Limit,40.0/3.6);
  double Begin=0;
  for(size_t I=0;I<TambaramRoute::SegmentCount;++I){const auto G=TambaramRoute::GeometryFor(1,I);
   if(D<Begin+G.lengthMetres){if(std::abs(G.curvaturePerMetre)>1e-9)Limit=std::min(Limit,std::sqrt(.65/std::abs(G.curvaturePerMetre)));break;}Begin+=G.lengthMetres;}
  return Limit;
 }
 static double PermittedSpeed(double Head,double Length,double Ceiling){
  double Limit=Ceiling;
  // Keep a restriction until the complete train clears it, and brake before
  // future restrictions with a 6s response margin and .35m/s² design decel.
  auto Restriction=[&](double From,double To,double Target){
   if(To<Head-Length||From>Head+2500)return;
   const double Gap=std::max(0.0,From-Head-25),A=.35,Lag=6;
   const double Approach=std::max(Target,-A*Lag+std::sqrt(A*A*Lag*Lag+Target*Target+2*A*Gap));
   Limit=std::min(Limit,Approach);
  };
  Restriction(0,1200,30.0/3.6);Restriction(TambaramRoute::TambaramBerthHead-900,TambaramRoute::FinishHead,40.0/3.6);
  double Begin=0;for(size_t I=0;I<TambaramRoute::SegmentCount;++I){const auto G=TambaramRoute::GeometryFor(1,I);
   if(std::abs(G.curvaturePerMetre)>1e-9)Restriction(Begin,Begin+G.lengthMetres,std::sqrt(.65/std::abs(G.curvaturePerMetre)));
   Begin+=G.lengthMetres;}
  return Limit;
 }
 static double AuthorityStoppingDistance(double Speed){return Speed*Speed/(2*.35)+std::abs(Speed)*6+4;}
 static double ApplicableLimit(double Head,double Length,double Ceiling){
  double Limit=Ceiling;auto Restrict=[&](double From,double To,double V){if(To>=Head-Length&&From<=Head)Limit=std::min(Limit,V);};
  Restrict(0,1200,30.0/3.6);Restrict(TambaramRoute::TambaramBerthHead-900,TambaramRoute::FinishHead,40.0/3.6);
  double Begin=0;for(size_t I=0;I<TambaramRoute::SegmentCount;++I){const auto G=TambaramRoute::GeometryFor(1,I);if(std::abs(G.curvaturePerMetre)>1e-9)Restrict(Begin,Begin+G.lengthMetres,std::sqrt(.65/std::abs(G.curvaturePerMetre)));Begin+=G.lengthMetres;}return Limit;
 }
};
}
