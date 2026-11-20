#include "PluginEditor.h"

using namespace atmui;

static juce::Font uiFont (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

static juce::String noteName (float midi)
{
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };
    const int n = (int) std::lround (midi);
    return juce::String (names[((n % 12) + 12) % 12]) + juce::String (n / 12 - 1);
}

//==============================================================================
AtmLookAndFeel::AtmLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, ink);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, pinkSoft);
    setColour (juce::Label::textColourId, ink);
    setColour (juce::ComboBox::textColourId, juce::Colours::white);
    setColour (juce::ComboBox::arrowColourId, pinkSoft);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff2a0820));
    setColour (juce::PopupMenu::textColourId, juce::Colours::white);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pink);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextEditor::textColourId, ink);
    setColour (juce::TextEditor::backgroundColourId, juce::Colours::white);
    setColour (juce::TextEditor::highlightColourId, pinkSoft);
    setColour (juce::CaretComponent::caretColourId, pink);
}

void AtmLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                       float startAngle, float endAngle, juce::Slider&)
{
    const auto area = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (4.0f);
    const float size = juce::jmin (area.getWidth(), area.getHeight());
    const auto c = area.getCentre();
    const float rOuter = size * 0.5f;
    const float rKnob = rOuter * 0.72f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    // value track
    juce::Path track;
    track.addCentredArc (c.x, c.y, rOuter - 3.0f, rOuter - 3.0f, 0.0f, startAngle, endAngle, true);
    g.setColour (chassisLo);
    g.strokePath (track, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path value;
    value.addCentredArc (c.x, c.y, rOuter - 3.0f, rOuter - 3.0f, 0.0f, startAngle, angle, true);
    g.setColour (pink);
    g.strokePath (value, juce::PathStrokeType (3.5f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // drop shadow
    for (int i = 4; i > 0; --i)
    {
        g.setColour (juce::Colours::black.withAlpha (0.035f * (float) i));
        g.fillEllipse (c.x - rKnob - (float) i * 0.5f, c.y - rKnob + 2.0f + (float) i, (rKnob + (float) i * 0.5f) * 2.0f, (rKnob + (float) i * 0.5f) * 2.0f);
    }

    // knob body (white plastic)
    juce::ColourGradient body (juce::Colours::white, c.x, c.y - rKnob, juce::Colour (0xffe4e0dc), c.x, c.y + rKnob, false);
    g.setGradientFill (body);
    g.fillEllipse (c.x - rKnob, c.y - rKnob, rKnob * 2.0f, rKnob * 2.0f);
    g.setColour (juce::Colour (0xffcfcac5));
    g.drawEllipse (c.x - rKnob, c.y - rKnob, rKnob * 2.0f, rKnob * 2.0f, 1.0f);

    // inner cap
    const float rCap = rKnob * 0.78f;
    juce::ColourGradient cap (juce::Colour (0xfff0edea), c.x, c.y + rCap, juce::Colours::white, c.x, c.y - rCap, false);
    g.setGradientFill (cap);
    g.fillEllipse (c.x - rCap, c.y - rCap, rCap * 2.0f, rCap * 2.0f);

    // pointer
    juce::Path p;
    p.addRoundedRectangle (-1.8f, -rCap + 3.0f, 3.6f, rCap * 0.55f, 1.8f);
    g.setColour (ink);
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x, c.y));
}

void AtmLookAndFeel::drawComboBox (juce::Graphics& g, int width, int height, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height).reduced (0.5f);
    g.setColour (juce::Colour (0xff3a0d2c));
    g.fillRoundedRectangle (r, 6.0f);
    g.setColour (box.hasKeyboardFocus (true) ? pink : juce::Colour (0xff6b1d4f));
    g.drawRoundedRectangle (r, 6.0f, 1.2f);

    juce::Path arrow;
    const float ax = (float) width - 16.0f, ay = (float) height * 0.5f;
    arrow.addTriangle (ax - 4.0f, ay - 2.0f, ax + 4.0f, ay - 2.0f, ax, ay + 3.0f);
    g.setColour (pinkSoft);
    g.fillPath (arrow);
}

