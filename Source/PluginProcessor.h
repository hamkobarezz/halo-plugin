#pragma once
#include "Params.h"
#include "Presets.h"
#include "dsp/Common.h"
#include "dsp/Dynamics.h"
#include "dsp/Equalizer.h"
#include "dsp/Space.h"
#include "dsp/Drive.h"
#include "dsp/Master.h"
#include <unordered_map>

namespace halo
{
// Values the UI reads to animate and meter (written by the audio thread)
struct Meters
{
    std::atomic<float> inL { 0 }, inR { 0 }, outL { 0 }, outR { 0 };
    std::atomic<float> compGr { 0 }, essGr { 0 }, noiseGr { 0 }, eqAct { 0 }, mulAct { 0 }, revAct { 0 }, dlyAct { 0 },
                       satAct { 0 }, glueGr { 0 }, limGr { 0 }, autoGain { 0 }, lufsM { -70 }, lufsS { -70 }, lufsI { -70 }, peak { 0 };
    std::atomic<bool> resetLoudness { false };
    float uiLim = 0, uiOut = 0, uiComp = 0; // message-thread copy of limGr, refreshed once per UI frame
};

class HaloProcessor : public juce::AudioProcessor, private juce::Timer
{
public:
    HaloProcessor();
    ~HaloProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override {}
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    void processBlockBypassed (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    const juce::String getName() const override { return "HALO"; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 12.0; }

    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram (int) override {}
    const juce::String getProgramName (int) override { return presets.currentName; }
    void changeProgramName (int, const juce::String&) override {}

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    juce::AudioProcessorParameter* getBypassParameter() const override { return apvts.getParameter ("bypass"); }

    // A/B compare
    void selectSlot (int s);
    void copyToOtherSlot();
    int getSlot() const { return slot; }

    juce::UndoManager undo;
    juce::AudioProcessorValueTreeState apvts;
    PresetManager presets;
    Meters meters;
    AudioFifo preFifo, postFifo;
    std::atomic<double> currentFs { 44100.0 };

private:
    static constexpr int Chunk = 512;
    void timerCallback() override;
    float pv (const char* id) const { return params.at (id)->load (std::memory_order_relaxed); }
    bool pb (const char* id) const { return pv (id) > 0.5f; }
    void updateModules();
    void updateAutoLoudness (int n);
    void processChunk (float* l, float* r, int n);
    int latencyFor (int q) const;

    struct Fade
    {
        juce::SmoothedValue<float> g { 1.0f };
        void prepare (double fs, bool on) { g.reset (fs, 0.02); g.setCurrentAndTargetValue (on ? 1.0f : 0.0f); }
        bool active() const { return g.getTargetValue() > 0.5f || g.isSmoothing(); }
    };
    template <typename Fn> void runFaded (Fade& f, bool on, float* l, float* r, int n, Fn&& fn);

    std::unordered_map<std::string, std::atomic<float>*> params;

    VocalCompressor comp;
    Enhancer enh;
    Equalizer eq;
    Doubler mul;
    FDNReverb rev;
    StereoDelay dly;
    Saturator sat;
    MasterSection master;
    LoudnessMeter loud;

    Fade fComp, fEnh, fEq, fMul, fRev, fDly;
    juce::SmoothedValue<float> inG { 1.0f }, outG { 1.0f }, mixS { 1.0f }, bypassS { 0.0f };
    DelayLine dryLine[2];
    juce::AudioBuffer<float> work, tmp, dryBuf;
    int quality = -1;
    std::atomic<int> latencyNow { 0 };
    double bpm = 120.0;
    bool autoOn = false;
    float autoTarget = -14.0f, autoTol = 0.5f, autoGainDb = 0.0f;
    float peakHold[4] {};

    juce::ValueTree slots[2];
    int slot = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HaloProcessor)
};
} // namespace halo
