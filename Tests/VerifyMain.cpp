#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstdio>
using namespace juce;

struct Rig
{
    ViseProcessor p; double sr = 48000.0; int blk = 512;
    Rig() { p.setPlayConfigDetails (2, 2, sr, blk); p.prepareToPlay (sr, blk); }
    void reset() { p.prepareToPlay (sr, blk); }
    void defaults() { set ("input",5); set ("attack",3); set ("release",5); set ("output",5); set ("ratio",3); set ("audio",0);
                      set ("hp",0); set ("dethp",0); set ("link",1); set ("brit",0); set ("in",1); set ("mix",100); reset(); }
    void set (const char* id, float v) { auto* q = p.apvts.getParameter (id); q->setValueNotifyingHost (q->convertTo0to1 (v)); }
    // run stereo (l,r); returns outputs
    void run (const std::vector<float>& l, const std::vector<float>& r, std::vector<float>& ol, std::vector<float>& orr)
    {
        const size_t N = l.size(); ol.resize (N); orr.resize (N);
        AudioBuffer<float> b (2, blk); MidiBuffer m;
        for (size_t s = 0; s < N; s += (size_t) blk)
        {
            int n = (int) std::min<size_t> ((size_t) blk, N - s);
            b.setSize (2, n, false, false, true);
            for (int i = 0; i < n; ++i) { b.setSample (0, i, l[s + (size_t) i]); b.setSample (1, i, r[s + (size_t) i]); }
            p.processBlock (b, m);
            for (int i = 0; i < n; ++i) { ol[s + (size_t) i] = b.getSample (0, i); orr[s + (size_t) i] = b.getSample (1, i); }
        }
    }
    std::vector<float> runMono (const std::vector<float>& x) { std::vector<float> a, b; run (x, x, a, b); return a; }
};

static std::vector<float> sine (double sr, float f, float dbfs, double sec)
{
    std::vector<float> v ((size_t) (sr * sec)); const float a = std::pow (10.0f, dbfs / 20.0f);
    for (size_t i = 0; i < v.size(); ++i) v[i] = a * std::sin (2.0 * MathConstants<double>::pi * f * (double) i / sr);
    return v;
}
static float rmsDb (const std::vector<float>& x, size_t a, size_t b)
{ double s = 0; for (size_t i = a; i < b; ++i) s += x[i] * x[i]; return (float) (10 * std::log10 (s / (double) (b - a) + 1e-20)); }
static float goertzelDb (const std::vector<float>& x, size_t a, size_t n, double sr, double f)
{
    double re = 0, im = 0; for (size_t i = 0; i < n; ++i) { double w = 2 * MathConstants<double>::pi * f * (double) i / sr;
        double win = 0.5 - 0.5 * std::cos (2 * MathConstants<double>::pi * (double) i / (double) n);
        re += x[a + i] * win * std::cos (w); im += x[a + i] * win * std::sin (w); }
    return (float) (20 * std::log10 (2 * std::sqrt (re * re + im * im) / (double) n / 0.5 + 1e-12));
}
// envelope: peak over 2 ms hops, in dB
static std::vector<float> envDb (const std::vector<float>& x, double sr)
{ std::vector<float> e; size_t h = (size_t) (sr * 0.002); for (size_t i = 0; i + h <= x.size(); i += h / 2)
  { float pk = 0; for (size_t j = 0; j < h; ++j) pk = std::max (pk, std::abs (x[i + j])); e.push_back (20 * std::log10 (pk + 1e-9f)); } return e; }

