#include "RailTambaramService.h"
#include "RailTambaramDriving.h"
#include <iostream>
int main(){
 const auto d=Rail::MakeLiveTambaramService();
 std::cout<<"{\"berth\":"<<std::fixed<<d.berthHead<<",\"step\":10,\"limits\":[";
 for(int h=0;h<=int(d.berthHead)+20;h+=10){if(h)std::cout<<',';
  std::cout<<Rail::TambaramDriving::PermittedSpeed(double(h),189.33,d.speedLimit);}
 std::cout<<"]}";
}
