#include "PluginEditor.h"

namespace colours
{
const juce::Colour bg       { 0xff070404 };
const juce::Colour panel    { 0xd8120909 };
const juce::Colour edge     { 0xff3d1515 };
const juce::Colour blood    { 0xffb3141c };
const juce::Colour bloodHot { 0xffff3b22 };
const juce::Colour ember    { 0xffff7a1a };
const juce::Colour bone     { 0xffd8cfc4 };
const juce::Colour ash      { 0xff86716a };
}

static juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
}

//==============================================================================
PossessedLookAndFeel::PossessedLookAndFeel()
{
    using namespace colours;
    setColour (juce::Slider::textBoxTextColourId, bone);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, blood);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff140a0a));
    setColour (juce::TextEditor::textColourId, bone);
    setColour (juce::TextEditor::outlineColourId, juce::Colours::transparentBlack);
    setColour (juce::TextEditor::focusedOutlineColourId, blood);
    setColour (juce::Label::textColourId, ash);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff140a0a));
    setColour (juce::ComboBox::outlineColourId, edge);
    setColour (juce::ComboBox::textColourId, bone);
    setColour (juce::ComboBox::arrowColourId, bloodHot);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff120808));
    setColour (juce::PopupMenu::textColourId, bone);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, blood);
    setColour (juce::TextButton::textColourOffId, bone);
    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff100707));
    setColour (juce::AlertWindow::textColourId, bone);
    setColour (juce::AlertWindow::outlineColourId, blood);
    setColour (juce::TextButton::textColourOnId, bloodHot);
}

void PossessedLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                             float startAngle, float endAngle, juce::Slider& s)
{
    using namespace colours;
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (5.0f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float thick = std::max (2.5f, r * 0.12f);
    const bool hot = s.isMouseOverOrDragging();

    juce::Path track; track.addCentredArc (c.x, c.y, r - thick, r - thick, 0, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff221010));
    g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    juce::Path val; val.addCentredArc (c.x, c.y, r - thick, r - thick, 0, startAngle, angle, true);
    // ember glow under the value arc
    g.setColour (bloodHot.withAlpha (hot ? 0.35f : 0.18f));
    g.strokePath (val, juce::PathStrokeType (thick * 2.6f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setGradientFill (juce::ColourGradient (blood, c.x - r, c.y + r, hot ? ember : bloodHot, c.x + r, c.y - r, false));
    g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // charred body
    const float br = r - thick * 2.3f;
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2c1c1a), c.x - br * 0.4f, c.y - br, juce::Colour (0xff080505), c.x + br * 0.3f, c.y + br, true));
    g.fillEllipse (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (juce::Colour (0xff000000).withAlpha (0.8f)); g.drawEllipse (c.x - br, c.y - br, br * 2, br * 2, 1.5f);
    g.setColour (edge.withAlpha (0.8f)); g.drawEllipse (c.x - br + 1.5f, c.y - br + 1.5f, br * 2 - 3, br * 2 - 3, 0.8f);

    // claw-mark pointer
    juce::Path p;
    p.startNewSubPath (0.0f, -br * 0.92f); p.lineTo (-2.2f, -br * 0.35f); p.lineTo (0.0f, -br * 0.42f); p.lineTo (2.2f, -br * 0.35f); p.closeSubPath();
    g.setColour (hot ? ember : bone); g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
}

void PossessedLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    using namespace colours;
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (box.findColour (juce::ComboBox::backgroundColourId)); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.isMouseOver() ? blood : edge); g.drawRoundedRectangle (r, 3.0f, 1.0f);
    juce::Path arrow; const float ax = (float) w - 12.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 4, ay - 2, ax + 4, ay - 2, ax, ay + 3);
    g.setColour (bloodHot); g.fillPath (arrow);
}

void PossessedLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    using namespace colours;
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (down ? blood : juce::Colour (0xff140a0a)); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (hi ? bloodHot : edge); g.drawRoundedRectangle (r, 3.0f, 1.0f);
}

