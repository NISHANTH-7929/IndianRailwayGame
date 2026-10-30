// Reproduce scene checkpoints through ordinary fixed-step service commands.
// These are isolated visual review fixtures, never player progress records.
#include "RailTambaramService.h"
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
int main(int argc,char** argv){
 if(argc!=2||std::filesystem::exists(argv[1]))return 2;
 const std::filesystem::path root(argv[1]);std::filesystem::create_directories(root);
 const auto d=Rail::MakeLiveTambaramService();Rail::OperationsWorld w;Rail::ServiceRuntime s(d);
 auto r=s.Select();if(r.ok)r=s.ConfirmFormation();if(r.ok)r=s.Start(w);
 if(r.ok)r=s.Input(w,Rail::CommandKind::Brake,0);if(r.ok)r=s.Input(w,Rail::CommandKind::Throttle,1);
 std::set<std::string> captured;
 auto save=[&](const std::string& name){
  if(captured.count(name)||!r.ok)return;
  Rail::SnapshotRecord record;r=s.Capture(w,1,record);if(!r.ok)return;
  Rail::OperationsWorld cold;Rail::ServiceRuntime service(d);
  r=Rail::ServiceRuntime::Restore(record,d,cold,service);
  if(!r.ok||cold.Json()!=w.Json()||service.Json()!=s.Json()){r=Rail::Result::Fail("Fixture recovery differs");return;}
  const auto dir=root/(name+"-isolated-saves");std::filesystem::create_directories(dir);
  const auto& bytes=record.Bytes();std::ofstream out(dir/"checkpoint-0.irs",std::ios::binary);out.write(reinterpret_cast<const char*>(bytes.data()),bytes.size());
  std::ofstream(root/(name+"-logical.json"))<<w.Json();
  std::cout<<name<<" tick="<<w.Current().tick<<" head="<<w.Spatial(d.player)->Current().headStationMetres<<"\n";
  captured.insert(name);
 };
 const double distances[]={1200,4300,11400,15600,22300};
 const char* names[]={"urban","riverside","industrial","airport","suburban"};
 for(unsigned i=0;i<90000&&r.ok&&s.Current().phase==Rail::ServicePhase::Running;++i){
  r=s.Update(w,1.0/30);const auto* player=w.Spatial(d.player);if(!player)break;
  const auto& p=player->Current();
  for(int n=0;n<5;++n)if(p.headStationMetres>=distances[n])save(names[n]);
  if(p.head.edge==Rail::EdgeId(90001))save("crossover");
  for(unsigned n=3;n<=5;++n){
   const auto* other=w.Spatial(Rail::TrainId(n));if(!other)continue;
   const auto delta=other->Current().vehicles.front().bodyCentre-p.vehicles.front().bodyCentre;
   if(delta.Length()<35)save("opposing-"+std::to_string(n));
  }
  if(captured.size()==9)break;
 }
 std::ofstream(root/"fixture-report.json")<<"{\"passed\":"<<(r.ok&&captured.size()==9?"true":"false")<<",\"fixtures\":"<<captured.size()<<",\"fixedStepServiceSimulation\":true,\"liveJourneyQualification\":false}";
 if(!r.ok)std::cerr<<r.error;
 return r.ok&&captured.size()==9?0:1;
}