juce::Font AtmLookAndFeel::getComboBoxFont (juce::ComboBox&) { return uiFont (14.0f, true); }
juce::Font AtmLookAndFeel::getPopupMenuFont()               { return uiFont (14.0f); }
juce::Font AtmLookAndFeel::getLabelFont (juce::Label& l)     { return l.getFont(); }

void AtmLookAndFeel::positionComboBoxText (juce::ComboBox& box, juce::Label& label)
{
    label.setBounds (8, 1, box.getWidth() - 28, box.getHeight() - 2);
    label.setFont (getComboBoxFont (box));
}

//==============================================================================
bool NoteKeyboard::isBlack (int pc)
{
    return pc == 1 || pc == 3 || pc == 6 || pc == 8 || pc == 10;
}

juce::Rectangle<float> NoteKeyboard::keyBounds (int pc) const
{
    static const int whiteIndex[] = { 0, -1, 1, -1, 2, 3, -1, 4, -1, 5, -1, 6 };
    static const float blackPos[] = { 0, 0.68f, 0, 1.70f, 0, 0, 3.66f, 0, 4.68f, 0, 5.70f, 0 };
    const float kw = (float) getWidth() / 7.0f;
    const float h = (float) getHeight();
    if (! isBlack (pc))
        return { whiteIndex[pc] * kw, 0.0f, kw, h };
    return { blackPos[pc] * kw, 0.0f, kw * 0.62f, h * 0.6f };
}

int NoteKeyboard::keyAt (juce::Point<float> pt) const
{
    for (int pc = 0; pc < 12; ++pc)
        if (isBlack (pc) && keyBounds (pc).contains (pt)) return pc;
    for (int pc = 0; pc < 12; ++pc)
        if (! isBlack (pc) && keyBounds (pc).contains (pt)) return pc;
    return -1;
}

