#include "PitchEngine.h"

#include <algorithm>
#include <cmath>

namespace atm
{

namespace
{
    constexpr double kPi = 3.14159265358979323846;
    constexpr float kLowestHz = 70.0f;

    // Interval masks (bit i = semitone i above the key is in the scale)
    int scaleMask (Scale s)
    {
        auto m = [] (std::initializer_list<int> l) { int r = 0; for (int i : l) r |= (1 << i); return r; };
        switch (s)
        {
            case Scale::Chromatic:       return 0xFFF;
            case Scale::Major:           return m ({ 0, 2, 4, 5, 7, 9, 11 });
            case Scale::NaturalMinor:    return m ({ 0, 2, 3, 5, 7, 8, 10 });
            case Scale::HarmonicMinor:   return m ({ 0, 2, 3, 5, 7, 8, 11 });
            case Scale::MelodicMinor:    return m ({ 0, 2, 3, 5, 7, 9, 11 });
            case Scale::MajorPentatonic: return m ({ 0, 2, 4, 7, 9 });
            case Scale::MinorPentatonic: return m ({ 0, 3, 5, 7, 10 });
            case Scale::Blues:           return m ({ 0, 3, 5, 6, 7, 10 });
            case Scale::Dorian:          return m ({ 0, 2, 3, 5, 7, 9, 10 });
            case Scale::Mixolydian:      return m ({ 0, 2, 4, 5, 7, 9, 10 });
            case Scale::Phrygian:        return m ({ 0, 1, 3, 5, 7, 8, 10 });
            case Scale::Lydian:          return m ({ 0, 2, 4, 6, 7, 9, 11 });
            default:                     return 0xFFF;
        }
    }

