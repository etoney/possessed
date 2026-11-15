#include "PluginEditor.h"

namespace pal
{
const juce::Colour ironDark  { 0xff1d1815 };
const juce::Colour iron      { 0xff3a322c };
const juce::Colour rustDeep  { 0xff4a1d0c };
const juce::Colour rust      { 0xff9a4518 };
const juce::Colour rustLight { 0xffc0702a };
const juce::Colour paint     { 0xffd6c7a6 };   // dirty cream stencil paint
const juce::Colour paintDim  { 0xffa8997c };
const juce::Colour signal    { 0xffd2452a };   // rusty red for active things
const juce::Colour signalHot { 0xffff6a3d };
const juce::Colour hazard    { 0xffc99a22 };   // worn safety yellow
const juce::Colour screen    { 0xff0c0a08 };
const juce::Colour leather   { 0xff4a1a16 };   // worn oxblood leather
const juce::Colour leatherDark { 0xff1c0a08 };
}

//==============================================================================
namespace tex
{
inline float hash (int x, int y, int seed)
{
    uint32_t h = (uint32_t) x * 374761393u + (uint32_t) y * 668265263u + (uint32_t) seed * 2246822519u;
    h = (h ^ (h >> 13)) * 1274126177u;
    h ^= h >> 16;
    return (float) (h & 0xffffffu) / 16777215.0f;
}
inline float smooth (float t) { return t * t * (3.0f - 2.0f * t); }
inline float smoothstep (float a, float b, float x) { return smooth (juce::jlimit (0.0f, 1.0f, (x - a) / (b - a))); }
inline float vnoise (float x, float y, int seed)
{
    const int xi = (int) std::floor (x), yi = (int) std::floor (y);
    const float fx = x - (float) xi, fy = y - (float) yi;
    const float a = hash (xi, yi, seed), b = hash (xi + 1, yi, seed), c = hash (xi, yi + 1, seed), d = hash (xi + 1, yi + 1, seed);
    const float u = smooth (fx), v = smooth (fy);
    return a + (b - a) * u + (c - a) * v + (a - b - c + d) * u * v;
}
inline float fbm (float x, float y, int seed, int octaves)
{
    float s = 0, amp = 0.5f, f = 1, norm = 0;
    for (int i = 0; i < octaves; ++i) { s += amp * vnoise (x * f, y * f, seed + i * 31); norm += amp; f *= 2.03f; amp *= 0.5f; }
    return s / norm;
}
struct RGB { float r, g, b; };
inline RGB rgb (juce::Colour c) { return { c.getFloatRed(), c.getFloatGreen(), c.getFloatBlue() }; }
inline RGB mix (RGB a, RGB b, float t) { return { a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t }; }
inline RGB mul (RGB a, float k) { return { a.r * k, a.g * k, a.b * k }; }

inline RGB rustAt (float x, float y, float rustBias, int seed)
{
    const float n      = fbm (x / 170.0f, y / 170.0f, seed, 4);
    const float detail = fbm (x / 36.0f, y / 36.0f, seed + 7, 3);
    const float fine   = vnoise (x / 7.0f, y / 7.0f, seed + 13);
    const float mask   = smoothstep (0.45f, 0.62f, n + (detail - 0.5f) * 0.35f + rustBias);
    RGB base = mix (rgb (pal::ironDark), rgb (pal::iron), detail);
    RGB r    = mix (rgb (pal::rustDeep), rgb (pal::rust), fbm (x / 20.0f, y / 20.0f, seed + 3, 2));
    if (fine > 0.72f) r = mix (r, rgb (pal::rustLight), (fine - 0.72f) * 2.5f);
    return mix (base, r, mask);
}

// Paint-chip a rendered image: knock out chunks of alpha with noise, plus grain.
inline void distress (juce::Image& img, float scale, float wear, int seed)
{
    juce::Image::BitmapData bd (img, juce::Image::BitmapData::readWrite);
    for (int y = 0; y < bd.height; ++y)
        for (int x = 0; x < bd.width; ++x)
        {
            auto c = bd.getPixelColour (x, y);
            if (c.getAlpha() == 0) continue;
            const float n = fbm ((float) x / (7.0f * scale), (float) y / (7.0f * scale), seed, 3);
            const float chip = n < wear ? 0.08f : 1.0f;
            const float grain = 0.7f + 0.3f * hash (x, y, seed + 99);
            bd.setPixelColour (x, y, c.withMultipliedAlpha (chip * grain));
        }
}

inline juce::Font stencil (float size) { return juce::Font (juce::FontOptions ("Impact", size, juce::Font::plain)); }
inline juce::Font plainBold (float size) { return juce::Font (juce::FontOptions (size, juce::Font::bold)); }

inline juce::Image distressedText (const juce::String& text, int w, int h, juce::Font font, juce::Colour col,
                                   juce::Justification just, float scale, float wear, int seed, float kerning = 0.0f)
{
    juce::Image img (juce::Image::ARGB, juce::roundToInt ((float) w * scale), juce::roundToInt ((float) h * scale), true);
    {
        juce::Graphics g (img);
        g.addTransform (juce::AffineTransform::scale (scale));
        g.setColour (col);
        g.setFont (font.withExtraKerningFactor (kerning));
        g.drawFittedText (text, 0, 0, w, h, just, 1, 0.55f);   // squeeze rather than clip if the font runs wide
    }
    distress (img, scale, wear, seed);
    return img;
}
} // namespace tex

