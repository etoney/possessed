// Factory presets and parameter-ID mapping (shared by the plugin and the offline tests).
#pragma once
#include "PossessedEngine.h"
#include <cstring>
#include <utility>
#include <vector>

namespace poss
{
struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };

// { id, value } pairs applied over the defaults. Choice params use their index.
inline const std::vector<Preset>& factoryPresets()
{
    static const std::vector<Preset> p {
        { "Init (Clean)", {} },
        { "Demon", { { "outputDb", -4.5f }, { "pitch", -12 }, { "formant", -5 }, { "layerPitch", -19 }, { "layerFormant", -7 }, { "layerLevel", 0.55f },
                     { "growl", 0.7f }, { "rasp", 0.4f }, { "distType", 0 }, { "distDrive", 0.3f },
                     { "filterType", 1 }, { "cutoff", 6000 }, { "resonance", 0.1f },
                     { "legion", 0.25f }, { "legionSpread", 0.3f }, { "reverbMix", 0.25f }, { "reverbSize", 0.5f } } },
        { "Disturbed Child", { { "outputDb", 4.0f }, { "pitch", 6 }, { "formant", 5 }, { "layerPitch", 6.4f }, { "layerFormant", 7 }, { "layerLevel", 0.3f },
                               { "whisper", 0.1f }, { "wobble", 0.35f }, { "reverseMix", 0.3f }, { "reverseLength", 0.6f },
                               { "delayMix", 0.15f }, { "delayTime", 420 }, { "delayFeedback", 0.35f },
                               { "reverbMix", 0.35f }, { "reverbSize", 0.7f } } },
        { "The Haunting", { { "outputDb", 10.0f }, { "pitch", -2 }, { "formant", -1 }, { "whisper", 0.75f }, { "layerPitch", 12 }, { "layerFormant", 3 },
                            { "layerLevel", 0.3f }, { "filterType", 2 }, { "cutoff", 1800 }, { "resonance", 0.3f },
                            { "wobble", 0.45f }, { "reverseMix", 0.55f }, { "reverseLength", 1.0f },
                            { "delayMix", 0.3f }, { "delayTime", 650 }, { "delayFeedback", 0.55f },
                            { "reverbMix", 0.6f }, { "reverbSize", 0.95f } } },
        { "Possessed", { { "outputDb", 3.5f }, { "pitch", -7 }, { "formant", -3 }, { "layerPitch", 5 }, { "layerFormant", 2 }, { "layerLevel", 0.45f },
                         { "growl", 0.35f }, { "rasp", 0.3f }, { "ringMix", 0.15f }, { "ringFreq", 47 },
                         { "distType", 0 }, { "distDrive", 0.25f }, { "wobble", 0.2f }, { "reverbMix", 0.3f } } },
        { "Legion", { { "outputDb", 2.5f }, { "pitch", -5 }, { "formant", -2 }, { "layerPitch", -12 }, { "layerFormant", -4 }, { "layerLevel", 0.6f },
                      { "legion", 1.0f }, { "legionSpread", 0.8f }, { "growl", 0.25f },
                      { "reverbMix", 0.35f }, { "reverbSize", 0.6f } } },
        { "Hellhound", { { "outputDb", -7.5f }, { "pitch", -17 }, { "formant", -8 }, { "growl", 1.0f }, { "rasp", 0.7f }, { "distType", 1 }, { "distDrive", 0.4f },
                         { "filterType", 1 }, { "cutoff", 2600 }, { "resonance", 0.25f }, { "reverbMix", 0.2f }, { "reverbSize", 0.4f } } },
        { "Ghost Whisper", { { "outputDb", 6.5f }, { "whisper", 1.0f }, { "formant", 2 }, { "filterType", 3 }, { "cutoff", 400 }, { "resonance", 0.1f },
                             { "wobble", 0.25f }, { "delayMix", 0.25f }, { "delayTime", 400 }, { "delayFeedback", 0.4f },
                             { "reverbMix", 0.55f }, { "reverbSize", 0.85f } } },
        { "The Entity", { { "outputDb", 5.0f }, { "pitch", -9 }, { "formant", 3 }, { "ringMix", 0.55f }, { "ringFreq", 31 }, { "distType", 2 }, { "distDrive", 0.35f },
                          { "legion", 0.4f }, { "legionSpread", 0.2f }, { "delayMix", 0.2f }, { "delayTime", 180 },
                          { "delayFeedback", 0.5f }, { "reverbMix", 0.3f } } },
        { "Backwards Prayer", { { "outputDb", -1.5f }, { "pitch", -3 }, { "formant", -2 }, { "whisper", 0.2f }, { "reverseMix", 0.85f }, { "reverseLength", 1.2f },
                                { "reverbMix", 0.7f }, { "reverbSize", 1.0f }, { "mix", 0.85f } } },
        { "Static From Beyond", { { "outputDb", -2.5f }, { "pitch", -4 }, { "formant", -1 }, { "distType", 3 }, { "distDrive", 0.6f }, { "filterType", 2 },
                                  { "cutoff", 1500 }, { "resonance", 0.3f }, { "wobble", 0.5f }, { "ringMix", 0.1f }, { "ringFreq", 900 },
                                  { "delayMix", 0.2f }, { "delayTime", 300 }, { "reverbMix", 0.25f } } },
        { "Old Witch", { { "outputDb", 3.0f }, { "pitch", 4 }, { "formant", 1 }, { "rasp", 0.75f }, { "wobble", 0.3f }, { "legion", 0.2f },
                         { "legionSpread", 0.6f }, { "reverbMix", 0.3f }, { "reverbSize", 0.55f } } },
        { "The Abyss", { { "outputDb", -2.5f }, { "pitch", -24 }, { "formant", -10 }, { "layerPitch", -12 }, { "layerFormant", -6 }, { "layerLevel", 0.5f },
                         { "growl", 0.6f }, { "filterType", 1 }, { "cutoff", 3500 }, { "delayMix", 0.2f }, { "delayTime", 900 },
                         { "delayFeedback", 0.5f }, { "reverbMix", 0.75f }, { "reverbSize", 1.0f } } },
        { "Siren of the Dead", { { "outputDb", -1.5f }, { "pitch", 12 }, { "formant", 4 }, { "whisper", 0.3f }, { "layerPitch", 7 }, { "layerFormant", 4 },
                                 { "layerLevel", 0.45f }, { "wobble", 0.6f }, { "reverseMix", 0.3f },
                                 { "reverbMix", 0.65f }, { "reverbSize", 0.9f } } },
    };
    return p;
}

