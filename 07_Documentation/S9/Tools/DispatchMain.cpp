#include "RailDispatchDiagnostics.h"
#include <iostream>
int main(){const auto report=Rail::RunDispatchTests();std::cout<<report.Json()<<std::endl;return report.Passed()?0:1;}