//==============================================================================
// The Castle Danger crest: a castle silhouette under a blood moon, on a riveted iron medallion.
static void drawCastleCrest (juce::Graphics& g, juce::Rectangle<float> r)
{
    const auto c = r.getCentre();
    const float R = std::min (r.getWidth(), r.getHeight()) * 0.5f;

    // iron ring
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (r.translated (1.5f, 2.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a5a4c), c.x - R * 0.6f, c.y - R * 0.8f, juce::Colour (0xff1a1410), c.x + R, c.y + R, true));
    g.fillEllipse (r);
    g.setColour (pal::rust.withAlpha (0.55f)); g.drawEllipse (r.reduced (1.0f), 1.6f);
    for (int i = 0; i < 8; ++i)
    {
        const float a = (float) i / 8.0f * juce::MathConstants<float>::twoPi + 0.39f;
        const float rx = c.x + std::cos (a) * R * 0.88f, ry = c.y + std::sin (a) * R * 0.88f;
        g.setColour (juce::Colour (0xff8a7a68)); g.fillEllipse (rx - 1.8f, ry - 1.8f, 3.6f, 3.6f);
        g.setColour (juce::Colours::black.withAlpha (0.5f)); g.drawEllipse (rx - 1.8f, ry - 1.8f, 3.6f, 3.6f, 0.6f);
    }

    // night sky + blood moon
    const auto inner = r.reduced (R * 0.2f);
    const float ir = inner.getWidth() * 0.5f;
    juce::Graphics::ScopedSaveState ss (g);
    juce::Path clip; clip.addEllipse (inner);
    g.reduceClipRegion (clip);
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff2a0c08), c.x, inner.getY(), juce::Colour (0xff080505), c.x, inner.getBottom(), false));
    g.fillEllipse (inner);
    const juce::Rectangle<float> moon (c.x - ir * 0.55f, inner.getY() + ir * 0.12f, ir * 1.1f, ir * 1.1f);
    juce::DropShadow (pal::signalHot.withAlpha (0.55f), (int) (ir * 0.45f), {}).drawForPath (g, [&] { juce::Path p; p.addEllipse (moon); return p; }());
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xffff7a4a), moon.getX() + moon.getWidth() * 0.35f, moon.getY() + moon.getHeight() * 0.3f,
                                             juce::Colour (0xff8a1408), moon.getRight(), moon.getBottom(), true));
    g.fillEllipse (moon);
    g.setColour (juce::Colour (0x40400000));
    g.fillEllipse (moon.getX() + moon.getWidth() * 0.55f, moon.getY() + moon.getHeight() * 0.25f, moon.getWidth() * 0.16f, moon.getWidth() * 0.16f);
    g.fillEllipse (moon.getX() + moon.getWidth() * 0.25f, moon.getY() + moon.getHeight() * 0.55f, moon.getWidth() * 0.12f, moon.getWidth() * 0.12f);

    // castle silhouette
    const float x0 = inner.getX(), y0 = inner.getY(), w = inner.getWidth(), h = inner.getHeight();
    auto X = [&] (float f) { return x0 + f * w; };
    auto Y = [&] (float f) { return y0 + f * h; };
    juce::Path castle;
    auto tower = [&] (float l, float rgt, float top, int merlons)
    {
        castle.addRectangle (X (l), Y (top), X (rgt) - X (l), Y (1.05f) - Y (top));
        const float mw = (rgt - l) / (float) (merlons * 2 - 1);
        for (int i = 0; i < merlons; ++i)
            castle.addRectangle (X (l + (float) i * 2.0f * mw), Y (top - 0.06f), X (l + mw) - X (l), Y (0.06f) - Y (0.0f) + 0.5f);
    };
    tower (0.08f, 0.30f, 0.50f, 3);           // left tower
    tower (0.70f, 0.92f, 0.50f, 3);           // right tower
    tower (0.38f, 0.62f, 0.36f, 3);           // keep
    tower (0.30f, 0.70f, 0.64f, 5);           // curtain wall
    castle.addTriangle (X (0.08f), Y (0.44f), X (0.19f), Y (0.27f), X (0.30f), Y (0.44f));   // spires
    castle.addTriangle (X (0.70f), Y (0.44f), X (0.81f), Y (0.27f), X (0.92f), Y (0.44f));
    castle.setUsingNonZeroWinding (true);
    g.setColour (juce::Colour (0xff070505)); g.fillPath (castle);
    // gate + lit windows
    juce::Path gate;
    gate.addRectangle (X (0.44f), Y (0.84f), X (0.56f) - X (0.44f), Y (0.2f) - Y (0.0f));
    gate.addEllipse (X (0.44f), Y (0.79f), X (0.56f) - X (0.44f), Y (0.1f) - Y (0.0f));
    g.setColour (juce::Colour (0xff2a0c06)); g.fillPath (gate);
    g.setColour (pal::signalHot.withAlpha (0.9f));
    for (auto p : { juce::Point<float> (0.19f, 0.62f), { 0.81f, 0.62f }, { 0.5f, 0.5f } })
        g.fillRect (X (p.x) - w * 0.018f, Y (p.y), w * 0.036f, h * 0.07f);
}

//==============================================================================
PossessedLookAndFeel::PossessedLookAndFeel()
{
    setColour (juce::Slider::textBoxTextColourId, pal::paintDim);
    setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour (juce::Slider::textBoxHighlightColourId, pal::rust.withAlpha (0.6f));
    setColour (juce::Label::textColourId, pal::paintDim);
    setColour (juce::ComboBox::textColourId, pal::paint);
    setColour (juce::ComboBox::arrowColourId, pal::signal);
    setColour (juce::PopupMenu::backgroundColourId, juce::Colour (0xff1a1411));
    setColour (juce::PopupMenu::textColourId, pal::paint);
    setColour (juce::PopupMenu::headerTextColourId, pal::rustLight);
    setColour (juce::PopupMenu::highlightedBackgroundColourId, pal::rust);
    setColour (juce::PopupMenu::highlightedTextColourId, juce::Colours::white);
    setColour (juce::TextButton::textColourOffId, pal::paint);
    setColour (juce::TextButton::textColourOnId, juce::Colours::white);
    setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f0b09));
    setColour (juce::TextEditor::textColourId, pal::paint);
    setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff5a3420));
    setColour (juce::TextEditor::focusedOutlineColourId, pal::rustLight);
    setColour (juce::TextEditor::highlightColourId, pal::rust.withAlpha (0.6f));
    setColour (juce::CaretComponent::caretColourId, pal::signalHot);

    setColour (juce::AlertWindow::backgroundColourId, juce::Colour (0xff1a1411));
    setColour (juce::AlertWindow::textColourId, pal::paint);
    setColour (juce::AlertWindow::outlineColourId, pal::rust);
    setColour (juce::ComboBox::backgroundColourId, juce::Colour (0xff0f0b09));
    setColour (juce::ComboBox::outlineColourId, juce::Colour (0xff6a3a20));

    rustDisc = juce::Image (juce::Image::ARGB, 128, 128, true);
    juce::Image::BitmapData bd (rustDisc, juce::Image::BitmapData::writeOnly);
    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x)
        {
            const float n = tex::fbm ((float) x / 22.0f, (float) y / 22.0f, 501, 4);
            const float a = tex::smoothstep (0.5f, 0.7f, n);
            const auto c = tex::rustAt ((float) x * 2.0f, (float) y * 2.0f, 0.3f, 777);
            bd.setPixelColour (x, y, juce::Colour::fromFloatRGBA (c.r, c.g, c.b, a * 0.85f));
        }
}

juce::Font PossessedLookAndFeel::getComboBoxFont (juce::ComboBox&) { return tex::plainBold (13.0f); }
juce::Font PossessedLookAndFeel::getTextButtonFont (juce::TextButton&, int h) { return tex::stencil (juce::jmin (15.0f, (float) h * 0.62f)).withExtraKerningFactor (0.08f); }

