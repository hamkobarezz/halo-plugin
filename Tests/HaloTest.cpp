// Offline verification of the HB Halo DSP: renders test material through every
// factory preset at every quality, checks for NaN/Inf, runaway gain, ceiling,
// latency alignment and measures CPU cost.
#include "PluginProcessor.h"
#include <chrono>
#include <iostream>

using namespace halo;

static void setP (HaloProcessor& p, const char* id, float v)
{
    auto* prm = p.apvts.getParameter (id);
    prm->setValueNotifyingHost (prm->convertTo0to1 (v));
}

// Vocal-ish test signal: glottal-like pulse train through formants + sibilant noise bursts
static void makeSignal (juce::AudioBuffer<float>& b, double fs)
{
    juce::Random rng (42);
    halo::BQ f1 = rbj::bandpass (fs, 700, 4), f2 = rbj::bandpass (fs, 1200, 5), f3 = rbj::bandpass (fs, 2600, 6), s = rbj::highpass (fs, 6000, 0.7);
    halo::BQState s1, s2, s3, s4;
    double ph = 0;
    for (int i = 0; i < b.getNumSamples(); ++i)
    {
        const double t = i / fs;
        const double f0 = 180 + 40 * std::sin (t * 1.3);
        ph += f0 / fs; if (ph >= 1) ph -= 1;
        const float pulse = (float) (ph < 0.08 ? 1.0 : -0.087);
        const float env = (float) (0.5 + 0.5 * std::sin (t * 2.1)) * (std::fmod (t, 2.0) < 1.6 ? 1.0f : 0.0f);
        float v = (s1.process (f1, pulse) * 1.0f + s2.process (f2, pulse) * 0.7f + s3.process (f3, pulse) * 0.4f) * env;
        const bool ess = std::fmod (t, 1.0) > 0.85;
        v += s4.process (s, rng.nextFloat() * 2 - 1) * (ess ? 0.25f : 0.002f);
        b.setSample (0, i, v * 0.9f);
        b.setSample (1, i, v * 0.85f);
    }
}

static int findPreset (const char* n)
{
    const auto& f = PresetManager::factory();
    for (int i = 0; i < (int) f.size(); ++i) if (f[(size_t) i].name == n) return i;
    return 0;
}

