#include "PluginProcessor.h"
#include "PluginEditor.h"

juce::StringArray VocalMaxProcessor::keyNames()
{
    return { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
}

juce::StringArray VocalMaxProcessor::scaleNames()
{
    juce::StringArray s;
    for (int i = 0; i < (int) atm::Scale::NumScales; ++i)
        s.add (atm::PitchEngine::scaleName ((atm::Scale) i));
    return s;
}

juce::AudioProcessorValueTreeState::ParameterLayout VocalMaxProcessor::createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "key", 1 }, "Key", keyNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "scale", 1 }, "Scale", scaleNames(), 0));
    layout.add (std::make_unique<AudioParameterChoice> (ParameterID { "input", 1 }, "Input Type",
                                                        StringArray { "Soprano", "Alto/Tenor", "Low Male", "Instrument" }, 1));

    auto msRange = NormalisableRange<float> (0.0f, 400.0f, 0.1f);
    msRange.setSkewForCentre (50.0f);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "retune", 1 }, "Retune Speed", msRange, 20.0f,
                                                       AudioParameterFloatAttributes().withLabel ("ms")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "humanize", 1 }, "Humanize",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "flex", 1 }, "Flex-Tune",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f));
    layout.add (std::make_unique<AudioParameterInt> (ParameterID { "transpose", 1 }, "Transpose", -12, 12, 0,
                                                     AudioParameterIntAttributes().withLabel ("st")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "reference", 1 }, "Reference A4",
                                                       NormalisableRange<float> (415.0f, 465.0f, 0.1f), 440.0f,
                                                       AudioParameterFloatAttributes().withLabel ("Hz")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                                                       NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                                                       AudioParameterFloatAttributes().withLabel ("%")));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "output", 1 }, "Output",
                                                       NormalisableRange<float> (-24.0f, 12.0f, 0.1f), 0.0f,
                                                       AudioParameterFloatAttributes().withLabel ("dB")));

    const auto names = keyNames();
    for (int i = 0; i < 12; ++i)
        layout.add (std::make_unique<AudioParameterBool> (ParameterID { "rm" + String (i), 1 },
                                                          "Remove " + names[i], false));
    return layout;
}

VocalMaxProcessor::VocalMaxProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "PARAMS", createLayout())
{
    for (auto& h : historyIn)  h.store (0.0f);
    for (auto& h : historyOut) h.store (0.0f);
}

bool VocalMaxProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto in = layouts.getMainInputChannelSet();
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return in == out;
}

void VocalMaxProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    engine.prepare (sampleRate);
    setLatencySamples (engine.getLatency());

    monoBuffer.setSize (1, samplesPerBlock, false, true, false);
    dryBuffer.setSize (1, samplesPerBlock, false, true, false);

    mixSmooth.reset (sampleRate, 0.05);
    gainSmooth.reset (sampleRate, 0.05);
    mixSmooth.setCurrentAndTargetValue (apvts.getRawParameterValue ("mix")->load() / 100.0f);
    gainSmooth.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));
    updateEngineParams();
}

void VocalMaxProcessor::updateEngineParams()
{
    atm::EngineParams p;
    p.key        = (int) apvts.getRawParameterValue ("key")->load();
    p.scale      = (atm::Scale) (int) apvts.getRawParameterValue ("scale")->load();
    p.inputType  = (atm::InputType) (int) apvts.getRawParameterValue ("input")->load();
    p.retuneMs   = apvts.getRawParameterValue ("retune")->load();
    p.humanize   = apvts.getRawParameterValue ("humanize")->load();
    p.flexTune   = apvts.getRawParameterValue ("flex")->load();
    p.transpose  = apvts.getRawParameterValue ("transpose")->load();
    p.referenceHz = apvts.getRawParameterValue ("reference")->load();
    for (int i = 0; i < 12; ++i)
        p.removed[(size_t) i] = apvts.getRawParameterValue ("rm" + juce::String (i))->load() > 0.5f;
    engine.setParams (p);
}

void VocalMaxProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();
    const int numCh = getTotalNumOutputChannels();

    for (int ch = getTotalNumInputChannels(); ch < numCh; ++ch)
        buffer.clear (ch, 0, numSamples);

    if (numSamples == 0)
        return;

    if (monoBuffer.getNumSamples() < numSamples)
    {
        monoBuffer.setSize (1, numSamples, false, true, true);
        dryBuffer.setSize (1, numSamples, false, true, true);
    }

    updateEngineParams();

    // sum to mono (vocals) -> pitch engine
    float* mono = monoBuffer.getWritePointer (0);
    const int inCh = juce::jmax (1, juce::jmin (numCh, buffer.getNumChannels()));
    juce::FloatVectorOperations::copy (mono, buffer.getReadPointer (0), numSamples);
    for (int ch = 1; ch < inCh; ++ch)
        juce::FloatVectorOperations::add (mono, buffer.getReadPointer (ch), numSamples);
    if (inCh > 1)
        juce::FloatVectorOperations::multiply (mono, 1.0f / (float) inCh, numSamples);

    engine.process (mono, numSamples);
    float* dry = dryBuffer.getWritePointer (0);
    engine.getDelayedDry (dry, numSamples);

    mixSmooth.setTargetValue (apvts.getRawParameterValue ("mix")->load() / 100.0f);
    gainSmooth.setTargetValue (juce::Decibels::decibelsToGain (apvts.getRawParameterValue ("output")->load()));

    for (int i = 0; i < numSamples; ++i)
    {
        const float m = mixSmooth.getNextValue();
        const float g = gainSmooth.getNextValue();
        mono[i] = (mono[i] * m + dry[i] * (1.0f - m)) * g;
    }

    for (int ch = 0; ch < numCh; ++ch)
        buffer.copyFrom (ch, 0, mono, numSamples);

    // pitch history for GUI (~ every 10 ms)
    historyCounter += numSamples;
    const int interval = (int) (getSampleRate() * 0.01);
    while (historyCounter >= interval && interval > 0)
    {
        historyCounter -= interval;
        const bool voiced = engine.meterVoiced.load();
        const int w = historyWrite.load();
        historyIn[(size_t) w].store (voiced ? engine.meterDetectedMidi.load() : 0.0f);
        historyOut[(size_t) w].store (voiced ? engine.meterOutputMidi.load() : 0.0f);
        historyWrite.store ((w + 1) % historySize);
    }
}

juce::AudioProcessorEditor* VocalMaxProcessor::createEditor()
{
    return new VocalMaxEditor (*this);
}

void VocalMaxProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = apvts.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void VocalMaxProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VocalMaxProcessor();
}