    void rangeFor (InputType t, float& lo, float& hi)
    {
        switch (t)
        {
            case InputType::Soprano:    lo = 170.0f; hi = 1300.0f; break;
            case InputType::AltoTenor:  lo = 90.0f;  hi = 900.0f;  break;
            case InputType::LowMale:    lo = kLowestHz; hi = 500.0f; break;
            case InputType::Instrument: lo = kLowestHz; hi = 1300.0f; break;
            default:                    lo = 90.0f;  hi = 900.0f;  break;
        }
    }
}

bool PitchEngine::scaleContains (Scale s, int key, int pitchClass)
{
    const int interval = ((pitchClass - key) % 12 + 12) % 12;
    return (scaleMask (s) >> interval) & 1;
}

const char* PitchEngine::scaleName (Scale s)
{
    switch (s)
    {
        case Scale::Chromatic:       return "Chromatic";
        case Scale::Major:           return "Major";
        case Scale::NaturalMinor:    return "Minor";
        case Scale::HarmonicMinor:   return "Harmonic Minor";
        case Scale::MelodicMinor:    return "Melodic Minor";
        case Scale::MajorPentatonic: return "Major Pentatonic";
        case Scale::MinorPentatonic: return "Minor Pentatonic";
        case Scale::Blues:           return "Blues";
        case Scale::Dorian:          return "Dorian";
        case Scale::Mixolydian:      return "Mixolydian";
        case Scale::Phrygian:        return "Phrygian";
        case Scale::Lydian:          return "Lydian";
        default:                     return "Chromatic";
    }
}

void PitchEngine::prepare (double sampleRate)
{
    sr = sampleRate > 0.0 ? sampleRate : 44100.0;

    decim = std::max (1, (int) std::lround (sr / 48000.0));
    detSr = sr / decim;

    maxPeriod = (int) std::ceil (sr / kLowestHz);
    grainCap  = (int) std::lround (maxPeriod * 1.5);
    latency   = 2 * grainCap + maxPeriod / 2 + 16;
    unvoicedPeriod = (float) (sr / 200.0);
    hop = std::max (32, (int) std::lround (detSr * 0.0058)); // ~6 ms (in detection-rate samples)

    inBuf.assign (bufSize, 0.0f);
    olaBuf.assign (bufSize, 0.0f);
    wBuf.assign (bufSize, 0.0f);
    detBuf.assign (detBufSize, 0.0f);

    const int maxLagDet = (int) std::ceil (detSr / kLowestHz) + 2;
    const int maxWin = std::max (maxLagDet, (int) (detSr * 0.008)) + 2;
    yinFrame.assign ((size_t) (maxWin + maxLagDet + 4), 0.0f);
    yinDiff.assign ((size_t) (maxLagDet + 4), 0.0f);

    reset();
}

void PitchEngine::reset()
{
    std::fill (inBuf.begin(), inBuf.end(), 0.0f);
    std::fill (olaBuf.begin(), olaBuf.end(), 0.0f);
    std::fill (wBuf.begin(), wBuf.end(), 0.0f);
    std::fill (detBuf.begin(), detBuf.end(), 0.0f);
    n = 0;
    detN = 0;
    decimAcc = 0.0f;
    decimCount = 0;
    hopCounter = 0;
    f0Hist = { 0, 0, 0 };
    f0HistPos = 0;
    currentTarget = -1.0f;
    smoothedCorrection = 0.0f;
    noteHeldSeconds = 0.0f;
    unvoicedFrames = 0;
    frameWrite = 0;
    frameCount = 0;
    nextSynthMark = 0.0;
    analysisMark = 0.0;
    analysisInit = false;
    meterVoiced = false;
}

void PitchEngine::process (float* data, int numSamples)
{
    for (int i = 0; i < numSamples; ++i)
    {
        const float x = data[i];
        inBuf[(size_t) (n & bufMask)] = x;
        ++n;

        // decimated copy for pitch detection
        decimAcc += x;
        if (++decimCount >= decim)
        {
            detBuf[(size_t) (detN & detBufMask)] = decimAcc / (float) decim;
            ++detN;
            decimAcc = 0.0f;
            decimCount = 0;

            if (++hopCounter >= hop)
            {
                hopCounter = 0;
                analyseFrame();
            }
        }

        const int64_t r = n - 1 - latency; // output position

        while (nextSynthMark <= (double) (r + grainCap))
            placeGrain();

        float y = 0.0f;
        if (r >= 0)
        {
            const size_t idx = (size_t) (r & bufMask);
            const float w = wBuf[idx];
            y = olaBuf[idx] / std::max (w, 0.25f);
            olaBuf[idx] = 0.0f;
            wBuf[idx] = 0.0f;
        }
        data[i] = y;
    }
}

void PitchEngine::getDelayedDry (float* dest, int numSamples) const
{
    const int64_t start = n - numSamples - latency;
    for (int i = 0; i < numSamples; ++i)
    {
        const int64_t p = start + i;
        dest[i] = p >= 0 ? inBuf[(size_t) (p & bufMask)] : 0.0f;
    }
}

float PitchEngine::detectPitchYin (float& confidence)
{
    confidence = 0.0f;
    float lo, hi;
    rangeFor (params.inputType, lo, hi);

    const int maxLag = std::min ((int) std::ceil (detSr / lo), (int) yinDiff.size() - 3);
    const int minLag = std::max (2, (int) std::floor (detSr / hi));
    const int W = std::max (maxLag, (int) (detSr * 0.008));
    const int total = W + maxLag;

    if (detN < total || total > (int) yinFrame.size())
        return 0.0f;

    // copy most recent samples, remove DC
    double mean = 0.0;
    const int64_t start = detN - total;
    for (int j = 0; j < total; ++j)
    {
        const float v = detBuf[(size_t) ((start + j) & detBufMask)];
        yinFrame[(size_t) j] = v;
        mean += v;
    }
    mean /= total;
    double energy = 0.0;
    for (int j = 0; j < total; ++j)
    {
        yinFrame[(size_t) j] -= (float) mean;
        energy += (double) yinFrame[(size_t) j] * yinFrame[(size_t) j];
    }
    const double rms = std::sqrt (energy / total);
    if (rms < 0.0015) // about -56 dBFS: treat as silence
        return 0.0f;

    // difference function
    const float* x = yinFrame.data();
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        float sum = 0.0f;
        const float* xt = x + tau;
        for (int j = 0; j < W; ++j)
        {
            const float d = x[j] - xt[j];
            sum += d * d;
        }
        yinDiff[(size_t) tau] = sum;
    }

