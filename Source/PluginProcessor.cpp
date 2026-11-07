#include "PluginProcessor.h"
#include "PluginEditor.h"

using APVTS = juce::AudioProcessorValueTreeState;

APVTS::ParameterLayout PossessedProcessor::createLayout()
{
    using namespace juce;
    std::vector<std::unique_ptr<RangedAudioParameter>> ps;
    auto add = [&] (const char* id, const char* name, float lo, float hi, float def, float skewCentre = -1.0f, const char* suffix = "", float interval = 0.0f)
    {
        NormalisableRange<float> r (lo, hi, interval);
        if (skewCentre > 0) r.setSkewForCentre (skewCentre);
        const String sfx (suffix);
        auto toText = [sfx] (float v, int)
        {
            if (sfx == "st") return (v > 0.04f ? "+" : "") + String (v, 1) + " st";
            if (sfx == "Hz") return v >= 1000.0f ? String (v / 1000.0f, 1) + " kHz" : String (roundToInt (v)) + " Hz";
            if (sfx == "dB") return String (v, 1) + " dB";
            if (sfx == "s")  return String (v, 2) + " s";
            if (sfx == "ms") return String (roundToInt (v)) + " ms";
            return String (roundToInt (v * 100.0f)) + "%";
        };
        auto fromText = [sfx] (const String& t)
        {
            const float v = t.retainCharacters ("-+.0123456789").getFloatValue();
            if (sfx == "Hz" && t.containsIgnoreCase ("k")) return v * 1000.0f;
            if (sfx.isEmpty()) return v / 100.0f;
            return v;
        };
        ps.push_back (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, r, def,
                                                             AudioParameterFloatAttributes().withLabel (suffix)
                                                                 .withStringFromValueFunction (toText)
                                                                 .withValueFromStringFunction (fromText)));
    };
    auto choice = [&] (const char* id, const char* name, StringArray items, int def)
    {
        ps.push_back (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, items, def));
    };

    add ("inputDb", "Input", -24, 24, 0, -1, "dB");
    add ("pitch", "Pitch", -24, 24, 0, -1, "st", 0.1f);
    add ("formant", "Formant", -12, 12, 0, -1, "st", 0.1f);
    add ("whisper", "Whisper", 0, 1, 0);
    add ("layerPitch", "Layer Pitch", -24, 24, -12, -1, "st", 0.1f);
    add ("layerFormant", "Layer Formant", -12, 12, 0, -1, "st", 0.1f);
    add ("layerLevel", "Layer Level", 0, 1, 0);
    add ("growl", "Growl", 0, 1, 0);
    add ("rasp", "Rasp", 0, 1, 0);
    add ("ringMix", "Ring Mix", 0, 1, 0);
    add ("ringFreq", "Ring Freq", 5, 2000, 80, 150, "Hz");
    choice ("distType", "Distortion Type", { "Tube", "Fuzz", "Bitcrush", "Radio" }, 0);
    add ("distDrive", "Distortion Drive", 0, 1, 0);
    choice ("filterType", "Filter Type", { "Off", "Low-pass", "Band-pass", "High-pass" }, 0);
    add ("cutoff", "Cutoff", 60, 18000, 2000, 1500, "Hz");
    add ("resonance", "Resonance", 0, 1, 0.2f);
    add ("wobble", "Wobble", 0, 1, 0);
    add ("legion", "Legion", 0, 1, 0);
    add ("legionSpread", "Legion Spread", 0, 1, 0.5f);
    add ("reverseMix", "Reverse Mix", 0, 1, 0);
    add ("reverseLength", "Reverse Length", 0.1f, 1.5f, 0.5f, -1, "s");
    add ("delayMix", "Delay Mix", 0, 1, 0);
    add ("delayTime", "Delay Time", 20, 1500, 350, 300, "ms");
    add ("delayFeedback", "Delay Feedback", 0, 0.9f, 0.4f);
    add ("reverbMix", "Reverb Mix", 0, 1, 0);
    add ("reverbSize", "Reverb Size", 0, 1, 0.6f);
    add ("mix", "Dry/Wet", 0, 1, 1);
    add ("outputDb", "Output", -36, 12, 0, -1, "dB");
    return { ps.begin(), ps.end() };
}

PossessedProcessor::PossessedProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "POSSESSED", createLayout())
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            paramPtrs.push_back ({ rp->getParameterID(), apvts.getRawParameterValue (rp->getParameterID()) });
}

bool PossessedProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto& out = layouts.getMainOutputChannelSet();
    const auto& in = layouts.getMainInputChannelSet();
    if (out != juce::AudioChannelSet::stereo() && out != juce::AudioChannelSet::mono()) return false;
    return in == out || (in == juce::AudioChannelSet::mono() && out == juce::AudioChannelSet::stereo());
}

