#pragma once
#include <cmath>
#include <algorithm>

// Kick Duck envelope maths — identical to the Max for Live version.
// Returns the bass "envelope" (0..1) at position x (0..1) inside one beat.
namespace kickduck
{
constexpr double kPi = 3.14159265358979323846;
constexpr double kFade = 0.012; // 12 ms curved end of every bass cycle (no click)

inline double ease (double u) { return 0.5 - 0.5 * std::cos (kPi * u); }
// k < 1: fast rise then flat (convex), k > 1: slow start (concave). Finite slopes.
inline double bend (double u, double k) { return u / (u + k * (1.0 - u)); }

struct ModeDef
{
    const char* name;
    const char* sub;
    double floorLevel, hold, rise;
    bool beatRelative;
    double k;
};

inline const ModeDef& mode (int i)
{
    static const ModeDef modes[4] = {
        { "TIGHT",  "ROLLING BASS",  0.0, 0.010, 0.070, false, 0.8 },
        { "PUMP",   "SIDECHAIN",     0.0, 0.0,   0.55,  true,  4.0 },
        { "SMOOTH", "PADS / CHORDS", 0.5, 0.010, 0.85,  true,  1.6 },
        { "CUT",    "HARD TECHNO",   0.0, -1.0,  0.012, false, 1.0 } // hold -1 = a 1/16
    };
    return modes[std::clamp (i, 0, 3)];
}

struct Shape
{
    int modeIndex = 1;
    double length = 50.0;  // 0..100, 50 = x1
    double curve = -100.0; // -100..100
};

inline double envelope (double x, double beatSeconds, const Shape& s)
{
    const auto& m = mode (s.modeIndex);
    const double f = std::pow (2.0, (s.length - 50.0) / 25.0);   // LENGTH: 0.25x .. 4x
    double k = m.k * std::pow (2.0, -s.curve / 30.0);              // CURVE
    double hold = m.hold < 0 ? beatSeconds / 4.0 - 0.006 : m.hold;
    double rise = m.beatRelative ? m.rise * beatSeconds : m.rise;
    if (m.hold < 0) hold *= f;
    else { hold *= f; rise *= f; }

    const double room = beatSeconds - kFade - 0.002;
    if (hold + rise > room) { const double sc = room / (hold + rise); hold *= sc; rise *= sc; }

    // never steeper than ~15 % gain per ms -> no clicks at any setting
    const double kmax = std::max (1.0, 0.15 * rise * 1000.0);
    k = std::clamp (k, 1.0 / kmax, kmax);

    auto shapeAt = [&] (double t) {
        if (t < hold) return m.floorLevel;
        if (t < hold + rise) return m.floorLevel + (1.0 - m.floorLevel) * bend ((t - hold) / rise, k);
        return 1.0;
    };

    const double t = x * beatSeconds, fs = beatSeconds - kFade;
    if (t < fs) return shapeAt (t);
    const double from = shapeAt (fs);
    return from + (m.floorLevel - from) * ease ((t - fs) / kFade);
}

// final gain applied to the audio
inline double gain (double x, double beatSeconds, const Shape& s, double mix01)
{
    return 1.0 - mix01 * (1.0 - envelope (x, beatSeconds, s));
}
} // namespace kickduck
