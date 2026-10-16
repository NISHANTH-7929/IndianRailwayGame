#include "RailKernel.h"
#include <iostream>
int main(){const auto report=Rail::RunKernelTests();std::cout<<report.Json()<<std::endl;return report.Passed()?0:1;}