//==============================================================================
void DemonEyes::update (float level, float dt)
{
    time += dt;
    fire += ((rnd.nextFloat() * 2.0f - 1.0f) - fire) * std::min (1.0f, dt * 12.0f);
    const float targetGlow = juce::jlimit (0.0f, 1.0f, 0.25f + std::sqrt (level) * 1.4f);
    glow += (targetGlow - glow) * std::min (1.0f, dt * (targetGlow > glow ? 14.0f : 2.5f));
    dilate += ((level > 0.25f ? 1.0f : 0.0f) - dilate) * std::min (1.0f, dt * 3.0f);

    // wander the gaze
    if ((lookTimer -= dt) <= 0)
    {
        lookTimer = 0.8f + rnd.nextFloat() * 3.0f;
        lookTargetX = (rnd.nextFloat() * 2 - 1) * 0.9f;
        lookTargetY = (rnd.nextFloat() * 2 - 1) * 0.35f;
    }
    lookX += (lookTargetX - lookX) * std::min (1.0f, dt * 9.0f);
    lookY += (lookTargetY - lookY) * std::min (1.0f, dt * 9.0f);

    // blink (sometimes twice)
    if (blinkPhase < 0 && (blinkTimer -= dt) <= 0) { blinkPhase = 0; blinkTimer = rnd.nextFloat() < 0.2f ? 0.25f : 2.5f + rnd.nextFloat() * 5.0f; }
    float lid = 1.0f;
    if (blinkPhase >= 0)
    {
        blinkPhase += dt / 0.2f;
        lid = blinkPhase >= 1 ? 1.0f : 1.0f - std::sin (juce::MathConstants<float>::pi * blinkPhase);
        if (blinkPhase >= 1) blinkPhase = -1;
    }
    const float awake = 0.62f + 0.38f * glow; // heavy-lidded when quiet, wide when the voice is loud
    openness = juce::jlimit (0.0f, 1.0f, awake * lid);
    repaint();
}

void DemonEyes::drawEye (juce::Graphics& g, juce::Point<float> c, float w, float h, float tilt, bool mirror)
{
    using namespace colours;
    const float sx = mirror ? -1.0f : 1.0f;
    const float oh = h * std::max (0.04f, openness);
    const auto xf = juce::AffineTransform::scale (sx, 1.0f).rotated (tilt * sx).translated (c);
    const float heat = juce::jlimit (0.0f, 1.0f, glow * (0.85f + 0.15f * fire));

    // narrow, angular eye: outer corner high, upper lid slashing down to a sharp inner corner
    juce::Path eye;
    eye.startNewSubPath (-w * 0.52f, -h * 0.42f);
    eye.cubicTo (-w * 0.25f, -h * 0.2f - oh * 0.55f, w * 0.15f, -oh * 0.25f, w * 0.5f, h * 0.3f);
    eye.cubicTo (w * 0.15f, h * 0.22f + oh * 0.4f, -w * 0.3f, oh * 0.42f, -w * 0.52f, -h * 0.42f);
    eye.closeSubPath();
    eye.applyTransform (xf);

    // heat haze around the socket
    juce::DropShadow (blood.withAlpha (0.9f * heat), (int) (w * 0.45f), {}).drawForPath (g, eye);
    juce::DropShadow (blood.withAlpha (0.8f * heat), (int) (w * 0.22f), {}).drawForPath (g, eye);
    juce::DropShadow (ember.withAlpha (0.8f * heat), (int) (w * 0.08f), {}).drawForPath (g, eye);

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (eye);
        const auto core = c + juce::Point<float> (lookX * w * 0.16f, h * 0.08f + lookY * h * 0.2f);
        const float R = w * 0.5f;
        juce::ColourGradient grad (juce::Colour (0xfffffbe8).withMultipliedBrightness (0.55f + 0.45f * heat), core.x, core.y,
                                   juce::Colour (0xff120000), core.x + R * 1.1f, core.y, true);
        grad.addColour (0.10, juce::Colour (0xffffe27a).withMultipliedBrightness (0.5f + 0.5f * heat));
        grad.addColour (0.32, juce::Colour (0xffff7410).withMultipliedBrightness (0.45f + 0.55f * heat));
        grad.addColour (0.62, juce::Colour (0xffc40812));
        grad.addColour (0.85, juce::Colour (0xff4a0004));
        g.setGradientFill (grad);
        g.fillPath (eye);

        // molten streaks
        juce::Random streakRnd (mirror ? 5 : 9);
        g.setColour (juce::Colour (0xff5a0000).withAlpha (0.5f));
        for (int i = 0; i < 12; ++i)
        {
            const float a = streakRnd.nextFloat() * juce::MathConstants<float>::twoPi;
            const float r0 = R * (0.18f + streakRnd.nextFloat() * 0.1f), r1 = R * (0.6f + streakRnd.nextFloat() * 0.5f);
            g.drawLine (core.x + std::cos (a) * r0, core.y + std::sin (a) * r0 * 0.6f, core.x + std::cos (a) * r1, core.y + std::sin (a) * r1 * 0.6f, 0.9f);
        }

        // a small black void at the centre of the fire, ringed by a burning rim
        const float pr = w * (0.045f + 0.025f * (1.0f - dilate));
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a0000).withAlpha (0.0f), core.x, core.y,
                                                 juce::Colour (0xffff3000).withAlpha (0.0f), core.x + pr * 2.4f, core.y, true));
        juce::ColourGradient rim (juce::Colour (0xff000000), core.x, core.y, juce::Colour (0x00ff2000), core.x + pr * 2.4f, core.y, true);
        rim.addColour (0.38, juce::Colour (0xff000000));
        rim.addColour (0.5, juce::Colour (0xffb00000));
        rim.addColour (0.7, juce::Colour (0x80ff3000));
        g.setGradientFill (rim);
        g.fillEllipse (core.x - pr * 2.4f, core.y - pr * 2.4f, pr * 4.8f, pr * 4.8f);

        // lids burn darker at the edges
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.85f), c.x, c.y - oh * 0.9f,
                                                 juce::Colours::transparentBlack, c.x, c.y - oh * 0.05f, false));
        g.fillPath (eye);
    }
    g.setColour (juce::Colours::black); g.strokePath (eye, juce::PathStrokeType (2.5f));

    // heavy, knotted brow bearing down on the eye, sloping hard toward the nose
    {
        juce::Path brow;
        brow.startNewSubPath (-w * 0.7f, -h * 0.55f);
        brow.cubicTo (-w * 0.3f, -h * 0.45f - h * 0.35f, w * 0.15f, -h * 0.55f, w * 0.6f, h * 0.22f);   // lower edge, hugging the lid
        brow.cubicTo (w * 0.45f, -h * 0.5f, -w * 0.1f, -h * 1.25f, -w * 0.62f, -h * 1.05f);              // top of the ridge
        brow.quadraticTo (-w * 0.78f, -h * 0.85f, -w * 0.7f, -h * 0.55f);
        brow.closeSubPath();
        brow.applyTransform (xf);
        juce::ColourGradient bg (juce::Colour (0xff2a1210), c.x - w * 0.2f * sx, c.y - h * 1.1f, juce::Colour (0xff030101), c.x + w * 0.2f * sx, c.y, false);
        g.setGradientFill (bg);
        g.fillPath (brow);
        g.setColour (juce::Colours::black); g.strokePath (brow, juce::PathStrokeType (1.0f));

        juce::Path edge; // fire catching the underside of the brow
        edge.startNewSubPath (-w * 0.7f, -h * 0.55f);
        edge.cubicTo (-w * 0.3f, -h * 0.45f - h * 0.35f, w * 0.15f, -h * 0.55f, w * 0.6f, h * 0.22f);
        edge.applyTransform (xf);
        g.setColour (bloodHot.withAlpha (0.3f + 0.55f * heat)); g.strokePath (edge, juce::PathStrokeType (1.6f));

    }
}

