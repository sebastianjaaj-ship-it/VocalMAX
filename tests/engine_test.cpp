// Offline test for the pitch engine (not part of the plugin build).
#include "../Source/PitchEngine.h"
#include <cmath>
#include <cstdio>
#include <vector>

static double measureF0 (const std::vector<float>& x, size_t start, size_t len, double sr)
{
    // simple normalised autocorrelation peak search 70..1000 Hz
    int minLag = (int) (sr / 1000), maxLag = (int) (sr / 70);
    double best = -1; int bestLag = 0; std::vector<double> cs;
    for (int lag = minLag; lag <= maxLag; ++lag)
    {
        double s = 0, e1 = 0, e2 = 0;
        for (size_t i = start; i < start + len; ++i) { s += x[i] * x[i + lag]; e1 += x[i] * x[i]; e2 += x[i + lag] * x[i + lag]; }
        double c = s / std::sqrt (e1 * e2 + 1e-12);
        cs.push_back(c);
        if (c > best) { best = c; }
    }
    for (int lag = minLag; lag <= maxLag; ++lag) if (cs[lag-minLag] > 0.9*best && cs[lag-minLag] >= cs[lag-minLag-1 < 0 ? 0 : lag-minLag-1] && (lag==maxLag || cs[lag-minLag] >= cs[lag-minLag+1])) { bestLag = lag; break; }
    // parabolic refine
    auto ac = [&] (int lag) { double s = 0; for (size_t i = start; i < start + len; ++i) s += x[i] * x[i + lag]; return s; };
    double a = ac (bestLag - 1), b = ac (bestLag), c = ac (bestLag + 1);
    double t = bestLag + 0.5 * (a - c) / (a - 2 * b + c);
    return sr / t;
}

static void run (double sr, double inHz, atm::EngineParams p, const char* label)
{
    atm::PitchEngine e;
    e.prepare (sr);
    e.setParams (p);
    const size_t N = (size_t) (sr * 1.5);
    std::vector<float> in (N), out (N);
    double ph = 0;
    for (size_t i = 0; i < N; ++i)
    {
        double hz = inHz * (1.0 + 0.004 * std::sin (2 * M_PI * 5.5 * i / sr)); // light vibrato
        ph += 2 * M_PI * hz / sr;
        // vocal-ish: several harmonics with decaying amplitude
        double v = 0; for (int h = 1; h <= 8; ++h) v += std::sin (h * ph) / (h * 1.3);
        in[i] = (float) (0.3 * v);
    }
    out = in;
    for (size_t pos = 0; pos < N; pos += 512)
        e.process (out.data() + pos, (int) std::min<size_t> (512, N - pos));

    // check delayed dry alignment from last block length
    size_t s = (size_t) (sr * 0.8);
    double fin = measureF0 (in, s, 4096, sr), fout = measureF0 (out, s, 4096, sr);
    double peak = 0; for (size_t i = s; i < s + 4096; ++i) peak = std::max (peak, (double) std::abs (out[i]));
    double peakIn = 0; for (size_t i = s; i < s + 4096; ++i) peakIn = std::max (peakIn, (double) std::abs (in[i]));
    printf ("%-34s sr=%6.0f in=%7.2f Hz out=%7.2f Hz  (%.1f cents from 440)  gain=%.2f lat=%d meter=%.2f\n",
            label, sr, fin, fout, 1200 * std::log2 (fout / 440.0), peak / peakIn, e.getLatency(), e.meterDetectedMidi.load());
}

int main()
{
    atm::EngineParams p;
    p.retuneMs = 0; p.scale = atm::Scale::Chromatic;
    run (44100, 452.0, p, "sharp A, retune 0");
    run (48000, 430.0, p, "flat A, retune 0");
    run (96000, 455.0, p, "sharp A, 96k");
    p.retuneMs = 50;
    run (48000, 448.0, p, "retune 50ms");
    p.retuneMs = 0; p.transpose = 12;
    run (48000, 220.0, p, "220 + 12st -> 440");
    p.transpose = -7; // 659.25 -> 440
    run (48000, 659.25, p, "E5 - 7st -> A4");
    p.transpose = 0; p.scale = atm::Scale::Major; p.key = 7; // G major: A is in, so 445 -> 440
    run (48000, 445.0, p, "G major");
    p.inputType = atm::InputType::LowMale;
    run (44100, 112.0, p, "low male 112 Hz (A2=110)");
    return 0;
}