void PossessedLookAndFeel::drawButtonBackground (juce::Graphics& g, juce::Button& b, const juce::Colour&, bool hi, bool down)
{
    auto r = b.getLocalBounds().toFloat().reduced (0.5f);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (1.0f, 1.5f), 3.0f);
    g.setGradientFill (juce::ColourGradient (down ? pal::rustDeep : juce::Colour (0xff3a2a20), 0, r.getY(),
                                             down ? juce::Colour (0xff2a120a) : juce::Colour (0xff17110e), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (hi ? pal::rustLight : juce::Colour (0xff6a3a20)); g.drawRoundedRectangle (r, 3.0f, 1.2f);
}

void PossessedLookAndFeel::drawComboBox (juce::Graphics& g, int w, int h, bool, int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float> (0, 0, (float) w, (float) h).reduced (0.5f);
    g.setColour (juce::Colour (0xff0f0b09)); g.fillRoundedRectangle (r, 3.0f);
    g.setColour (box.isMouseOver (true) ? pal::rustLight : juce::Colour (0xff6a3a20)); g.drawRoundedRectangle (r, 3.0f, 1.2f);
    juce::Path arrow; const float ax = (float) w - 16.0f, ay = (float) h * 0.5f;
    arrow.addTriangle (ax - 5, ay - 3, ax + 5, ay - 3, ax, ay + 4);
    g.setColour (pal::signal); g.fillPath (arrow);
}

void PossessedLookAndFeel::drawRotarySlider (juce::Graphics& g, int x, int y, int w, int h, float pos,
                                        float startAngle, float endAngle, juce::Slider& s)
{
    auto b = juce::Rectangle<float> ((float) x, (float) y, (float) w, (float) h).reduced (3.0f);
    const float r = std::min (b.getWidth(), b.getHeight()) * 0.5f;
    const auto c = b.getCentre();
    const float angle = startAngle + pos * (endAngle - startAngle);
    const float thick = juce::jmax (2.5f, r * 0.11f);
    const float alpha = s.isEnabled() ? 1.0f : 0.3f;

    // tick marks stamped round the dial
    g.setColour (pal::paintDim.withAlpha (0.45f * alpha));
    for (int i = 0; i <= 10; ++i)
    {
        const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle) - juce::MathConstants<float>::halfPi;
        const float r1 = r + 1.0f, r0 = r - (i % 5 == 0 ? 5.0f : 3.0f);
        g.drawLine (c.x + std::cos (a) * r0, c.y + std::sin (a) * r0, c.x + std::cos (a) * r1, c.y + std::sin (a) * r1, i % 5 == 0 ? 1.4f : 0.9f);
    }

    const float ar = r - thick - 4.0f;
    juce::Path track; track.addCentredArc (c.x, c.y, ar, ar, 0, startAngle, endAngle, true);
    g.setColour (juce::Colour (0xff120e0c).withAlpha (alpha)); g.strokePath (track, juce::PathStrokeType (thick + 2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (juce::Colour (0xff3a2418).withAlpha (alpha)); g.strokePath (track, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    juce::Path val; val.addCentredArc (c.x, c.y, ar, ar, 0, startAngle, angle, true);
    const auto arcCol = (s.isMouseOverOrDragging() ? pal::signalHot : pal::signal).withAlpha (alpha);
    g.setColour (arcCol.withAlpha (0.25f * alpha));
    g.strokePath (val, juce::PathStrokeType (thick + 4.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    g.setColour (arcCol);
    g.strokePath (val, juce::PathStrokeType (thick, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    const float br = ar - thick * 1.4f;
    if (br < 4.0f) return;
    const juce::Rectangle<float> body (c.x - br, c.y - br, br * 2, br * 2);
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillEllipse (body.translated (1.5f, 2.5f));
    g.setGradientFill (juce::ColourGradient (juce::Colour (0xff6a5a4c).withAlpha (alpha), c.x - br * 0.5f, c.y - br * 0.7f,
                                             juce::Colour (0xff17120f), c.x + br * 0.6f, c.y + br, true));
    g.fillEllipse (body);
    {
        juce::Graphics::ScopedSaveState ss (g);
        juce::Path clip; clip.addEllipse (body);
        g.reduceClipRegion (clip);
        const float off = (float) (((juce::uint64) (juce::pointer_sized_int) &s >> 4) % 64);
        g.setOpacity (0.9f * alpha);
        g.drawImage (rustDisc, body.expanded (br * 0.4f).translated (-off * 0.3f, off * 0.2f), juce::RectanglePlacement::stretchToFit);
    }
    g.setColour (juce::Colour (0xff0f0b09).withAlpha (0.8f));
    for (int i = 0; i < 32; ++i)
    {
        const float a = (float) i / 32.0f * juce::MathConstants<float>::twoPi;
        g.drawLine (c.x + std::cos (a) * br * 0.87f, c.y + std::sin (a) * br * 0.87f,
                    c.x + std::cos (a) * br, c.y + std::sin (a) * br, 1.0f);
    }
    g.setColour (juce::Colour (0xff0b0807)); g.drawEllipse (body, 1.2f);

    juce::Path p; p.addRoundedRectangle (-1.8f, -br * 0.92f, 3.6f, br * 0.6f, 1.2f);
    g.setColour (juce::Colours::black.withAlpha (0.5f));
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c.x + 0.8f, c.y + 1.0f));
    g.setColour (pal::paint.withAlpha (alpha));
    g.fillPath (p, juce::AffineTransform::rotation (angle).translated (c));
}


namespace colours   // eyes, dialogs and FX-rack accents (rusty-family values; the eyes keep their hellfire)
{
const juce::Colour blood    { 0xffb3141c };
const juce::Colour bloodHot { 0xffff3b22 };
const juce::Colour ember    { 0xffff7a1a };
const juce::Colour bone     { 0xffd6c7a6 };
const juce::Colour ash      { 0xffa8997c };
const juce::Colour edge     { 0xff5a3420 };
}

static juce::Font font (float size, bool bold = false)
{
    return juce::Font (juce::FontOptions (size, bold ? juce::Font::bold : juce::Font::plain));
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

    // ---- FX rack
    const char* fxTitles[5] = { "STUTTER", "REPEAT", "TAPE STOP", "FREEZE", "DIVE" };
    const char* fxOn[5] = { "stutterOn", "repeatOn", "tapeOn", "freezeOn", "diveOn" };
    for (size_t i = 0; i < 5; ++i)
    {
        auto& f = fxSections[i];
        f.title = fxTitles[i]; f.onParam = fxOn[i];
        f.power.setTooltip ("Engage " + f.title + " (automatable)");
        addAndMakeVisible (f.power);
        f.att = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, f.onParam, f.power);
    }
    fxKnobs[0] = { makeKnob ("stutterSteps", "Steps"), makeKnob ("stutterDepth", "Depth"), makeKnob ("stutterSmooth", "Smooth") };
    fxKnobs[1] = { makeKnob ("repeatDrop", "Drop") };
    fxKnobs[2] = { makeKnob ("tapeTime", "Time") };
    fxKnobs[3] = { makeKnob ("freezeSize", "Size"), makeKnob ("freezeMix", "Mix") };
    fxKnobs[4] = { makeKnob ("diveAmount", "Amount"), makeKnob ("diveTime", "Time") };
    for (int i = 0; i < 16; ++i)
    {
        steps[(size_t) i].setTooltip ("Step " + juce::String (i + 1) + ": lit = volume cut");
        addAndMakeVisible (steps[(size_t) i]);
        stepAtts[(size_t) i] = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "step" + juce::String (i), steps[(size_t) i]);
    }
    tailsButton.setTooltip ("Let delay and reverb ring through the cut steps");
    addAndMakeVisible (tailsButton);
    tailsAtt = std::make_unique<juce::AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, "stutterTails", tailsButton);
    stutterRateBox.addItemList ({ "1/4", "1/8", "1/16", "1/32", "1/8T", "1/16T" }, 1);
    repeatLengthBox.addItemList ({ "1/4", "1/8", "1/16", "1/32", "1/64" }, 1);
    addAndMakeVisible (stutterRateBox); addAndMakeVisible (repeatLengthBox);
    stutterRateAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "stutterRate", stutterRateBox);
    repeatLengthAtt = std::make_unique<juce::AudioProcessorValueTreeState::ComboBoxAttachment> (proc.apvts, "repeatLength", repeatLengthBox);

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

    setSize (1180, 850);
    startTimerHz (30);
}

PossessedEditor::~PossessedEditor() { setLookAndFeel (nullptr); }

PossessedEditor::Knob* PossessedEditor::makeKnob (const juce::String& id, const juce::String& text, bool big)
{
    auto k = std::make_unique<Knob>();
    k->big = big;
    k->slider.setTextBoxStyle (juce::Slider::TextBoxBelow, false, 74, 17);
    k->slider.setColour (juce::Slider::textBoxTextColourId, pal::paint.withAlpha (0.88f));
    k->slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colour (0xff3a2418));
    k->slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colour (0x90000000));
    k->slider.setMouseDragSensitivity (220);
    k->label.setText (text.toUpperCase(), juce::dontSendNotification);
    k->label.setJustificationType (juce::Justification::centred);
    k->label.setFont (tex::stencil (big ? 17.0f : 14.0f).withExtraKerningFactor (0.1f));
    k->label.setColour (juce::Label::textColourId, big ? pal::paint : pal::paintDim);
    k->label.setMinimumHorizontalScale (0.6f);
    addAndMakeVisible (k->slider); addAndMakeVisible (k->label);
    k->att = std::make_unique<juce::AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k->slider);
    if (auto* prm = proc.apvts.getParameter (id))
        k->slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    knobs.push_back (std::move (k));
    return knobs.back().get();
}