void DemonEyes::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float ew = b.getWidth() * 0.31f, eh = std::min (b.getHeight() * 0.36f, ew * 0.5f);
    const float cy = b.getCentreY() + eh * 0.35f, dx = b.getWidth() * 0.235f;
    drawEye (g, { b.getCentreX() - dx, cy }, ew, eh, 0.1f, false);
    drawEye (g, { b.getCentreX() + dx, cy }, ew, eh, 0.1f, true);

    // deep angry furrow between the brows
    const float fx = b.getCentreX();
    for (int i = -1; i <= 1; i += 2)
    {
        juce::Path f;
        f.startNewSubPath (fx + (float) i * ew * 0.05f, cy - eh * 1.25f);
        f.quadraticTo (fx + (float) i * ew * 0.13f, cy - eh * 0.55f, fx + (float) i * ew * 0.07f, cy + eh * 0.05f);
        g.setColour (juce::Colours::black.withAlpha (0.85f)); g.strokePath (f, juce::PathStrokeType (2.0f));
        g.setColour (colours::blood.withAlpha (0.25f * glow)); g.strokePath (f, juce::PathStrokeType (0.7f), juce::AffineTransform::translation ((float) i, 0));
    }
}

//==============================================================================
PossessedEditor::PossessedEditor (PossessedProcessor& p)
    : AudioProcessorEditor (&p), proc (p)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (eyes);
    eyes.setInterceptsMouseClicks (false, false);

    auto sec = [&] (const juce::String& title, std::vector<Knob*> ks, juce::ComboBox* combo = nullptr)
    { sections.push_back ({ title, std::move (ks), {}, combo }); };

    sec ("VOICE",   { makeKnob ("pitch", "Pitch", true), makeKnob ("formant", "Formant", true), makeKnob ("whisper", "Whisper") });
    sec ("LAYER",   { makeKnob ("layerPitch", "Pitch"), makeKnob ("layerFormant", "Formant"), makeKnob ("layerLevel", "Level") });
    sec ("DEMON",   { makeKnob ("growl", "Growl", true), makeKnob ("rasp", "Rasp", true) });
    sec ("RING MOD", { makeKnob ("ringMix", "Mix"), makeKnob ("ringFreq", "Freq") });
    sec ("DISTORT", { makeKnob ("distDrive", "Drive") }, &distTypeBox);
    sec ("FILTER",  { makeKnob ("cutoff", "Cutoff"), makeKnob ("resonance", "Reso") }, &filterTypeBox);
    sec ("WOBBLE",  { makeKnob ("wobble", "Warble") });
    sec ("IN / OUT", { makeKnob ("inputDb", "Input"), makeKnob ("mix", "Dry/Wet"), makeKnob ("outputDb", "Output") });
    sec ("LEGION",  { makeKnob ("legion", "Voices"), makeKnob ("legionSpread", "Spread") });
    sec ("REVERSE", { makeKnob ("reverseMix", "Mix"), makeKnob ("reverseLength", "Length") });
    sec ("DELAY",   { makeKnob ("delayMix", "Mix"), makeKnob ("delayTime", "Time"), makeKnob ("delayFeedback", "Feedback") }, &delayTypeBox);
    sec ("REVERB",  { makeKnob ("reverbMix", "Mix"), makeKnob ("reverbSize", "Size") });
    sec ("LO-FI",   { makeKnob ("lofi", "Lo-Fi"), makeKnob ("lofiNoise", "Noise") });

    presetBox.onChange = [this]
    {
        const int id = presetBox.getSelectedId();
        if (id >= kUserIdBase && id - kUserIdBase < userFiles.size())
        {
            if (! proc.loadUserPreset (userFiles[id - kUserIdBase]))
                showMessage ("Couldn't load preset", "That preset file is missing or damaged.");
        }
        else if (id >= 1 && id <= proc.getNumPrograms())
            proc.setCurrentProgram (id - 1);
    };
    addAndMakeVisible (presetBox);
    prevButton.onClick = [this] { stepPreset (-1); };
    nextButton.onClick = [this] { stepPreset (1); };
    saveButton.onClick = [this] { promptSave(); };
    deleteButton.onClick = [this] { confirmDelete(); };
    for (auto* b : { &prevButton, &nextButton, &saveButton, &deleteButton }) addAndMakeVisible (*b);
    refreshPresetBox();

    distTypeBox.addItemList ({ "Tube", "Fuzz", "Bitcrush", "Radio", "Saturate" }, 1);
    delayTypeBox.addItemList ({ "Digital", "Ping-Pong", "Analog", "Tape" }, 1);
    addAndMakeVisible (delayTypeBox);
    delayTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "delayType", delayTypeBox);
    filterTypeBox.addItemList ({ "Off", "Low-pass", "Band-pass", "High-pass" }, 1);
    addAndMakeVisible (distTypeBox); addAndMakeVisible (filterTypeBox);
    distTypeAtt   = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "distType", distTypeBox);
    filterTypeAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "filterType", filterTypeBox);

    setSize (1060, 720);
    startTimerHz (30);
}

