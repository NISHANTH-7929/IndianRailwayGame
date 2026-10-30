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
// Presentation only. Air flows follow the simulated brake-cylinder rate (no fabricated pressure);
// the train's continuous sound itself is synthesised by LocoSoundSynth (RailLocoSound.h).
struct ServiceAudioMix {
    double applyFlow = 0, releaseFlow = 0, ventFlow = 0; // A9 service exhaust, distributor release, emergency vent
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
    double applyFlow = 0, releaseFlow = 0, ventFlow = 0;
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
    ServiceAudioMix Update(const LongitudinalState& h, double dt) {
        dt = std::isfinite(dt) ? std::clamp(dt, 0.0, .1) : 0;
        const auto unit = [](double x) { return std::isfinite(x) ? std::clamp(x, 0.0, 1.0) : 0.0; };
        const double brake = unit(h.appliedBrake);
        const bool first = !initialized;
        if (first) {
            initialized = true;
            previousBrake = trendStartBrake = brake;
            // Recovery/resume reconstructs state; never replay an old emergency.
            emergencyPlayed = h.command.emergency;
        }
        ServiceAudioMix result;
        if (!first && dt > 0) {
            eventCooldown = std::max(0.0, eventCooldown - dt);
            const double change = brake - previousBrake;
            // Exhaust flow follows the cylinder: normalised to a full-service fill (~4 s) and the
            // 18 s graduated release; the emergency vent decays as the brake pipe empties (~2.5 s).
            const double alpha = 1 - std::exp(-dt / .1), rate = change / dt;
            applyFlow += ((h.command.emergency ? 0 : unit(rate / .25)) - applyFlow) * alpha;
            releaseFlow += (unit(-rate * 18) - releaseFlow) * alpha;
            ventFlow = h.command.emergency ? (emergencyPlayed ? ventFlow * std::exp(-dt / 2.5) : 1.0) : 0.0;
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
        result.applyFlow = applyFlow;
        result.releaseFlow = releaseFlow * (.3 + .7 * brake);
        result.ventFlow = ventFlow;
        return result;
    }
};
}
