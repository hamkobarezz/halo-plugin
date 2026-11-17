#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

// Per-department "character" presets: each one only touches the parameters
// owned by its department, so you can mix e.g. a PUNCHY dynamics with a DREAM space.
namespace halo::dept
{
enum Id { Dynamics = 0, Tone, Space, Fx, Master, Count };

struct Preset { const char* name; std::vector<std::pair<const char*, float>> values; };

inline const char* title (int d)
{
    static const char* t[Count] = { "DYNAMICS", "TONE", "SPACE", "FX", "MASTER" };
    return t[d];
}

// parameters each department owns (presets reset these to default before applying)
inline const std::vector<const char*>& params (int d)
{
    static const std::vector<const char*> p[Count] = {
        { "comp_input", "comp_amount", "comp_style", "comp_speed", "comp_output", "comp_auto", "comp_mix", "comp_hpf",
          "enh_deess", "enh_deess_freq", "enh_denoise", "enh_pre" },
        { "eq_pre", "eq_hpf", "eq_lpf", "eq_low_freq", "eq_low_gain", "eq_m1_freq", "eq_m1_gain", "eq_m1_q",
          "eq_m2_freq", "eq_m2_gain", "eq_m2_q", "eq_high_freq", "eq_high_gain", "enh_air", "enh_body" },
        { "rev_mix", "rev_type", "rev_decay", "rev_predelay", "rev_damp", "rev_width", "rev_duck", "rev_lowcut",
          "dly_mix", "dly_type", "dly_sync", "dly_rate", "dly_time", "dly_feedback", "dly_tone", "dly_duck" },
        { "sat_type", "sat_drive", "sat_mix", "sat_gain", "sat_prefx", "sat_clip",
          "mul_amount", "mul_spread", "mul_offset", "mul_detune", "mul_low", "mul_voices" },
        { "mst_tilt", "mst_width", "mst_mono", "mst_glue", "mst_loud", "mst_ceiling", "mst_release", "mst_auto", "mst_target", "mst_tol" } };
    return p[d];
}

// the on/off switches behind each department's power button (first one is the reference)
inline const std::vector<const char*>& powers (int d)
{
    static const std::vector<const char*> p[Count] = { { "comp_on" }, { "eq_on" }, { "space_on" }, { "sat_on", "mul_on" }, { "mst_on" } };
    return p[d];
}

inline const std::vector<Preset>& presets (int d)
{
    static const std::vector<Preset> list[Count] = {
        { // ---------------- DYNAMICS
            { "SMOOTH",    { { "comp_style", 1 }, { "comp_amount", 40 }, { "comp_speed", 40 }, { "enh_deess", 30 } } },
            { "PUNCHY",    { { "comp_style", 2 }, { "comp_amount", 55 }, { "comp_speed", 70 }, { "enh_deess", 35 } } },
            { "GLUE",      { { "comp_style", 3 }, { "comp_amount", 35 }, { "comp_speed", 35 }, { "enh_deess", 25 } } },
            { "CLEAN",     { { "comp_style", 0 }, { "comp_amount", 30 }, { "comp_speed", 55 }, { "enh_deess", 20 } } },
            { "GENTLE",    { { "comp_style", 1 }, { "comp_amount", 20 }, { "comp_speed", 35 }, { "enh_deess", 15 } } },
            { "BROADCAST", { { "comp_style", 1 }, { "comp_amount", 60 }, { "comp_speed", 50 }, { "enh_deess", 45 }, { "enh_denoise", 25 } } },
            { "SQUASH",    { { "comp_style", 2 }, { "comp_amount", 80 }, { "comp_speed", 85 }, { "comp_mix", 55 }, { "enh_deess", 40 } } },
            { "AGGRO",     { { "comp_style", 2 }, { "comp_amount", 75 }, { "comp_speed", 90 }, { "comp_input", 3 }, { "enh_deess", 45 } } } },
        { // ---------------- TONE
            { "NATURAL",   { { "eq_hpf", 80 }, { "enh_air", 15 }, { "enh_body", 10 } } },
            { "NYC",       { { "eq_hpf", 100 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 3500 }, { "eq_m2_gain", 2.5f },
                             { "eq_high_freq", 11000 }, { "eq_high_gain", 3 }, { "enh_air", 40 }, { "enh_body", 15 } } },
            { "WARM",      { { "eq_low_freq", 150 }, { "eq_low_gain", 2 }, { "eq_m1_gain", -1 }, { "eq_high_freq", 9000 }, { "eq_high_gain", -1.5f },
                             { "enh_air", 5 }, { "enh_body", 40 } } },
            { "BRIGHT",    { { "eq_hpf", 110 }, { "eq_m2_freq", 5000 }, { "eq_m2_gain", 2 }, { "eq_high_freq", 12000 }, { "eq_high_gain", 4 },
                             { "enh_air", 55 }, { "enh_body", 5 } } },
            { "MODERN",    { { "eq_hpf", 120 }, { "eq_m1_freq", 280 }, { "eq_m1_gain", -3 }, { "eq_m2_freq", 4500 }, { "eq_m2_gain", 2 },
                             { "eq_high_freq", 13000 }, { "eq_high_gain", 3 }, { "enh_air", 50 }, { "enh_body", 10 } } },
            { "DARK",      { { "eq_lpf", 9000 }, { "eq_m1_freq", 500 }, { "eq_m1_gain", 1 }, { "eq_high_freq", 8000 }, { "eq_high_gain", -3 },
                             { "enh_air", 0 }, { "enh_body", 30 } } },
            { "RADIO",     { { "eq_hpf", 350 }, { "eq_lpf", 4500 }, { "eq_m1_freq", 1200 }, { "eq_m1_gain", 5 }, { "eq_m1_q", 0.8f },
                             { "enh_air", 0 }, { "enh_body", 0 } } },
            { "LO-FI",     { { "eq_hpf", 250 }, { "eq_lpf", 6000 }, { "eq_m2_freq", 1500 }, { "eq_m2_gain", 3 }, { "enh_air", 0 }, { "enh_body", 20 } } } },
        { // ---------------- SPACE
            { "DRY",       { } },
            { "ROOM",      { { "rev_type", 0 }, { "rev_mix", 18 }, { "rev_decay", 30 } } },
            { "PLATE",     { { "rev_type", 2 }, { "rev_mix", 28 }, { "rev_decay", 45 }, { "dly_type", 1 }, { "dly_rate", 5 }, { "dly_mix", 12 } } },
            { "MASSIVE",   { { "rev_type", 3 }, { "rev_mix", 38 }, { "rev_decay", 75 }, { "rev_predelay", 40 }, { "rev_duck", 30 },
                             { "dly_type", 1 }, { "dly_rate", 9 }, { "dly_mix", 20 }, { "dly_feedback", 45 }, { "dly_duck", 50 } } },
            { "CATHEDRAL", { { "rev_type", 4 }, { "rev_mix", 45 }, { "rev_decay", 70 }, { "rev_predelay", 60 } } },
            { "SLAPBACK",  { { "rev_type", 0 }, { "rev_mix", 10 }, { "dly_type", 4 }, { "dly_mix", 22 } } },
            { "PING PONG", { { "rev_type", 2 }, { "rev_mix", 15 }, { "dly_type", 1 }, { "dly_rate", 5 }, { "dly_mix", 28 },
                             { "dly_feedback", 45 }, { "dly_duck", 55 } } },
            { "TAPE ECHO", { { "rev_type", 0 }, { "rev_mix", 12 }, { "dly_type", 3 }, { "dly_rate", 8 }, { "dly_mix", 30 },
                             { "dly_feedback", 50 }, { "dly_tone", 30 } } },
            { "DREAM",     { { "rev_type", 5 }, { "rev_mix", 50 }, { "rev_decay", 80 }, { "rev_predelay", 80 }, { "dly_type", 3 },
                             { "dly_rate", 9 }, { "dly_mix", 25 }, { "dly_feedback", 55 }, { "dly_tone", 40 } } } },
        { // ---------------- FX
            { "CLEAN",     { { "sat_drive", 0 } } },
            { "TAPE SAT",  { { "sat_type", 0 }, { "sat_drive", 30 }, { "sat_mix", 80 } } },
            { "TUBE WARM", { { "sat_type", 1 }, { "sat_drive", 35 }, { "sat_mix", 70 } } },
            { "DOUBLER",   { { "sat_drive", 10 }, { "mul_amount", 40 }, { "mul_voices", 0 }, { "mul_spread", 100 }, { "mul_offset", 35 }, { "mul_detune", 20 } } },
            { "WIDE STACK",{ { "sat_type", 0 }, { "mul_amount", 60 }, { "mul_voices", 3 }, { "mul_spread", 100 }, { "mul_detune", 45 }, { "mul_offset", 55 } } },
            { "CRUNCH",    { { "sat_type", 3 }, { "sat_drive", 45 }, { "sat_mix", 60 } } },
            { "BROKEN",    { { "sat_type", 2 }, { "sat_drive", 55 }, { "sat_mix", 80 }, { "sat_clip", 1 } } },
            { "FOLD",      { { "sat_type", 5 }, { "sat_drive", 40 }, { "sat_mix", 45 }, { "mul_amount", 20 } } } },
        { // ---------------- MASTER
            { "TRANSPARENT", { } },
            { "GENTLE",    { { "mst_glue", 15 }, { "mst_loud", 3 }, { "mst_ceiling", -1 }, { "mst_width", 105 } } },
            { "STREAMING", { { "mst_glue", 25 }, { "mst_ceiling", -1 }, { "mst_mono", 100 }, { "mst_auto", 1 }, { "mst_target", -14 } } },
            { "WARM",      { { "mst_glue", 30 }, { "mst_loud", 6 }, { "mst_tilt", -1 }, { "mst_release", 120 } } },
            { "WIDE",      { { "mst_glue", 15 }, { "mst_loud", 4 }, { "mst_width", 140 }, { "mst_mono", 120 } } },
            { "LOUD",      { { "mst_glue", 35 }, { "mst_auto", 1 }, { "mst_target", -8 }, { "mst_ceiling", -0.3f }, { "mst_mono", 140 }, { "mst_release", 40 }, { "mst_tilt", 1 } } },
            { "CLUB",      { { "mst_glue", 40 }, { "mst_loud", 13 }, { "mst_ceiling", -0.2f }, { "mst_mono", 160 }, { "mst_release", 30 }, { "mst_width", 110 } } } } };
    return list[d];
}

inline float presetValue (const Preset& p, juce::RangedAudioParameter& prm)
{
    for (auto& [id, v] : p.values)
        if (prm.getParameterID() == id) return prm.convertTo0to1 (v);
    return prm.getDefaultValue();
}

inline void apply (juce::AudioProcessorValueTreeState& s, int d, int index)
{
    const auto& list = presets (d);
    if (index < 0 || index >= (int) list.size()) return;
    for (auto* id : params (d))
        if (auto* prm = s.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (presetValue (list[(size_t) index], *prm));
            prm->endChangeGesture();
        }
}

// index of the department preset the current settings match, or -1 (custom)
inline int match (juce::AudioProcessorValueTreeState& s, int d)
{
    const auto& list = presets (d);
    for (int i = 0; i < (int) list.size(); ++i)
    {
        bool same = true;
        for (auto* id : params (d))
            if (auto* prm = s.getParameter (id))
                if (std::abs (prm->getValue() - presetValue (list[(size_t) i], *prm)) > 0.002f) { same = false; break; }
        if (same) return i;
    }
    return -1;
}

inline bool isOn (juce::AudioProcessorValueTreeState& s, int d) { return s.getRawParameterValue (powers (d)[0])->load() > 0.5f; }
inline void setOn (juce::AudioProcessorValueTreeState& s, int d, bool on)
{
    for (auto* id : powers (d))
        if (auto* prm = s.getParameter (id))
        {
            prm->beginChangeGesture();
            prm->setValueNotifyingHost (on ? 1.0f : 0.0f);
            prm->endChangeGesture();
        }
}
} // namespace halo::dept