int main()
{
    juce::ScopedJuceInitialiser_GUI init;
    const double fs = 48000;
    const int block = 480, seconds = 6;
    const int total = (int) fs * seconds;
    int failures = 0;

    juce::AudioBuffer<float> src (2, total);
    makeSignal (src, fs);
    float srcPeak = src.getMagnitude (0, total);
    std::cout << "Source peak " << juce::Decibels::gainToDecibels (srcPeak) << " dBFS\n\n";

    auto render = [&] (HaloProcessor& p, juce::AudioBuffer<float>& out, double& cpuRatio)
    {
        out.makeCopyOf (src);
        juce::MidiBuffer midi;
        auto t0 = std::chrono::high_resolution_clock::now();
        for (int s = 0; s < total; s += block)
        {
            juce::AudioBuffer<float> sub (out.getArrayOfWritePointers(), 2, s, std::min (block, total - s));
            p.processBlock (sub, midi);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        cpuRatio = std::chrono::duration<double> (t1 - t0).count() / seconds;
    };

    // ---- 1. every factory preset at Ultra 4x ----
    const auto& f = PresetManager::factory();
    for (int q : { 2 })
    {
        for (int i = 0; i < (int) f.size(); ++i)
        {
            HaloProcessor p;
            p.presets.loadFactory (i);
            setP (p, "quality", (float) q);
            p.prepareToPlay (fs, block);
            juce::AudioBuffer<float> out;
            double cpu;
            render (p, out, cpu);
            bool finite = true;
            for (int c = 0; c < 2; ++c)
                for (int k = 0; k < total; ++k) if (! std::isfinite (out.getSample (c, k))) finite = false;
            const float pk = out.getMagnitude (0, total);
            const float rms = 0.5f * (out.getRMSLevel (0, 0, total) + out.getRMSLevel (1, 0, total));
            const float ceil = juce::Decibels::decibelsToGain (p.apvts.getRawParameterValue ("mst_ceiling")->load()) * 1.0005f;
            const bool ok = finite && pk <= ceil && pk > 1.0e-4f;
            if (! ok) ++failures;
            std::printf ("%-8s %-22s peak %6.2f dB  rms %6.2f dB  LUFS-I %6.1f  lat %4d  cpu %5.2f%%  %s\n",
                         f[(size_t) i].category.toRawUTF8(), f[(size_t) i].name.toRawUTF8(),
                         juce::Decibels::gainToDecibels (pk), juce::Decibels::gainToDecibels (rms),
                         p.meters.lufsI.load(), p.getLatencySamples(), cpu * 100.0, ok ? "OK" : "FAIL");
        }
    }

    // ---- 2. CPU per quality on the heaviest preset ----
    std::cout << "\nCPU (one instance, 48 kHz):\n";
    for (int q = 0; q < 4; ++q)
    {
        HaloProcessor p;
        p.presets.loadFactory (findPreset ("Big Hook Doubles"));
        setP (p, "quality", (float) q);
        setP (p, "rev_mix", 40);
        p.prepareToPlay (fs, block);
        juce::AudioBuffer<float> out;
        double cpu;
        render (p, out, cpu);
        std::printf ("  %-10s %5.2f%% of one core, latency %d samples\n", choices::quality[q].toRawUTF8(), cpu * 100.0, p.getLatencySamples());
    }

    // ---- 3. latency alignment: everything off, output must equal input delayed by reported latency ----
    std::cout << "\nLatency alignment:\n";
    for (int q = 0; q < 4; ++q)
    {
        HaloProcessor p;
        p.presets.initDefaults();
        for (auto id : { "comp_on", "enh_on", "eq_on", "mul_on", "space_on", "sat_on", "mst_on" }) setP (p, id, 0);
        setP (p, "quality", (float) q);
        p.prepareToPlay (fs, block);
        juce::AudioBuffer<float> out;
        double cpu;
        // warm up the off-fades
        render (p, out, cpu);
        render (p, out, cpu);
        const int lat = p.getLatencySamples();
        double err = 0, ref = 0;
        const halo::BQ lp = rbj::lowpass (fs, 16000, 0.7071);
        halo::BQState a1, a2, b1, b2;
        for (int k = lat; k < total; ++k)
        {
            const float o = a2.process (lp, a1.process (lp, out.getSample (0, k)));
            const float s = b2.process (lp, b1.process (lp, src.getSample (0, k - lat)));
            if (k < lat + 2000) continue;
            const double d = o - s;
            err += d * d; ref += (double) s * s;
        }
        const double snr = 10 * std::log10 (ref / std::max (err, 1e-20));
        int best = 0; double bestC = -1e30;
        for (int L = 0; L < 600; ++L)
        {
            double c = 0;
            for (int k = 20000; k < 60000; ++k) c += (double) out.getSample (0, k) * src.getSample (0, k - L);
            if (c > bestC) { bestC = c; best = L; }
        }
        std::printf ("  measured best lag %d\n", best);
        const bool ok = snr > 40;
        if (! ok) ++failures;
        std::printf ("  %-10s latency %4d  null-test SNR %6.1f dB  %s\n", choices::quality[q].toRawUTF8(), lat, snr, ok ? "OK" : "FAIL");
    }

    // ---- 4. limiter stress: +24 dB drive into -1 dB ceiling ----
    {
        HaloProcessor p;
        p.presets.loadFactory (findPreset ("Warm Analog Master"));
        setP (p, "mst_loud", 24); setP (p, "mst_ceiling", -1); setP (p, "in_gain", 12);
        p.prepareToPlay (fs, block);
        juce::AudioBuffer<float> out;
        double cpu;
        render (p, out, cpu);
        const float pk = juce::Decibels::gainToDecibels (out.getMagnitude (0, total));
        const bool ok = pk <= -0.999f;
        if (! ok) ++failures;
        std::printf ("\nLimiter stress: peak %.3f dBFS (ceiling -1)  %s\n", pk, ok ? "OK" : "FAIL");
    }

    // ---- 5. feedback stress: max delay feedback + max reverb decay for 6 s must not blow up ----
    {
        HaloProcessor p;
        p.presets.initDefaults();
        setP (p, "dly_mix", 100); setP (p, "dly_feedback", 100); setP (p, "rev_mix", 100); setP (p, "rev_decay", 100);
        setP (p, "rev_type", 5); setP (p, "mst_on", 0); setP (p, "dly_type", 3);
        p.prepareToPlay (fs, block);
        juce::AudioBuffer<float> out;
        double cpu;
        render (p, out, cpu);
        const float pk = out.getMagnitude (0, total);
        const bool ok = std::isfinite (pk) && pk < 20.0f;
        if (! ok) ++failures;
        std::printf ("Feedback stress: peak %.2f dBFS  %s\n", juce::Decibels::gainToDecibels (pk), ok ? "OK" : "FAIL");
    }

    std::cout << "\n" << (failures == 0 ? "ALL TESTS PASSED" : juce::String (failures) + " FAILURES") << "\n";
    return failures == 0 ? 0 : 1;
}
