#include "PluginProcessor.h"
#if ! HALO_TEST_BUILD
 #include "PluginEditor.h"
#endif

namespace halo
{
static inline void atomicMax (std::atomic<float>& a, float v) noexcept
{
    float cur = a.load (std::memory_order_relaxed);
    while (v > cur && ! a.compare_exchange_weak (cur, v, std::memory_order_relaxed)) {}
}

HaloProcessor::HaloProcessor()
    : AudioProcessor (BusesProperties()
                          .withInput ("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts (*this, &undo, "HALO", createLayout()),
      presets (apvts)
{
    for (auto* p : getParameters())
        if (auto* rp = dynamic_cast<juce::RangedAudioParameter*> (p))
            params[rp->getParameterID().toStdString()] = apvts.getRawParameterValue (rp->getParameterID());
    presets.loadFactory (0);
    undo.clearUndoHistory();
    startTimerHz (10);
}

HaloProcessor::~HaloProcessor() { stopTimer(); }

bool HaloProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo()) return false;
    return layouts.getMainInputChannelSet() == out;
}

int HaloProcessor::latencyFor (int q) const
{
    return (int) std::lround (OversamplerBank::latencyFor (const_cast<Saturator&> (sat).getBank(), q) + master.latencyFor (q));
}

void HaloProcessor::prepareToPlay (double sampleRate, int)
{
    currentFs = sampleRate;
    comp.prepare (sampleRate);
    enh.prepare (sampleRate);
    eq.prepare (sampleRate);
    mul.prepare (sampleRate);
    rev.prepare (sampleRate);
    dly.prepare (sampleRate);
    sat.prepare (sampleRate, Chunk);
    master.prepare (sampleRate, Chunk);
    loud.prepare (sampleRate);
    for (auto& d : dryLine) d.prepare (32768);
    work.setSize (2, Chunk);
    tmp.setSize (2, Chunk);
    dryBuf.setSize (2, Chunk);

    fComp.prepare (sampleRate, pb ("comp_on"));
    fEnh.prepare (sampleRate, pb ("enh_on"));
    fEq.prepare (sampleRate, pb ("eq_on"));
    fMul.prepare (sampleRate, pb ("mul_on"));
    fRev.prepare (sampleRate, pb ("space_on"));
    fDly.prepare (sampleRate, pb ("space_on"));
    for (auto* s : { &inG, &outG, &mixS, &bypassS }) s->reset (sampleRate, 0.03);
    inG.setCurrentAndTargetValue (dbToGain (pv ("in_gain")));
    outG.setCurrentAndTargetValue (dbToGain (pv ("out_gain")));
    mixS.setCurrentAndTargetValue (pv ("mix") * 0.01f);
    bypassS.setCurrentAndTargetValue (pb ("bypass") ? 1.0f : 0.0f);

    quality = -1;
    updateModules();
    setLatencySamples (latencyNow.load());
}

void HaloProcessor::timerCallback()
{
    const int l = latencyNow.load();
    if (getLatencySamples() != l) setLatencySamples (l);
}

void HaloProcessor::updateModules()
{
    const int q = (int) pv ("quality");
    if (q != quality)
    {
        quality = q;
        sat.setQuality (q);
        master.setQuality (q);
        latencyNow = (int) std::lround (sat.latency() + master.latency());
    }

    inG.setTargetValue (dbToGain (pv ("in_gain")));
    outG.setTargetValue (dbToGain (pv ("out_gain")));
    mixS.setTargetValue (pv ("mix") * 0.01f);
    bypassS.setTargetValue (pb ("bypass") ? 1.0f : 0.0f);

    comp.set (pv ("comp_input"), pv ("comp_amount") * 0.01f, (int) pv ("comp_style"), pv ("comp_speed") * 0.01f,
              pv ("comp_output"), pb ("comp_auto"), pv ("comp_mix") * 0.01f, pv ("comp_hpf"));

    // de-ess / de-noise belong to the DYNAMICS department, body / air to TONE
    const float dynOn = pb ("comp_on") ? 1.0f : 0.0f, toneOn = pb ("eq_on") ? 1.0f : 0.0f;
    enh.set (pv ("enh_deess") * 0.01f * dynOn, pv ("enh_deess_freq"), pv ("enh_denoise") * 0.01f * dynOn,
             pv ("enh_air") * 0.01f * toneOn, pv ("enh_body") * 0.01f * toneOn);

    Equalizer::Params e;
    e.hpf = pv ("eq_hpf"); e.lpf = pv ("eq_lpf");
    e.lowF = pv ("eq_low_freq"); e.lowG = pv ("eq_low_gain");
    e.m1F = pv ("eq_m1_freq"); e.m1G = pv ("eq_m1_gain"); e.m1Q = pv ("eq_m1_q");
    e.m2F = pv ("eq_m2_freq"); e.m2G = pv ("eq_m2_gain"); e.m2Q = pv ("eq_m2_q");
    e.highF = pv ("eq_high_freq"); e.highG = pv ("eq_high_gain");
    eq.set (e);

    mul.set (pv ("mul_amount") * 0.01f, pv ("mul_spread") * 0.01f, pv ("mul_offset") * 0.01f, pv ("mul_detune") * 0.01f,
             pv ("mul_low"), ((int) pv ("mul_voices") + 1) * 2);

    rev.set ((int) pv ("rev_type"), pv ("rev_mix") * 0.01f, pv ("rev_decay") * 0.01f, pv ("rev_predelay"),
             pv ("rev_damp") * 0.01f, pv ("rev_width") * 0.01f, pv ("rev_duck") * 0.01f, pv ("rev_lowcut"));

    const int rate = std::clamp ((int) pv ("dly_rate"), 0, 11);
    const float t = pb ("dly_sync") ? choices::dlyBeats[rate] * 60.0f / (float) bpm : pv ("dly_time") * 0.001f;
    dly.set ((int) pv ("dly_type"), pv ("dly_mix") * 0.01f, t, pv ("dly_feedback") * 0.01f, pv ("dly_tone") * 0.01f, pv ("dly_duck") * 0.01f);

    sat.set (pb ("sat_on"), (int) pv ("sat_type"), pv ("sat_drive") * 0.01f, pv ("sat_mix") * 0.01f, pv ("sat_gain"), pb ("sat_clip"));

    autoOn = pb ("mst_auto") && pb ("mst_on");
    autoTarget = pv ("mst_target");
    autoTol = pv ("mst_tol");
    master.set (pb ("mst_on"), pv ("mst_tilt"), pv ("mst_width") * 0.01f, pv ("mst_mono"), pv ("mst_glue") * 0.01f,
                pv ("mst_loud") + autoGainDb, pv ("mst_ceiling"), pv ("mst_release"));
}

// Auto loudness: rides the limiter drive so the short-term loudness settles on the typed
// target, but only corrects while it is outside the +/- tolerance window ("room for leakage"),
// so normal musical movement inside the window is left alone. Runs every 100 ms meter update.
void HaloProcessor::updateAutoLoudness (int n)
{
    const double fs = currentFs.load();
    const float dt = (float) (n / fs);
    if (! autoOn) { autoGainDb += (0.0f - autoGainDb) * std::min (1.0f, dt * 2.0f); }
    else if (loud.shortTerm > -50.0f)
    {
        const float err = autoTarget - loud.shortTerm;
        if (std::abs (err) > autoTol)
        {
            const float excess = err > 0 ? err - autoTol * 0.5f : err + autoTol * 0.5f;
            autoGainDb += excess * std::min (1.0f, dt * 0.8f);
        }
        autoGainDb = std::clamp (autoGainDb, -12.0f, 24.0f);
    }
    meters.autoGain = autoGainDb;
}

template <typename Fn>
void HaloProcessor::runFaded (Fade& f, bool on, float* l, float* r, int n, Fn&& fn)
{
    f.g.setTargetValue (on ? 1.0f : 0.0f);
    if (! f.active()) return;
    if (! f.g.isSmoothing() && f.g.getCurrentValue() >= 1.0f) { fn (l, r, n); return; }
    auto* tl = tmp.getWritePointer (0);
    auto* tr = tmp.getWritePointer (1);
    std::copy (l, l + n, tl);
    std::copy (r, r + n, tr);
    fn (l, r, n);
    for (int i = 0; i < n; ++i)
    {
        const float g = f.g.getNextValue();
        l[i] = tl[i] + (l[i] - tl[i]) * g;
        r[i] = tr[i] + (r[i] - tr[i]) * g;
    }
}

void HaloProcessor::processChunk (float* l, float* r, int n)
{
    auto* dl = dryBuf.getWritePointer (0);
    auto* dr = dryBuf.getWritePointer (1);
    const int lat = latencyNow.load (std::memory_order_relaxed);
    for (int i = 0; i < n; ++i)
    {
        dryLine[0].push (l[i]); dryLine[1].push (r[i]);
        dl[i] = dryLine[0].tap (lat); dr[i] = dryLine[1].tap (lat);
    }

    float pkL = 0, pkR = 0;
    for (int i = 0; i < n; ++i)
    {
        const float g = inG.getNextValue();
        l[i] *= g; r[i] *= g;
        pkL = std::max (pkL, std::abs (l[i])); pkR = std::max (pkR, std::abs (r[i]));
    }
    atomicMax (meters.inL, pkL); atomicMax (meters.inR, pkR);
    preFifo.push (l, r, n);

    const bool enhPre = pb ("enh_pre"), eqPre = pb ("eq_pre"), satPre = pb ("sat_prefx");
    auto runEnh = [&] { runFaded (fEnh, pb ("enh_on"), l, r, n, [&] (float* a, float* b, int k) { enh.process (a, b, k); }); };
    auto runEq  = [&] { runFaded (fEq, pb ("eq_on"), l, r, n, [&] (float* a, float* b, int k) { eq.process (a, b, k); }); };

    if (enhPre) runEnh();
    if (eqPre) runEq();
    runFaded (fComp, pb ("comp_on"), l, r, n, [&] (float* a, float* b, int k) { comp.process (a, b, k); });
    if (! enhPre) runEnh();
    if (! eqPre) runEq();
    if (satPre) sat.process (l, r, n);
    runFaded (fMul, pb ("mul_on"), l, r, n, [&] (float* a, float* b, int k) { mul.process (a, b, k); });
    runFaded (fDly, pb ("space_on"), l, r, n, [&] (float* a, float* b, int k) { dly.process (a, b, k); });
    runFaded (fRev, pb ("space_on"), l, r, n, [&] (float* a, float* b, int k) { rev.process (a, b, k); });
    if (! satPre) sat.process (l, r, n);
    master.process (l, r, n);

    float oL = 0, oR = 0;
    for (int i = 0; i < n; ++i)
    {
        const float go = outG.getNextValue(), m = mixS.getNextValue(), byp = bypassS.getNextValue();
        float a = l[i] * go, b = r[i] * go;
        a = dl[i] + (a - dl[i]) * m;
        b = dr[i] + (b - dr[i]) * m;
        a += (dl[i] - a) * byp;
        b += (dr[i] - b) * byp;
        if (! std::isfinite (a)) a = 0;
        if (! std::isfinite (b)) b = 0;
        l[i] = a; r[i] = b;
        oL = std::max (oL, std::abs (a)); oR = std::max (oR, std::abs (b));
    }
    atomicMax (meters.outL, oL); atomicMax (meters.outR, oR);
    postFifo.push (l, r, n);

    if (meters.resetLoudness.exchange (false)) loud.reset();
    loud.process (l, r, n);
    updateAutoLoudness (n);
    meters.lufsM = loud.momentary; meters.lufsS = loud.shortTerm; meters.lufsI = loud.integrated;
    atomicMax (meters.peak, std::max (oL, oR));

    atomicMax (meters.compGr, fComp.active() ? comp.grDb : 0.0f);
    atomicMax (meters.essGr, fEnh.active() ? enh.essGr : 0.0f);
    atomicMax (meters.noiseGr, fEnh.active() ? enh.noiseGr : 0.0f);
    atomicMax (meters.mulAct, fMul.active() ? mul.activity : 0.0f);
    atomicMax (meters.revAct, fRev.active() ? rev.activity : 0.0f);
    atomicMax (meters.dlyAct, fDly.active() ? dly.activity : 0.0f);
    atomicMax (meters.satAct, sat.activity);
    atomicMax (meters.glueGr, master.glueGr);
    atomicMax (meters.limGr, master.limGr);
    atomicMax (meters.eqAct, std::max (oL, oR));
}

void HaloProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int numIn = getTotalNumInputChannels(), numOut = getTotalNumOutputChannels();
    for (int c = numIn; c < numOut; ++c) buffer.clear (c, 0, buffer.getNumSamples());