    // cumulative mean normalised difference
    yinDiff[0] = 1.0f;
    double running = 0.0;
    for (int tau = 1; tau <= maxLag; ++tau)
    {
        running += yinDiff[(size_t) tau];
        yinDiff[(size_t) tau] = running > 0.0 ? (float) (yinDiff[(size_t) tau] * tau / running) : 1.0f;
    }

    int best = -1;
    const float threshold = 0.15f;
    for (int tau = minLag; tau <= maxLag; ++tau)
    {
        if (yinDiff[(size_t) tau] < threshold)
        {
            while (tau + 1 <= maxLag && yinDiff[(size_t) tau + 1] < yinDiff[(size_t) tau])
                ++tau;
            best = tau;
            break;
        }
    }

    if (best < 0)
    {
        // fallback: global minimum if it is still reasonably periodic
        float minVal = 1.0f;
        for (int tau = minLag; tau <= maxLag; ++tau)
            if (yinDiff[(size_t) tau] < minVal) { minVal = yinDiff[(size_t) tau]; best = tau; }
        if (minVal > 0.25f)
            return 0.0f;
    }

    float betterTau = (float) best;
    if (best > 1 && best < maxLag)
    {
        const float s0 = yinDiff[(size_t) best - 1], s1 = yinDiff[(size_t) best], s2 = yinDiff[(size_t) best + 1];
        const float denom = s0 - 2.0f * s1 + s2;
        if (std::abs (denom) > 1e-9f)
            betterTau += 0.5f * (s0 - s2) / denom;
    }

    confidence = 1.0f - yinDiff[(size_t) best];
    return (float) (detSr / betterTau);
}

float PitchEngine::chooseTarget (float midi)
{
    std::array<bool, 12> allowed {};
    bool any = false;
    for (int pc = 0; pc < 12; ++pc)
    {
        allowed[(size_t) pc] = scaleContains (params.scale, params.key, pc) && ! params.removed[(size_t) pc];
        any = any || allowed[(size_t) pc];
    }
    if (! any)
        return midi; // nothing to correct to

    const int centre = (int) std::lround (midi);
    float candidate = midi;
    float bestDist = 1e9f;
    for (int note = centre - 7; note <= centre + 7; ++note)
    {
        const int pc = ((note % 12) + 12) % 12;
        if (! allowed[(size_t) pc]) continue;
        const float d = std::abs (midi - (float) note);
        if (d < bestDist) { bestDist = d; candidate = (float) note; }
    }

    const float hopSec = (float) (hop / detSr);
    bool keep = false;
    if (currentTarget >= 0.0f)
    {
        const int cpc = (((int) currentTarget % 12) + 12) % 12;
        const float dCur = std::abs (midi - currentTarget);
        keep = allowed[(size_t) cpc] && dCur < 1.5f && dCur <= bestDist + 0.12f; // hysteresis
    }

    if (keep)
    {
        noteHeldSeconds += hopSec;
        return currentTarget;
    }

    if (candidate != currentTarget)
        noteHeldSeconds = 0.0f;
    currentTarget = candidate;
    return candidate;
}

