#pragma once

#include <JuceHeader.h>
#include "PitchEngine.h"

class VocalMaxProcessor : public juce::AudioProcessor
{
public:
    VocalMaxProcessor();
    ~VocalMaxProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return "Default"; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    static juce::StringArray keyNames();
    static juce::StringArray scaleNames();

    juce::AudioProcessorValueTreeState apvts;
    atm::PitchEngine engine;

    // pitch history for the on-screen graph (written by audio thread)
    static constexpr int historySize = 256;
    std::array<std::atomic<float>, historySize> historyIn {};
    std::array<std::atomic<float>, historySize> historyOut {};
    std::atomic<int> historyWrite { 0 };

private:
    void updateEngineParams();

    juce::AudioBuffer<float> monoBuffer, dryBuffer;
    juce::SmoothedValue<float> mixSmooth, gainSmooth;
    int historyCounter = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMaxProcessor)
};
