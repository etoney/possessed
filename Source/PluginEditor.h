#pragma once
#include <juce_audio_utils/juce_audio_utils.h>
#include "PluginProcessor.h"

class PossessedLookAndFeel : public juce::LookAndFeel_V4
{
public:
    PossessedLookAndFeel();
    void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                           float startAngle, float endAngle, juce::Slider&) override;
    void drawComboBox (juce::Graphics&, int w, int h, bool down, int bx, int by, int bw, int bh, juce::ComboBox&) override;
    juce::Font getComboBoxFont (juce::ComboBox&) override { return juce::Font (juce::FontOptions (13.0f)); }
    void drawButtonBackground (juce::Graphics&, juce::Button&, const juce::Colour&, bool hi, bool down) override;
};

// A pair of glowing, slit-pupilled demon eyes that watch, blink and flare with the signal.
class DemonEyes : public juce::Component
{
public:
    void update (float level, float dt);
    void paint (juce::Graphics&) override;
private:
    void drawEye (juce::Graphics&, juce::Point<float> c, float w, float h, float tilt, bool mirror);
    float glow = 0.3f, openness = 1.0f, blinkTimer = 3.0f, blinkPhase = -1.0f;
    float lookX = 0, lookTargetX = 0, lookY = 0, lookTargetY = 0, lookTimer = 1.0f, dilate = 0.0f, time = 0;
    juce::Random rnd;
};

class PossessedEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit PossessedEditor (PossessedProcessor&);
    ~PossessedEditor() override;
    void paint (juce::Graphics&) override;
    void paintOverChildren (juce::Graphics&) override;
    void resized() override;

private:
    struct Knob
    {
        juce::Slider slider { juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::TextBoxBelow };
        juce::Label label;
        std::unique_ptr<juce::AudioProcessorValueTreeState::SliderAttachment> att;
        bool big = false;
    };
    struct Section { juce::String title; std::vector<Knob*> knobs; juce::Rectangle<int> bounds; juce::ComboBox* combo = nullptr; };

    Knob* makeKnob (const juce::String& id, const juce::String& text, bool big = false);
    void layoutSection (Section&, juce::Rectangle<int>);
    void timerCallback() override;
    void buildBackground();

    PossessedProcessor& proc;
    PossessedLookAndFeel lnf;
    std::vector<std::unique_ptr<Knob>> knobs;
    std::vector<Section> sections;

    juce::ComboBox presetBox, distTypeBox, filterTypeBox;
    juce::TextButton prevButton { "<" }, nextButton { ">" };
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distTypeAtt, filterTypeAtt;
    DemonEyes eyes;
    juce::Image background;
    juce::Rectangle<int> meterArea;
    float flicker = 0.0f, inMeter = 0.0f, outMeter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PossessedEditor)
};