void PitchEngine::analyseFrame()
{
    float conf = 0.0f;
    float f0 = detectPitchYin (conf);

    // 3-point median to kill octave glitches
    f0Hist[(size_t) f0HistPos] = f0;
    f0HistPos = (f0HistPos + 1) % 3;
    if (f0 > 0.0f && f0Hist[0] > 0.0f && f0Hist[1] > 0.0f && f0Hist[2] > 0.0f)
    {
        auto h = f0Hist;
        std::sort (h.begin(), h.end());
        // only replace when current value is an outlier (> ~5 semitones from median)
        if (std::abs (std::log2 (f0 / h[1])) > 0.42f)
            f0 = h[1];
    }

    Frame fr;
    float L, H;
    rangeFor (params.inputType, L, H);
    const int maxLag = (int) std::ceil (detSr / L);
    const int W = std::max (maxLag, (int) (detSr * 0.008));
    fr.centre = n - (int64_t) ((W + maxLag) * decim / 2);

    const float hopSec = (float) (hop / detSr);
    const bool voiced = f0 > 0.0f && (sr / f0) <= (double) maxPeriod;

    if (voiced)
    {
        unvoicedFrames = 0;
        const float ref = std::max (1.0f, params.referenceHz);
        const float midi = 69.0f + 12.0f * std::log2 (f0 / ref);
        const float target = chooseTarget (midi);
        float corr = target - midi;

        if (params.flexTune > 0.0f)
        {
            const float flexSemis = params.flexTune / 100.0f;
            const float s = std::min (1.0f, std::abs (corr) / flexSemis);
            corr *= s * s;
        }

        const float humanFactor = 1.0f + (params.humanize / 100.0f) * 4.0f * std::min (1.0f, noteHeldSeconds / 0.6f);
        const float tau = (params.retuneMs / 1000.0f) * humanFactor;
        if (tau <= 0.0005f)
            smoothedCorrection = corr;
        else
            smoothedCorrection += (1.0f - std::exp (-hopSec / tau)) * (corr - smoothedCorrection);

        const float shift = smoothedCorrection + params.transpose;
        float ratio = std::pow (2.0f, shift / 12.0f);
        ratio = std::clamp (ratio, 0.5f, 2.0f);

        fr.voiced = true;
        fr.period = (float) (sr / f0);
        fr.ratio = ratio;

        meterDetectedMidi = midi;
        meterTargetMidi = target;
        meterOutputMidi = midi + std::log2 (ratio) * 12.0f;
        meterVoiced = true;
    }
    else
    {
        if (++unvoicedFrames * hopSec > 0.08f)
        {
            smoothedCorrection = 0.0f;
            currentTarget = -1.0f;
            noteHeldSeconds = 0.0f;
        }
        fr.voiced = false;
        fr.period = unvoicedPeriod;
        fr.ratio = 1.0f;
        meterVoiced = false;
    }

    frames[(size_t) frameWrite] = fr;
    frameWrite = (frameWrite + 1) % numFrames;
    frameCount = std::min (frameCount + 1, numFrames);
}

const PitchEngine::Frame& PitchEngine::frameAt (double t) const
{
    static const Frame silent { 0, false, 0.0f, 1.0f };
    if (frameCount == 0)
        return silent;

    // newest -> oldest; return the newest frame whose centre is <= t
    for (int k = 1; k <= frameCount; ++k)
    {
        const int idx = (frameWrite - k + numFrames) % numFrames;
        if ((double) frames[(size_t) idx].centre <= t)
            return frames[(size_t) idx];
    }
    return frames[(size_t) ((frameWrite - frameCount + numFrames) % numFrames)];
}

void PitchEngine::placeGrain()
{
    const double sm = nextSynthMark;
    const Frame& f = frameAt (sm);

    double T = (f.voiced && f.period > 1.0f) ? f.period : unvoicedPeriod;
    double ratio = f.voiced ? f.ratio : 1.0;
    const double Ts = T / ratio;

    if (! analysisInit || std::abs (sm - analysisMark) > 2.0 * T)
    {
        analysisMark = sm;
        analysisInit = true;
    }
    while (sm - analysisMark > 0.5 * T) analysisMark += T;
    while (analysisMark - sm > 0.5 * T) analysisMark -= T;

    const int H = std::clamp ((int) std::lround (ratio > 1.0 ? std::max (Ts, T / std::sqrt (ratio)) : std::max (T, Ts)), 16, grainCap);
    const int64_t smi = (int64_t) std::llround (sm);
    const int64_t ami = (int64_t) std::llround (analysisMark);
    const double wScale = kPi / (double) (H + 1);

    for (int k = -H; k <= H; ++k)
    {
        const float w = (float) (0.5 + 0.5 * std::cos (wScale * k));
        const size_t o = (size_t) ((smi + k) & bufMask);
        olaBuf[o] += inBuf[(size_t) ((ami + k) & bufMask)] * w;
        wBuf[o] += w;
    }

    nextSynthMark += std::max (Ts, 16.0);
}

} // namespace atm
