#include "RailOperations.h"
#include <fstream>
#include <iostream>
int main(){const auto r=Rail::RunAITests();std::cout<<r.Json()<<std::endl;
std::ofstream("s2-regression.json")<<Rail::RunKernelTests().Json();
std::ofstream("s3-regression.json")<<Rail::RunHandlingTests().Json();
std::ofstream("s4-regression.json")<<Rail::RunOperationsTests().Json();return r.Passed()?0:1;}
