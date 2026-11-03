#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <atomic>
#include <cmath>
#include <memory>
#include <vector>

// VISE-8 : a behavioural model of a digitally-controlled analogue knee compressor with
// switchable harmonic distortion. Built from published specifications and measured
// behaviour of this class of unit, NOT from any vendor's code or circuit data.
namespace vise
{
    // ----- knob -> physical value maps (shared by DSP and UI text) -----
    inline float knobToInputDb  (float v) { return (v - 5.0f) * 4.0f; }   // -20 .. +20 dB
    inline float knobToOutputDb (float v) { return (v - 5.0f) * 3.0f; }   // -15 .. +15 dB
    inline float knobToAttackSec (float v) { return 50.0e-6f * std::pow (1000.0f, v / 10.0f); }   // 50us .. 50ms
    inline float knobToReleaseSec (float v) { return 0.05f * std::pow (70.0f, v / 10.0f); }       // 50ms .. 3.5s
    inline float knobToOptoReleaseSec (float v) { return 0.3f * std::pow (66.7f, v / 10.0f); }    // 0.3s .. 20s

    inline const char* const ratioNames[8] = { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1 Opto", "20:1", "Nuke" };
    inline const char* const audioNames[3] = { "Clean", "Dist 2", "Dist 3" };
    inline constexpr int kOptoIndex = 5;

    class ViseEngine
    {
    public:
        struct Params
        {
            float inputDb = 0, outputDb = 0, attack = 3, release = 5, mix = 1, link = 1;
            int ratio = 3, audio = 0;
            bool hp = false, detHp = false, brit = false, engaged = true;
        };

        void prepare (double sampleRate, int maxBlock, int numChannels);
        void reset();
        int getLatencySamples() const noexcept { return latency; }
        void process (juce::AudioBuffer<float>& buffer, const Params& p);

        // static gain computer (dB over threshold -> dB of gain reduction); public for tests
        static float gainComputer (float levelDb, int ratioIdx, bool brit) noexcept;

        std::atomic<float> meterGR { 0.0f }, meterThd { 0.0f };

    private:
        static constexpr float kThresholdDb = -18.0f;

        double sr = 44100.0;
        int nch = 2, latency = 0, maxBlockSize = 512;

        std::unique_ptr<juce::dsp::Oversampling<float>> oversampling;
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::None> dryDelay { 4096 };
        juce::AudioBuffer<float> dryBuf;
        std::vector<float> grBuf[2];

        juce::dsp::IIR::Filter<float> audioHp1[2], audioHp2[2], detHpF[2];
        juce::SmoothedValue<float, juce::ValueSmoothingTypes::Linear> inSm, outSm;

        float pk[2] {}, rms[2] {}, grEnv[2] {}, grFast[2] {}, grSlow[2] {}, dcX[2] {}, dcY[2] {};
        float thdSmoothed = 0.0f, grSmoothed = 0.0f;
    };
}