void NoteKeyboard::paint (juce::Graphics& g)
{
    const int key = (int) proc.apvts.getRawParameterValue ("key")->load();
    const auto scale = (atm::Scale) (int) proc.apvts.getRawParameterValue ("scale")->load();
    static const char* names[] = { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" };

    auto drawKey = [&] (int pc)
    {
        auto r = keyBounds (pc).reduced (1.5f, 0.0f);
        const bool inScale = atm::PitchEngine::scaleContains (scale, key, pc);
        const bool removed = proc.apvts.getRawParameterValue ("rm" + juce::String (pc))->load() > 0.5f;
        const bool active = inScale && ! removed;
        const bool black = isBlack (pc);

        juce::Colour fill = black ? juce::Colour (0xff3b3740) : juce::Colours::white;
        if (active) fill = black ? juce::Colour (0xffd81b6e) : juce::Colour (0xffffd3e5);
        g.setColour (juce::Colours::black.withAlpha (0.12f));
        g.fillRoundedRectangle (r.translated (0.0f, 2.0f), 5.0f);
        g.setColour (fill);
        g.fillRoundedRectangle (r, 5.0f);
        g.setColour (black ? juce::Colours::black.withAlpha (0.4f) : chassisLo);
        g.drawRoundedRectangle (r, 5.0f, 1.0f);

        // LED
        const float ledR = 3.5f;
        const auto ledC = juce::Point<float> (r.getCentreX(), r.getBottom() - (black ? 10.0f : 26.0f));
        g.setColour (active ? pink : juce::Colour (0xffb9b3b8).withAlpha (black ? 0.5f : 1.0f));
        g.fillEllipse (ledC.x - ledR, ledC.y - ledR, ledR * 2.0f, ledR * 2.0f);
        if (active)
        {
            g.setColour (pink.withAlpha (0.3f));
            g.fillEllipse (ledC.x - ledR * 2.2f, ledC.y - ledR * 2.2f, ledR * 4.4f, ledR * 4.4f);
        }

        if (! black)
        {
            g.setColour (removed && inScale ? inkSoft : ink);
            g.setFont (uiFont (11.0f, true));
            g.drawText (names[pc], r.removeFromBottom (16.0f), juce::Justification::centred);
        }
        if (removed && inScale)
        {
            g.setColour (inkSoft);
            g.drawLine (r.getX() + 6, r.getY() + 6, r.getRight() - 6, r.getY() + 20, 1.5f);
        }
    };

    for (int pc = 0; pc < 12; ++pc) if (! isBlack (pc)) drawKey (pc);
    for (int pc = 0; pc < 12; ++pc) if (isBlack (pc))   drawKey (pc);
}

void NoteKeyboard::mouseDown (const juce::MouseEvent& e)
{
    const int pc = keyAt (e.position);
    if (pc < 0) return;
    if (auto* param = proc.apvts.getParameter ("rm" + juce::String (pc)))
    {
        const bool removed = param->getValue() > 0.5f;
        param->beginChangeGesture();
        param->setValueNotifyingHost (removed ? 0.0f : 1.0f);
        param->endChangeGesture();
    }
    repaint();
}

//==============================================================================
void PitchScreen::paint (juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // bezel
    g.setColour (juce::Colour (0xff1c1a1e));
    g.fillRoundedRectangle (bounds, 16.0f);
    auto glass = bounds.reduced (8.0f);
    juce::ColourGradient grad (screenTop, glass.getX(), glass.getY(), screenBot, glass.getRight(), glass.getBottom(), false);
    grad.addColour (0.55, juce::Colour (0xff4d0b33));
    g.setGradientFill (grad);
    g.fillRoundedRectangle (glass, 11.0f);

    // subtle scanlines
    g.setColour (juce::Colours::black.withAlpha (0.10f));
    for (float yy = glass.getY(); yy < glass.getBottom(); yy += 3.0f)
        g.drawHorizontalLine ((int) yy, glass.getX() + 4.0f, glass.getRight() - 4.0f);

    auto content = glass.reduced (18.0f, 12.0f);

    // header
    auto header = content.removeFromTop (18.0f);
    g.setFont (uiFont (11.0f, true));
    g.setColour (pinkSoft.withAlpha (0.8f));
    g.drawText ("AUTO MODE", header.removeFromLeft (90.0f), juce::Justification::centredLeft);
    g.drawText ("PITCH CORRECTION", header.removeFromLeft (140.0f), juce::Justification::centredLeft);
    const bool voiced = proc.engine.meterVoiced.load();
    g.setColour (voiced ? pink : pinkSoft.withAlpha (0.35f));
    g.fillEllipse (header.getRight() - 10.0f, header.getCentreY() - 4.0f, 8.0f, 8.0f);
    g.setColour (pinkSoft.withAlpha (0.8f));
    g.drawText ("SIGNAL", header.withTrimmedRight (16.0f), juce::Justification::centredRight);

    // title block (left) + note readout (right)
    auto top = content.removeFromTop (70.0f);
    auto readout = top.removeFromRight (200.0f);

    g.setColour (juce::Colours::white);
    g.setFont (uiFont (44.0f, true));
    g.drawText ("vocalmax", top.removeFromTop (50.0f), juce::Justification::centredLeft);
    auto pill = top.removeFromTop (18.0f).removeFromLeft (48.0f);
    g.setColour (pink);
    g.fillRoundedRectangle (pill, 9.0f);
    g.setColour (juce::Colours::white);
    g.setFont (uiFont (11.0f, true));
    g.drawText ("PRO", pill, juce::Justification::centred);

    const float detected = proc.engine.meterDetectedMidi.load();
    const float target = proc.engine.meterTargetMidi.load();
    const float output = proc.engine.meterOutputMidi.load();
    g.setFont (uiFont (40.0f, true));
    g.setColour (voiced ? juce::Colours::white : juce::Colours::white.withAlpha (0.3f));
    g.drawText (voiced ? noteName (target) : "--", readout.removeFromTop (46.0f), juce::Justification::centredRight);
    g.setFont (uiFont (12.0f, true));
    g.setColour (pinkSoft);
    const float cents = (detected - target) * 100.0f;
    g.drawText (voiced ? juce::String ("IN ") + (cents >= 0 ? "+" : "") + juce::String ((int) std::lround (cents)) + " ct" : "NO PITCH",
                readout, juce::Justification::centredRight);

    // pitch graph
    content.removeFromTop (6.0f);
    auto graph = content;
    g.setColour (juce::Colours::black.withAlpha (0.22f));
    g.fillRoundedRectangle (graph, 8.0f);
    graph = graph.reduced (6.0f, 4.0f);

    if (voiced)
        viewCentre += (output - viewCentre) * 0.15f;
    const float span = 7.0f; // semitones visible
    auto yFor = [&] (float midi) { return graph.getCentreY() - (midi - viewCentre) / span * graph.getHeight(); };

    const int key = (int) proc.apvts.getRawParameterValue ("key")->load();
    const auto scale = (atm::Scale) (int) proc.apvts.getRawParameterValue ("scale")->load();
    g.setFont (uiFont (9.5f, true));
    for (int note = (int) std::floor (viewCentre - span); note <= (int) std::ceil (viewCentre + span); ++note)
    {
        const float yy = yFor ((float) note);
        if (yy < graph.getY() || yy > graph.getBottom()) continue;
        const int pc = ((note % 12) + 12) % 12;
        const bool inScale = atm::PitchEngine::scaleContains (scale, key, pc)
                             && proc.apvts.getRawParameterValue ("rm" + juce::String (pc))->load() < 0.5f;
        g.setColour (pinkSoft.withAlpha (inScale ? 0.28f : 0.08f));
        g.drawHorizontalLine ((int) yy, graph.getX() + 30.0f, graph.getRight());
        if (inScale)
        {
            g.setColour (pinkSoft.withAlpha (0.7f));
            g.drawText (noteName ((float) note), juce::Rectangle<float> (graph.getX(), yy - 6.0f, 28.0f, 12.0f), juce::Justification::centredLeft);
        }
    }

    const int N = VocalMaxProcessor::historySize;
    const int w = proc.historyWrite.load();
    auto drawTrace = [&] (std::array<std::atomic<float>, VocalMaxProcessor::historySize>& hist, juce::Colour col, float thickness)
    {
        juce::Path path;
        bool started = false;
        for (int i = 0; i < N; ++i)
        {
            const float v = hist[(size_t) ((w + i) % N)].load();
            const float xx = graph.getX() + 30.0f + (graph.getWidth() - 30.0f) * (float) i / (float) (N - 1);
            if (v <= 0.0f) { started = false; continue; }
            const float yy = juce::jlimit (graph.getY(), graph.getBottom(), yFor (v));
            if (! started) { path.startNewSubPath (xx, yy); started = true; }
            else path.lineTo (xx, yy);
        }
        g.setColour (col);
        g.strokePath (path, juce::PathStrokeType (thickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    };
    g.saveState();
    g.reduceClipRegion (graph.toNearestInt());
    drawTrace (proc.historyIn, juce::Colours::white.withAlpha (0.35f), 1.5f);
    drawTrace (proc.historyOut, pink.brighter (0.3f), 2.4f);
    g.restoreState();

    // glass reflection
    g.setGradientFill (juce::ColourGradient (juce::Colours::white.withAlpha (0.07f), glass.getX(), glass.getY(),
                                             juce::Colours::transparentWhite, glass.getX(), glass.getCentreY(), false));
    g.fillRoundedRectangle (glass.withHeight (glass.getHeight() * 0.5f), 11.0f);
}

//==============================================================================
VocalMaxEditor::VocalMaxEditor (VocalMaxProcessor& p)
    : AudioProcessorEditor (&p), proc (p), screen (p), keyboard (p)
{
    setLookAndFeel (&lnf);

    addAndMakeVisible (screen);
    addAndMakeVisible (keyboard);

    setupSelector (selectors[0], "key", "KEY", VocalMaxProcessor::keyNames());
    setupSelector (selectors[1], "scale", "SCALE", VocalMaxProcessor::scaleNames());
    setupSelector (selectors[2], "input", "INPUT", { "Soprano", "Alto/Tenor", "Low Male", "Instrument" });

    setupKnob (knobs[0], "retune", "RETUNE SPEED", " ms");
    setupKnob (knobs[1], "humanize", "HUMANIZE", "");
    setupKnob (knobs[2], "flex", "FLEX-TUNE", "");
    setupKnob (knobs[3], "transpose", "TRANSPOSE", " st");
    setupKnob (knobs[4], "reference", "REF A4", " Hz");
    setupKnob (knobs[5], "mix", "MIX", " %");
    setupKnob (knobs[6], "output", "OUTPUT", " dB");

    setSize (840, 600);
    startTimerHz (30);
}

VocalMaxEditor::~VocalMaxEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void VocalMaxEditor::setupKnob (Knob& k, const juce::String& paramId, const juce::String& title, const juce::String& suffix)
{
    k.slider.setSliderStyle (juce::Slider::RotaryHorizontalVerticalDrag);
    k.slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 90, 18);
    k.slider.setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
    addAndMakeVisible (k.slider);
    k.attachment = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, paramId, k.slider);
    k.slider.setTextValueSuffix (suffix);
    k.slider.setNumDecimalPlacesToDisplay (paramId == "reference" ? 1 : 0);
    k.slider.updateText();

    k.label.setText (title, juce::dontSendNotification);
    k.label.setFont (uiFont (11.5f, true));
    k.label.setColour (juce::Label::textColourId, ink);
    k.label.setJustificationType (juce::Justification::centred);
    addAndMakeVisible (k.label);
}

void VocalMaxEditor::setupSelector (Selector& s, const juce::String& paramId, const juce::String& title, const juce::StringArray& items)
{
    s.box.addItemList (items, 1);
    addAndMakeVisible (s.box);
    s.attachment = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, paramId, s.box);
    s.box.onChange = [this] { keyboard.repaint(); };

    s.label.setText (title, juce::dontSendNotification);
    s.label.setFont (uiFont (11.0f, true));
    s.label.setColour (juce::Label::textColourId, inkSoft);
    s.label.setJustificationType (juce::Justification::centredLeft);
    addAndMakeVisible (s.label);
}

