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

// A pair of burning demon eyes that watch, blink and flare with the signal.
class DemonEyes : public juce::Component
{
public:
    void update (float level, float dt);
    void paint (juce::Graphics&) override;
private:
    void drawEye (juce::Graphics&, juce::Point<float> c, float w, float h, float tilt, bool mirror);
    float glow = 0.3f, openness = 1.0f, blinkTimer = 3.0f, blinkPhase = -1.0f;
    float lookX = 0, lookTargetX = 0, lookY = 0, lookTargetY = 0, lookTimer = 1.0f, dilate = 0.0f, time = 0, fire = 0;
    juce::Random rnd;
};

// In-window modal dialog (message, confirm, or text entry), drawn over the editor
class PresetDialog : public juce::Component
{
public:
    PresetDialog (const juce::String& title, const juce::String& message, bool withTextField,
                  const juce::String& initialText, juce::StringArray buttons,
                  std::function<void (int buttonIndex, const juce::String& text)> onClose);
    void paint (juce::Graphics&) override;
    void resized() override;
    bool keyPressed (const juce::KeyPress&) override;
    void parentHierarchyChanged() override;
    void mouseDown (const juce::MouseEvent&) override {} // swallow clicks behind the panel
private:
    void finish (int index);
    juce::String title, message;
    juce::TextEditor field;
    bool hasField;
    juce::OwnedArray<juce::TextButton> buttons;
    std::function<void (int, const juce::String&)> onClose;
    juce::Rectangle<int> panel;
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
    juce::TextButton prevButton { "<" }, nextButton { ">" }, saveButton { "Save Preset..." }, deleteButton { "Delete" };
    juce::Array<juce::File> userFiles;
    int lastPresetVersion = -1;
    std::unique_ptr<PresetDialog> dialog;
    static constexpr int kUserIdBase = 1001;

    void refreshPresetBox();
    void stepPreset (int delta);
    void promptSave();
    void confirmDelete();
    void showDialog (const juce::String& title, const juce::String& message, bool withTextField, const juce::String& initialText,
                     juce::StringArray buttons, std::function<void (int, const juce::String&)> onResult);
    void showMessage (const juce::String& title, const juce::String& text);
    std::unique_ptr<juce::AudioProcessorValueTreeState::ComboBoxAttachment> distTypeAtt, filterTypeAtt;
    DemonEyes eyes;
    juce::Image background;
    juce::Rectangle<int> meterArea;
    float flicker = 0.0f, inMeter = 0.0f, outMeter = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PossessedEditor)
};
