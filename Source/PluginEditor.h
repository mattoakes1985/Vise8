#pragma once
#include "PluginProcessor.h"

namespace ui
{
    // Large satin-white knob with serrated skirt, printed 0-10 scale and pointer.
    class KnobLookAndFeel : public juce::LookAndFeel_V4
    {
    public:
        void drawRotarySlider (juce::Graphics&, int x, int y, int w, int h, float pos,
                               float startAngle, float endAngle, juce::Slider&) override;
    };

    class Knob : public juce::Slider
    {
    public:
        explicit Knob (bool smallKnob = false) : small (smallKnob)
        {
            setSliderStyle (juce::Slider::RotaryVerticalDrag);
            setTextBoxStyle (juce::Slider::NoTextBox, true, 0, 0);
            setRotaryParameters (juce::degreesToRadians (225.0f), juce::degreesToRadians (495.0f), true);
            setLookAndFeel (&laf);
            setPopupDisplayEnabled (true, true, nullptr);
            setMouseDragSensitivity (220);
            setDoubleClickReturnValue (true, 5.0);
        }
        ~Knob() override { setLookAndFeel (nullptr); }
        bool small;
    private:
        static inline KnobLookAndFeel laf;
    };

    // Square illuminated push button with LED window.
    class PanelButton : public juce::Button
    {
    public:
        PanelButton (const juce::String& text, juce::Colour led) : juce::Button (text), ledColour (led) {}
        void paintButton (juce::Graphics&, bool over, bool down) override;
        juce::Colour ledColour;
    };

    // Radio-style group bound to a choice parameter.
    class ChoiceStrip : public juce::Component
    {
    public:
        ChoiceStrip (juce::RangedAudioParameter& param, const juce::StringArray& labels, juce::Colour led);
        void resized() override;
    private:
        juce::RangedAudioParameter& parameter;
        juce::OwnedArray<PanelButton> buttons;
        juce::ParameterAttachment attachment;
    };

    class LedMeter : public juce::Component
    {
    public:
        enum Kind { GainReduction, Distortion };
        explicit LedMeter (Kind k) : kind (k) {}
        void setValue (float v) { if (v != value) { value = v; repaint(); } }
        void paint (juce::Graphics&) override;
    private:
        Kind kind;
        float value = 0.0f;
    };
}

class ViseEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit ViseEditor (ViseProcessor&);
    ~ViseEditor() override;
    void resized() override;
    void paint (juce::Graphics&) override {}

    void updateMeters();
    static constexpr int panelW = 1100, panelH = 290;

private:
    class Panel;
    void timerCallback() override;

    ViseProcessor& processor;
    std::unique_ptr<Panel> panel;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ViseEditor)
};
