#include "RailEgmoreService.h"
#include <iostream>
#include <set>
int main(){
 using namespace Rail;
 const auto d=MakeEgmoreService(); OperationsWorld w; ServiceRuntime s(d);
 auto r=s.Select();if(r.ok)r=s.ConfirmFormation();if(r.ok)r=s.Start(w);
 if(r.ok)r=s.Input(w,CommandKind::Brake,0);if(r.ok)r=s.Input(w,CommandKind::Throttle,1);
 std::set<StationPhase> phases;bool releasedChecked=false,routeSeen=false;bool passed=true;
 for(int i=0;i<21020&&r.ok&&s.Current().phase==ServicePhase::Running;++i){
  if(s.Current().station==StationPhase::ReadyToDepart&&!s.Current().departureAuthorized&&releasedChecked)r=s.Depart(w);
  if(r.ok)r=s.Update(w,1.0/30);
  const auto p=s.Current().station;const bool active=w.Current().routes.at(d.trafficRoute).owner.Active();routeSeen|=active;const bool released=routeSeen&&!active;
  if((p>=StationPhase::Arrival&&phases.insert(p).second)||(released&&!releasedChecked)){
   SnapshotRecord saved;OperationsWorld restored;ServiceRuntime rs(d);
   auto q=s.Capture(w,3,saved);if(q.ok)q=ServiceRuntime::Restore(saved,d,restored,rs);
   const bool ws=q.ok&&restored.Json()==w.Json(),ss=q.ok&&rs.Json()==s.Json();
   std::cout<<"phase="<<StationPhaseName(p)<<" tick="<<s.Current().ticks<<" released="<<released<<" loaded="<<q.ok<<" world="<<ws<<" service="<<ss<<" error="<<q.error<<"\n";
   if(!ws||!ss){passed=false;std::cout<<"WORLD "<<w.Json()<<"\nRESTORED "<<restored.Json()<<"\nSERVICE "<<s.Json()<<"\nRESTORED "<<rs.Json()<<"\n";}
   if(released)releasedChecked=true;
  }
 }
 std::cout<<"success="<<s.Current().success<<" releasedChecked="<<releasedChecked<<" error="<<r.error<<"\n";
 std::cout<<"traffic head="<<w.Spatial(d.traffic)->Current().headStationMetres<<" speed="<<w.Handling(d.traffic)->speedMps<<" ai="<<w.AI(d.traffic)->Json()<<"\n";
 return passed&&r.ok&&s.Current().success&&releasedChecked?0:1;
}