    if (auto* ph = getPlayHead())
        if (auto pos = ph->getPosition())
            if (auto b = pos->getBpm())
                if (*b > 20.0 && *b < 999.0) bpm = *b;

    updateModules();

#if HALO_DEMO
    {
        static double ph = 0, t = 0; static uint32_t seed = 1; static BQState s1, s2, s3;
        const double fs = currentFs.load();
        const BQ f1 = rbj::bandpass (fs, 700, 4), f2 = rbj::bandpass (fs, 1250, 5), f3 = rbj::highpass (fs, 6500, 0.7);
        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            t += 1.0 / fs;
            ph += (170.0 + 50.0 * std::sin (t * 1.1)) / fs; if (ph >= 1) ph -= 1;
            const float pulse = ph < 0.08 ? 1.0f : -0.087f;
            const float env = (float) (0.55 + 0.45 * std::sin (t * 2.3)) * (std::fmod (t, 2.2) < 1.7 ? 1.0f : 0.05f);
            seed = seed * 1664525u + 1013904223u;
            const float nz = (float) (seed >> 8) / 8388608.0f - 1.0f;
            float v = (s1.process (f1, pulse) + 0.7f * s2.process (f2, pulse)) * env * 0.8f;
            v += s3.process (f3, nz) * (std::fmod (t, 1.1) > 0.95 ? 0.3f : 0.003f);
            for (int c = 0; c < buffer.getNumChannels(); ++c) buffer.setSample (c, i, v);
        }
    }