void PossessedEditor::layoutSection (Section& s, juce::Rectangle<int> area)
{
    s.bounds = area;
    if (s.combo != nullptr) s.combo->setBounds (area.getRight() - 112, area.getY() + 6, 100, 22);
    auto inner = area.reduced (10).withTrimmedTop (20);
    if (s.title == "IN / OUT") { meterArea = inner.removeFromRight (54); inner.removeFromRight (6); }
    if (s.knobs.empty()) return;
    const int w = inner.getWidth() / (int) s.knobs.size();
    for (auto* k : s.knobs)
    {
        auto cell = inner.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (18));
        k->slider.setBounds (cell);
    }
}

void PossessedEditor::resized()
{
    const int W = getWidth();
    // header: the eyes peer out of a dark porthole, then the logo, the crest badge and the preset rack
    eyesWell   = { 14, 10, 262, 100 };
    eyes.setBounds (eyesWell.reduced (4, 2).translated (0, 2));
    logoArea   = { 292, 8, 330, 70 };
    badgeArea  = { 636, 14, 262, 88 };
    crestArea  = { 644, 20, 76, 76 };
    presetArea = { 912, 14, W - 926, 88 };
    presetBox.setBounds (presetArea.getX(), 36, presetArea.getWidth(), 28);
    {
        auto row = juce::Rectangle<int> (presetArea.getX(), 72, presetArea.getWidth(), 28);
        prevButton.setBounds (row.removeFromLeft (36)); row.removeFromLeft (6);
        nextButton.setBounds (row.removeFromLeft (36)); row.removeFromLeft (6);
        deleteButton.setBounds (row.removeFromRight (78)); row.removeFromRight (6);
        saveButton.setBounds (row);
    }
    stripeArea = { 0, 118, W, 12 };

    auto r = getLocalBounds().withTrimmedTop (140).reduced (12, 0).withTrimmedBottom (12);

    const int gap = 8;
    auto fxRow = r.removeFromBottom (186);
    rackLabelArea = r.removeFromBottom (gap + 14).withTrimmedTop (gap - 2);   // stencilled FX RACK label
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
    auto b = split (row2, { 1.7f, 2.15f, 2.3f, 1.0f, 3.25f });
    for (int i = 0; i < 5; ++i) layoutSection (sections[(size_t) (3 + i)], b[(size_t) i]);
    auto c = split (row3, { 2.0f, 2.0f, 3.0f, 2.0f, 2.0f });
    for (int i = 0; i < 5; ++i) layoutSection (sections[(size_t) (8 + i)], c[(size_t) i]);
    layoutFxRack (fxRow);

    buildBackground();
}

