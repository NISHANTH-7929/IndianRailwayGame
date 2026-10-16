#pragma once
#include "Simulation/RailHandling.h"
#include <algorithm>
#include <cmath>

namespace Rail {
// Presentation-only encounter envelope. Distance is listener-to-source in metres.
struct PassingAudioMix { double gain=0, pitch=1; bool horn=false; };
struct PassingAudioDynamics {
    bool initialized=false, hornPlayed=false;
    double previousDistance=0, gain=0, pitch=1;
    void Suspend() { initialized=false;gain=0;pitch=1; } // retain encounter latch
    PassingAudioMix Update(double distance,double dt,bool moving,bool allowHorn=true) {
        if(!std::isfinite(distance)||distance<0||!std::isfinite(dt)||dt<=0) return {};
        dt=std::min(dt,.1);
        const double radial=initialized?(distance-previousDistance)/dt:0;
        const bool continuous=initialized&&std::abs(radial)<120;
        if(distance>1200)hornPlayed=false;
        const bool trigger=allowHorn&&continuous&&moving&&!hornPlayed&&distance<550&&radial< -1;
        if(trigger)hornPlayed=true;
        const double edge=std::clamp((1200-distance)/300.0,0.0,1.0);
        const double target=edge*edge/(1+std::pow(distance/70.0,1.6));
        gain+=(target-gain)*(1-std::exp(-dt*6));
        // Relative radial approximation, bounded to prevent camera cuts becoming chirps.
        const double targetPitch=continuous?std::clamp(343.0/(343.0+radial),.85,1.18):1;
        pitch+=(targetPitch-pitch)*(1-std::exp(-dt*5));
        previousDistance=distance;initialized=true;
        return {gain,pitch,trigger};
    }
};
// Presentation only. No fabricated pneumatic pressure or wheel-slip state.
struct ServiceAudioMix {
    double rail = 0, railPitch = 1, motor = 0, motorPitch = 1, coast = 0, squeal = 0;
    bool brakeApply = false, brakeRelease = false, emergency = false;
};
struct ServiceAudioPerspective {
    double outsideGain = 1, motorGain = 1, hornGain = .65, lowPassHz = 20000;
};
struct ServiceAudioDynamics {
    bool perspectiveInitialized = false;
    double cabBlend = 0;
    bool initialized = false, emergencyPlayed = false, trendPlayed = false;
    int brakeTrend = 0;
    double previousBrake = 0, trendStartBrake = 0, steadySeconds = 0, eventCooldown = 0;
    ServiceAudioMix smoothed;
    void Reset() { *this = ServiceAudioDynamics{}; }
    ServiceAudioPerspective Perspective(bool inCab, double dt) {
        dt = std::isfinite(dt) ? std::clamp(dt, 0.0, .1) : 0;
        const double goal = inCab ? 1.0 : 0.0;
        if (!perspectiveInitialized) { cabBlend = goal; perspectiveInitialized = true; }
        else cabBlend += (goal - cabBlend) * (1 - std::exp(-dt * 5));
        // Closed-cab approximation: soften external high frequencies, retain more
        // structure-borne motor sound, and crossfade camera changes.
        return {1 - .35 * cabBlend, 1 - .20 * cabBlend, .65 - .20 * cabBlend,
            20000 * std::pow(3200.0 / 20000, cabBlend)};
    }
    ServiceAudioMix Update(const LongitudinalState& h, double curvaturePerMetre, double dt) {
        dt = std::isfinite(dt) ? std::clamp(dt, 0.0, .1) : 0;
        const auto unit = [](double x) { return std::isfinite(x) ? std::clamp(x, 0.0, 1.0) : 0.0; };
        const double speed = std::isfinite(h.speedMps) ? std::abs(h.speedMps) : 0;
        const double brake = unit(h.appliedBrake);
        const bool first = !initialized;
        if (first) {
            initialized = true;
            previousBrake = trendStartBrake = brake;
            // Recovery/resume reconstructs state; never replay an old emergency.
            emergencyPlayed = h.command.emergency;
        }
        ServiceAudioMix target;
        const double moving = unit(speed / 1.5);
        const double speedBody = std::sqrt(unit(speed / 30.0));
        const bool powering = !h.command.emergency && h.command.direction != 0 && brake < .01 &&
            std::isfinite(h.tractionN) && std::abs(h.tractionN) > 1;
        const double forceLoad = h.massKg > 0 ? unit(std::abs(h.tractionN) / h.massKg / .6) : 0;
        const double load = powering ? unit(h.appliedThrottle) * (.35 + .65 * forceLoad) : 0;
        target.rail = moving * (.12 + .33 * speedBody);
        target.railPitch = .72 + .68 * unit(speed / 35.0);
        target.motor = load * (.18 + .12 * moving);
        target.motorPitch = .65 + 1.1 * unit(speed / 32.0);
        target.coast = moving * speedBody * (.045 + .08 * (1 - load));
        // Curve contact and brake friction are audible approximations, not simulated slip.
        const double curve = std::isfinite(curvaturePerMetre) ? unit((std::abs(curvaturePerMetre) - .0006) / .006) : 0;
        const double brakeForce = h.massKg > 0 ? unit(std::abs(h.brakingN) / h.massKg) : 0;
        target.squeal = moving * std::max(.12 * curve, .10 * brake * brakeForce);
        const double gainAlpha = 1 - std::exp(-dt * 4), pitchAlpha = 1 - std::exp(-dt * 2);
        auto smooth = [](double& value, double goal, double alpha) { value += (goal - value) * alpha; };
        smooth(smoothed.rail, target.rail, gainAlpha);
        smooth(smoothed.motor, target.motor, gainAlpha);
        smooth(smoothed.coast, target.coast, gainAlpha);
        smooth(smoothed.squeal, target.squeal, gainAlpha);
        smooth(smoothed.railPitch, target.railPitch, pitchAlpha);
        smooth(smoothed.motorPitch, target.motorPitch, pitchAlpha);
        ServiceAudioMix result = smoothed;
        if (!first && dt > 0) {
            eventCooldown = std::max(0.0, eventCooldown - dt);
            const double change = brake - previousBrake;
            // Compare rates so the 18-second release is detected at high FPS.
            const double threshold = .005 * dt;
            const int trend = change > threshold ? 1 : (change < -threshold ? -1 : 0);
            if (trend) {
                steadySeconds = 0;
                if (trend != brakeTrend) { brakeTrend = trend; trendStartBrake = previousBrake; trendPlayed = false; }
                if (!trendPlayed && std::abs(brake - trendStartBrake) >= .025 && eventCooldown <= 0) {
                    result.brakeApply = trend > 0 && !h.command.emergency;
                    result.brakeRelease = trend < 0 && !h.command.emergency;
                    trendPlayed = true;
                    eventCooldown = .3;
                }
            } else {
                steadySeconds += dt;
                if (steadySeconds > .15) brakeTrend = 0;
            }
            if (!h.command.emergency) emergencyPlayed = false;
            if (h.command.emergency && !emergencyPlayed && brake > .01) {
                result.emergency = true;
                result.brakeApply = result.brakeRelease = false;
                emergencyPlayed = true;
            }
        }
        previousBrake = brake;
        return result;
    }
};
}
