#include "PluginProcessor.h"
#include "PluginEditor.h"

using namespace juce;

static String dbText (float db) { return String (db, 1) + " dB"; }
static String timeText (float sec)
{
    if (sec < 0.001f)  return String (sec * 1.0e6f, 0) + " us";
    if (sec < 1.0f)    return String (sec * 1000.0f, sec < 0.01f ? 1 : 0) + " ms";
    return String (sec, 2) + " s";
}

AudioProcessorValueTreeState::ParameterLayout ViseProcessor::createLayout()
{
    using namespace vise;
    std::vector<std::unique_ptr<RangedAudioParameter>> p;
    auto knob = [] (const char* id, const char* name, float def, std::function<String (float)> text)
    {
        return std::make_unique<AudioParameterFloat> (
            ParameterID { id, 1 }, name, NormalisableRange<float> (0.0f, 10.0f, 0.01f), def,
            AudioParameterFloatAttributes().withStringFromValueFunction ([text] (float v, int) { return text (v); }));
    };

    p.push_back (knob ("input",   "Input",   5.0f, [] (float v) { return dbText (knobToInputDb (v)); }));
    p.push_back (knob ("attack",  "Attack",  3.0f, [] (float v) { return timeText (knobToAttackSec (v)); }));
    p.push_back (knob ("release", "Release", 5.0f, [] (float v) { return timeText (knobToReleaseSec (v)); }));
    p.push_back (knob ("output",  "Output",  5.0f, [] (float v) { return dbText (knobToOutputDb (v)); }));

    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "ratio", 1 }, "Ratio",
                 StringArray (ratioNames, 8), 3));
    p.push_back (std::make_unique<AudioParameterChoice> (ParameterID { "audio", 1 }, "Audio Mode",
                 StringArray (audioNames, 3), 0));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "hp", 1 },    "Audio HP", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "dethp", 1 }, "Detector HP", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "link", 1 },  "Stereo Link", true));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "brit", 1 },  "Brit Mode", false));
    p.push_back (std::make_unique<AudioParameterBool> (ParameterID { "in", 1 },    "In", true));
    p.push_back (std::make_unique<AudioParameterFloat> (ParameterID { "mix", 1 }, "Mix",
                 NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f,
                 AudioParameterFloatAttributes().withLabel ("%")));
    return { p.begin(), p.end() };
}

ViseProcessor::ViseProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", AudioChannelSet::stereo(), true)
                                       .withOutput ("Output", AudioChannelSet::stereo(), true)),
      apvts (*this, nullptr, "STATE", createLayout())
{
}

bool ViseProcessor::isBusesLayoutSupported (const BusesLayout& l) const
{
    const auto o = l.getMainOutputChannelSet();
    return (o == AudioChannelSet::mono() || o == AudioChannelSet::stereo()) && o == l.getMainInputChannelSet();
}

void ViseProcessor::prepareToPlay (double sr, int block)
{
    engine.prepare (sr, block, getTotalNumInputChannels());
    setLatencySamples (engine.getLatencySamples());
}

void ViseProcessor::processBlock (AudioBuffer<float>& buffer, MidiBuffer&)
{
    ScopedNoDenormals noDenormals;
    for (int c = getTotalNumInputChannels(); c < getTotalNumOutputChannels(); ++c)
        buffer.clear (c, 0, buffer.getNumSamples());

    auto get = [this] (const char* id) { return apvts.getRawParameterValue (id)->load(); };
    vise::ViseEngine::Params p;
    p.inputDb  = vise::knobToInputDb (get ("input"));
    p.outputDb = vise::knobToOutputDb (get ("output"));
    p.attack   = get ("attack");
    p.release  = get ("release");
    p.ratio    = jlimit (0, 7, (int) get ("ratio"));
    p.audio    = jlimit (0, 2, (int) get ("audio"));
    p.hp       = get ("hp") > 0.5f;
    p.detHp    = get ("dethp") > 0.5f;
    p.link     = get ("link") > 0.5f ? 1.0f : 0.0f;
    p.brit     = get ("brit") > 0.5f;
    p.engaged  = get ("in") > 0.5f;
    p.mix      = get ("mix") * 0.01f;
    engine.process (buffer, p);
}

AudioProcessorEditor* ViseProcessor::createEditor() { return new ViseEditor (*this); }

void ViseProcessor::getStateInformation (MemoryBlock& dest)
{
    if (auto xml = apvts.copyState().createXml()) copyXmlToBinary (*xml, dest);
}

void ViseProcessor::setStateInformation (const void* data, int size)
{
    if (auto xml = getXmlFromBinary (data, size))
        if (xml->hasTagName (apvts.state.getType())) apvts.replaceState (ValueTree::fromXml (*xml));
}

AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new ViseProcessor(); }