void PossessedEditor::buildBackground()
{
    const float scale = 2.0f;
    const int W = getWidth(), H = getHeight();
    if (W <= 0 || H <= 0) return;
    juce::Image base (juce::Image::RGB, W, H, false);

    struct PlateInfo { juce::Rectangle<float> r; juce::String title; bool hot; };
    std::vector<PlateInfo> plates;
    for (auto& s : sections) plates.push_back ({ s.bounds.toFloat(), s.title, s.title == "VOICE" || s.title == "DEMON" || s.title == "LO-FI" });
    for (auto& f : fxSections) plates.push_back ({ f.bounds.toFloat(), f.title, false });
    plates.push_back ({ badgeArea.toFloat(), {}, false });
    const auto stripe = stripeArea.toFloat();
    const auto well = eyesWell.toFloat();

    {
        juce::Image::BitmapData bd (base, juce::Image::BitmapData::writeOnly);
        for (int py = 0; py < H; ++py)
        {
            const float y = (float) py;
            for (int px = 0; px < W; ++px)
            {
                const float x = (float) px;
                tex::RGB col;
                int inside = -1; float edge = 1e9f; bool shadow = false;
                for (int i = 0; i < (int) plates.size(); ++i)
                {
                    const auto& pr = plates[(size_t) i].r;
                    if (pr.contains (x, y))
                    {
                        inside = i;
                        edge = std::min ({ x - pr.getX(), pr.getRight() - x, y - pr.getY(), pr.getBottom() - y });
                        break;
                    }
                    if (pr.translated (3.0f, 4.0f).contains (x, y)) shadow = true;
                }
                if (well.translated (3.0f, 4.0f).contains (x, y) && ! well.contains (x, y)) shadow = true;

                if (stripe.contains (x, y))
                {
                    // worn hazard stripes, painted in dried blood
                    const bool red = std::fmod (x + y + 1000.0f, 28.0f) < 14.0f;
                    const float wear = tex::fbm (x / 12.0f, y / 4.0f, 2024, 3);
                    col = red ? tex::rgb (juce::Colour (0xff8a1810)) : tex::rgb (juce::Colour (0xff141010));
                    col = tex::mix (col, tex::rustAt (x, y, 0.3f, 99), tex::smoothstep (0.5f, 0.68f, wear));
                    col = tex::mul (col, 0.85f + 0.15f * tex::hash (px, py, 3));
                }
                else if (well.contains (x, y))
                {
                    // the porthole the eyes stare out of: near-black with a smoky red haze
                    const float cx = (x - well.getCentreX()) / (well.getWidth() * 0.5f), cy = (y - well.getCentreY()) / (well.getHeight() * 0.5f);
                    const float haze = juce::jlimit (0.0f, 1.0f, 1.0f - std::sqrt (cx * cx * 0.7f + cy * cy));
                    const float smoke = tex::fbm (x / 26.0f, y / 18.0f, 666, 4);
                    col = tex::mix (tex::rgb (juce::Colour (0xff040303)), tex::rgb (juce::Colour (0xff2a0806)), haze * (0.45f + 0.55f * smoke));
                }
                else if (inside >= 0)
                {
                    // dark diamond-plate steel, rust creeping in from the edges
                    const float d = tex::fbm (x / 30.0f, y / 30.0f, 211, 3);
                    tex::RGB steel = tex::mix (tex::rgb (juce::Colour (0xff1e1a17)), tex::rgb (juce::Colour (0xff332c27)), d);
                    const float u = std::fmod (x + y + 2000.0f, 16.0f) / 16.0f, v = std::fmod (x - y + 2000.0f, 16.0f) / 16.0f;
                    const float tread = (std::abs (u - 0.5f) < 0.08f && std::abs (v - 0.25f) < 0.18f) || (std::abs (v - 0.5f) < 0.08f && std::abs (u - 0.75f) < 0.18f) ? 1.0f : 0.0f;
                    steel = tex::mix (steel, tex::rgb (juce::Colour (0xff4a4038)), tread * 0.55f);
                    const float edgeRust = (1.0f - juce::jlimit (0.0f, 1.0f, edge / 14.0f)) * tex::smoothstep (0.35f, 0.6f, tex::vnoise (x / 6.0f, y / 6.0f, 17));
                    const float bloom = tex::smoothstep (0.6f, 0.75f, tex::fbm (x / 55.0f, y / 55.0f, 404, 3)) * 0.7f;
                    col = tex::mix (steel, tex::rustAt (x, y, 0.15f, 1337), juce::jlimit (0.0f, 1.0f, edgeRust + bloom));
                    col = tex::mul (col, 0.88f);
                }
                else
                {
                    col = tex::rustAt (x, y, 0.1f, 42);
                    if (shadow) col = tex::mul (col, 0.45f);
                }

                if (! well.contains (x, y))
                {
                    const float streak = tex::fbm (x / 9.0f, y / 240.0f, 909, 3);
                    const float drip = tex::smoothstep (0.58f, 0.78f, streak) * (0.25f + 0.5f * y / (float) H);
                    col = tex::mul (col, 1.0f - drip * 0.6f);
                    const float runoff = tex::smoothstep (0.66f, 0.82f, tex::fbm (x / 5.0f, y / 120.0f, 31337, 2));
                    col = tex::mix (col, tex::rgb (pal::rustDeep), runoff * 0.35f);
                    const float oil = tex::smoothstep (0.7f, 0.8f, tex::fbm (x / 70.0f, y / 50.0f, 555, 3));
                    col = tex::mul (col, 1.0f - oil * 0.45f);
                    if (tex::hash (px, py, 5) > 0.988f) col = tex::mul (col, 0.45f);
                }
                const float grime = 0.72f + 0.28f * tex::fbm (x / 260.0f, y / 260.0f, 77, 2);
                const float dx = (x / (float) W - 0.5f) * 2.0f, dy = (y / (float) H - 0.5f) * 2.0f;
                const float vig = 1.0f - 0.45f * std::min (1.0f, (dx * dx + dy * dy) * 0.6f);
                col = tex::mul (col, grime * vig);
                bd.setPixelColour (px, py, juce::Colour::fromFloatRGBA (juce::jlimit (0.0f, 1.0f, col.r), juce::jlimit (0.0f, 1.0f, col.g),
                                                                        juce::jlimit (0.0f, 1.0f, col.b), 1.0f));
            }
        }
    }

    const int PW = (int) ((float) W * scale), PH = (int) ((float) H * scale);
    background = juce::Image (juce::Image::RGB, PW, PH, false);
    juce::Graphics g (background);
    g.setImageResamplingQuality (juce::Graphics::highResamplingQuality);
    g.drawImage (base, 0, 0, PW, PH, 0, 0, W, H);
    g.addTransform (juce::AffineTransform::scale (scale));
    juce::Random rnd (1991);

    auto rivet = [&] (float cx, float cy, float rad)
    {
        g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillEllipse (cx - rad + 1, cy - rad + 1.5f, rad * 2, rad * 2);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a7a68), cx - rad * 0.4f, cy - rad * 0.5f, juce::Colour (0xff2a201a), cx + rad, cy + rad, true));
        g.fillEllipse (cx - rad, cy - rad, rad * 2, rad * 2);
        g.setColour (pal::rust.withAlpha (0.55f)); g.drawEllipse (cx - rad, cy - rad, rad * 2, rad * 2, 1.2f);
        g.setColour (pal::rustDeep.withAlpha (0.35f)); g.fillRect (cx - 0.8f, cy + rad, 1.6f, 3.0f + rnd.nextFloat() * 9.0f);
    };
    auto plateFrame = [&] (juce::Rectangle<float> pr, bool hot)
    {
        g.setColour (juce::Colours::white.withAlpha (0.07f)); g.drawLine (pr.getX() + 2, pr.getY() + 1, pr.getRight() - 2, pr.getY() + 1, 1.0f);
        g.setColour (juce::Colours::black.withAlpha (0.55f)); g.drawRect (pr, 1.0f);
        if (hot) { g.setColour (pal::signal.withAlpha (0.45f)); g.drawRect (pr.reduced (2.0f), 1.5f); }
        rivet (pr.getX() + 6, pr.getY() + 6, 3.0f); rivet (pr.getRight() - 6, pr.getY() + 6, 3.0f);
        rivet (pr.getX() + 6, pr.getBottom() - 6, 3.0f); rivet (pr.getRight() - 6, pr.getBottom() - 6, 3.0f);
    };

    // scratches
    for (int i = 0; i < 420; ++i)
    {
        const float x = rnd.nextFloat() * (float) W, y = rnd.nextFloat() * (float) H;
        if (well.contains (x, y)) continue;
        const float len = 6.0f + rnd.nextFloat() * 70.0f, ang = rnd.nextFloat() * juce::MathConstants<float>::pi;
        g.setColour ((rnd.nextBool() ? juce::Colour (0xffd0c0a8) : juce::Colours::black).withAlpha (0.04f + rnd.nextFloat() * 0.1f));
        g.drawLine (x, y, x + std::cos (ang) * len, y + std::sin (ang) * len, 0.5f + rnd.nextFloat() * 0.6f);
    }

    // plates + stencilled titles
    int seed = 10;
    for (auto& pl : plates)
    {
        plateFrame (pl.r, pl.hot);
        if (pl.title.isEmpty()) continue;
        auto title = tex::distressedText (pl.title, (int) pl.r.getWidth() - 40, 18, tex::stencil (15.0f), pl.hot ? pal::signalHot : pal::paint,
                                          juce::Justification::centredLeft, scale, 0.2f, seed++, 0.12f);
        g.setOpacity (1.0f);
        g.drawImage (title, juce::Rectangle<float> (pl.r.getX() + 14, pl.r.getY() + 4, pl.r.getWidth() - 40, 18.0f));
    }
    // recessed wells for the step grid and the meters
    for (auto rr : { stepArea, meterArea })
        if (! rr.isEmpty()) { g.setColour (juce::Colours::black.withAlpha (0.6f)); g.fillRoundedRectangle (rr.toFloat().expanded (4.0f), 4.0f); }

    // the eyes' porthole: heavy riveted iron frame
    {
        g.setColour (juce::Colours::black.withAlpha (0.8f)); g.drawRect (well, 3.0f);
        g.setColour (juce::Colour (0xff3a2a20)); g.drawRect (well.expanded (3.0f), 3.0f);
        g.setColour (pal::rust.withAlpha (0.5f)); g.drawRect (well.expanded (4.5f), 1.0f);
        for (float fx : { 0.0f, 0.5f, 1.0f })
            for (float fy : { 0.0f, 1.0f })
                rivet (well.getX() + fx * well.getWidth() + (fx == 0.0f ? -1.5f : fx == 1.0f ? 1.5f : 0.0f), well.getY() + fy * well.getHeight() + (fy == 0.0f ? -1.5f : 1.5f), 3.2f);
    }

    // FX RACK label stencilled on the rust
    {
        auto lbl = tex::distressedText ("FX RACK", 200, 16, tex::stencil (15.0f), pal::signalHot.withAlpha (0.9f), juce::Justification::centredLeft, scale, 0.22f, 31, 0.3f);
        g.setOpacity (1.0f);
        g.drawImage (lbl, juce::Rectangle<float> ((float) rackLabelArea.getX() + 4, (float) rackLabelArea.getY(), 200.0f, 16.0f));
    }

    // hazard stripe edges
    g.setColour (juce::Colours::black.withAlpha (0.75f));
    g.drawLine (0, stripe.getY(), (float) W, stripe.getY(), 1.5f);
    g.drawLine (0, stripe.getBottom(), (float) W, stripe.getBottom(), 1.5f);

    // CASTLE DANGER MUSIC COMPANY badge with the castle crest
    const auto badge = badgeArea.toFloat();
    g.setColour (pal::paint.withAlpha (0.22f)); g.drawRect (badge.reduced (5.0f), 1.0f);
    drawCastleCrest (g, crestArea.toFloat());
    const float tx = (float) crestArea.getRight() + 8.0f, tw = badge.getRight() - tx - 10.0f;
    auto l1 = tex::distressedText ("CASTLE DANGER", (int) tw, 28, tex::stencil (25.0f), juce::Colour (0xffeadfc4), juce::Justification::centredLeft, scale, 0.24f, 4242, 0.08f);
    auto l2 = tex::distressedText ("MUSIC COMPANY", (int) tw, 20, tex::stencil (17.0f), pal::rustLight, juce::Justification::centredLeft, scale, 0.24f, 4343, 0.2f);
    auto l3 = tex::distressedText ("WE MAKE MUSIC", (int) tw, 14, tex::plainBold (9.0f), pal::paintDim, juce::Justification::centredLeft, scale, 0.15f, 4444, 0.3f);
    g.setColour (juce::Colours::black);
    g.setOpacity (0.7f); g.drawImage (l1, juce::Rectangle<float> (tx + 1.5f, badge.getY() + 13.0f, tw, 28.0f));
    g.setOpacity (1.0f); g.drawImage (l1, juce::Rectangle<float> (tx, badge.getY() + 11.0f, tw, 28.0f));
    g.setOpacity (0.7f); g.drawImage (l2, juce::Rectangle<float> (tx + 1.5f, badge.getY() + 40.0f, tw, 20.0f));
    g.setOpacity (1.0f); g.drawImage (l2, juce::Rectangle<float> (tx, badge.getY() + 38.0f, tw, 20.0f));
    g.drawImage (l3, juce::Rectangle<float> (tx + 1.0f, badge.getY() + 60.0f, tw, 14.0f));

    // stamped subtitle under the logo
    auto sub = tex::distressedText ("PURE VOCAL DESTRUCTION  //  V" + juce::String (JucePlugin_VersionString),
                                    330, 14, tex::plainBold (10.5f), pal::paintDim, juce::Justification::centredLeft, scale, 0.15f, 99, 0.16f);
    g.setOpacity (1.0f);
    g.drawImage (sub, juce::Rectangle<float> ((float) logoArea.getX() + 4, 90.0f, 330.0f, 14.0f));

    // PRESET label
    auto plab = tex::distressedText ("PRESET", 120, 16, tex::stencil (14.0f), pal::paint, juce::Justification::centredLeft, scale, 0.2f, 7, 0.15f);
    g.drawImage (plab, juce::Rectangle<float> ((float) presetArea.getX() + 2, (float) presetArea.getY(), 120.0f, 16.0f));

    // the logo: POSSESSED in rusty blood red, dripping (drawn live so it can flicker)
    {
        const int lw = logoArea.getWidth(), lh = logoArea.getHeight() + 14;
        logoImage = juce::Image (juce::Image::ARGB, (int) ((float) lw * scale), (int) ((float) lh * scale), true);
        juce::Graphics lg (logoImage);
        lg.addTransform (juce::AffineTransform::scale (scale));
        auto f = tex::stencil (62.0f).withExtraKerningFactor (0.04f);
        {
            juce::GlyphArrangement all; all.addLineOfText (f, "POSSESSED", 0, 0);
            const float wAll = all.getBoundingBox (0, -1, true).getWidth();
            if (wAll > (float) lw - 8.0f) f = f.withHeight (62.0f * ((float) lw - 8.0f) / wAll);
        }
        const float textH = 62.0f;
        lg.setGradientFill (juce::ColourGradient (pal::signalHot, 0, 8.0f, juce::Colour (0xff7a1008), 0, textH, false));
        lg.setFont (f);
        lg.drawText ("POSSESSED", 2, 0, lw - 4, (int) textH, juce::Justification::centredLeft, false);
        // drips running off the bottom of the letters
        juce::GlyphArrangement ga; ga.addLineOfText (f, "POSSESSED", 2.0f, 0.0f);
        const auto wb = ga.getBoundingBox (0, -1, true);
        float base = textH * 0.8f;   // measured below: the lowest row the letters actually reach
        {
            juce::Image::BitmapData bd (logoImage, juce::Image::BitmapData::readOnly);
            for (int y = bd.height - 1; y >= 0; --y)
            {
                int solid = 0;
                for (int x = 0; x < bd.width; x += 2) if (bd.getPixelColour (x, y).getAlpha() > 160) ++solid;
                if (solid > 6) { base = (float) y / scale; break; }
            }
        }
        const float x0 = 2.0f, x1 = 2.0f + std::min ((float) lw - 6.0f, wb.getWidth());
        tex::distress (logoImage, scale, 0.3f, 666);
        // fresh blood drips over the worn paint (drawn after the distress pass so they stay solid)
        juce::Graphics dg (logoImage);
        dg.addTransform (juce::AffineTransform::scale (scale));
        juce::Random drip (13);
        for (int i = 0; i < 11; ++i)
        {
            const float x = x0 + 8 + drip.nextFloat() * (x1 - x0 - 16);
            const float len = 5.0f + drip.nextFloat() * 14.0f, wdt = 2.0f + drip.nextFloat() * 2.2f;
            dg.setColour (juce::Colour (0xffa0160c));
            dg.fillRoundedRectangle (x - wdt * 0.5f, base - 5, wdt, len, wdt * 0.5f);
            dg.fillEllipse (x - wdt * 0.8f, base - 5 + len - wdt, wdt * 1.6f, wdt * 1.9f);
        }

    }
}

