// VocalMAX - real-time pitch correction engine
// Pure C++ (no framework dependency): YIN pitch detection + TD-PSOLA resynthesis.
// PSOLA works on pitch-synchronous grains, so vocal formants are preserved naturally.
#pragma once

#include <array>
#include <atomic>
#include <cstdint>
#include <vector>

namespace atm
{

enum class Scale : int
{
    Chromatic = 0, Major, NaturalMinor, HarmonicMinor, MelodicMinor,
    MajorPentatonic, MinorPentatonic, Blues, Dorian, Mixolydian, Phrygian, Lydian,
    NumScales
};

enum class InputType : int
{
    Soprano = 0, AltoTenor, LowMale, Instrument, NumTypes
};

struct EngineParams
{
    int   key            = 0;      // 0 = C ... 11 = B
    Scale scale          = Scale::Chromatic;
    InputType inputType  = InputType::AltoTenor;
    float retuneMs       = 20.0f;  // 0 = instant (robotic), up to 400 ms
    float humanize       = 0.0f;   // 0..100
    float flexTune       = 0.0f;   // 0..100 (cents of natural deviation allowed)
    float transpose      = 0.0f;   // semitones, -12..12
    float referenceHz    = 440.0f; // tuning of A4
    std::array<bool, 12> removed {}; // per pitch-class removal (true = never target)
};

class PitchEngine
{
public:
    PitchEngine() = default;

    void prepare (double sampleRate);
    void reset();
    void setParams (const EngineParams& p) { params = p; }

    // Processes one mono buffer in place. Output is delayed by getLatency() samples.
    void process (float* data, int numSamples);

    // Returns the input delayed by the same latency as the output (for dry/wet mix).
    // Must be called right after process() with a buffer of the same length.
    void getDelayedDry (float* dest, int numSamples) const;

    int getLatency() const { return latency; }

    // ---- metering (written on audio thread, read by GUI) ----
    std::atomic<float> meterDetectedMidi { 0.0f };
    std::atomic<float> meterOutputMidi   { 0.0f };
    std::atomic<float> meterTargetMidi   { 0.0f };
    std::atomic<bool>  meterVoiced       { false };

    static bool scaleContains (Scale s, int key, int pitchClass);
    static const char* scaleName (Scale s);

private:
    struct Frame
    {
        int64_t centre = 0;
        bool    voiced = false;
        float   period = 0.0f;   // analysis period in samples
        float   ratio  = 1.0f;   // pitch shift ratio
    };

    void analyseFrame();
    float detectPitchYin (float& confidence);
    float chooseTarget (float midi);
    const Frame& frameAt (double t) const;
    void placeGrain();

    double sr = 44100.0;
    EngineParams params;

    static constexpr int bufSize = 1 << 15;
    static constexpr int bufMask = bufSize - 1;
    std::vector<float> inBuf, olaBuf, wBuf;

    int64_t n = 0;           // absolute input sample counter (index of next sample to write)
    int latency = 0;
    int maxPeriod = 0;       // largest period ever analysed (lowest detectable pitch)
    int grainCap = 0;        // max grain half length
    int hop = 0, hopCounter = 0;
    float unvoicedPeriod = 256.0f;

    // YIN scratch
    std::vector<float> yinFrame, yinDiff;

    // pitch tracking state
    std::array<float, 3> f0Hist { 0, 0, 0 };
    int f0HistPos = 0;
    float currentTarget = -1.0f;
    float smoothedCorrection = 0.0f;
    float noteHeldSeconds = 0.0f;
    int unvoicedFrames = 0;
    float dcPrevX = 0.0f, dcPrevY = 0.0f;

    static constexpr int numFrames = 128;
    std::array<Frame, numFrames> frames {};
    int frameWrite = 0, frameCount = 0;

    // PSOLA synthesis state
    double nextSynthMark = 0.0;
    double analysisMark = 0.0;
    bool analysisInit = false;

    // decimated signal used only for pitch detection (keeps CPU low at high sample rates)
    static constexpr int detBufSize = 1 << 14;
    static constexpr int detBufMask = detBufSize - 1;
    std::vector<float> detBuf;
    int decim = 1;
    double detSr = 44100.0;
    int64_t detN = 0;
    float decimAcc = 0.0f;
    int decimCount = 0;
};

} // namespace atm
