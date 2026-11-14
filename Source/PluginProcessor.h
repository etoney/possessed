#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "Presets.h"

class PossessedProcessor : public juce::AudioProcessor
{
public:
    PossessedProcessor();
    ~PossessedProcessor() override = default;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "Possessed"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 15.0; }

    int getNumPrograms() override;
    int getCurrentProgram() override { return currentProgram; }
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createLayout();
    juce::AudioProcessorValueTreeState apvts;

    std::atomic<float> inLevel { 0.0f }, outLevel { 0.0f }; // for the meters

    // ---- user presets (stored as files, shared by every project and DAW)
    static juce::File getUserPresetFolder();
    static juce::Array<juce::File> getUserPresets();
    static juce::File fileForPresetName (const juce::String& name);
    bool saveUserPreset (const juce::String& name);
    bool loadUserPreset (const juce::File& file);
    juce::String getCurrentPresetName() const { return currentPresetName; }
    std::atomic<int> presetVersion { 0 }; // bumped whenever a preset is loaded or saved
    std::atomic<int> currentStep { -1 };  // stutter playhead, for the UI
    static bool isFxParam (const juce::String& id); // FX rack params are kept when loading factory presets

private:
    void pullParams();
    void pullFxParams();
    std::array<std::atomic<float>*, 16> stepPtrs {};

    poss::Engine engine;
    std::vector<std::pair<juce::String, std::atomic<float>*>> paramPtrs;
    int currentProgram = 0;
    juce::String currentPresetName { "Init (Clean)" };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PossessedProcessor)
};