void PossessedEditor::paint (juce::Graphics& g)
{
    using namespace colours;
    if (background.isValid()) g.drawImage (background, getLocalBounds().toFloat());
    else g.fillAll (pal::ironDark);

    if (logoImage.isValid())
    {
        const float alpha = juce::jlimit (0.45f, 1.0f, 1.0f - flicker * 0.6f);
        const auto la = logoArea.toFloat().withHeight ((float) logoArea.getHeight() + 14.0f);
        g.setColour (juce::Colours::black);
        g.setOpacity (0.65f); g.drawImage (logoImage, la.translated (2.5f, 3.5f));
        g.setOpacity (alpha); g.drawImage (logoImage, la);
        g.setOpacity (1.0f);
    }

    // FX rack plates light up while engaged
    for (auto& f : fxSections)
    {
        if (! f.power.getToggleState()) continue;
        auto rb = f.bounds.toFloat();
        for (int i = 1; i <= 6; ++i)   // glow around the outside of the plate only
        {
            g.setColour (pal::signalHot.withAlpha (0.16f * (1.0f - (float) i / 7.0f)));
            g.drawRect (rb.expanded ((float) i * 1.5f), 1.5f);
        }
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xff8a1408).withAlpha (0.16f), rb.getX(), rb.getY(), juce::Colours::transparentBlack, rb.getX(), rb.getY() + rb.getHeight() * 0.6f, false));
        g.fillRect (rb);
        g.setColour (pal::signalHot); g.drawRect (rb.reduced (1.0f), 2.0f);
        g.setColour (pal::signalHot); g.setFont (tex::stencil (15.0f).withExtraKerningFactor (0.12f));
        g.drawText (f.title, juce::Rectangle<float> (rb.getX() + 14, rb.getY() + 4, rb.getWidth() - 40, 18.0f), juce::Justification::centredLeft);
    }
}