PossessedEditor::~PossessedEditor() { setLookAndFeel (nullptr); }

PossessedEditor::Knob* PossessedEditor::makeKnob (const juce::String& id, const juce::String& text, bool big)
{
    auto k = std::make_unique<Knob>();
    k->big = big;
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 16);
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    k->slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    k->label.setText (text, juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (font (big ? 14.0f : 12.0f, big));
    if (big) k->label.setColour (juce::Label::textColourId, colours::bone);
    addAndMakeVisible (k->slider); addAndMakeVisible (k->label);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

void PossessedEditor::layoutSection (Section& s, juce::Rectangle<int> area)
{
    s.bounds = area;
    if (s.combo != nullptr) s.combo->setBounds (area.getRight() - 92, area.getY() + 5, 84, 20);
    auto inner = area.reduced (8).withTrimmedTop (20);
    if (s.title == "IN / OUT") { meterArea = inner.removeFromRight (40); inner.removeFromRight (4); }
    if (s.knobs.empty()) return;
    const int w = inner.getWidth() / (int) s.knobs.size();
    for (auto* k : s.knobs)
    {
        auto cell = inner.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell);
    }
}

void PossessedEditor::resized()
{
    auto r = getLocalBounds().reduced (12);
    auto header = r.removeFromTop (150);
    auto right = header.removeFromRight (300);
    auto presetRow = right.withSizeKeepingCentre (300, 28).translated (0, 14);
    prevButton.setBounds (presetRow.removeFromLeft (28));
    nextButton.setBounds (presetRow.removeFromRight (28));
    presetBox.setBounds (presetRow.reduced (4, 0));
    auto buttonRow = juce::Rectangle<int> (presetBox.getX(), presetBox.getBottom() + 8, presetBox.getWidth(), 24);
    deleteButton.setBounds (buttonRow.removeFromRight (70));
    buttonRow.removeFromRight (6);
    saveButton.setBounds (buttonRow);
    eyes.setBounds (header.withSizeKeepingCentre (360, 150).translated (95, 0));
    r.removeFromTop (6);

    const int gap = 8;
    const int rowH = (r.getHeight() - 2 * gap) / 3;
    auto split = [gap] (juce::Rectangle<int> row, std::vector<float> weights)
    {
        std::vector<juce::Rectangle<int>> out; float total = 0; for (auto w : weights) total += w;
        const int avail = row.getWidth() - gap * (int) (weights.size() - 1);
        for (size_t i = 0; i < weights.size(); ++i)
        {
            const int w = i + 1 == weights.size() ? row.getWidth() : (int) ((float) avail * weights[i] / total);
            out.push_back (row.removeFromLeft (w));
            row.removeFromLeft (gap);
        }
        return out;
    };

    auto row1 = r.removeFromTop (rowH); r.removeFromTop (gap);
    auto row2 = r.removeFromTop (rowH); r.removeFromTop (gap);
    auto row3 = r;
    auto a = split (row1, { 3.2f, 3.0f, 2.2f });
    for (int i = 0; i < 3; ++i) layoutSection (sections[(size_t) i], a[(size_t) i]);
    auto b = split (row2, { 1.9f, 1.8f, 2.3f, 1.0f, 3.4f });
    for (int i = 0; i < 5; ++i) layoutSection (sections[(size_t) (3 + i)], b[(size_t) i]);
    auto c = split (row3, { 2.0f, 2.0f, 3.0f, 2.0f, 2.0f });
    for (int i = 0; i < 5; ++i) layoutSection (sections[(size_t) (8 + i)], c[(size_t) i]);

    buildBackground();
}

