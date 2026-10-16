#include "RailEgmoreService.h"
#include <cmath>
#include <iostream>
using namespace Rail;
int main(){
 unsigned checks=0,failed=0;auto check=[&](bool ok,const char* name){++checks;if(!ok){++failed;std::cout<<"FAIL "<<name<<"\n";}};
 std::array<ServiceDefinition,3> definitions{{ServiceDefinition{},ServiceDefinition{},MakeEgmoreService()}};
 definitions[1].contentId=FormationContent::LhbId;definitions[1].id="S8-Shantipur-LHB-01";
 definitions[1].version="IR-S8-LHB-service-1";definitions[1].berthHead=FormationContent::ServiceBerthHead;
 std::array<SnapshotRecord,3> records;
 for(unsigned i=0;i<3;++i){OperationsWorld w;ServiceRuntime s(definitions[i]);check(s.Select().ok&&s.ConfirmFormation().ok&&s.Start(w).ok&&s.Capture(w,2,records[i]).ok,"initialize records");}
 for(unsigned i=0;i<3;++i)for(unsigned j=0;j<3;++j)if(i!=j){
  OperationsWorld w;w.InitializeFixture();ServiceRuntime s;const auto wb=w.Json(),sb=s.Json();
  check(!ServiceRuntime::Compatible(records[i],definitions[j]),"cross-content compatibility");
  check(!ServiceRuntime::Restore(records[i],definitions[j],w,s).ok&&w.Json()==wb&&s.Json()==sb,"cross-content transactional rejection");
 }
 for(int change=0;change<3;++change){auto d=definitions[2];if(change==0)d.id+="-future";if(change==1)d.version+="-future";if(change==2)d.dwellTicks+=30;
  OperationsWorld w;w.InitializeFixture();ServiceRuntime s;const auto wb=w.Json(),sb=s.Json();
  check(d.Validate().ok&&!ServiceRuntime::Compatible(records[2],d),"same-content header compatibility");
  check(!ServiceRuntime::Restore(records[2],d,w,s).ok&&w.Json()==wb&&s.Json()==sb,"same-content transactional rejection");
 }
 for(unsigned other=0;other<2;++other)for(unsigned order=0;order<2;++order){
  std::array<std::vector<uint8_t>,2> slots;slots[order]=records[2].Bytes();slots[1-order]=records[other].Bytes();const auto before=slots;unsigned writes=0;
  CheckpointIO io{[&](unsigned i){return slots[i];},[&](unsigned i,const std::vector<uint8_t>& b){++writes;slots[i]=b;return Result{};}};
  ServiceRuntime s(definitions[2]);OperationsWorld w;RecoverySelection selected;CheckpointCommit commit;
  check(s.Recover(io,w,selected).ok&&selected.slot==int(order),"mixed recovery selects Egmore");
  check(!s.Save(w,io,commit).ok&&writes==0&&slots==before,"mixed writes preserve incompatible record");
 }
 OperationsWorld w,cold;ServiceRuntime s(definitions[2]),restored(definitions[2]);
 check(s.Select().ok&&s.ConfirmFormation().ok&&s.SetMode(DrivingMode::Standard).ok&&s.Start(w).ok,"standard start");
 check(s.Update(w,1.0/60).ok&&w.Current().tick==0&&std::abs(s.Current().accumulator-1.0/60)<1e-12,"half step retained");
 check(s.Input(w,CommandKind::Brake,0).ok&&s.Input(w,CommandKind::Throttle,.5).ok,"pending controls queued");
 SnapshotRecord a,b;check(s.Capture(w,8,a).ok&&ServiceRuntime::Restore(a,definitions[2],cold,restored).ok&&restored.Capture(cold,8,b).ok&&a.Bytes()==b.Bytes(),"pending half-step exact recovery");
 bool same=true;for(unsigned i=0;i<120&&same;++i){same=s.Update(w,1.0/60).ok&&restored.Update(cold,1.0/60).ok&&s.Capture(w,9,a).ok&&restored.Capture(cold,9,b).ok&&a.Bytes()==b.Bytes();if(i==0)same=same&&w.Current().tick==1&&s.Current().accumulator<1e-12;}
 check(same,"half-step continuation exact");
 std::cout<<"checks="<<checks<<" failed="<<failed<<"\n";return failed?1:0;
}
