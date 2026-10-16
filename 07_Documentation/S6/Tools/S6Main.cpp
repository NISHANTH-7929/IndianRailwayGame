#include "RailOperations.h"
#include "RailCheckpointStore.h"
#include <fstream>
#include <iostream>
int main(){
 const auto store=Rail::RunCheckpointTests(); std::ofstream("checkpoint-tests.json")<<store.Json(); const auto world=Rail::RunWorldSnapshotTests(); std::ofstream("world-snapshot-tests.json")<<world.Json(); const auto content=Rail::RunContentTests(); const auto record=Rail::RunSnapshotRecordTests(); std::ofstream("snapshot-record-tests.json")<<record.Json();
 const auto s2=Rail::RunKernelTests(),s3=Rail::RunHandlingTests(),s4=Rail::RunOperationsTests(),s5=Rail::RunAITests();
 std::cout<<content.Json()<<std::endl;
 std::ofstream("s2-regression.json")<<s2.Json();
 std::ofstream("s3-regression.json")<<s3.Json();
 std::ofstream("s4-regression.json")<<s4.Json();
 std::ofstream("s5-regression.json")<<s5.Json();
 return store.Passed()&&world.Passed()&&record.Passed()&&content.Passed()&&s2.Passed()&&s3.Passed()&&s4.Passed()&&s5.Passed()?0:1;
}



