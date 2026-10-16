#include "RailService.h"
#include <fstream>
#include <iostream>
int main(){
 bool passed=true;
 const Rail::TestReport suites[]={Rail::RunKernelTests(),Rail::RunHandlingTests(),Rail::RunOperationsTests(),Rail::RunAITests(),Rail::RunContentTests(),Rail::RunSnapshotRecordTests(),Rail::RunWorldSnapshotTests(),Rail::RunCheckpointTests(),Rail::RunServiceTests()};
 const char* names[]={"s2-regression","s3-regression","s4-regression","s5-regression","s6-content","s6-record","s6-world","s6-checkpoint","service-tests"};
 for(unsigned i=0;i<9;++i){passed&=suites[i].Passed();std::ofstream(std::string(names[i])+".json")<<suites[i].Json();std::cout<<names[i]<<": "<<suites[i].cases.size()<<" cases "<<(suites[i].Passed()?"PASS":"FAIL")<<'\n';for(const auto& c:suites[i].cases)if(!c.passed)std::cout<<c.name<<" "<<c.detail<<'\n';}
 return passed?0:1;
}