void VocalMaxEditor::paint (juce::Graphics& g)
{
    g.fillAll (juce::Colour (0xffd9d4cf));

    auto body = getLocalBounds().toFloat().reduced (8.0f);
    // shadow
    g.setColour (juce::Colours::black.withAlpha (0.18f));
    g.fillRoundedRectangle (body.translated (0.0f, 4.0f), 26.0f);
    // chassis
    juce::ColourGradient grad (juce::Colours::white, body.getX(), body.getY(), chassis.darker (0.04f), body.getX(), body.getBottom(), false);
    g.setGradientFill (grad);
    g.fillRoundedRectangle (body, 26.0f);
    g.setColour (juce::Colours::white);
    g.drawRoundedRectangle (body.reduced (1.5f), 25.0f, 1.5f);
    g.setColour (chassisLo);
    g.drawRoundedRectangle (body, 26.0f, 1.0f);

    // screws
    auto screw = [&] (float cx, float cy)
    {
        g.setColour (juce::Colour (0xffd2cdc8));
        g.fillEllipse (cx - 5.0f, cy - 5.0f, 10.0f, 10.0f);
        g.setColour (juce::Colour (0xffa9a39e));
        g.drawLine (cx - 3.0f, cy, cx + 3.0f, cy, 1.2f);
    };
    screw (body.getX() + 20.0f, body.getY() + 20.0f);
    screw (body.getRight() - 20.0f, body.getY() + 20.0f);
    screw (body.getX() + 20.0f, body.getBottom() - 20.0f);
    screw (body.getRight() - 20.0f, body.getBottom() - 20.0f);

    // knob section panel line
    g.setColour (chassisLo);
    g.drawHorizontalLine (screen.getBottom() + 14, 40.0f, (float) getWidth() - 40.0f);

    // decorative jacks (bottom-left)
    auto jack = [&] (float cx, float cy, const juce::String& txt)
    {
        g.setColour (juce::Colour (0xffcfc9c4));
        g.fillEllipse (cx - 13.0f, cy - 13.0f, 26.0f, 26.0f);
        g.setColour (pink);
        g.fillEllipse (cx - 9.0f, cy - 9.0f, 18.0f, 18.0f);
        g.setColour (juce::Colour (0xff3a0d2c));
        g.fillEllipse (cx - 4.0f, cy - 4.0f, 8.0f, 8.0f);
        g.setColour (inkSoft);
        g.setFont (uiFont (10.0f, true));
        g.drawText (txt, juce::Rectangle<float> (cx - 20.0f, cy + 14.0f, 40.0f, 14.0f), juce::Justification::centred);
    };
    const float jy = (float) keyboard.getBounds().getCentreY() - 6.0f;
    jack (70.0f, jy, "IN");
    jack (118.0f, jy, "OUT");

    // brand plate (bottom-right)
    auto plate = juce::Rectangle<float> ((float) keyboard.getRight() + 26.0f, jy - 22.0f, (float) getWidth() - 40.0f - ((float) keyboard.getRight() + 26.0f), 44.0f);
    g.setColour (ink);
    g.setFont (uiFont (15.0f, true));
    g.drawText ("VocalMAX", plate.removeFromTop (22.0f), juce::Justification::centredRight);
    g.setColour (inkSoft);
    g.setFont (uiFont (10.5f, true));
    g.drawText ("VOCAL PITCH PROCESSOR", plate, juce::Justification::centredRight);

    // keyboard caption
    g.setColour (inkSoft);
    g.setFont (uiFont (10.0f, true));
    g.drawText ("TARGET NOTES  (CLICK TO REMOVE / RESTORE)",
                keyboard.getBounds().toFloat().withY ((float) keyboard.getY() - 18.0f).withHeight (14.0f),
                juce::Justification::centred);
}

