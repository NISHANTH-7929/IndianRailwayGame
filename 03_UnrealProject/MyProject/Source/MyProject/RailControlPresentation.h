#pragma once
#include "Simulation/RailHandling.h"

// Read-only presentation contract shared by the HUD and cab instruments.
// Service requests are driver intent, not the emergency/assistance-adjusted
// actuators. Never write this derived view back into the simulation.
namespace Rail {
struct ControlPresentation {
 double power,brake;
 bool emergency;
 int direction;
 static ControlPresentation From(const LongitudinalState& H){
  return {H.appliedThrottle,H.appliedBrake,H.command.emergency,H.command.direction};
 }
};
}
