#include "RailOperations.h"
#include <fstream>
#include <iostream>
int main(){const auto r=Rail::RunOperationsTests();std::cout<<r.Json()<<std::endl;std::ofstream("s2-regression.json")<<Rail::RunKernelTests().Json();std::ofstream("s3-regression.json")<<Rail::RunHandlingTests().Json();return r.Passed()?0:1;}
