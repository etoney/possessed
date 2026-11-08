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
    // distType: 0 Tube, 1 Fuzz, 2 Bitcrush, 3 Radio, 4 Saturate | filterType: 0 Off, 1 LP, 2 BP, 3 HP
    // delayType: 0 Digital, 1 Ping-Pong, 2 Analog, 3 Tape
    static const std::vector<Preset> p {
        { "Init (Clean)", {} },
        { "Demon", { { "outputDb", -5.5f }, { "pitch", -12 }, { "formant", -5 }, { "layerPitch", -19 }, { "layerFormant", -7 }, { "layerLevel", 0.45f },
                     { "growl", 0.6f }, { "rasp", 0.3f }, { "distType", 4 }, { "distDrive", 0.45f },
                     { "filterType", 1 }, { "cutoff", 7000 }, { "resonance", 0.1f },
                     { "legion", 0.15f }, { "legionSpread", 0.3f }, { "reverbMix", 0.18f }, { "reverbSize", 0.45f } } },
        { "Lizzy's Playhouse", { { "outputDb", 0.0f }, { "pitch", 11 }, { "formant", 6.5f }, { "layerPitch", 22 }, { "layerFormant", 8 }, { "layerLevel", 0.12f },
                                 { "wobble", 0.2f }, { "reverseMix", 0.12f }, { "reverseLength", 0.5f }, { "distType", 4 }, { "distDrive", 0.2f },
                                 { "delayType", 3 }, { "delayMix", 0.14f }, { "delayTime", 380 }, { "delayFeedback", 0.3f },
                                 { "reverbMix", 0.22f }, { "reverbSize", 0.4f }, { "lofi", 0.35f }, { "lofiNoise", 0.15f } } },
        { "The Haunting", { { "outputDb", 3.0f }, { "pitch", -2 }, { "formant", -1 }, { "whisper", 0.35f }, { "layerPitch", 12 }, { "layerFormant", 3 },
                            { "layerLevel", 0.2f }, { "filterType", 1 }, { "cutoff", 6500 }, { "resonance", 0.15f },
                            { "wobble", 0.3f }, { "reverseMix", 0.25f }, { "reverseLength", 0.8f },
                            { "delayType", 3 }, { "delayMix", 0.2f }, { "delayTime", 600 }, { "delayFeedback", 0.45f },
                            { "reverbMix", 0.4f }, { "reverbSize", 0.85f }, { "mix", 0.8f } } },
        { "Possessed", { { "outputDb", 0.0f }, { "pitch", -7 }, { "formant", -3 }, { "layerPitch", 5 }, { "layerFormant", 2 }, { "layerLevel", 0.4f },
                         { "growl", 0.3f }, { "rasp", 0.25f }, { "ringMix", 0.1f }, { "ringFreq", 47 },
                         { "distType", 4 }, { "distDrive", 0.4f }, { "wobble", 0.15f }, { "reverbMix", 0.2f } } },
        { "Legion", { { "outputDb", -1.5f }, { "pitch", -5 }, { "formant", -2 }, { "layerPitch", -12 }, { "layerFormant", -4 }, { "layerLevel", 0.5f },
                      { "legion", 0.9f }, { "legionSpread", 0.7f }, { "growl", 0.2f }, { "distType", 4 }, { "distDrive", 0.3f },
                      { "reverbMix", 0.3f }, { "reverbSize", 0.6f }, { "mix", 0.9f } } },
        { "Hellhound", { { "outputDb", -5.0f }, { "pitch", -14 }, { "formant", -7 }, { "growl", 0.9f }, { "rasp", 0.5f }, { "distType", 1 }, { "distDrive", 0.3f },
                         { "filterType", 1 }, { "cutoff", 4000 }, { "resonance", 0.2f }, { "reverbMix", 0.15f }, { "reverbSize", 0.4f },
                         { "mix", 0.85f } } },
        { "The Entity", { { "outputDb", 2.0f }, { "pitch", -6 }, { "formant", 3 }, { "ringMix", 0.28f }, { "ringFreq", 31 }, { "distType", 2 }, { "distDrive", 0.25f },
                          { "legion", 0.3f }, { "legionSpread", 0.2f }, { "delayType", 0 }, { "delayMix", 0.18f }, { "delayTime", 180 },
                          { "delayFeedback", 0.45f }, { "reverbMix", 0.2f }, { "mix", 0.9f } } },
        { "Static From Beyond", { { "outputDb", 0.0f }, { "pitch", -3 }, { "formant", -1 }, { "distType", 3 }, { "distDrive", 0.45f }, { "wobble", 0.35f },
                                  { "lofi", 0.3f }, { "lofiNoise", 0.35f }, { "delayType", 3 }, { "delayMix", 0.2f }, { "delayTime", 300 },
                                  { "delayFeedback", 0.35f }, { "reverbMix", 0.25f }, { "mix", 0.9f } } },
        { "Old Witch", { { "outputDb", -2.5f }, { "pitch", 4 }, { "formant", 1 }, { "rasp", 0.65f }, { "wobble", 0.25f }, { "legion", 0.15f }, { "legionSpread", 0.6f },
                         { "distType", 4 }, { "distDrive", 0.3f }, { "delayType", 2 }, { "delayMix", 0.12f }, { "delayTime", 260 },
                         { "delayFeedback", 0.3f }, { "reverbMix", 0.25f }, { "reverbSize", 0.5f } } },
        { "Siren of the Dead", { { "outputDb", 2.5f }, { "pitch", 12 }, { "formant", 4 }, { "whisper", 0.15f }, { "layerPitch", 7 }, { "layerFormant", 4 },
                                 { "layerLevel", 0.35f }, { "wobble", 0.45f }, { "reverseMix", 0.2f },
                                 { "delayType", 3 }, { "delayMix", 0.2f }, { "delayTime", 520 }, { "delayFeedback", 0.5f },
                                 { "reverbMix", 0.5f }, { "reverbSize", 0.9f }, { "mix", 0.75f } } },
        { "Ventriloquist Dummy", { { "outputDb", 6.5f }, { "pitch", 4 }, { "formant", 9 }, { "filterType", 2 }, { "cutoff", 1700 }, { "resonance", 0.25f },
                                   { "distType", 4 }, { "distDrive", 0.25f }, { "wobble", 0.1f },
                                   { "delayType", 0 }, { "delayMix", 0.12f }, { "delayTime", 90 }, { "delayFeedback", 0.1f },
                                   { "reverbMix", 0.1f }, { "reverbSize", 0.2f } } },
        { "Behind You", { { "outputDb", -8.0f }, { "whisper", 0.85f }, { "formant", 1 }, { "distType", 4 }, { "distDrive", 0.3f }, { "filterType", 3 },
                          { "cutoff", 250 }, { "resonance", 0.1f }, { "legion", 0.2f }, { "legionSpread", 0.3f },
                          { "reverbMix", 0.08f }, { "reverbSize", 0.2f } } },
        { "The Drowned", { { "outputDb", 1.0f }, { "pitch", -2 }, { "formant", -2 }, { "filterType", 1 }, { "cutoff", 900 }, { "resonance", 0.55f },
                           { "wobble", 0.7f }, { "legion", 0.25f }, { "legionSpread", 0.6f }, { "delayType", 2 }, { "delayMix", 0.2f },
                           { "delayTime", 240 }, { "delayFeedback", 0.45f }, { "reverbMix", 0.35f }, { "reverbSize", 0.7f }, { "lofi", 0.1f } } },
        { "Found Footage", { { "outputDb", -3.5f }, { "pitch", -1 }, { "formant", -0.5f }, { "lofi", 0.75f }, { "lofiNoise", 0.45f }, { "distType", 4 }, { "distDrive", 0.35f },
                             { "wobble", 0.2f }, { "delayType", 3 }, { "delayMix", 0.1f }, { "delayTime", 310 }, { "delayFeedback", 0.25f },
                             { "reverbMix", 0.12f }, { "reverbSize", 0.35f } } },
        { "Cult Chant", { { "outputDb", -1.0f }, { "pitch", -5 }, { "formant", -2.5f }, { "layerPitch", -12 }, { "layerFormant", -5 }, { "layerLevel", 0.45f },
                          { "legion", 1.0f }, { "legionSpread", 0.5f }, { "distType", 4 }, { "distDrive", 0.25f },
                          { "delayType", 2 }, { "delayMix", 0.2f }, { "delayTime", 500 }, { "delayFeedback", 0.35f },
                          { "reverbMix", 0.4f }, { "reverbSize", 0.85f }, { "mix", 0.75f } } },
        { "Wendigo", { { "outputDb", -4.0f }, { "pitch", -3 }, { "formant", 4 }, { "rasp", 0.45f }, { "wobble", 0.3f }, { "layerPitch", 12 }, { "layerFormant", 6 },
                       { "layerLevel", 0.2f }, { "distType", 0 }, { "distDrive", 0.3f }, { "delayType", 3 }, { "delayMix", 0.15f },
                       { "delayTime", 430 }, { "delayFeedback", 0.35f }, { "reverbMix", 0.3f }, { "reverbSize", 0.7f } } },
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
        { "lofi", &Params::lofi }, { "lofiNoise", &Params::lofiNoise },
    };
    for (auto& e : floats) if (std::strcmp (e.id, id) == 0) { p.*(e.f) = v; return; }
    if (std::strcmp (id, "distType") == 0)   { p.distType = (int) (v + 0.5f); return; }
    if (std::strcmp (id, "filterType") == 0) { p.filterType = (int) (v + 0.5f); return; }
    if (std::strcmp (id, "delayType") == 0)  { p.delayType = (int) (v + 0.5f); return; }
}
} // namespace poss