void PossessedEditor::buildBackground()
{
    using namespace colours;
    const int W = std::max (1, getWidth()), H = std::max (1, getHeight());
    background = juce::Image (juce::Image::ARGB, W, H, true);
    juce::Graphics g (background);
    juce::Random rnd (666);
    g.fillAll (bg);

    // hellfire glow from below, ember haze behind the eyes
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x70500000), (float) W * 0.5f, (float) H * 1.05f,
                                             juce::Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.35f, true));
    g.fillRect (0, 0, W, H);
    const auto ec = eyes.getBounds().getCentre().toFloat();
    g.setGradientFill (juce::ColourGradient (juce::Colour (0x40800000), ec.x, ec.y, juce::Colours::transparentBlack, ec.x + 260, ec.y, true));
    g.fillRect (0, 0, W, H);

    // faint summoning sigil behind the eyes
    {
        const float R = 118.0f;
        g.setColour (blood.withAlpha (0.13f));
        g.drawEllipse (ec.x - R, ec.y - R, R * 2, R * 2, 1.4f);
        g.drawEllipse (ec.x - R * 0.86f, ec.y - R * 0.86f, R * 1.72f, R * 1.72f, 0.8f);
        juce::Path star;
        for (int i = 0; i < 5; ++i)
        {
            // inverted star: first point at the bottom
            const float a2 = juce::MathConstants<float>::halfPi + (float) (i * 2 % 5) * juce::MathConstants<float>::twoPi / 5.0f;
            const juce::Point<float> pt (ec.x + std::cos (a2) * R * 0.86f, ec.y + std::sin (a2) * R * 0.86f);
            if (i == 0) star.startNewSubPath (pt); else star.lineTo (pt);
        }
        star.closeSubPath();
        g.strokePath (star, juce::PathStrokeType (0.9f));
        for (int i = 0; i < 40; ++i) // rune ticks
        {
            const float ang = (float) i / 40.0f * juce::MathConstants<float>::twoPi;
            const float r0 = R * 0.89f, r1 = R * (i % 5 == 0 ? 0.99f : 0.95f);
            g.drawLine (ec.x + std::cos (ang) * r0, ec.y + std::sin (ang) * r0, ec.x + std::cos (ang) * r1, ec.y + std::sin (ang) * r1, 1.0f);
        }
    }

    // cracks creeping in from the edges
    for (int c = 0; c < 14; ++c)
    {
        juce::Path crack;
        const int side = c % 4;
        juce::Point<float> pnt (side == 0 ? 0.0f : side == 1 ? (float) W : rnd.nextFloat() * (float) W,
                                side == 2 ? 0.0f : side == 3 ? (float) H : rnd.nextFloat() * (float) H);
        float ang = side == 0 ? 0.0f : side == 1 ? juce::MathConstants<float>::pi : side == 2 ? juce::MathConstants<float>::halfPi : -juce::MathConstants<float>::halfPi;
        crack.startNewSubPath (pnt);
        const int segs = 6 + rnd.nextInt (10);
        for (int s = 0; s < segs; ++s)
        {
            ang += (rnd.nextFloat() - 0.5f) * 1.1f;
            pnt += { std::cos (ang) * (8.0f + rnd.nextFloat() * 22.0f), std::sin (ang) * (8.0f + rnd.nextFloat() * 22.0f) };
            crack.lineTo (pnt);
        }
        g.setColour (juce::Colours::black.withAlpha (0.8f)); g.strokePath (crack, juce::PathStrokeType (2.2f));
        g.setColour (bloodHot.withAlpha (0.10f + rnd.nextFloat() * 0.12f)); g.strokePath (crack, juce::PathStrokeType (0.8f));
    }

    // grime / grain
    for (int i = 0; i < W * H / 60; ++i)
    {
        g.setColour (rnd.nextBool() ? juce::Colours::black.withAlpha (0.25f) : juce::Colour (0xff5a1a10).withAlpha (0.08f));
        g.fillRect ((float) rnd.nextInt (W), (float) rnd.nextInt (H), 1.0f + rnd.nextFloat(), 1.0f + rnd.nextFloat());
    }

    // vignette
    g.setGradientFill (juce::ColourGradient (juce::Colours::transparentBlack, (float) W * 0.5f, (float) H * 0.5f,
                                             juce::Colours::black.withAlpha (0.8f), 0, 0, true));
    g.fillRect (0, 0, W, H);

    // section panels
    for (auto& s : sections)
    {
        auto rb = s.bounds.toFloat();
        g.setColour (panel); g.fillRoundedRectangle (rb, 5.0f);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0x18ff2000), rb.getX(), rb.getY(), juce::Colours::transparentBlack, rb.getX(), rb.getY() + 30, false));
        g.fillRoundedRectangle (rb, 5.0f);
        const bool hot = s.title == "VOICE" || s.title == "DEMON" || s.title == "LO-FI";
        g.setColour (hot ? blood.withAlpha (0.75f) : edge); g.drawRoundedRectangle (rb, 5.0f, 1.0f);
        g.setColour (hot ? bloodHot : ash); g.setFont (font (11.0f, true));
        g.drawText (s.title, s.bounds.reduced (10, 5).removeFromTop (16), juce::Justification::centredLeft);
        // a scratch across the corner
        g.setColour (juce::Colours::black.withAlpha (0.6f));
        g.drawLine (rb.getRight() - 30 - rnd.nextFloat() * 30, rb.getBottom() - 4, rb.getRight() - 6, rb.getBottom() - 18 - rnd.nextFloat() * 10, 1.0f);
    }
}