void PossessedProcessor::pullParams()
{
    for (auto& [id, ptr] : paramPtrs) poss::setParam (engine.params, id.toRawUTF8(), ptr->load());
}

void PossessedProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    pullParams();
    engine.prepare (sampleRate, samplesPerBlock);
    engine.snapSmoothers();
    setLatencySamples (engine.getLatency());
}

void PossessedProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    pullParams();

    const int n = buffer.getNumSamples();
    const int inCh = getTotalNumInputChannels();
    if (inCh == 1 && buffer.getNumChannels() > 1) buffer.copyFrom (1, 0, buffer, 0, 0, n); // mono in -> stereo out

    inLevel.store (std::max (inLevel.load() * 0.8f, buffer.getMagnitude (0, 0, n)));
    float* L = buffer.getWritePointer (0);
    float* R = buffer.getNumChannels() > 1 ? buffer.getWritePointer (1) : L;
    engine.process (L, R, n);
    outLevel.store (std::max (outLevel.load() * 0.8f, buffer.getMagnitude (0, n)));

    for (int ch = 2; ch < buffer.getNumChannels(); ++ch) buffer.clear (ch, 0, n);
}

int PossessedProcessor::getNumPrograms() { return (int) poss::factoryPresets().size(); }

const juce::String PossessedProcessor::getProgramName (int index)
{
    return juce::isPositiveAndBelow (index, getNumPrograms()) ? poss::factoryPresets()[(size_t) index].name : "";
}

void PossessedProcessor::setCurrentProgram (int index)
{
    if (! juce::isPositiveAndBelow (index, getNumPrograms())) return;
    currentProgram = index;
    currentPresetName = poss::factoryPresets()[(size_t) index].name;
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "inputDb") // keep the user's input gain staging
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto& [id, v] : poss::factoryPresets()[(size_t) index].values)
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 (v));
    ++presetVersion;
}

//==============================================================================
juce::File PossessedProcessor::getUserPresetFolder()
{
    auto base = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory); // ~/Library on macOS, %APPDATA% on Windows
   #if JUCE_MAC
    base = base.getChildFile ("Application Support");
   #endif
    return base.getChildFile ("Castle Danger").getChildFile ("Possessed").getChildFile ("Presets");
}

juce::Array<juce::File> PossessedProcessor::getUserPresets()
{
    auto files = getUserPresetFolder().findChildFiles (juce::File::findFiles, false, "*.possessed");
    std::sort (files.begin(), files.end(), [] (const juce::File& a, const juce::File& b)
               { return a.getFileNameWithoutExtension().compareNatural (b.getFileNameWithoutExtension()) < 0; });
    return files;
}

juce::File PossessedProcessor::fileForPresetName (const juce::String& name)
{
    return getUserPresetFolder().getChildFile (juce::File::createLegalFileName (name.trim()) + ".possessed");
}

bool PossessedProcessor::saveUserPreset (const juce::String& rawName)
{
    const auto name = rawName.trim();
    if (name.isEmpty()) return false;
    const auto file = fileForPresetName (name);
    if (! file.getParentDirectory().createDirectory()) return false;

    juce::XmlElement xml ("PossessedPreset");
    xml.setAttribute ("name", name);
    xml.setAttribute ("pluginVersion", JucePlugin_VersionString);
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            auto* e = xml.createNewChildElement ("PARAM");
            e->setAttribute ("id", rp->getParameterID());
            e->setAttribute ("value", (double) rp->convertFrom0to1 (rp->getValue()));
        }
    if (! xml.writeTo (file)) return false;
    currentPresetName = name;
    ++presetVersion;
    return true;
}

bool PossessedProcessor::loadUserPreset (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName ("PossessedPreset")) return false;
    // Start from defaults so parameters added in later versions behave sensibly
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "inputDb")
                rp->setValueNotifyingHost (rp->getDefaultValue());
    for (auto* e : xml->getChildWithTagNameIterator ("PARAM"))
    {
        const auto id = e->getStringAttribute ("id");
        if (id == "inputDb") continue; // like factory presets, keep the user's input gain staging
        if (auto* rp = apvts.getParameter (id))
            rp->setValueNotifyingHost (rp->convertTo0to1 ((float) e->getDoubleAttribute ("value")));
    }
    currentPresetName = xml->getStringAttribute ("name", file.getFileNameWithoutExtension());
    ++presetVersion;
    return true;
}

void PossessedProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("program", currentProgram, nullptr);
    state.setProperty ("presetName", currentPresetName, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void PossessedProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto vt = juce::ValueTree::fromXml (*xml);
            currentProgram = vt.getProperty ("program", 0);
            currentPresetName = vt.getProperty ("presetName", getProgramName (currentProgram)).toString();
            apvts.replaceState (vt);
            ++presetVersion;
        }
}

juce::AudioProcessorEditor* PossessedProcessor::createEditor() { return new PossessedEditor (*this); }

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new PossessedProcessor(); }
