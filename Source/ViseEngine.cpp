#include "ViseEngine.h"

namespace vise
{
static inline float dbToGain (float db) noexcept { return std::pow (10.0f, db * 0.05f); }

float ViseEngine::gainComputer (float level, int ri, bool brit) noexcept
{
    // ratio, knee width (dB). Lower ratios get wider knees (inference from "knee" character).
    static const float R[8] = { 1.0f, 2.0f, 3.0f, 4.0f, 6.0f, 10.0f, 20.0f, 1000.0f };
    static const float W[8] = { 0.0f, 12.0f, 9.0f, 6.0f, 4.0f, 8.0f, 2.0f, 0.5f };

    const float r = R[ri], w = W[ri], over = level - kThresholdDb;
    if (r <= 1.0f) return 0.0f;
    const float slope = 1.0f - 1.0f / r;

    float gr;
    if (2.0f * over < -w)               gr = 0.0f;
    else if (2.0f * std::abs (over) <= w) gr = slope * (over + w * 0.5f) * (over + w * 0.5f) / (2.0f * w);
    else                                gr = slope * over;

    // "Brit" curve: 1176-style, effective ratio relaxes on deep hits (inference)
    if (brit && over > 10.0f)
        gr = slope * (10.0f + (over - 10.0f) * 0.6f);
    return gr;
}

void ViseEngine::prepare (double sampleRate, int maxBlock, int numChannels)
{
    sr = sampleRate;
    maxBlockSize = maxBlock;
    nch = juce::jlimit (1, 2, numChannels);

    oversampling = std::make_unique<juce::dsp::Oversampling<float>> (
        (size_t) nch, (size_t) 2, juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true, true);
    oversampling->initProcessing ((size_t) maxBlock);
    latency = (int) std::ceil (oversampling->getLatencyInSamples());

    juce::dsp::ProcessSpec spec { sr, (juce::uint32) maxBlock, (juce::uint32) nch };
    dryDelay.prepare (spec);
    dryDelay.setDelay ((float) latency);

    dryBuf.setSize (2, maxBlock);
    for (auto& g : grBuf) g.assign ((size_t) maxBlock, 0.0f);

    auto hp2 = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 80.0f, 1.0f);
    auto hp1 = juce::dsp::IIR::Coefficients<float>::makeFirstOrderHighPass (sr, 80.0f);
    auto dh  = juce::dsp::IIR::Coefficients<float>::makeHighPass (sr, 110.0f, 0.707f);
    for (int c = 0; c < 2; ++c) { audioHp1[c].coefficients = hp1; audioHp2[c].coefficients = hp2; detHpF[c].coefficients = dh; }

    inSm.reset (sr, 0.02);  outSm.reset (sr, 0.02);
    inSm.setCurrentAndTargetValue (1.0f);  outSm.setCurrentAndTargetValue (1.0f);
    reset();
}

void ViseEngine::reset()
{
    if (oversampling) oversampling->reset();
    dryDelay.reset();
    for (int c = 0; c < 2; ++c)
    {
        pk[c] = rms[c] = grEnv[c] = grFast[c] = grSlow[c] = dcX[c] = dcY[c] = 0.0f;
        audioHp1[c].reset(); audioHp2[c].reset(); detHpF[c].reset();
    }
    thdSmoothed = grSmoothed = 0.0f;
}