// Write a parameter value (by ID) into the engine's Params struct.
inline void setParam (Params& p, const char* id, float v)
{
    struct E { const char* id; float Params::* f; };
    static const E floats[] = {
        { "inputDb", &Params::inputDb }, { "pitch", &Params::pitch }, { "formant", &Params::formant }, { "whisper", &Params::whisper },
        { "layerPitch", &Params::layerPitch }, { "layerFormant", &Params::layerFormant }, { "layerLevel", &Params::layerLevel },
        { "growl", &Params::growl }, { "rasp", &Params::rasp }, { "ringMix", &Params::ringMix }, { "ringFreq", &Params::ringFreq },
        { "distDrive", &Params::distDrive }, { "cutoff", &Params::cutoff }, { "resonance", &Params::resonance },
        { "wobble", &Params::wobble }, { "legion", &Params::legion }, { "legionSpread", &Params::legionSpread },
        { "reverseMix", &Params::reverseMix }, { "reverseLength", &Params::reverseLength },
        { "delayMix", &Params::delayMix }, { "delayTime", &Params::delayTime }, { "delayFeedback", &Params::delayFeedback },
        { "reverbMix", &Params::reverbMix }, { "reverbSize", &Params::reverbSize }, { "mix", &Params::mix }, { "outputDb", &Params::outputDb },
    };
    for (auto& e : floats) if (std::strcmp (e.id, id) == 0) { p.*(e.f) = v; return; }
    if (std::strcmp (id, "distType") == 0)   { p.distType = (int) (v + 0.5f); return; }
    if (std::strcmp (id, "filterType") == 0) { p.filterType = (int) (v + 0.5f); return; }
}
} // namespace poss
