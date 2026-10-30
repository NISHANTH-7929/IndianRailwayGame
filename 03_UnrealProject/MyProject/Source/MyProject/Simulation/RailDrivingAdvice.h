#pragma once
#include <algorithm>
#include <cmath>

namespace Rail {
// Presentation only. This advisory never submits controls or grants authority.
enum class DrivingAdvice { None, Overspeed, Restriction, BrakeForBerth, PositionAtBerth, HoldDwell };
inline DrivingAdvice StandardDrivingAdvice(double speed, double limit, double approach,
                                           double berthRemaining, double tolerance, bool dwelling) {
 if(!std::isfinite(speed)||!std::isfinite(limit)||!std::isfinite(approach)||
    !std::isfinite(berthRemaining)||!std::isfinite(tolerance)||limit<=0||approach<0||tolerance<=0)
  return DrivingAdvice::None;
 const double v=std::abs(speed);
 // Match the service scoring threshold, in metres per second.
 if(v>limit+.1)return DrivingAdvice::Overspeed;
 if(dwelling)return DrivingAdvice::HoldDwell;
 if(berthRemaining<=10&&berthRemaining>=-tolerance)return DrivingAdvice::PositionAtBerth;
 // Same conservative .35 m/s2 and six-second response allowance as the
 // route restriction envelope. An estimate, not a braking guarantee.
 const double stop=v*v/(2*.35)+v*6+4;
 if(v>.05&&berthRemaining>0&&berthRemaining<=stop)return DrivingAdvice::BrakeForBerth;
 if(approach+.1<limit&&v>approach)return DrivingAdvice::Restriction;
 return DrivingAdvice::None;
}
}
