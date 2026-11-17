#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace halo
{
namespace choices
{
    inline const juce::StringArray quality   { "Eco 1x", "High 2x", "Ultra 4x", "Insane 8x" };
    inline const juce::StringArray compStyle { "Clean", "Opto", "FET", "Vari-Mu" };
    inline const juce::StringArray voices    { "2 Voices", "4 Voices", "6 Voices", "8 Voices" };
    inline const juce::StringArray revType   { "Room", "Studio", "Plate", "Hall", "Cathedral", "Ambient" };
    inline const juce::StringArray dlyType   { "Mono", "Ping-Pong", "Wide", "Tape", "Slap" };
    inline const juce::StringArray dlyRate   { "1/32", "1/16T", "1/16", "1/16D", "1/8T", "1/8", "1/8D", "1/4T", "1/4", "1/4D", "1/2", "1/1" };
    inline constexpr float dlyBeats[] { 0.125f, 0.1666667f, 0.25f, 0.375f, 0.3333333f, 0.5f, 0.75f, 0.6666667f, 1.0f, 1.5f, 2.0f, 4.0f };
    inline const juce::StringArray satType   { "Tape", "Tube", "Broken Tube", "Transistor", "Diode", "Fold" };
}

inline juce::String fmtHz (float v)
{
    return v >= 1000.0f ? juce::String (v / 1000.0f, v >= 10000.0f ? 1 : 2) + " kHz" : juce::String ((int) std::round (v)) + " Hz";
}

inline juce::AudioProcessorValueTreeState::ParameterLayout createLayout()
{
    using namespace juce;
    AudioProcessorValueTreeState::ParameterLayout layout;

    auto logRange = [] (float lo, float hi)
    {
        return NormalisableRange<float> (lo, hi,
            [] (float a, float b, float t) { return a * std::pow (b / a, t); },
            [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); },
            [] (float, float, float v) { return v; });
    };

    auto pct = [&] (const char* id, const char* name, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (0, 100, 0.01f), def,
            AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String ((int) std::round (v)) + "%"; })));
    };
    auto db = [&] (const char* id, const char* name, float lo, float hi, float def)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, NormalisableRange<float> (lo, hi, 0.01f), def,
            AudioParameterFloatAttributes().withLabel ("dB").withStringFromValueFunction ([] (float v, int) { return (v > 0.005f ? "+" : "") + String (v, 1) + " dB"; })));
    };
    auto hz = [&] (const char* id, const char* name, float lo, float hi, float def, const char* offText = nullptr, bool offIsLow = true)
    {
        String off = offText != nullptr ? String (offText) : String();
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, logRange (lo, hi), def,
            AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction ([=] (float v, int)
            {
                if (off.isNotEmpty() && ((offIsLow && v <= lo * 1.05f) || (! offIsLow && v >= hi * 0.977f))) return off;
                return fmtHz (v);
            })));
    };
    auto ms = [&] (const char* id, const char* name, float lo, float hi, float def, bool log)
    {
        layout.add (std::make_unique<AudioParameterFloat> (ParameterID { id, 1 }, name, log ? logRange (lo, hi) : NormalisableRange<float> (lo, hi, 0.01f), def,
            AudioParameterFloatAttributes().withLabel ("ms").withStringFromValueFunction ([] (float v, int) { return v >= 1000 ? String (v / 1000.0f, 2) + " s" : String (v, v < 10 ? 1 : 0) + " ms"; })));
    };
    auto onoff = [&] (const char* id, const char* name, bool def) { layout.add (std::make_unique<AudioParameterBool> (ParameterID { id, 1 }, name, def)); };
    auto choice = [&] (const char* id, const char* name, const StringArray& c, int def) { layout.add (std::make_unique<AudioParameterChoice> (ParameterID { id, 1 }, name, c, def)); };

    // ---- Global ----
    db ("in_gain", "Input", -24, 24, 0);
    db ("out_gain", "Output", -24, 24, 0);
    pct ("mix", "Dry/Wet", 100);
    onoff ("bypass", "Bypass", false);
    choice ("quality", "Quality", choices::quality, 2);

    // ---- Compress ----
    onoff ("comp_on", "Compress On", true);
    db ("comp_input", "Comp Input", -24, 24, 0);
    pct ("comp_amount", "Compression", 35);
    choice ("comp_style", "Comp Style", choices::compStyle, 1);
    pct ("comp_speed", "Comp Speed", 50);
    db ("comp_output", "Comp Output", -24, 24, 0);
    onoff ("comp_auto", "Comp Auto Gain", true);
    pct ("comp_mix", "Comp Mix", 100);
    hz ("comp_hpf", "Comp SC HPF", 20, 500, 80);

    // ---- Enhance ----
    onoff ("enh_on", "Enhance On", true);
    pct ("enh_deess", "De-Ess", 25);
    hz ("enh_deess_freq", "De-Ess Freq", 2500, 12000, 6500);
    pct ("enh_denoise", "De-Noise", 0);
    pct ("enh_air", "Air", 20);
    pct ("enh_body", "Body", 10);
    onoff ("enh_pre", "Enhance Pre Comp", false);

    // ---- EQ ----
    onoff ("eq_on", "EQ On", true);
    onoff ("eq_pre", "EQ Pre Comp", false);
    hz ("eq_hpf", "Low Cut", 10, 1000, 10, "Off", true);
    hz ("eq_lpf", "High Cut", 1000, 22000, 22000, "Off", false);
    hz ("eq_low_freq", "Low Freq", 20, 1000, 120);
    db ("eq_low_gain", "Low Gain", -18, 18, 0);
    hz ("eq_m1_freq", "Mid 1 Freq", 80, 6000, 400);
    db ("eq_m1_gain", "Mid 1 Gain", -18, 18, 0);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "eq_m1_q", 1 }, "Mid 1 Q", logRange (0.1f, 10.0f), 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "Q " + String (v, 2); })));
    hz ("eq_m2_freq", "Mid 2 Freq", 400, 16000, 3000);
    db ("eq_m2_gain", "Mid 2 Gain", -18, 18, 0);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "eq_m2_q", 1 }, "Mid 2 Q", logRange (0.1f, 10.0f), 1.0f,
        AudioParameterFloatAttributes().withStringFromValueFunction ([] (float v, int) { return "Q " + String (v, 2); })));
    hz ("eq_high_freq", "High Freq", 1000, 20000, 10000);
    db ("eq_high_gain", "High Gain", -18, 18, 0);

    // ---- Multiply ----
    onoff ("mul_on", "Multiply On", true);
    pct ("mul_amount", "Multiply Amount", 0);
    pct ("mul_spread", "Spread", 70);
    pct ("mul_offset", "Offset", 40);
    pct ("mul_detune", "Detune", 35);
    hz ("mul_low", "Multiply Low Cut", 20, 800, 200);
    choice ("mul_voices", "Voices", choices::voices, 1);

    // ---- Space ----
    onoff ("space_on", "Space On", true);
    pct ("rev_mix", "Reverb", 0);
    choice ("rev_type", "Reverb Type", choices::revType, 2);
    pct ("rev_decay", "Decay", 45);
    ms ("rev_predelay", "Pre-Delay", 0, 250, 20, false);
    pct ("rev_damp", "Damping", 40);
    pct ("rev_width", "Reverb Width", 100);
    pct ("rev_duck", "Reverb Duck", 0);
    hz ("rev_lowcut", "Reverb Low Cut", 20, 1000, 180);
    pct ("dly_mix", "Delay", 0);
    choice ("dly_type", "Delay Type", choices::dlyType, 1);
    onoff ("dly_sync", "Delay Sync", true);
    choice ("dly_rate", "Delay Rate", choices::dlyRate, 5);
    ms ("dly_time", "Delay Time", 1, 2000, 250, true);
    pct ("dly_feedback", "Feedback", 35);
    pct ("dly_tone", "Delay Tone", 60);
    pct ("dly_duck", "Delay Duck", 30);

    // ---- Drive ----
    onoff ("sat_on", "Drive On", true);
    choice ("sat_type", "Drive Type", choices::satType, 1);
    pct ("sat_drive", "Drive", 15);
    pct ("sat_mix", "Drive Mix", 100);
    db ("sat_gain", "Drive Gain", -24, 24, 0);
    onoff ("sat_prefx", "Drive Pre FX", false);
    onoff ("sat_clip", "Drive Clip", false);

    // ---- Master ----
    onoff ("mst_on", "Master On", true);
    db ("mst_tilt", "Tilt", -6, 6, 0);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mst_width", 1 }, "Width", NormalisableRange<float> (0, 200, 0.01f), 100.0f,
        AudioParameterFloatAttributes().withLabel ("%").withStringFromValueFunction ([] (float v, int) { return String ((int) std::round (v)) + "%"; })));
    hz ("mst_mono", "Bass Mono", 20, 400, 20, "Off", true);
    pct ("mst_glue", "Glue", 0);
    db ("mst_loud", "Loudness", 0, 24, 0);
    db ("mst_ceiling", "Ceiling", -12, 0, -0.5f);
    ms ("mst_release", "Release", 1, 1000, 80, true);
    onoff ("mst_auto", "Auto Loudness", false);
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mst_target", 1 }, "Target Loudness", NormalisableRange<float> (-30, -4, 0.1f), -14.0f,
        AudioParameterFloatAttributes().withLabel ("LUFS").withStringFromValueFunction ([] (float v, int) { return String (v, 1) + " LUFS"; })));
    layout.add (std::make_unique<AudioParameterFloat> (ParameterID { "mst_tol", 1 }, "Loudness Tolerance", NormalisableRange<float> (0.1f, 3.0f, 0.1f), 0.5f,
        AudioParameterFloatAttributes().withLabel ("LU").withStringFromValueFunction ([] (float v, int) { return "+/-" + String (v, 1) + " LU"; })));

    return layout;
}
} // namespace halo
