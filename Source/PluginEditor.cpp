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
    const float oh = h * std::max (0.03f, openness);

    // almond with a sharp, angry inner corner (inner corner = +x for the left eye)
    juce::Path eye;
    eye.startNewSubPath (-w * 0.5f, -h * 0.05f);
    eye.cubicTo (-w * 0.25f, -oh * 1.05f, w * 0.22f, -oh * 0.95f, w * 0.5f, oh * 0.18f);
    eye.cubicTo (w * 0.2f, oh * 0.75f, -w * 0.25f, oh * 0.7f, -w * 0.5f, -h * 0.05f);
    eye.closeSubPath();
    const auto xf = juce::AffineTransform::scale (sx, 1.0f).rotated (tilt * sx).translated (c);
    eye.applyTransform (xf);

    // brow shadow / ridge
    {
        juce::Path brow;
        brow.startNewSubPath (-w * 0.65f, -h * 0.55f);
        brow.quadraticTo (-w * 0.1f, -h * 1.25f, w * 0.62f, -h * 0.25f);
        brow.quadraticTo (w * 0.1f, -h * 0.75f, -w * 0.65f, -h * 0.55f);
        brow.closeSubPath();
        brow.applyTransform (xf);
        g.setColour (juce::Colour (0xff000000).withAlpha (0.85f)); g.fillPath (brow);
        g.setColour (blood.withAlpha (0.15f + 0.25f * glow)); g.strokePath (brow, juce::PathStrokeType (1.0f));
    }

    // outer glow
    const float gl = glow * std::sqrt (openness);
    juce::DropShadow (blood.withAlpha (0.55f * gl), (int) (w * 0.55f), {}).drawForPath (g, eye);
    juce::DropShadow (ember.withAlpha (0.45f * gl), (int) (w * 0.18f), {}).drawForPath (g, eye);

    {
        juce::Graphics::ScopedSaveState ss (g);
        g.reduceClipRegion (eye);
        const auto iris = c + juce::Point<float> (lookX * w * 0.17f, lookY * h * 0.3f);
        const float ir = w * 0.36f;
        juce::ColourGradient grad (juce::Colour (0xfffff3a0).withMultipliedBrightness (0.6f + 0.4f * glow), iris.x, iris.y,
                                   juce::Colour (0xff1a0000), iris.x + ir * 1.3f, iris.y, true);
        grad.addColour (0.18, juce::Colour (0xffffc23a).withMultipliedBrightness (0.55f + 0.45f * glow));
        grad.addColour (0.45, juce::Colour (0xffff5a0a).withMultipliedBrightness (0.5f + 0.5f * glow));
        grad.addColour (0.75, juce::Colour (0xffa00008));
        g.setGradientFill (grad);
        g.fillPath (eye);

        // veins radiating from the pupil
        g.setColour (juce::Colour (0xff3a0000).withAlpha (0.55f));
        for (int i = 0; i < 18; ++i)
        {
            const float a = (float) i / 18.0f * juce::MathConstants<float>::twoPi + 0.3f;
            const float r0 = ir * (0.25f + 0.05f * (float) (i % 3)), r1 = ir * (0.85f + 0.15f * (float) ((i * 7) % 4) / 3.0f);
            g.drawLine (iris.x + std::cos (a) * r0, iris.y + std::sin (a) * r0, iris.x + std::cos (a) * r1, iris.y + std::sin (a) * r1, 0.8f);
        }

        // slit pupil
        const float pw = w * (0.05f + 0.11f * dilate), ph = h * 1.7f;
        juce::Path pupil; pupil.addEllipse (-pw * 0.5f, -ph * 0.5f, pw, ph);
        pupil.applyTransform (juce::AffineTransform::rotation (tilt * sx * 0.3f).translated (iris));
        juce::DropShadow (juce::Colour (0xff300000), 6, {}).drawForPath (g, pupil);
        g.setColour (juce::Colours::black); g.fillPath (pupil);

        // wet highlight
        g.setColour (juce::Colours::white.withAlpha (0.35f + 0.3f * glow));
        g.fillEllipse (iris.x - ir * 0.42f, iris.y - h * 0.32f, w * 0.06f, w * 0.045f);

        // upper lid shadow
        g.setGradientFill (juce::ColourGradient (juce::Colours::black.withAlpha (0.75f), c.x, c.y - oh, juce::Colours::transparentBlack, c.x, c.y - oh * 0.2f, false));
        g.fillPath (eye);
    }
    g.setColour (juce::Colours::black); g.strokePath (eye, juce::PathStrokeType (2.0f));
}

void DemonEyes::paint (juce::Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const float ew = b.getWidth() * 0.34f, eh = std::min (b.getHeight() * 0.32f, ew * 0.42f);
    const float cy = b.getCentreY() + eh * 0.2f;
    drawEye (g, { b.getCentreX() - b.getWidth() * 0.22f, cy }, ew, eh, 0.22f, false);
    drawEye (g, { b.getCentreX() + b.getWidth() * 0.22f, cy }, ew, eh, 0.22f, true);
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
    sec ("DELAY",   { makeKnob ("delayMix", "Mix"), makeKnob ("delayTime", "Time"), makeKnob ("delayFeedback", "Feedback") });
    sec ("REVERB",  { makeKnob ("reverbMix", "Mix"), makeKnob ("reverbSize", "Size") });

    for (int i = 0; i < proc.getNumPrograms(); ++i) presetBox.addItem (proc.getProgramName (i), i + 1);
    presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    presetBox.onChange = [this] { proc.setCurrentProgram (presetBox.getSelectedId() - 1); };
    addAndMakeVisible (presetBox);
    auto step = [this] (int d)
    {
        const int n = proc.getNumPrograms();
        proc.setCurrentProgram ((proc.getCurrentProgram() + d + n) % n);
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    };
    prevButton.onClick = [step] { step (-1); };
    nextButton.onClick = [step] { step (1); };
    addAndMakeVisible (prevButton); addAndMakeVisible (nextButton);

    distTypeBox.addItemList ({ "Tube", "Fuzz", "Bitcrush", "Radio" }, 1);
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
    auto c = split (row3, { 2.0f, 2.0f, 3.0f, 2.0f });
    for (int i = 0; i < 4; ++i) layoutSection (sections[(size_t) (8 + i)], c[(size_t) i]);

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
        const bool hot = s.title == "VOICE" || s.title == "DEMON";
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
    g.drawText ("evil voice transformer  v" JucePlugin_VersionString, 24, 100, 300, 16, juce::Justification::centredLeft);
    g.setColour (bone); g.setFont (font (13.0f, true));
    g.drawText ("C A S T L E   D A N G E R   M C", 24, 120, 320, 18, juce::Justification::centredLeft);

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

    if (presetBox.getSelectedId() != proc.getCurrentProgram() + 1)
        presetBox.setSelectedId (proc.getCurrentProgram() + 1, juce::dontSendNotification);
    repaint (0, 0, 380, 150);
    if (! meterArea.isEmpty()) repaint (meterArea);
}