#endif

    const int numCh = std::min (buffer.getNumChannels(), 2);
    if (numCh == 0) return;
    const int total = buffer.getNumSamples();
    auto* wl = work.getWritePointer (0);
    auto* wr = work.getWritePointer (1);

    for (int start = 0; start < total; start += Chunk)
    {
        const int n = std::min (Chunk, total - start);
        float* L = buffer.getWritePointer (0) + start;
        float* R = numCh > 1 ? buffer.getWritePointer (1) + start : nullptr;
        std::copy (L, L + n, wl);
        if (R != nullptr) std::copy (R, R + n, wr); else std::copy (L, L + n, wr);
        processChunk (wl, wr, n);
        if (R != nullptr) { std::copy (wl, wl + n, L); std::copy (wr, wr + n, R); }
        else for (int i = 0; i < n; ++i) L[i] = 0.5f * (wl[i] + wr[i]);
    }
#if HALO_DEMO
    buffer.clear();
#endif
}

void HaloProcessor::processBlockBypassed (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    // keep the dry path latency-aligned while the host bypasses us
    const int lat = latencyNow.load();
    const int numCh = std::min (buffer.getNumChannels(), 2);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
        for (int c = 0; c < 2; ++c)
        {
            const float x = buffer.getSample (std::min (c, numCh - 1), i);
            dryLine[c].push (x);
            if (c < numCh) buffer.setSample (c, i, dryLine[c].tap (lat));
        }
}

juce::AudioProcessorEditor* HaloProcessor::createEditor()
{
#if HALO_TEST_BUILD
    return nullptr;
#else
    return new HaloEditor (*this);
#endif
}

void HaloProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    state.setProperty ("presetName", presets.currentName, nullptr);
    state.setProperty ("presetIndex", presets.currentIndex, nullptr);
    if (auto xml = state.createXml()) copyXmlToBinary (*xml, destData);
}

void HaloProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary (data, sizeInBytes))
        if (xml->hasTagName (apvts.state.getType()))
        {
            auto tree = juce::ValueTree::fromXml (*xml);
            apvts.replaceState (tree);
            presets.currentName = tree.getProperty ("presetName", "Init").toString();
            presets.currentIndex = (int) tree.getProperty ("presetIndex", -1);
            undo.clearUndoHistory();
        }
}

void HaloProcessor::selectSlot (int s)
{
    if (s == slot || s < 0 || s > 1) return;
    slots[slot] = apvts.copyState();
    if (slots[s].isValid()) apvts.replaceState (slots[s].createCopy());
    slot = s;
}

void HaloProcessor::copyToOtherSlot() { slots[1 - slot] = apvts.copyState(); }

} // namespace halo

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new halo::HaloProcessor(); }
