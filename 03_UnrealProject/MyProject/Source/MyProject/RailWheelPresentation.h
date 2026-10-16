#pragma once
#include "CoreMinimal.h"
namespace RailWheelPresentation {
// X forward, axle +Y, Z up: positive rotation makes the bottom tread move -X.
// Absolute route distance preserves phase through pause, reload and reverse.
inline double Angle(double DistanceMetres,double RadiusMetres){return FMath::Fmod(DistanceMetres/RadiusMetres,2*PI);}
}