void PossessedEditor::paint (juce::Graphics& g)
{
    using namespace colours;
    g.drawImageAt (background, 0, 0);

    // ---- blood-drip title
    const float alpha = juce::jlimit (0.4f, 1.0f, 1.0f - flicker * 0.7f);
    juce::GlyphArrangement ga;
    ga.addLineOfText (font (50.0f, true), "POSSESSED", 22.0f, 66.0f);
    juce::Path title; ga.createPath (title);
    const auto tb = title.getBounds();
    juce::DropShadow (blood.withAlpha (0.7f * alpha), 16, {}).drawForPath (g, title);
    g.setGradientFill (juce::ColourGradient (bloodHot.withAlpha (alpha), 0, tb.getY(), juce::Colour (0xff5a0006).withAlpha (alpha), 0, tb.getBottom(), false));
    g.fillPath (title);

    juce::Random drip (13);
    g.setColour (juce::Colour (0xff7a0008).withAlpha (alpha));
    for (int i = 0; i < 9; ++i)
    {
        const float x = tb.getX() + 6 + drip.nextFloat() * (tb.getWidth() - 12);
        const float len = 6.0f + drip.nextFloat() * 22.0f, wdt = 1.6f + drip.nextFloat() * 1.8f;
        g.fillRoundedRectangle (x - wdt * 0.5f, tb.getBottom() - 3, wdt, len, wdt * 0.5f);
        g.fillEllipse (x - wdt, tb.getBottom() - 3 + len - wdt, wdt * 2.0f, wdt * 2.2f);
    }

    g.setColour (ash); g.setFont (font (12.0f));
    g.drawText ("Pure Vocal Destruction  v" JucePlugin_VersionString, 24, 100, 300, 16, juce::Justification::centredLeft);
    g.setColour (bone); g.setFont (font (12.0f, true).withExtraKerningFactor (0.18f));
    g.drawText ("CASTLE DANGER MUSIC COMPANY", 24, 120, 300, 18, juce::Justification::centredLeft, false);

    g.setColour (ash); g.setFont (font (11.0f, true));
    g.drawText ("PRESET", presetBox.getX(), presetBox.getY() - 20, 120, 16, juce::Justification::centredLeft);
}

void PossessedEditor::paintOverChildren (juce::Graphics& g)
{
    using namespace colours;
    if (meterArea.isEmpty()) return;
    auto m = meterArea.toFloat().reduced (0, 6);
    const float bw = (m.getWidth() - 4) * 0.5f;
    auto drawBar = [&] (juce::Rectangle<float> bar, float lvl, const char* lbl)
    {
        g.setColour (juce::Colour (0xff0a0505)); g.fillRoundedRectangle (bar.withTrimmedBottom (14), 2.0f);
        const float db = juce::Decibels::gainToDecibels (lvl, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 60.0f) / 60.0f);
        auto inner = bar.withTrimmedBottom (14).reduced (2);
        auto fill = inner.removeFromBottom (inner.getHeight() * frac);
        g.setGradientFill (juce::ColourGradient (blood, 0, inner.getBottom() + fill.getHeight(), ember, 0, bar.getY(), false));
        g.fillRect (fill);
        g.setColour (ash); g.setFont (font (10.0f, true));
        g.drawText (lbl, bar.removeFromBottom (12).expanded (6, 0), juce::Justification::centred, false);
    };
    drawBar (m.removeFromLeft (bw), inMeter, "IN");
    m.removeFromLeft (4);
    drawBar (m.removeFromLeft (bw), outMeter, "OUT");
}