void ViseEngine::process (juce::AudioBuffer<float>& buf, const Params& p)
{
    const int n  = buf.getNumSamples();
    const int ch = juce::jmin (buf.getNumChannels(), nch);
    if (n == 0 || ch == 0) return;

    // ---- delayed dry copy (also used for latency-matched bypass) ----
    for (int i = 0; i < n; ++i)
        for (int c = 0; c < ch; ++c)
        {
            dryDelay.pushSample (c, buf.getSample (c, i));
            dryBuf.setSample (c, i, dryDelay.popSample (c));
        }

    if (! p.engaged)
    {
        for (int c = 0; c < ch; ++c) buf.copyFrom (c, 0, dryBuf, c, 0, n);
        meterGR.store (0.0f); meterThd.store (0.0f);
        return;
    }

    const bool opto = (p.ratio == kOptoIndex);
    static const float attMul[8] = { 1.0f, 0.5f, 0.5f, 0.5f, 1.0f, 2.5f, 0.7f, 0.7f };
    static const float relMul[8] = { 1.0f, 1.4f, 1.2f, 1.0f, 0.9f, 1.0f, 0.8f, 0.5f };

    float aSec = knobToAttackSec (p.attack) * attMul[p.ratio] * (p.brit ? 0.6f : 1.0f);
    if (opto) aSec = juce::jmax (aSec, 0.002f);
    float rSec = (opto ? knobToOptoReleaseSec (p.release) : knobToReleaseSec (p.release)) * relMul[p.ratio];

    auto coefFor = [this] (float sec) { return std::exp (-1.0f / (juce::jmax (sec, 1.0e-5f) * (float) sr)); };
    const float aCoef = coefFor (aSec), rCoef = coefFor (rSec);
    const float rFastCoef = coefFor (juce::jlimit (0.08f, 0.6f, 0.08f * std::pow (rSec / 0.2f, 0.35f)));
    const float slowACoef = coefFor (0.08f), pkCoef = coefFor (0.005f), rmsCoef = coefFor (0.02f);

    inSm.setTargetValue (dbToGain (p.inputDb));
    outSm.setTargetValue (dbToGain (p.outputDb));
    const float link = juce::jlimit (0.0f, 1.0f, p.link);

    float grBlockSum = 0.0f;

    // ---- stage 1 : input gain, audio HP, detector, gain computer, VCA ----
    for (int i = 0; i < n; ++i)
    {
        const float ig = inSm.getNextValue();
        float x[2] {}, lvl[2] {};

        for (int c = 0; c < ch; ++c)
        {
            x[c] = buf.getSample (c, i) * ig;
            if (p.hp) x[c] = audioHp1[c].processSample (audioHp2[c].processSample (x[c]));

            const float s = p.detHp ? detHpF[c].processSample (x[c]) : x[c];
            if (opto)
            {
                rms[c] = rmsCoef * rms[c] + (1.0f - rmsCoef) * s * s;
                lvl[c] = 10.0f * std::log10 (rms[c] + 1.0e-10f) + 3.0f;   // +3 dB: sine rms -> peak scale
            }
            else
            {
                pk[c] = juce::jmax (std::abs (s), pk[c] * pkCoef);
                lvl[c] = 20.0f * std::log10 (pk[c] + 1.0e-6f);
            }
        }
        const float lvlMax = ch > 1 ? juce::jmax (lvl[0], lvl[1]) : lvl[0];

        for (int c = 0; c < ch; ++c)
        {
            const float use = link * lvlMax + (1.0f - link) * lvl[c];
            const float target = gainComputer (use, p.ratio, p.brit);
            float gr;
            if (! opto)
            {
                const float cf = target > grEnv[c] ? aCoef : rCoef;
                grEnv[c] = cf * grEnv[c] + (1.0f - cf) * target;
                gr = grEnv[c];
            }
            else
            {
                // two-stage, programme-dependent release: slow stage lingers after sustained compression
                const float cf1 = target > grFast[c] ? aCoef : rFastCoef;
                grFast[c] = cf1 * grFast[c] + (1.0f - cf1) * target;
                const float cf2 = target > grSlow[c] ? slowACoef : rCoef;
                grSlow[c] = cf2 * grSlow[c] + (1.0f - cf2) * target;
                gr = grFast[c] + 0.6f * juce::jmax (0.0f, grSlow[c] - grFast[c]);
            }
            grBuf[c][(size_t) i] = gr;
            buf.setSample (c, i, x[c] * dbToGain (-gr));
            if (c == 0) grBlockSum += gr;
        }
    }

    // ---- stage 2 : harmonic distortion in the VCA/output stage, 4x oversampled ----
    float peakA = 0.0f, c2Sum = 0.0f, c3Sum = 0.0f;
    {
        auto blk = juce::dsp::AudioBlock<float> (buf).getSubBlock (0, (size_t) n).getSubsetChannelBlock (0, (size_t) ch);
        auto up = oversampling->processSamplesUp (juce::dsp::AudioBlock<const float> (blk));
        const size_t m = up.getNumSamples(), factor = m / (size_t) n;
        const float britBoost = p.brit ? 1.3f : 1.0f;

        for (int c = 0; c < ch; ++c)
        {
            float* d = up.getChannelPointer ((size_t) c);
            for (size_t j = 0; j < m; ++j)
            {
                const float gr = grBuf[c][j / factor];
                float c2, c3;
                switch (p.audio)
                {
                    case 1:  c2 = 0.02f + 0.008f * gr; c3 = 0.010f + 0.001f * gr; break;   // Dist 2
                    case 2:  c2 = 0.001f;              c3 = 0.12f + 0.02f * gr;  break;   // Dist 3
                    default: c2 = 0.0008f;             c3 = 0.002f;               break;   // Clean
                }
                c2 *= britBoost; c3 *= britBoost;

                const float x = d[j];
                float y = x + c2 * x * x - c3 * x * x * x;
                { const float u = y * (1.0f / 1.6f), u2 = u * u;   // flat-below-knee ceiling (no low-level THD)
                  y = y / std::sqrt (std::sqrt (1.0f + u2 * u2)); }
                d[j] = y;

                if (c == 0 && (j % factor) == 0)
                { peakA = juce::jmax (peakA, std::abs (x)); c2Sum += c2; c3Sum += c3; }
            }
        }
        oversampling->processSamplesDown (blk);
    }

    // ---- stage 3 : DC block, output gain, parallel mix ----
    const float wet = juce::jlimit (0.0f, 1.0f, p.mix), dry = 1.0f - wet;
    for (int i = 0; i < n; ++i)
    {
        const float og = outSm.getNextValue();
        for (int c = 0; c < ch; ++c)
        {
            const float in = buf.getSample (c, i);
            const float y = in - dcX[c] + 0.9995f * dcY[c];
            dcX[c] = in; dcY[c] = y;
            buf.setSample (c, i, wet * y * og + dry * dryBuf.getSample (c, i));
        }
    }

    // ---- meters ----
    const float grAvg = grBlockSum / (float) n;
    grSmoothed = grAvg > grSmoothed ? grAvg : grSmoothed * 0.85f + grAvg * 0.15f;
    const float c2m = c2Sum / (float) n, c3m = c3Sum / (float) n;
    const float thd = 100.0f * std::sqrt (std::pow (c2m * peakA * 0.5f, 2.0f) + std::pow (c3m * peakA * peakA * 0.25f, 2.0f));
    thdSmoothed = thd > thdSmoothed ? thd : thdSmoothed * 0.9f;
    meterGR.store (grSmoothed);
    meterThd.store (thdSmoothed);
}
}