void PossessedEditor::paintOverChildren (juce::Graphics& g)
{
    if (meterArea.isEmpty()) return;
    auto m = meterArea.toFloat().reduced (2.0f, 4.0f);
    const float bw = (m.getWidth() - 6) * 0.5f;
    auto drawBar = [&] (juce::Rectangle<float> bar, float lvl, const char* lbl)
    {
        auto lab = bar.removeFromBottom (14);
        g.setColour (juce::Colour (0xff0c0a08)); g.fillRect (bar);
        const float db = juce::Decibels::gainToDecibels (lvl, -60.0f);
        const float frac = juce::jlimit (0.0f, 1.0f, (db + 48.0f) / 48.0f);
        const int segs = 18;
        const float sh = (bar.getHeight() - 4.0f) / (float) segs;
        for (int i = 0; i < segs; ++i)
        {
            const float t = (float) (i + 1) / (float) segs;
            const auto cell = juce::Rectangle<float> (bar.getX() + 3, bar.getBottom() - 2 - (float) (i + 1) * sh, bar.getWidth() - 6, sh - 1.5f);
            const bool lit = t <= frac + 1e-4f;
            const auto c = t > 0.94f ? pal::signalHot : (t > 0.8f ? pal::hazard : pal::paint.darker (0.15f));
            g.setColour (lit ? c : c.withAlpha (0.08f)); g.fillRect (cell);
        }
        g.setColour (pal::paintDim); g.setFont (tex::stencil (12.0f).withExtraKerningFactor (0.1f));
        g.drawText (lbl, lab.expanded (8, 0), juce::Justification::centred, false);
    };
    drawBar (m.removeFromLeft (bw), inMeter, "IN");
    m.removeFromLeft (6);
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
    repaint (logoArea.expanded (4).withHeight (logoArea.getHeight() + 22));

    const int cur = proc.currentStep.load();
    const int len = (int) proc.apvts.getRawParameterValue ("stutterSteps")->load();
    const bool stutOn = fxSections[0].power.getToggleState();
    for (int i = 0; i < 16; ++i) steps[(size_t) i].setState (i == cur, i < len, stutOn);
    for (auto& f : fxSections)
        if (f.power.getToggleState() != f.wasOn) { f.wasOn = f.power.getToggleState(); repaint (f.bounds.expanded (16)); }
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
        field.setColour (juce::TextEditor::backgroundColourId, juce::Colour (0xff0f0b09));
        field.setColour (juce::TextEditor::outlineColourId, juce::Colour (0xff5a3420));
        field.setColour (juce::TextEditor::focusedOutlineColourId, pal::rustLight);
        field.setColour (juce::TextEditor::textColourId, pal::paint);
        field.setColour (juce::TextEditor::highlightColourId, pal::rust.withAlpha (0.6f));
        field.setColour (juce::CaretComponent::caretColourId, pal::signalHot);
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
    g.fillAll (juce::Colours::black.withAlpha (0.72f));
    auto p = panel.toFloat();
    g.setColour (juce::Colour (0xff1a1411)); g.fillRect (p);
    g.setGradientFill (juce::ColourGradient (pal::rustDeep.withAlpha (0.5f), p.getX(), p.getY(), juce::Colours::transparentBlack, p.getX(), p.getY() + p.getHeight() * 0.6f, false));
    g.fillRect (p);
    g.setColour (pal::rust); g.drawRect (p, 2.0f);
    g.setColour (juce::Colours::black.withAlpha (0.6f)); g.drawRect (p.reduced (2.0f), 1.0f);
    for (auto pt : { juce::Point<float> (p.getX() + 8, p.getY() + 8), { p.getRight() - 8, p.getY() + 8 }, { p.getX() + 8, p.getBottom() - 8 }, { p.getRight() - 8, p.getBottom() - 8 } })
    {
        g.setColour (juce::Colour (0xff6a5a4c)); g.fillEllipse (pt.x - 3, pt.y - 3, 6, 6);
        g.setColour (pal::rust.withAlpha (0.6f)); g.drawEllipse (pt.x - 3, pt.y - 3, 6, 6, 1.0f);
    }
    auto r = panel.reduced (22);
    g.setColour (pal::signalHot); g.setFont (tex::stencil (19.0f).withExtraKerningFactor (0.1f));
    g.drawText (title, r.removeFromTop (30), juce::Justification::centredLeft);
    g.setColour (pal::paint); g.setFont (tex::plainBold (13.0f));
    g.drawFittedText (message, r.removeFromTop (hasField ? 22 : 50), juce::Justification::topLeft, 3);
}

