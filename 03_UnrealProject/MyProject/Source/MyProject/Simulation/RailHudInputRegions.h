#pragma once
#include <array>
#include <cmath>

namespace Rail {
// Shared viewport fractions for the driving HUD and cab pointer arbitration.
namespace HudInputRegions {
constexpr double LeftEdge = .18;
constexpr double RightEdge = .82;
constexpr double EdgeTop = .24;
constexpr double TopEdge = .18;
constexpr double BottomEdge = .80;
constexpr int TouchCount = 10;
constexpr int MousePointer = TouchCount;
constexpr int PointerCount = TouchCount + 1;

inline bool Captures(double X, double Y, bool DrivingHud, bool BlockAll = false) {
    if (BlockAll || !std::isfinite(X) || !std::isfinite(Y) || X < 0 || X > 1 || Y < 0 || Y > 1) return true;
    if (!DrivingHud) return Y >= .86;
    return Y <= TopEdge || Y >= BottomEdge || (Y >= EdgeTop && (X <= LeftEdge || X >= RightEdge));
}

struct PointerSample {
    double X = 0, Y = 0;
    bool Down = false;
};

struct Gesture {
    bool Pan = false, Tap = false;
    double DeltaX = 0, DeltaY = 0, TapX = 0, TapY = 0;
};

// A contact is admitted once, on its initial press. HUD contacts never become
// camera contacts when dragged into the centre, and a second finger cannot
// steal the first camera gesture. Mouse is separate from emulated touch input.
class PointerRouter {
public:
    using Samples = std::array<PointerSample, PointerCount>;

    void Cancel() {
        Active = -1;
        WaitForRelease = true;
    }

    Gesture Update(const Samples& SamplesNow, double Width, double Height, bool AllowCab, bool DrivingHud, bool BlockAll = false) {
        Gesture Result;
        bool AnyTouch = false, AnyDown = false;
        for (int I = 0; I < PointerCount; ++I) {
            AnyDown = AnyDown || SamplesNow[I].Down;
            if (I < TouchCount) AnyTouch = AnyTouch || SamplesNow[I].Down;
        }
        if (WaitForRelease) {
            for (int I = 0; I < PointerCount; ++I) Pointers[I].Down = SamplesNow[I].Down;
            if (!AnyDown) { WaitForRelease = false; Pointers = {}; }
            return Result;
        }
        const bool Enabled = AllowCab && !BlockAll && Width > 0 && Height > 0;
        for (int I = 0; I < PointerCount; ++I) {
            const auto& Sample = SamplesNow[I];
            auto& Pointer = Pointers[I];
            const bool MouseSuppressed = I == MousePointer && AnyTouch;
            const bool Valid = std::isfinite(Sample.X) && std::isfinite(Sample.Y) && Sample.X >= 0 && Sample.X <= Width && Sample.Y >= 0 && Sample.Y <= Height;
            if (!Sample.Down) {
                if (Pointer.Down && Active == I) {
                    if (Enabled && !MouseSuppressed && !Pointer.Dragged) {
                        Result.Tap = true; Result.TapX = Pointer.StartX; Result.TapY = Pointer.StartY;
                    }
                    Active = -1;
                }
                Pointer = {};
                continue;
            }
            if (!Pointer.Down) {
                Pointer.Down = true; Pointer.Dragged = false;
                Pointer.StartX = Pointer.PreviousX = Sample.X;
                Pointer.StartY = Pointer.PreviousY = Sample.Y;
                if (Enabled && Valid && !MouseSuppressed && Active == -1 && !Captures(Sample.X / Width, Sample.Y / Height, DrivingHud)) Active = I;
                continue;
            }
            if (Active != I) continue;
            if (!Enabled || !Valid || MouseSuppressed) { Active = -1; continue; }
            const double FromStartX = Sample.X - Pointer.StartX, FromStartY = Sample.Y - Pointer.StartY;
            if (FromStartX * FromStartX + FromStartY * FromStartY > 14 * 14) Pointer.Dragged = true;
            if (Pointer.Dragged) {
                Result.Pan = true;
                Result.DeltaX += Sample.X - Pointer.PreviousX;
                Result.DeltaY += Sample.Y - Pointer.PreviousY;
            }
            Pointer.PreviousX = Sample.X; Pointer.PreviousY = Sample.Y;
        }
        return Result;
    }

private:
    struct PointerState {
        bool Down = false, Dragged = false;
        double StartX = 0, StartY = 0, PreviousX = 0, PreviousY = 0;
    };
    std::array<PointerState, PointerCount> Pointers{};
    int Active = -1;
    bool WaitForRelease = false;
};
}
}