void PossessedEditor::timerCallback()
{
    const float in = proc.inLevel.exchange (0.0f), out = proc.outLevel.exchange (0.0f);
    inMeter = std::max (in, inMeter * 0.85f);
    outMeter = std::max (out, outMeter * 0.85f);
    eyes.update (outMeter, 1.0f / 30.0f);

    auto& rnd = juce::Random::getSystemRandom();
    flicker = (rnd.nextFloat() < 0.04f + outMeter * 0.3f) ? rnd.nextFloat() : flicker * 0.5f;

    if (proc.presetVersion.load() != lastPresetVersion) refreshPresetBox();
    repaint (0, 0, 380, 150);
    if (! meterArea.isEmpty()) repaint (meterArea);
}

//==============================================================================
void PossessedEditor::refreshPresetBox()
{
    lastPresetVersion = proc.presetVersion.load();
    userFiles = PossessedProcessor::getUserPresets();
    const auto current = proc.getCurrentPresetName();

    presetBox.clear (juce::dontSendNotification);
    presetBox.addSectionHeading ("Factory");
    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.addSeparator();
    presetBox.addSectionHeading ("User Presets");
    if (userFiles.isEmpty()) { presetBox.addItem ("(none saved yet)", 999); presetBox.setItemEnabled (999, false); }
    for (int i = 0; i < userFiles.size(); ++i) presetBox.addItem (userFiles[i].getFileNameWithoutExtension(), kUserIdBase + i);

    int selected = 0;
    for (int i = 0; i < userFiles.size() && selected == 0; ++i)
        if (userFiles[i] == PossessedProcessor::fileForPresetName (current)) selected = kUserIdBase + i;
    for (int i = 0; i < proc.getNumPrograms() && selected == 0; ++i)
        if (proc.getProgramName (i) == current) selected = i + 1;
    if (selected != 0) presetBox.setSelectedId (selected, juce::dontSendNotification);
    else presetBox.setText (current, juce::dontSendNotification);
    deleteButton.setEnabled (selected >= kUserIdBase);
}

void PossessedEditor::stepPreset (int delta)
{
    std::vector<int> ids;
    for (int i = 0; i < proc.getNumPrograms(); ++i) ids.push_back (i + 1);
    for (int i = 0; i < userFiles.size(); ++i) ids.push_back (kUserIdBase + i);
    auto it = std::find (ids.begin(), ids.end(), presetBox.getSelectedId());
    const int pos = it == ids.end() ? 0 : (int) (it - ids.begin());
    const int n = (int) ids.size();
    presetBox.setSelectedId (ids[(size_t) (((pos + delta) % n + n) % n)], juce::sendNotificationSync);
}

void PossessedEditor::showDialog (const juce::String& title, const juce::String& message, bool withTextField, const juce::String& initialText,
                                  juce::StringArray buttons, std::function<void (int, const juce::String&)> onResult)
{
    juce::Component::SafePointer<PossessedEditor> safe (this);
    dialog = std::make_unique<PresetDialog> (title, message, withTextField, initialText, buttons,
        [safe, onResult] (int index, const juce::String& text)
        {
            // close first, then act (the result may open another dialog)
            juce::MessageManager::callAsync ([safe, onResult, index, text]
            {
                if (safe == nullptr) return;
                safe->dialog.reset();
                if (onResult) onResult (index, text);
            });
        });
    addAndMakeVisible (*dialog);
    dialog->setBounds (getLocalBounds());
}

void PossessedEditor::showMessage (const juce::String& title, const juce::String& text)
{
    showDialog (title, text, false, {}, { "OK" }, {});
}

void PossessedEditor::promptSave()
{
    const bool onUser = presetBox.getSelectedId() >= kUserIdBase;
    juce::Component::SafePointer<PossessedEditor> safe (this);
    auto doSave = [safe] (const juce::String& name)
    {
        if (safe == nullptr) return;
        if (! safe->proc.saveUserPreset (name))
            safe->showMessage ("Couldn't save preset", "Possessed couldn't write the preset file to:\n"
                                                         + PossessedProcessor::getUserPresetFolder().getFullPathName());
        else safe->refreshPresetBox();
    };

    showDialog ("SAVE PRESET", "Name your preset:", true, onUser ? proc.getCurrentPresetName() : juce::String(), { "Save", "Cancel" },
        [safe, doSave] (int button, const juce::String& text)
        {
            if (button != 0 || safe == nullptr) return;
            const auto name = text.trim();
            if (name.isEmpty()) { safe->showMessage ("SAVE PRESET", "Please enter a name for the preset."); return; }
            for (int i = 0; i < safe->proc.getNumPrograms(); ++i)
                if (safe->proc.getProgramName (i).equalsIgnoreCase (name))
                {
                    safe->showMessage ("SAVE PRESET", "\"" + name + "\" is a factory preset name. Please choose another name.");
                    return;
                }
            if (PossessedProcessor::fileForPresetName (name).existsAsFile())
            {
                safe->showDialog ("REPLACE PRESET?", "A preset named \"" + name + "\" already exists. Replace it with the current settings?",
                                  false, {}, { "Replace", "Cancel" },
                                  [doSave, name] (int b, const juce::String&) { if (b == 0) doSave (name); });
                return;
            }
            doSave (name);
        });
}

