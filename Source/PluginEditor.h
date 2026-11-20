#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"

namespace atmui
{
    const juce::Colour chassis   { 0xfff3f1ef };
    const juce::Colour chassisLo { 0xffdedad6 };
    const juce::Colour ink       { 0xff3b3740 };
    const juce::Colour inkSoft   { 0xff8d8792 };
    const juce::Colour pink      { 0xffff3d8b };
    const juce::Colour pinkSoft  { 0xffff9cc4 };
    const juce::Colour screenTop { 0xff2a0820 };
    const juce::Colour screenBot { 0xff7a0f4a };
}

class AtmLookAndFeel : public juce::LookAndFeel_V4
{
public:
    AtmLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override;
    juce::Font getPopupMenuFont() override;
    juce::Font getLabelFont (juce::Label&) override;
    void positionComboBoxText (juce::ComboBox&, juce::Label&) override;
};

// One-octave keyboard: lit keys are correction targets, click to remove/restore a note.
class NoteKeyboard : public juce::Component
{
public:
    explicit NoteKeyboard (VocalMaxProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;
    void mouseDown (const juce::MouseEvent&) override;

private:
    juce::Rectangle<float> keyBounds (int pc) const;
    static bool isBlack (int pc);
    int keyAt (juce::Point<float>) const;
    VocalMaxProcessor& proc;
};

class PitchScreen : public juce::Component
{
public:
    explicit PitchScreen (VocalMaxProcessor& p) : proc (p) {}
    void paint (juce::Graphics&) override;

private:
    VocalMaxProcessor& proc;
    float viewCentre = 69.0f;
};

class VocalMaxEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit VocalMaxEditor (VocalMaxProcessor&);
    ~VocalMaxEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    struct Knob
    {
        juce::Slider slider;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> attachment;
    };

    struct Selector
    {
        juce::ComboBox box;
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> attachment;
    };

    void setupKnob (Knob&, const juce::String& paramId, const juce::String& title, const juce::String& suffix);
    void setupSelector (Selector&, const juce::String& paramId, const juce::String& title, const juce::StringArray& items);

    VocalMaxProcessor& proc;
    AtmLookAndFeel lnf;

    PitchScreen screen;
    NoteKeyboard keyboard;
    std::array<Knob, 7> knobs;
    std::array<Selector, 3> selectors;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (VocalMaxEditor)
};
