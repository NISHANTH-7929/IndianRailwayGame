#pragma once
#include <algorithm>
#include <cmath>
namespace Rail::DrivingControl {
// A contact takes hold of the existing handle value. Touching another point in
// the lane cannot instantly command full traction or full service brake.
inline double DragValue(double startValue,double startY,double currentY,double travel){
 if(!std::isfinite(startValue)||!std::isfinite(startY)||!std::isfinite(currentY)||!std::isfinite(travel)||travel<=0)return 0;
 return std::clamp(std::round((startValue+(startY-currentY)/travel)*100.0)/100.0,0.0,1.0);
}
inline double StepValue(double value,int direction){
 return std::clamp(std::round((value+direction*.05)*100.0)/100.0,0.0,1.0);
}
}