void PossessedEditor::confirmDelete()
{
    const int id = presetBox.getSelectedId();
    if (id < kUserIdBase || id - kUserIdBase >= userFiles.size()) return;
    const auto file = userFiles[id - kUserIdBase];
    juce::Component::SafePointer<PossessedEditor> safe (this);
    showDialog ("DELETE PRESET?", "Delete \"" + file.getFileNameWithoutExtension() + "\"? This can't be undone.", false, {},
                { "Delete", "Cancel" }, [safe, file] (int b, const juce::String&)
                {
                    if (b != 0 || safe == nullptr) return;
                    file.deleteFile();
                    safe->refreshPresetBox();
                });
}

//==============================================================================
PresetDialog::PresetDialog (const juce::String& t, const juce::String& m, bool withTextField, const juce::String& initialText,
                            juce::StringArray names, std::function<void (int, const juce::String&)> cb)
    : title (t), message (m), hasField (withTextField), onClose (std::move (cb))
{
    setWantsKeyboardFocus (true);
    if (hasField)
    {
        field.setText (initialText, false);
        field.setInputRestrictions (60);
        field.setSelectAllWhenFocused (true);
        field.setFont (font (15.0f));
        field.setIndents (8, 6);
        field.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff070303));
        field.setColour (juce::TextEditor::outlineColourId, colours::edge);
        field.setColour (juce::TextEditor::focusedOutlineColourId, colours::bloodHot);
        field.setColour (juce::TextEditor::textColourId, colours::bone);
        field.setColour (juce::TextEditor::highlightColourId, colours::blood.withAlpha (0.6f));
        field.setColour (juce::CaretComponent::caretColourId, colours::bloodHot);
        field.onReturnKey = [this] { finish (0); };
        field.onEscapeKey = [this] { finish ((int) buttons.size() - 1); };
        addAndMakeVisible (field);
    }
    for (int i = 0; i < names.size(); ++i)
    {
        auto* b = buttons.add (new juce::TextButton (names[i]));
        b->onClick = [this, i] { finish (i); };
        addAndMakeVisible (b);
    }
}

void PresetDialog::parentHierarchyChanged()
{
    if (isShowing())
        juce::MessageManager::callAsync ([sp = juce::Component::SafePointer<PresetDialog> (this)]
        {
            if (sp == nullptr) return;
            if (sp->hasField) sp->field.grabKeyboardFocus(); else sp->grabKeyboardFocus();
        });
}

void PresetDialog::finish (int index)
{
    if (onClose == nullptr) return;
    auto cb = std::move (onClose);
    onClose = nullptr;
    cb (index, field.getText());
}

bool PresetDialog::keyPressed (const juce::KeyPress& k)
{
    if (k == juce::KeyPress::returnKey) { finish (0); return true; }
    if (k == juce::KeyPress::escapeKey) { finish ((int) buttons.size() - 1); return true; }
    return true; // keep keys from leaking to the host while the dialog is open
}

void PresetDialog::resized()
{
    panel = getLocalBounds().withSizeKeepingCentre (380, hasField ? 190 : 160);
    auto r = panel.reduced (20);
    r.removeFromTop (30); // title
    r.removeFromTop (hasField ? 22 : 50); // message
    if (hasField) { field.setBounds (r.removeFromTop (32)); r.removeFromTop (16); }
    auto row = r.removeFromBottom (28);
    for (int i = (int) buttons.size() - 1; i >= 0; --i) { buttons[i]->setBounds (row.removeFromRight (96)); row.removeFromRight (8); }
}

void PresetDialog::paint (juce::Graphics& g)
{
    using namespace colours;
    g.fillAll (juce::Colours::black.withAlpha (0.72f));
    auto p = panel.toFloat();
    juce::Path shape; shape.addRoundedRectangle (p, 6.0f);
    juce::DropShadow (blood.withAlpha (0.6f), 28, {}).drawForPath (g, shape);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff1c0c0b), p.getX(), p.getY(), juce::Colour (0xff080404), p.getX(), p.getBottom(), false));
    g.fillPath (shape);
    g.setColour (blood); g.strokePath (shape, juce::PathStrokeType (1.2f));
    auto r = panel.reduced (20);
    g.setColour (bloodHot); g.setFont (font (16.0f, true).withExtraKerningFactor (0.12f));
    g.drawText (title, r.removeFromTop (30), juce::Justification::centredLeft);
    g.setColour (bone); g.setFont (font (13.0f));
    g.drawFittedText (message, r.removeFromTop (hasField ? 22 : 50), juce::Justification::topLeft, 3);
}