void VocalMaxEditor::resized()
{
    auto area = getLocalBounds().reduced (40, 28);

    // selector row
    auto top = area.removeFromTop (30);
    const int selW[] = { 110, 200, 160 };
    for (size_t i = 0; i < selectors.size(); ++i)
    {
        auto cell = top.removeFromLeft (selW[i] + 60);
        selectors[i].label.setBounds (cell.removeFromLeft (52));
        selectors[i].box.setBounds (cell.removeFromLeft (selW[i]).reduced (0, 2));
        top.removeFromLeft (8);
    }

    area.removeFromTop (10);
    screen.setBounds (area.removeFromTop (236));
    area.removeFromTop (28);

    // knobs
    auto knobRow = area.removeFromTop (130);
    const int kw = knobRow.getWidth() / (int) knobs.size();
    for (auto& k : knobs)
    {
        auto cell = knobRow.removeFromLeft (kw);
        k.label.setBounds (cell.removeFromTop (16));
        k.slider.setBounds (cell.reduced (6, 0));
    }

    area.removeFromTop (24);
    auto bottom = area;
    keyboard.setBounds (bottom.withSizeKeepingCentre (330, juce::jmin (78, bottom.getHeight())));
}

void VocalMaxEditor::timerCallback()
{
    screen.repaint();
    keyboard.repaint();
}
