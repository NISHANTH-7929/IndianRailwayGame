#include "RailTambaramService.h"
#include <iostream>
int main(){const auto R=Rail::RunTambaramJourneyTests();std::cout<<R.Json()<<std::endl;return R.Passed()?0:1;}
