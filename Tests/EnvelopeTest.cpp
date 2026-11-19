// Sweeps every shape / knob / tempo combination and checks the envelope never jumps (no clicks).
#include "Envelope.h"
#include <cstdio>

int main()
{
    double worst = 0; int bad = 0;
    for (double bpm : { 80.0, 130.0, 175.0 })
        for (int m = 0; m < 4; ++m)
            for (double L = 0; L <= 100; L += 12.5)
                for (double C = -100; C <= 100; C += 25)
                {
                    kickduck::Shape s { m, L, C };
                    const double bl = 60.0 / bpm; const int n = 16384;
                    double first = kickduck::envelope (0, bl, s), prev = first, mx = 0;
                    for (int i = 1; i <= n; ++i)
                    {
                        const double v = i < n ? kickduck::envelope ((double) i / n, bl, s) : first;
                        if (v < -1e-9 || v > 1 + 1e-9) ++bad;
                        mx = std::max (mx, std::abs (v - prev)); prev = v;
                    }
                    worst = std::max (worst, mx / (bl * 1000.0 / n));
                }
    std::printf ("max gain change: %.1f%% per ms, out-of-range samples: %d\n", worst * 100, bad);
    return (worst <= 0.16 && bad == 0) ? 0 : 1;
}
