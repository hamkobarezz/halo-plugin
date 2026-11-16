#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace halo
{
struct FactoryPreset
{
    juce::String category, name;
    std::vector<std::pair<juce::String, float>> values;
};

class PresetManager
{
public:
    explicit PresetManager (juce::AudioProcessorValueTreeState& s);

    static const std::vector<FactoryPreset>& factory();
    juce::File userDir() const;
    juce::Array<juce::File> userPresets() const;

    void loadFactory (int index);
    bool loadUser (const juce::File& f);
    bool saveUser (const juce::String& name);
    void initDefaults();
    void step (int delta);

    juce::String currentName { "Init" };
    int currentIndex = -1; // index into the combined list (factory then user), -1 = none

private:
    void setParam (const juce::String& id, float realValue);
    juce::AudioProcessorValueTreeState& apvts;
};
} // namespace halo