//==============================================================================
void PowerButton::paintButton (juce::Graphics& g, bool hi, bool)
{
    // indicator lamp + stencilled label on a rusty plate (Dominatrix / Chop Shop style)
    auto r = getLocalBounds().toFloat().reduced (1.0f);
    const bool on = getToggleState();
    const auto lit = pal::signalHot;
    g.setColour (juce::Colours::black.withAlpha (0.55f)); g.fillRoundedRectangle (r.translated (1.0f, 1.5f), 3.0f);
    g.setGradientFill (juce::ColourGradient (on ? juce::Colour (0xff4a1a10) : juce::Colour (0xff2a221d), 0, r.getY(),
                                             juce::Colour (0xff120d0b), 0, r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour (on ? lit.withAlpha (0.85f) : (hi ? pal::rustLight : juce::Colour (0xff5a3420))); g.drawRoundedRectangle (r, 3.0f, 1.2f);
    const float d = r.getHeight() * 0.42f;
    auto led = juce::Rectangle<float> (d, d).withCentre ({ r.getX() + r.getHeight() * 0.55f, r.getCentreY() });
    if (on)
    {
        juce::Path lp; lp.addEllipse (led);
        juce::DropShadow (lit.withAlpha (0.9f), 10, {}).drawForPath (g, lp);
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xfffff0c0), led.getCentreX(), led.getCentreY(), lit, led.getX(), led.getY(), true));
    }
    else g.setColour (juce::Colour (0xff2a1c16));
    g.fillEllipse (led);
    g.setColour (juce::Colours::black.withAlpha (0.7f)); g.drawEllipse (led, 1.0f);
    g.setColour (on ? pal::paint : pal::paintDim);
    g.setFont (tex::stencil (13.5f).withExtraKerningFactor (0.1f));
    g.drawText (getButtonText(), r.withTrimmedLeft (r.getHeight() * 0.9f).withTrimmedRight (4), juce::Justification::centred);
}

void StepButton::paintButton (juce::Graphics& g, bool hi, bool)
{
    // chunky rusty pad; lit (cut) pads glow like a hot coil
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool cut = getToggleState();
    const float alpha = active ? 1.0f : 0.3f;
    g.setColour (juce::Colours::black.withAlpha (0.6f * alpha)); g.fillRoundedRectangle (r.translated (1.0f, 1.5f), 3.0f);
    if (cut)
        g.setGradientFill (juce::ColourGradient (juce::Colour (0xffb2361a).withAlpha (alpha), r.getCentreX(), r.getY(),
                                                 juce::Colour (0xff4a0e06).withAlpha (alpha), r.getCentreX(), r.getBottom(), false));
    else
        g.setGradientFill (juce::ColourGradient ((hi ? juce::Colour (0xff43332a) : juce::Colour (0xff302520)).withAlpha (alpha), r.getCentreX(), r.getY(),
                                                 juce::Colour (0xff120d0b).withAlpha (alpha), r.getCentreX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 3.0f);
    g.setColour ((cut ? pal::signalHot : (hi ? pal::rustLight : juce::Colour (0xff5a3420))).withAlpha (alpha));
    g.drawRoundedRectangle (r, 3.0f, cut ? 1.4f : 1.1f);
    g.setColour (juce::Colours::black.withAlpha (0.25f * alpha));
    g.drawLine (r.getRight() - 12, r.getBottom() - 3, r.getRight() - 3, r.getBottom() - 8, 1.0f);
    if (current && running)
    {
        juce::Path p; p.addRoundedRectangle (r, 3.0f);
        juce::DropShadow (pal::hazard.withAlpha (0.9f), 9, {}).drawForPath (g, p);
        g.setColour (pal::hazard); g.drawRoundedRectangle (r.reduced (0.5f), 3.0f, 2.0f);
    }
}

void PossessedEditor::placeKnobs (const std::vector<Knob*>& ks, juce::Rectangle<int> area)
{
    if (ks.empty()) return;
    const int w = area.getWidth() / (int) ks.size();
    for (auto* k : ks)
    {
        auto cell = area.removeFromLeft (w);
        k->label.setBounds (cell.removeFromTop (16));
        k->slider.setBounds (cell);
    }
}

void PossessedEditor::layoutFxRack (juce::Rectangle<int> row)
{
    const int gap = 8;
    const float weights[5] = { 4.6f, 1.6f, 2.05f, 1.95f, 1.95f };
    float total = 0; for (auto w : weights) total += w;
    const int avail = row.getWidth() - gap * 4;
    for (size_t i = 0; i < 5; ++i)
    {
        const int w = i == 4 ? row.getWidth() : (int) ((float) avail * weights[i] / total);
        auto b = row.removeFromLeft (w); row.removeFromLeft (gap);
        auto& f = fxSections[i];
        f.bounds = b;
        f.power.setBounds (b.getRight() - 66, b.getY() + 5, 56, 22);
        auto inner = b.reduced (10).withTrimmedTop (22);
        if (i == 0) // STUTTER: rate + tails in the header, knobs left, 2x8 step grid right
        {
            stutterRateBox.setBounds (f.power.getX() - 86, b.getY() + 5, 78, 22);
            tailsButton.setBounds (stutterRateBox.getX() - 76, b.getY() + 5, 68, 22);
            placeKnobs (fxKnobs[i], inner.removeFromLeft (190));
            inner.removeFromLeft (8);
            stepArea = inner.withSizeKeepingCentre (inner.getWidth(), std::min (inner.getHeight(), 112));
            const int cw = stepArea.getWidth() / 8, ch = stepArea.getHeight() / 2;
            for (int s = 0; s < 16; ++s)
                steps[(size_t) s].setBounds (stepArea.getX() + (s % 8) * cw + (s % 8 >= 4 ? 3 : 0) - 2, stepArea.getY() + (s / 8) * ch, cw - 2, ch - 2);
        }
        else if (i == 1) // REPEAT: length menu, drop knob
        {
            repeatLengthBox.setBounds (inner.removeFromTop (22).reduced (4, 0));
            inner.removeFromTop (4);
            placeKnobs (fxKnobs[i], inner);
        }
        else placeKnobs (fxKnobs[i], inner);
    }
}
