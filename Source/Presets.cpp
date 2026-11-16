#include "Presets.h"

namespace halo
{
PresetManager::PresetManager (juce::AudioProcessorValueTreeState& s) : apvts (s) {}

const std::vector<FactoryPreset>& PresetManager::factory()
{
    // Values are in real units (dB, Hz, %, ms, choice index, 0/1)
    static const std::vector<FactoryPreset> list = {
        // ======================= VOCALS =======================
        { "Vocals", "Easy Vocal Chain", {
            { "comp_amount", 40 }, { "comp_style", 1 }, { "enh_deess", 35 }, { "enh_air", 30 }, { "enh_body", 15 },
            { "eq_hpf", 90 }, { "eq_m1_freq", 320 }, { "eq_m1_gain", -2 }, { "eq_m1_q", 1.2f }, { "eq_m2_freq", 3200 }, { "eq_m2_gain", 1.5f },
            { "eq_high_gain", 1.5f }, { "mul_amount", 20 }, { "rev_type", 2 }, { "rev_mix", 28 }, { "rev_decay", 40 },
            { "dly_mix", 18 }, { "dly_type", 1 }, { "dly_rate", 5 }, { "dly_feedback", 30 }, { "sat_type", 1 }, { "sat_drive", 18 } } },
        { "Vocals", "Pop Lead Shine", {
            { "comp_amount", 50 }, { "comp_style", 0 }, { "comp_speed", 60 }, { "enh_deess", 45 }, { "enh_air", 45 }, { "enh_body", 15 },
            { "eq_hpf", 110 }, { "eq_m1_freq", 280 }, { "eq_m1_gain", -2.5f }, { "eq_m2_freq", 4500 }, { "eq_m2_gain", 2 }, { "eq_m2_q", 0.8f },
            { "eq_high_freq", 12000 }, { "eq_high_gain", 2.5f }, { "mul_amount", 15 }, { "rev_type", 3 }, { "rev_mix", 25 }, { "rev_decay", 45 },
            { "rev_predelay", 40 }, { "rev_duck", 40 }, { "dly_mix", 15 }, { "dly_type", 1 }, { "dly_rate", 6 }, { "dly_feedback", 25 },
            { "dly_duck", 50 }, { "sat_drive", 20 }, { "mst_glue", 15 } } },
        { "Vocals", "Rap Upfront", {
            { "comp_amount", 60 }, { "comp_style", 2 }, { "comp_speed", 70 }, { "comp_mix", 85 }, { "enh_deess", 40 }, { "enh_air", 25 },
            { "enh_body", 30 }, { "eq_hpf", 100 }, { "eq_m1_freq", 450 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 2800 }, { "eq_m2_gain", 2.5f },
            { "sat_type", 3 }, { "sat_drive", 25 }, { "sat_mix", 60 }, { "rev_type", 0 }, { "rev_mix", 12 }, { "rev_decay", 25 } } },
        { "Vocals", "Trap Vox", {
            { "comp_amount", 55 }, { "comp_style", 2 }, { "comp_speed", 65 }, { "enh_deess", 45 }, { "enh_air", 50 }, { "enh_body", 15 },
            { "eq_hpf", 120 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -3 }, { "eq_m2_freq", 5000 }, { "eq_m2_gain", 2 },
            { "eq_high_freq", 11000 }, { "eq_high_gain", 3 }, { "mul_amount", 25 }, { "mul_spread", 85 },
            { "rev_type", 2 }, { "rev_mix", 20 }, { "rev_decay", 35 }, { "dly_type", 1 }, { "dly_rate", 8 }, { "dly_mix", 20 },
            { "dly_feedback", 35 }, { "dly_duck", 60 }, { "sat_type", 0 }, { "sat_drive", 20 } } },
        { "Vocals", "R&B Silk", {
            { "comp_amount", 45 }, { "comp_style", 1 }, { "comp_speed", 35 }, { "enh_deess", 40 }, { "enh_air", 40 }, { "enh_body", 25 },
            { "eq_hpf", 80 }, { "eq_m1_freq", 250 }, { "eq_m1_gain", -1.5f }, { "eq_m2_freq", 3500 }, { "eq_m2_gain", 1 },
            { "eq_high_freq", 12000 }, { "eq_high_gain", 2 }, { "mul_amount", 30 }, { "mul_spread", 90 }, { "mul_offset", 30 }, { "mul_detune", 25 },
            { "rev_type", 3 }, { "rev_mix", 35 }, { "rev_decay", 55 }, { "rev_predelay", 30 }, { "rev_damp", 50 }, { "rev_duck", 30 },
            { "dly_type", 1 }, { "dly_rate", 9 }, { "dly_mix", 18 }, { "dly_feedback", 40 }, { "dly_duck", 50 }, { "sat_drive", 12 } } },
        { "Vocals", "Airy Indie", {
            { "comp_amount", 35 }, { "comp_style", 3 }, { "enh_deess", 30 }, { "enh_air", 55 }, { "enh_body", 10 },
            { "eq_m1_freq", 400 }, { "eq_m1_gain", -1 }, { "eq_high_freq", 14000 }, { "eq_high_gain", 3 },
            { "rev_type", 2 }, { "rev_mix", 30 }, { "rev_decay", 50 }, { "rev_predelay", 20 },
            { "dly_type", 3 }, { "dly_rate", 8 }, { "dly_mix", 15 }, { "dly_feedback", 30 }, { "dly_tone", 40 },
            { "sat_type", 0 }, { "sat_drive", 25 }, { "sat_mix", 70 } } },
        { "Vocals", "Telephone Lo-Fi", {
            { "comp_amount", 55 }, { "comp_style", 2 }, { "enh_deess", 10 }, { "enh_air", 0 }, { "enh_body", 0 },
            { "eq_hpf", 400 }, { "eq_lpf", 3800 }, { "eq_m1_freq", 1200 }, { "eq_m1_gain", 6 }, { "eq_m1_q", 0.8f },
            { "sat_type", 2 }, { "sat_drive", 45 }, { "sat_mix", 80 }, { "rev_type", 0 }, { "rev_mix", 10 }, { "dly_type", 4 }, { "dly_mix", 15 } } },
        { "Vocals", "Broken Radio", {
            { "comp_amount", 65 }, { "comp_style", 2 }, { "enh_air", 0 }, { "enh_body", 0 },
            { "eq_hpf", 300 }, { "eq_lpf", 5000 }, { "eq_m2_freq", 1800 }, { "eq_m2_gain", 5 }, { "eq_m2_q", 1.5f },
            { "sat_type", 2 }, { "sat_drive", 70 }, { "sat_clip", 1 }, { "dly_type", 3 }, { "dly_rate", 5 }, { "dly_mix", 22 },
            { "dly_feedback", 45 }, { "dly_tone", 30 }, { "rev_type", 0 }, { "rev_mix", 15 } } },
        { "Vocals", "Big Hook Doubles", {
            { "comp_amount", 50 }, { "comp_style", 1 }, { "enh_deess", 40 }, { "enh_air", 40 },
            { "mul_amount", 55 }, { "mul_voices", 3 }, { "mul_spread", 100 }, { "mul_offset", 50 }, { "mul_detune", 45 }, { "mul_low", 250 },
            { "rev_type", 3 }, { "rev_mix", 25 }, { "dly_type", 1 }, { "dly_rate", 5 }, { "dly_mix", 12 }, { "sat_drive", 18 }, { "mst_width", 120 } } },
        { "Vocals", "Ad-Lib Throw", {
            { "comp_amount", 60 }, { "comp_style", 2 }, { "eq_hpf", 250 }, { "eq_lpf", 9000 }, { "eq_m2_freq", 2500 }, { "eq_m2_gain", 3 },
            { "mul_amount", 35 }, { "mul_spread", 100 }, { "rev_type", 2 }, { "rev_mix", 25 }, { "rev_decay", 55 },
            { "dly_type", 1 }, { "dly_rate", 8 }, { "dly_mix", 40 }, { "dly_feedback", 55 }, { "dly_tone", 45 }, { "dly_duck", 60 },
            { "sat_type", 3 }, { "sat_drive", 30 } } },
        { "Vocals", "Cathedral Ballad", {
            { "comp_amount", 40 }, { "comp_style", 1 }, { "comp_speed", 30 }, { "enh_deess", 35 }, { "enh_air", 40 }, { "enh_body", 20 },
            { "rev_type", 4 }, { "rev_mix", 40 }, { "rev_decay", 60 }, { "rev_predelay", 60 }, { "rev_damp", 45 }, { "rev_duck", 25 },
            { "dly_type", 0 }, { "dly_rate", 8 }, { "dly_mix", 10 }, { "dly_feedback", 25 }, { "mul_amount", 10 }, { "sat_drive", 10 } } },
        { "Vocals", "Podcast Clean", {
            { "comp_amount", 55 }, { "comp_style", 1 }, { "comp_speed", 45 }, { "enh_denoise", 35 }, { "enh_deess", 40 }, { "enh_body", 25 },
            { "enh_air", 15 }, { "eq_hpf", 80 }, { "eq_m1_freq", 350 }, { "eq_m1_gain", -2.5f }, { "eq_m2_freq", 3000 }, { "eq_m2_gain", 1.5f },
            { "mst_glue", 10 }, { "mst_loud", 4 }, { "mst_ceiling", -1 }, { "sat_drive", 8 } } },
        { "Vocals", "Whisper Close", {
            { "comp_amount", 65 }, { "comp_style", 3 }, { "comp_speed", 40 }, { "enh_denoise", 25 }, { "enh_deess", 55 }, { "enh_air", 50 },
            { "enh_body", 35 }, { "eq_hpf", 70 }, { "rev_type", 0 }, { "rev_mix", 10 }, { "sat_type", 0 }, { "sat_drive", 15 } } },
        { "Vocals", "Backing Stack Glue", {
            { "comp_amount", 50 }, { "comp_style", 1 }, { "enh_deess", 50 }, { "enh_air", 30 }, { "eq_hpf", 180 }, { "eq_m1_freq", 300 },
            { "eq_m1_gain", -3 }, { "eq_high_gain", 1 }, { "mul_amount", 40 }, { "mul_voices", 2 }, { "mul_spread", 100 },
            { "rev_type", 3 }, { "rev_mix", 30 }, { "mst_width", 130 } } },
        { "Vocals", "Shimmer Dream", {
            { "comp_amount", 40 }, { "enh_air", 50 }, { "rev_type", 5 }, { "rev_mix", 45 }, { "rev_decay", 75 }, { "rev_predelay", 80 },
            { "rev_damp", 30 }, { "rev_duck", 35 }, { "dly_type", 1 }, { "dly_rate", 9 }, { "dly_mix", 25 }, { "dly_feedback", 55 },
            { "dly_tone", 70 }, { "dly_duck", 45 }, { "mul_amount", 30 } } },

        // ======================= PHONE =======================
        { "Phone", "Old Phone", {
            { "comp_amount", 60 }, { "comp_style", 2 }, { "enh_deess", 10 }, { "enh_air", 0 }, { "enh_body", 0 },
            { "eq_hpf", 450 }, { "eq_lpf", 3200 }, { "eq_m1_freq", 1500 }, { "eq_m1_gain", 6 }, { "eq_m1_q", 0.9f },
            { "sat_type", 2 }, { "sat_drive", 40 }, { "sat_mix", 75 } } },
        { "Phone", "Voice Note", {
            { "comp_amount", 55 }, { "comp_style", 0 }, { "comp_speed", 70 }, { "enh_air", 0 }, { "enh_body", 5 },
            { "eq_hpf", 180 }, { "eq_lpf", 7500 }, { "eq_m2_freq", 2500 }, { "eq_m2_gain", 4 },
            { "sat_type", 3 }, { "sat_drive", 20 }, { "sat_mix", 50 }, { "rev_type", 0 }, { "rev_mix", 8 }, { "rev_decay", 15 } } },
        { "Phone", "Payphone Grit", {
            { "comp_amount", 70 }, { "comp_style", 2 }, { "enh_air", 0 }, { "enh_body", 0 },
            { "eq_hpf", 600 }, { "eq_lpf", 2800 }, { "eq_m1_freq", 1200 }, { "eq_m1_gain", 8 }, { "eq_m1_q", 1.2f },
            { "sat_type", 4 }, { "sat_drive", 55 }, { "sat_clip", 1 }, { "dly_type", 4 }, { "dly_mix", 12 } } },
        { "Phone", "Walkie-Talkie", {
            { "comp_amount", 80 }, { "comp_style", 2 }, { "comp_speed", 90 }, { "enh_denoise", 30 }, { "enh_air", 0 }, { "enh_body", 0 },
            { "eq_hpf", 700 }, { "eq_lpf", 2600 }, { "eq_m1_freq", 1600 }, { "eq_m1_gain", 9 }, { "eq_m1_q", 1.5f },
            { "sat_type", 3 }, { "sat_drive", 70 }, { "sat_clip", 1 } } },
        { "Phone", "Video Call", {
            { "comp_amount", 50 }, { "comp_style", 1 }, { "enh_air", 5 }, { "eq_hpf", 250 }, { "eq_lpf", 6000 },
            { "eq_m2_freq", 3000 }, { "eq_m2_gain", 3 }, { "sat_type", 0 }, { "sat_drive", 15 },
            { "rev_type", 0 }, { "rev_mix", 12 }, { "rev_decay", 20 }, { "rev_width", 30 } } },

        // ======================= RADIO =======================
        { "Radio", "AM Radio", {
            { "comp_amount", 65 }, { "comp_style", 3 }, { "enh_air", 0 }, { "enh_body", 10 },
            { "eq_hpf", 300 }, { "eq_lpf", 4500 }, { "eq_m1_freq", 1000 }, { "eq_m1_gain", 5 }, { "eq_m1_q", 0.7f },
            { "sat_type", 2 }, { "sat_drive", 50 }, { "sat_mix", 70 }, { "rev_type", 0 }, { "rev_mix", 6 } } },
        { "Radio", "FM Broadcast", {
            { "comp_amount", 60 }, { "comp_style", 1 }, { "comp_speed", 55 }, { "enh_deess", 45 }, { "enh_air", 35 }, { "enh_body", 30 },
            { "eq_low_freq", 120 }, { "eq_low_gain", 2 }, { "eq_high_freq", 12000 }, { "eq_high_gain", 2 },
            { "sat_type", 0 }, { "sat_drive", 15 }, { "mst_glue", 30 }, { "mst_loud", 6 }, { "mst_ceiling", -1 } } },
        { "Radio", "Valve Radio", {
            { "comp_amount", 55 }, { "comp_style", 3 }, { "enh_air", 0 }, { "eq_hpf", 200 }, { "eq_lpf", 5500 },
            { "eq_m1_freq", 800 }, { "eq_m1_gain", 4 }, { "eq_m1_q", 0.7f }, { "sat_type", 1 }, { "sat_drive", 55 }, { "sat_mix", 80 },
            { "dly_type", 3 }, { "dly_rate", 5 }, { "dly_mix", 10 }, { "dly_feedback", 20 }, { "dly_tone", 25 } } },
        { "Radio", "Pirate Station", {
            { "comp_amount", 75 }, { "comp_style", 2 }, { "enh_air", 0 }, { "eq_hpf", 350 }, { "eq_lpf", 4000 },
            { "sat_type", 2 }, { "sat_drive", 75 }, { "sat_clip", 1 },
            { "dly_type", 3 }, { "dly_rate", 6 }, { "dly_mix", 20 }, { "dly_feedback", 40 }, { "dly_tone", 20 } } },
        { "Radio", "Late Night DJ", {
            { "comp_amount", 65 }, { "comp_style", 1 }, { "comp_speed", 35 }, { "enh_denoise", 20 }, { "enh_deess", 40 },
            { "enh_air", 30 }, { "enh_body", 55 }, { "eq_low_freq", 140 }, { "eq_low_gain", 3 }, { "eq_m1_freq", 350 }, { "eq_m1_gain", -2 },
            { "sat_drive", 20 }, { "rev_type", 1 }, { "rev_mix", 10 }, { "rev_decay", 20 }, { "mst_glue", 20 }, { "mst_loud", 4 } } },

        // ======================= SPACE =======================
        { "Space", "Infinite Space", {
            { "comp_amount", 40 }, { "enh_air", 40 }, { "mul_amount", 25 },
            { "rev_type", 5 }, { "rev_mix", 55 }, { "rev_decay", 95 }, { "rev_predelay", 120 }, { "rev_damp", 35 }, { "rev_duck", 40 },
            { "dly_type", 1 }, { "dly_rate", 10 }, { "dly_mix", 25 }, { "dly_feedback", 65 }, { "dly_tone", 55 }, { "dly_duck", 50 } } },
        { "Space", "Nebula Wash", {
            { "comp_amount", 40 }, { "mul_amount", 45 }, { "mul_voices", 3 }, { "mul_spread", 100 },
            { "rev_type", 5 }, { "rev_mix", 50 }, { "rev_decay", 80 }, { "rev_lowcut", 300 },
            { "dly_type", 3 }, { "dly_rate", 9 }, { "dly_mix", 30 }, { "dly_feedback", 60 }, { "dly_tone", 35 },
            { "sat_type", 0 }, { "sat_drive", 10 } } },
        { "Space", "Galaxy Throw", {
            { "comp_amount", 50 }, { "comp_style", 2 }, { "rev_type", 4 }, { "rev_mix", 35 }, { "rev_decay", 70 }, { "rev_duck", 50 },
            { "dly_type", 1 }, { "dly_rate", 8 }, { "dly_mix", 45 }, { "dly_feedback", 70 }, { "dly_tone", 60 }, { "dly_duck", 70 } } },
        { "Space", "Zero Gravity", {
            { "comp_amount", 45 }, { "enh_air", 50 }, { "eq_hpf", 150 }, { "eq_high_gain", 3 },
            { "mul_amount", 60 }, { "mul_voices", 3 }, { "mul_detune", 70 }, { "mul_offset", 60 },
            { "rev_type", 3 }, { "rev_mix", 40 }, { "rev_decay", 65 }, { "rev_predelay", 60 },
            { "dly_type", 2 }, { "dly_rate", 6 }, { "dly_mix", 20 }, { "dly_feedback", 45 } } },
        { "Space", "Deep Space Echo", {
            { "comp_amount", 45 }, { "eq_hpf", 200 }, { "eq_lpf", 9000 },
            { "dly_type", 3 }, { "dly_rate", 10 }, { "dly_mix", 40 }, { "dly_feedback", 80 }, { "dly_tone", 30 }, { "dly_duck", 40 },
            { "rev_type", 5 }, { "rev_mix", 35 }, { "rev_decay", 85 }, { "rev_damp", 60 } } },
        { "Space", "Starlight Plate", {
            { "comp_amount", 45 }, { "enh_air", 60 }, { "mul_amount", 20 },
            { "rev_type", 2 }, { "rev_mix", 40 }, { "rev_decay", 70 }, { "rev_predelay", 40 }, { "rev_damp", 20 },
            { "dly_type", 1 }, { "dly_rate", 5 }, { "dly_mix", 15 } } },

        // ======================= SINGERS =======================
        { "Singers", "Soul Diva", {
            { "comp_amount", 50 }, { "comp_style", 1 }, { "comp_speed", 40 }, { "enh_deess", 45 }, { "enh_air", 40 }, { "enh_body", 30 },
            { "eq_m1_freq", 300 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 3000 }, { "eq_m2_gain", 2 }, { "eq_high_gain", 2 },
            { "mul_amount", 15 }, { "rev_type", 3 }, { "rev_mix", 30 }, { "rev_decay", 55 }, { "rev_predelay", 35 }, { "rev_duck", 30 },
            { "dly_type", 1 }, { "dly_rate", 8 }, { "dly_mix", 12 }, { "sat_drive", 15 } } },
        { "Singers", "Gospel Choir", {
            { "comp_amount", 45 }, { "comp_style", 1 }, { "enh_air", 40 }, { "enh_deess", 40 },
            { "mul_amount", 70 }, { "mul_voices", 3 }, { "mul_spread", 100 }, { "mul_offset", 55 }, { "mul_detune", 40 }, { "mul_low", 250 },
            { "rev_type", 4 }, { "rev_mix", 40 }, { "rev_decay", 60 } } },
        { "Singers", "Pop Diva Stack", {
            { "comp_amount", 55 }, { "comp_style", 0 }, { "comp_speed", 60 }, { "enh_deess", 50 }, { "enh_air", 55 },
            { "mul_amount", 50 }, { "mul_voices", 2 }, { "mul_spread", 100 }, { "rev_type", 2 }, { "rev_mix", 25 },
            { "dly_type", 1 }, { "dly_rate", 6 }, { "dly_mix", 15 } } },
        { "Singers", "Rock Belter", {
            { "comp_amount", 65 }, { "comp_style", 2 }, { "comp_speed", 70 }, { "enh_deess", 35 },
            { "eq_hpf", 120 }, { "eq_m1_freq", 400 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 2200 }, { "eq_m2_gain", 3 },
            { "sat_type", 1 }, { "sat_drive", 35 }, { "sat_mix", 70 }, { "rev_type", 0 }, { "rev_mix", 15 }, { "dly_type", 4 }, { "dly_mix", 18 } } },
        { "Singers", "Opera Hall", {
            { "comp_amount", 30 }, { "comp_style", 3 }, { "comp_speed", 30 }, { "enh_deess", 25 }, { "enh_air", 35 }, { "enh_body", 20 },
            { "eq_hpf", 70 }, { "rev_type", 4 }, { "rev_mix", 45 }, { "rev_decay", 55 }, { "rev_predelay", 50 }, { "rev_damp", 40 } } },
        { "Singers", "Indie Falsetto", {
            { "comp_amount", 55 }, { "comp_style", 1 }, { "enh_deess", 40 }, { "enh_air", 60 }, { "enh_body", 10 },
            { "eq_m1_freq", 250 }, { "eq_m1_gain", -2 }, { "eq_high_freq", 13000 }, { "eq_high_gain", 3 },
            { "mul_amount", 25 }, { "mul_detune", 30 }, { "rev_type", 2 }, { "rev_mix", 35 }, { "rev_decay", 55 },
            { "dly_type", 3 }, { "dly_rate", 8 }, { "dly_mix", 15 }, { "dly_feedback", 35 } } },
        { "Singers", "Rap Double", {
            { "comp_amount", 60 }, { "comp_style", 2 }, { "enh_deess", 40 }, { "eq_hpf", 110 }, { "eq_m2_freq", 3000 }, { "eq_m2_gain", 2 },
            { "mul_amount", 45 }, { "mul_voices", 0 }, { "mul_spread", 100 }, { "mul_offset", 30 }, { "mul_detune", 15 },
            { "sat_type", 3 }, { "sat_drive", 20 }, { "rev_type", 0 }, { "rev_mix", 10 } } },
        { "Singers", "Choir Pad", {
            { "comp_amount", 40 }, { "enh_air", 45 }, { "eq_hpf", 200 }, { "eq_lpf", 12000 },
            { "mul_amount", 80 }, { "mul_voices", 3 }, { "mul_detune", 60 }, { "mul_offset", 70 }, { "mul_spread", 100 },
            { "rev_type", 5 }, { "rev_mix", 50 }, { "rev_decay", 70 } } },

        // ======================= ARTISTS (inspired-by vocal chains) =======================
        { "Artists", "Eminem Style", {
            { "comp_amount", 70 }, { "comp_style", 2 }, { "comp_speed", 80 }, { "comp_mix", 85 }, { "enh_deess", 45 }, { "enh_air", 35 }, { "enh_body", 25 },
            { "eq_hpf", 110 }, { "eq_m1_freq", 350 }, { "eq_m1_gain", -3 }, { "eq_m2_freq", 3500 }, { "eq_m2_gain", 3 },
            { "eq_high_freq", 11000 }, { "eq_high_gain", 2 }, { "sat_type", 3 }, { "sat_drive", 25 }, { "sat_mix", 50 },
            { "mul_amount", 20 }, { "mul_voices", 0 }, { "mul_spread", 60 }, { "mul_offset", 20 }, { "mul_detune", 10 },
            { "rev_type", 0 }, { "rev_mix", 8 }, { "dly_type", 4 }, { "dly_mix", 10 }, { "mst_glue", 15 } } },
        { "Artists", "Juice WRLD Style", {
            { "comp_amount", 55 }, { "comp_style", 1 }, { "enh_deess", 45 }, { "enh_air", 50 }, { "enh_body", 15 },
            { "eq_hpf", 120 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -2.5f }, { "eq_high_freq", 12000 }, { "eq_high_gain", 3 },
            { "mul_amount", 35 }, { "mul_spread", 90 }, { "mul_detune", 30 },
            { "rev_type", 2 }, { "rev_mix", 30 }, { "rev_decay", 55 }, { "rev_predelay", 30 }, { "rev_duck", 40 },
            { "dly_type", 1 }, { "dly_rate", 8 }, { "dly_mix", 22 }, { "dly_feedback", 40 }, { "dly_duck", 55 },
            { "sat_type", 0 }, { "sat_drive", 15 } } },
        { "Artists", "Future Style", {
            { "comp_amount", 60 }, { "comp_style", 2 }, { "enh_deess", 40 }, { "enh_air", 20 }, { "enh_body", 30 },
            { "eq_hpf", 100 }, { "eq_lpf", 14000 }, { "eq_m1_freq", 450 }, { "eq_m1_gain", -2 },
            { "sat_type", 1 }, { "sat_drive", 30 }, { "sat_mix", 60 }, { "mul_amount", 25 },
            { "rev_type", 3 }, { "rev_mix", 25 }, { "rev_decay", 50 }, { "rev_damp", 60 },
            { "dly_type", 3 }, { "dly_rate", 6 }, { "dly_mix", 25 }, { "dly_feedback", 50 }, { "dly_tone", 30 }, { "dly_duck", 55 } } },
        { "Artists", "Lil Wayne Style", {
            { "comp_amount", 65 }, { "comp_style", 2 }, { "comp_speed", 75 }, { "enh_deess", 35 }, { "enh_air", 40 },
            { "eq_hpf", 110 }, { "eq_m2_freq", 2000 }, { "eq_m2_gain", 3 }, { "eq_m2_q", 1.2f }, { "eq_high_gain", 2 },
            { "sat_type", 2 }, { "sat_drive", 18 }, { "sat_mix", 40 }, { "mul_amount", 15 },
            { "dly_type", 4 }, { "dly_mix", 15 }, { "rev_type", 0 }, { "rev_mix", 12 } } },
        { "Artists", "Drake Style", {
            { "comp_amount", 50 }, { "comp_style", 1 }, { "comp_speed", 35 }, { "enh_denoise", 10 }, { "enh_deess", 50 }, { "enh_air", 20 }, { "enh_body", 40 },
            { "eq_hpf", 90 }, { "eq_low_freq", 150 }, { "eq_low_gain", 1.5f }, { "eq_m1_freq", 400 }, { "eq_m1_gain", -1.5f },
            { "eq_m2_freq", 3000 }, { "eq_m2_gain", 1 }, { "eq_high_gain", -1 }, { "sat_type", 0 }, { "sat_drive", 20 }, { "sat_mix", 60 },
            { "mul_amount", 10 }, { "rev_type", 2 }, { "rev_mix", 18 }, { "rev_decay", 40 }, { "rev_damp", 55 }, { "rev_duck", 30 },
            { "dly_type", 1 }, { "dly_rate", 8 }, { "dly_mix", 10 }, { "dly_feedback", 25 }, { "dly_tone", 40 }, { "dly_duck", 60 } } },
        { "Artists", "Travis Scott Style", {
            { "comp_amount", 55 }, { "comp_style", 2 }, { "enh_deess", 40 }, { "enh_air", 30 }, { "eq_hpf", 150 }, { "eq_lpf", 12000 },
            { "sat_type", 1 }, { "sat_drive", 30 }, { "mul_amount", 40 }, { "mul_voices", 0 }, { "mul_spread", 100 }, { "mul_detune", 40 },
            { "rev_type", 3 }, { "rev_mix", 40 }, { "rev_decay", 70 }, { "rev_predelay", 40 }, { "rev_duck", 35 },
            { "dly_type", 1 }, { "dly_rate", 9 }, { "dly_mix", 30 }, { "dly_feedback", 55 }, { "dly_tone", 35 }, { "dly_duck", 50 } } },
        { "Artists", "Kendrick Style", {
            { "comp_amount", 60 }, { "comp_style", 0 }, { "comp_speed", 70 }, { "enh_deess", 40 }, { "enh_air", 30 }, { "enh_body", 20 },
            { "eq_hpf", 100 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 2800 }, { "eq_m2_gain", 2 },
            { "sat_type", 3 }, { "sat_drive", 15 }, { "rev_type", 0 }, { "rev_mix", 10 }, { "dly_type", 4 }, { "dly_mix", 8 } } },
        { "Artists", "The Weeknd Style", {
            { "comp_amount", 50 }, { "comp_style", 1 }, { "enh_deess", 45 }, { "enh_air", 55 },
            { "eq_hpf", 120 }, { "eq_high_freq", 12000 }, { "eq_high_gain", 3 }, { "mul_amount", 30 }, { "mul_voices", 0 }, { "mul_spread", 100 },
            { "rev_type", 2 }, { "rev_mix", 40 }, { "rev_decay", 65 }, { "rev_predelay", 40 }, { "rev_duck", 30 },
            { "dly_type", 1 }, { "dly_rate", 6 }, { "dly_mix", 22 }, { "dly_feedback", 45 }, { "dly_tone", 55 }, { "dly_duck", 45 },
            { "sat_type", 0 }, { "sat_drive", 10 } } },
        { "Artists", "Playboi Carti Style", {
            { "comp_amount", 75 }, { "comp_style", 2 }, { "comp_speed", 90 }, { "enh_air", 30 },
            { "eq_hpf", 200 }, { "eq_m2_freq", 3000 }, { "eq_m2_gain", 4 }, { "eq_high_gain", 3 },
            { "sat_type", 2 }, { "sat_drive", 45 }, { "sat_mix", 70 }, { "sat_clip", 1 }, { "mul_amount", 20 },
            { "rev_type", 0 }, { "rev_mix", 10 }, { "dly_type", 4 }, { "dly_mix", 10 } } },
        { "Artists", "Lil Baby Style", {
            { "comp_amount", 60 }, { "comp_style", 2 }, { "comp_speed", 70 }, { "enh_deess", 50 }, { "enh_air", 45 },
            { "eq_hpf", 120 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 4000 }, { "eq_m2_gain", 2 },
            { "eq_high_freq", 11000 }, { "eq_high_gain", 2 }, { "mul_amount", 15 }, { "rev_type", 2 }, { "rev_mix", 15 }, { "rev_decay", 35 },
            { "dly_type", 1 }, { "dly_rate", 5 }, { "dly_mix", 12 }, { "dly_duck", 60 }, { "sat_type", 0 }, { "sat_drive", 12 } } },
        { "Artists", "XXXTentacion Style", {
            { "comp_amount", 70 }, { "comp_style", 2 }, { "enh_air", 10 }, { "eq_hpf", 200 }, { "eq_lpf", 9000 },
            { "eq_m2_freq", 1500 }, { "eq_m2_gain", 3 }, { "sat_type", 2 }, { "sat_drive", 40 }, { "sat_mix", 60 },
            { "rev_type", 0 }, { "rev_mix", 15 }, { "dly_type", 3 }, { "dly_rate", 5 }, { "dly_mix", 15 } } },
        { "Artists", "Kid Cudi Style", {
            { "comp_amount", 45 }, { "comp_style", 1 }, { "enh_air", 30 }, { "eq_hpf", 120 }, { "eq_lpf", 12000 },
            { "mul_amount", 45 }, { "mul_voices", 3 }, { "mul_detune", 45 }, { "rev_type", 3 }, { "rev_mix", 40 }, { "rev_decay", 70 },
            { "dly_type", 3 }, { "dly_rate", 8 }, { "dly_mix", 20 }, { "dly_feedback", 45 }, { "dly_tone", 40 },
            { "sat_type", 0 }, { "sat_drive", 20 } } },
        { "Artists", "Billie Eilish Style", {
            { "comp_amount", 65 }, { "comp_style", 3 }, { "comp_speed", 40 }, { "enh_denoise", 25 }, { "enh_deess", 55 }, { "enh_air", 50 }, { "enh_body", 40 },
            { "eq_hpf", 70 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -1 }, { "sat_type", 0 }, { "sat_drive", 15 },
            { "rev_type", 0 }, { "rev_mix", 12 }, { "rev_decay", 25 } } },
        { "Artists", "Ariana Grande Style", {
            { "comp_amount", 55 }, { "comp_style", 0 }, { "enh_deess", 55 }, { "enh_air", 60 },
            { "eq_m1_freq", 350 }, { "eq_m1_gain", -2 }, { "eq_high_freq", 13000 }, { "eq_high_gain", 3 },
            { "mul_amount", 45 }, { "mul_voices", 3 }, { "mul_spread", 100 }, { "mul_detune", 30 },
            { "rev_type", 2 }, { "rev_mix", 30 }, { "rev_decay", 55 }, { "dly_type", 1 }, { "dly_rate", 6 }, { "dly_mix", 12 } } },

        // Lord Huron: vintage, reverb-soaked, tape slapback, warm & slightly lo-fi, soft doubles
        { "Artists", "Lord Huron Style", {
            { "comp_amount", 45 }, { "comp_style", 1 }, { "comp_speed", 35 }, { "enh_deess", 30 }, { "enh_air", 20 }, { "enh_body", 30 },
            { "eq_hpf", 110 }, { "eq_lpf", 11000 }, { "eq_m1_freq", 350 }, { "eq_m1_gain", -2 }, { "eq_m2_freq", 1800 }, { "eq_m2_gain", 2 }, { "eq_m2_q", 0.8f },
            { "sat_type", 0 }, { "sat_drive", 30 }, { "sat_mix", 70 }, { "mul_amount", 20 }, { "mul_detune", 20 },
            { "rev_type", 3 }, { "rev_mix", 40 }, { "rev_decay", 70 }, { "rev_predelay", 25 }, { "rev_damp", 55 }, { "rev_lowcut", 250 },
            { "dly_type", 3 }, { "dly_sync", 0 }, { "dly_time", 140 }, { "dly_mix", 22 }, { "dly_feedback", 20 }, { "dly_tone", 35 } } },
        { "Artists", "Lord Huron Night Echo", {
            { "comp_amount", 45 }, { "comp_style", 1 }, { "comp_speed", 30 }, { "enh_deess", 30 }, { "enh_air", 15 }, { "enh_body", 25 },
            { "eq_hpf", 120 }, { "eq_lpf", 10000 }, { "eq_m1_freq", 300 }, { "eq_m1_gain", -2 },
            { "sat_type", 1 }, { "sat_drive", 20 }, { "sat_mix", 70 },
            { "rev_type", 4 }, { "rev_mix", 45 }, { "rev_decay", 75 }, { "rev_predelay", 40 }, { "rev_damp", 60 }, { "rev_lowcut", 280 },
            { "dly_type", 3 }, { "dly_rate", 9 }, { "dly_mix", 25 }, { "dly_feedback", 45 }, { "dly_tone", 30 }, { "dly_duck", 35 } } },
        { "Artists", "Lord Huron Ghost Choir", {
            { "comp_amount", 40 }, { "comp_style", 3 }, { "enh_air", 20 }, { "enh_body", 20 }, { "eq_hpf", 150 }, { "eq_lpf", 9500 },
            { "mul_amount", 55 }, { "mul_voices", 3 }, { "mul_detune", 35 }, { "mul_offset", 60 }, { "mul_spread", 100 },
            { "rev_type", 5 }, { "rev_mix", 45 }, { "rev_decay", 70 }, { "rev_damp", 50 }, { "rev_lowcut", 300 },
            { "dly_type", 3 }, { "dly_sync", 0 }, { "dly_time", 120 }, { "dly_mix", 18 }, { "dly_feedback", 15 }, { "dly_tone", 30 },
            { "sat_type", 0 }, { "sat_drive", 20 } } },
        { "Artists", "Lord Huron Campfire", {
            { "comp_amount", 50 }, { "comp_style", 3 }, { "comp_speed", 40 }, { "enh_deess", 25 }, { "enh_air", 10 }, { "enh_body", 35 },
            { "eq_hpf", 90 }, { "eq_lpf", 12000 }, { "sat_type", 0 }, { "sat_drive", 25 }, { "sat_mix", 70 },
            { "rev_type", 1 }, { "rev_mix", 25 }, { "rev_decay", 45 }, { "rev_damp", 50 },
            { "dly_type", 3 }, { "dly_sync", 0 }, { "dly_time", 160 }, { "dly_mix", 15 }, { "dly_feedback", 15 }, { "dly_tone", 30 } } },

        // ======================= MIX =======================
        { "Mix", "Drum Bus Smash", {
            { "comp_amount", 70 }, { "comp_style", 2 }, { "comp_speed", 80 }, { "comp_mix", 50 }, { "comp_hpf", 60 },
            { "enh_deess", 0 }, { "enh_air", 20 }, { "enh_body", 20 }, { "eq_low_freq", 80 }, { "eq_low_gain", 2 },
            { "eq_m2_freq", 4000 }, { "eq_m2_gain", 1.5f }, { "sat_type", 0 }, { "sat_drive", 35 }, { "mst_glue", 30 } } },
        { "Mix", "Bass Thickener", {
            { "comp_amount", 45 }, { "comp_style", 1 }, { "comp_speed", 40 }, { "comp_hpf", 20 }, { "enh_deess", 0 }, { "enh_air", 0 },
            { "enh_body", 40 }, { "eq_hpf", 30 }, { "eq_low_freq", 90 }, { "eq_low_gain", 2 }, { "eq_m1_freq", 600 }, { "eq_m1_gain", -2 },
            { "sat_drive", 40 }, { "sat_mix", 60 }, { "mst_mono", 150 } } },
        { "Mix", "Guitar Width", {
            { "comp_amount", 30 }, { "comp_style", 0 }, { "enh_deess", 0 }, { "enh_air", 25 }, { "eq_hpf", 90 }, { "eq_m1_freq", 250 },
            { "eq_m1_gain", -2 }, { "mul_amount", 45 }, { "mul_voices", 0 }, { "mul_spread", 100 }, { "mul_offset", 60 }, { "mul_detune", 20 },
            { "mul_low", 300 }, { "rev_type", 0 }, { "rev_mix", 15 } } },
        { "Mix", "Synth Lead Wide", {
            { "comp_amount", 30 }, { "enh_deess", 0 }, { "enh_air", 30 }, { "mul_amount", 50 }, { "mul_voices", 3 }, { "mul_spread", 100 },
            { "dly_type", 1 }, { "dly_rate", 6 }, { "dly_mix", 25 }, { "dly_feedback", 40 }, { "rev_type", 2 }, { "rev_mix", 20 },
            { "sat_type", 5 }, { "sat_drive", 15 }, { "sat_mix", 40 } } },
        { "Mix", "808 Grit", {
            { "comp_amount", 40 }, { "comp_style", 2 }, { "comp_hpf", 20 }, { "enh_deess", 0 }, { "enh_air", 0 }, { "enh_body", 30 },
            { "eq_hpf", 25 }, { "eq_low_freq", 60 }, { "eq_low_gain", 2 }, { "sat_type", 4 }, { "sat_drive", 45 }, { "sat_mix", 70 },
            { "mst_mono", 120 } } },
        { "Mix", "Piano Polish", {
            { "comp_amount", 25 }, { "comp_style", 3 }, { "enh_deess", 0 }, { "enh_air", 30 }, { "enh_body", 10 },
            { "eq_m1_freq", 350 }, { "eq_m1_gain", -1.5f }, { "rev_type", 3 }, { "rev_mix", 22 }, { "rev_decay", 45 },
            { "sat_type", 0 }, { "sat_drive", 10 } } },
        { "Mix", "Vocal Bus Glue", {
            { "comp_amount", 25 }, { "comp_style", 3 }, { "comp_speed", 40 }, { "enh_deess", 30 }, { "enh_air", 25 },
            { "mst_glue", 20 }, { "mst_loud", 2 } } },

        // ======================= MASTER =======================
        { "Master", "Clean Master", {
            { "comp_on", 0 }, { "enh_deess", 0 }, { "enh_air", 10 }, { "enh_body", 0 }, { "eq_hpf", 25 }, { "sat_on", 0 },
            { "mst_glue", 20 }, { "mst_loud", 4 }, { "mst_ceiling", -1 }, { "mst_release", 80 } } },
        { "Master", "Streaming -14 LUFS", {
            { "comp_on", 0 }, { "enh_deess", 0 }, { "enh_air", 0 }, { "enh_body", 0 }, { "eq_hpf", 25 }, { "sat_on", 0 },
            { "mst_glue", 25 }, { "mst_loud", 6 }, { "mst_ceiling", -1 }, { "mst_mono", 100 } } },
        { "Master", "Loud Trap Master", {
            { "comp_on", 0 }, { "enh_deess", 0 }, { "enh_air", 15 }, { "enh_body", 0 }, { "eq_hpf", 22 }, { "eq_low_freq", 60 }, { "eq_low_gain", 1.5f },
            { "sat_type", 0 }, { "sat_drive", 15 }, { "mst_tilt", 1 }, { "mst_mono", 140 }, { "mst_glue", 35 }, { "mst_loud", 11 },
            { "mst_ceiling", -0.3f }, { "mst_release", 40 }, { "mst_width", 115 } } },
        { "Master", "Warm Analog Master", {
            { "comp_on", 0 }, { "enh_deess", 0 }, { "enh_air", 0 }, { "enh_body", 10 }, { "eq_high_freq", 12000 }, { "eq_high_gain", 1 },
            { "sat_type", 0 }, { "sat_drive", 25 }, { "sat_mix", 60 }, { "mst_tilt", -1 }, { "mst_glue", 30 }, { "mst_loud", 6 },
            { "mst_ceiling", -0.5f }, { "mst_release", 120 } } },
        { "Master", "Club Loud", {
            { "comp_on", 0 }, { "enh_deess", 0 }, { "enh_air", 10 }, { "enh_body", 0 }, { "eq_low_freq", 55 }, { "eq_low_gain", 2 },
            { "sat_type", 3 }, { "sat_drive", 15 }, { "sat_mix", 40 }, { "sat_clip", 1 }, { "mst_mono", 160 }, { "mst_glue", 40 },
            { "mst_loud", 13 }, { "mst_ceiling", -0.2f }, { "mst_release", 30 }, { "mst_width", 110 } } },
        { "Master", "Gentle Polish", {
            { "comp_on", 0 }, { "enh_deess", 0 }, { "enh_air", 15 }, { "enh_body", 0 }, { "sat_on", 0 }, { "mst_tilt", 0.5f },
            { "mst_glue", 15 }, { "mst_loud", 3 }, { "mst_width", 108 }, { "mst_ceiling", -1 } } },
    };
    return list;
}

juce::File PresetManager::userDir() const
{
    const auto docs = juce::File::getSpecialLocation (juce::File::userDocumentsDirectory);
    auto d = docs.getChildFile ("HALO Studios").getChildFile ("HALO").getChildFile ("Presets");
    if (! d.isDirectory())
    {
        d.createDirectory();
        // carry over presets saved before the rename
        auto old = docs.getChildFile ("HB Halo").getChildFile ("Presets");
        for (auto& f : old.findChildFiles (juce::File::findFiles, false, "*.halopreset"))
            f.copyFileTo (d.getChildFile (f.getFileName()));
    }
    return d;
}

juce::Array<juce::File> PresetManager::userPresets() const
{
    auto files = userDir().findChildFiles (juce::File::findFiles, false, "*.halopreset");
    files.sort();
    return files;
}

void PresetManager::setParam (const juce::String& id, float realValue)
{
    if (auto* p = apvts.getParameter (id))
    {
        p->beginChangeGesture();
        p->setValueNotifyingHost (p->convertTo0to1 (realValue));
        p->endChangeGesture();
    }
}

void PresetManager::initDefaults()
{
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            if (rp->getParameterID() != "bypass")
            {
                rp->beginChangeGesture();
                rp->setValueNotifyingHost (rp->getDefaultValue());
                rp->endChangeGesture();
            }
    currentName = "Init";
    currentIndex = -1;
}

void PresetManager::loadFactory (int index)
{
    const auto& f = factory();
    if (index < 0 || index >= (int) f.size()) return;
    initDefaults();
    for (auto& [id, v] : f[(size_t) index].values) setParam (id, v);
    currentName = f[(size_t) index].name;
    currentIndex = index;
}

bool PresetManager::loadUser (const juce::File& file)
{
    auto xml = juce::XmlDocument::parse (file);
    if (xml == nullptr || ! xml->hasTagName (apvts.state.getType())) return false;
    auto tree = juce::ValueTree::fromXml (*xml);
    for (auto* p : apvts.processor.getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
        {
            const auto id = rp->getParameterID();
            if (id == "bypass") continue;
            auto child = tree.getChildWithProperty ("id", id);
            if (child.isValid())
                setParam (id, (float) child.getProperty ("value"));
        }
    currentName = file.getFileNameWithoutExtension();
    auto users = userPresets();
    currentIndex = (int) factory().size() + users.indexOf (file);
    return true;
}

bool PresetManager::saveUser (const juce::String& name)
{
    auto clean = juce::File::createLegalFileName (name.trim());
    if (clean.isEmpty()) return false;
    auto file = userDir().getChildFile (clean + ".halopreset");
    auto state = apvts.copyState();
    if (auto xml = state.createXml())
        if (xml->writeTo (file))
        {
            currentName = clean;
            currentIndex = (int) factory().size() + userPresets().indexOf (file);
            return true;
        }
    return false;
}

void PresetManager::step (int delta)
{
    const int nf = (int) factory().size();
    auto users = userPresets();
    const int total = nf + users.size();
    if (total == 0) return;
    int idx = currentIndex < 0 ? (delta > 0 ? -1 : 0) : currentIndex;
    idx = ((idx + delta) % total + total) % total;
    if (idx < nf) loadFactory (idx);
    else loadUser (users[idx - nf]);
}
} // namespace halo