int main (int argc, char** argv)
{
    ScopedJuceInitialiser_GUI init;
    const String outDir = argc > 1 ? argv[1] : ".";
    Rig r;
    printf ("Reported latency: %d samples (%.2f ms @48k)\n\n", r.p.engine.getLatencySamples(), r.p.engine.getLatencySamples() / 48.0);

    // ---------- 1. static transfer ----------
    printf ("== 1. Static gain reduction (dB) vs. input level, 1 kHz, attack 3, release 5, clean ==\n");
    printf ("%-10s", "in dBFS"); for (int L = -36; L <= 0; L += 6) printf ("%7d", L); printf ("\n");
    for (int ri = 0; ri < 8; ++ri)
    {
        printf ("%-10s", vise::ratioNames[ri]);
        for (int L = -36; L <= 0; L += 6)
        {
            r.defaults(); r.set ("ratio", (float) ri); r.set ("input", 5); r.set ("attack", 3); r.set ("release", 5);
            auto in = sine (r.sr, 1000, (float) L, ri == 5 ? 4.0 : 2.0); auto o = r.runMono (in);
            size_t a = in.size() - 4800, b = in.size();
            printf ("%7.1f", rmsDb (in, a, b) - rmsDb (o, a, b));
        }
        printf ("\n");
    }

    // ---------- 2. timing ----------
    printf ("\n== 2. Time to 63%% of final GR (ms): burst -40 -> -6 dBFS, 4:1, then release ==\n");
    for (float k : { 0.0f, 3.0f, 6.0f, 10.0f })
    {
        r.defaults(); r.set ("ratio", 3); r.set ("attack", k); r.set ("release", 2);
        std::vector<float> in = sine (r.sr, 1000, -40, 0.3); auto b = sine (r.sr, 1000, -6, 1.0); in.insert (in.end(), b.begin(), b.end());
        auto o = r.runMono (in); auto ei = envDb (in, r.sr), eo = envDb (o, r.sr);
        const size_t t0 = (size_t) (0.3 / 0.001), tEnd = (size_t) (1.2 / 0.001);
        float fin = ei[tEnd] - eo[tEnd], base = ei[t0 - 5] - eo[t0 - 5]; size_t hit = 0;
        for (size_t i = t0; i < tEnd; ++i) if ((ei[i] - eo[i]) - base >= 0.63f * (fin - base)) { hit = i; break; }
        printf ("attack knob %4.1f : final GR %5.1f dB, 63%% in %6.2f ms\n", k, fin, (double) (hit - t0) * 1.0);
    }
    for (int opto = 0; opto < 2; ++opto)
        for (float k : { 0.0f, 5.0f, 10.0f })
        {
            r.defaults(); r.set ("ratio", opto ? 5 : 3); r.set ("attack", 3); r.set ("release", k);
            std::vector<float> in = sine (r.sr, 1000, -6, 1.5); auto q = sine (r.sr, 1000, -40, 25.0); in.insert (in.end(), q.begin(), q.end());
            auto o = r.runMono (in); auto ei = envDb (in, r.sr), eo = envDb (o, r.sr);
            const size_t t0 = (size_t) (1.5 / 0.001);
            float g0 = ei[t0 - 20] - eo[t0 - 20]; size_t hit = 0;
            for (size_t i = t0 + 3; i < ei.size(); ++i) if ((ei[i] - eo[i]) <= 0.37f * g0) { hit = i; break; }
            printf ("%-9s release knob %4.1f : GR at release %5.1f dB, falls to 37%% in %8.0f ms\n", opto ? "10:1 opto" : "4:1", k, g0, (double) (hit - t0));
        }

    // ---------- 3. harmonics ----------
    printf ("\n== 3. Harmonics re fundamental (dB), ratio 1:1, 1 kHz ==\n");
    const char* modes[] = { "Clean", "Dist 2", "Dist 3" };
    for (float inKnob : { 5.0f, 7.5f })
        for (int m = 0; m < 3; ++m)
        {
            r.defaults(); r.set ("ratio", 0); r.set ("audio", (float) m); r.set ("input", inKnob);
            auto in = sine (r.sr, 1000, -12, 2.0); auto o = r.runMono (in); size_t a = 48000;
            float f1 = goertzelDb (o, a, 48000, r.sr, 1000);
            printf ("input %3.1f  %-7s H2 %7.1f  H3 %7.1f  H4 %7.1f  H5 %7.1f  (THD %.3f%%)\n", inKnob, modes[m],
                goertzelDb (o, a, 48000, r.sr, 2000) - f1, goertzelDb (o, a, 48000, r.sr, 3000) - f1,
                goertzelDb (o, a, 48000, r.sr, 4000) - f1, goertzelDb (o, a, 48000, r.sr, 5000) - f1,
                100.0 * std::sqrt (std::pow (10.0, (goertzelDb (o, a, 48000, r.sr, 2000) - f1) / 10.0) + std::pow (10.0, (goertzelDb (o, a, 48000, r.sr, 3000) - f1) / 10.0)
                                   + std::pow (10.0, (goertzelDb (o, a, 48000, r.sr, 4000) - f1) / 10.0) + std::pow (10.0, (goertzelDb (o, a, 48000, r.sr, 5000) - f1) / 10.0)));
        }

    // ---------- 4. integrity ----------
    printf ("\n== 4. Integrity ==\n");
    { // bypass + mix=0 are latency-matched nulls
        for (int mode = 0; mode < 2; ++mode)
        {
            r.defaults(); if (mode == 0) r.set ("in", 0); else r.set ("mix", 0);
            Random rng (3); std::vector<float> in (48000); for (auto& s : in) s = rng.nextFloat() * 0.5f - 0.25f;
            auto o = r.runMono (in); int L = r.p.engine.getLatencySamples(); float worst = 0;
            for (size_t i = (size_t) L + 10; i < in.size(); ++i) worst = std::max (worst, std::abs (o[i] - in[i - (size_t) L]));
            printf ("%s: max |out - delayed in| = %.2e\n", mode == 0 ? "IN switch off  " : "Mix = 0       ", worst);
        }
        r.reset(); r.set ("in", 1); r.set ("mix", 100);
    }
    { // stereo link
        for (int link = 1; link >= 0; --link)
        {
            r.defaults(); r.set ("link", (float) link); r.set ("ratio", 3);
            auto L = sine (r.sr, 1000, -6, 1.5), R = sine (r.sr, 1000, -30, 1.5); std::vector<float> ol, orr; r.run (L, R, ol, orr);
            size_t a = 48000, b = 72000;
            printf ("Link %s: L GR %5.1f dB, R (quiet) GR %5.1f dB\n", link ? "on " : "off", rmsDb (L, a, b) - rmsDb (ol, a, b), rmsDb (R, a, b) - rmsDb (orr, a, b));
        }
    }
    { // robustness
        r.defaults(); r.set ("input", 10); r.set ("ratio", 7); r.set ("audio", 2); r.set ("brit", 1);
        Random rng (9); std::vector<float> in (96000); for (auto& s : in) s = (rng.nextFloat() * 2 - 1) * 4.0f; in[100] = 1.0e4f;
        auto o = r.runMono (in); bool bad = false; float mx = 0; for (float s : o) { if (! std::isfinite (s)) bad = true; mx = std::max (mx, std::abs (s)); }
        printf ("Torture (noise x4, +20 dB, Nuke, Dist3, Brit): %s, peak out %.2f\n", bad ? "NaN/Inf FOUND" : "all finite", mx);
        r.reset(); r.set ("input", 5);
    }

    // ---------- 5. UI snapshots ----------
    printf ("\n== 5. UI snapshots ==\n");
    for (int variant = 0; variant < 2; ++variant)
    {
        if (variant == 0) { r.set ("ratio", 3); r.set ("audio", 0); r.set ("input", 6); r.set ("attack", 3); r.set ("release", 5); r.set ("output", 5.5f); r.set ("hp", 0); r.set ("dethp", 0); r.set ("brit", 0); r.set ("mix", 100); }
        else           { r.set ("ratio", 5); r.set ("audio", 2); r.set ("input", 7.5f); r.set ("attack", 6); r.set ("release", 8); r.set ("output", 4); r.set ("hp", 1); r.set ("dethp", 1); r.set ("brit", 1); r.set ("mix", 65); }
        r.p.engine.meterGR.store (variant == 0 ? 7.0f : 13.5f); r.p.engine.meterThd.store (variant == 0 ? 0.4f : 3.6f);
        std::unique_ptr<AudioProcessorEditor> ed (r.p.createEditor());
        ed->setSize (1100, 290);
        static_cast<ViseEditor*> (ed.get())->updateMeters();
        auto img = ed->createComponentSnapshot (ed->getLocalBounds(), true, 1.5f);
        File f (outDir + "/vise8_ui_" + String (variant) + ".png"); f.deleteFile();
        FileOutputStream fo (f); PNGImageFormat().writeImageToStream (img, fo);
        printf ("wrote %s (%dx%d)\n", f.getFullPathName().toRawUTF8(), img.getWidth(), img.getHeight());
    }
    return 0;
}
