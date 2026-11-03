#include "PluginEditor.h"

using namespace juce;

static Font fnt (float size, int style = Font::plain) { return Font (FontOptions (size, style)); }

//==============================================================================
void ui::KnobLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos,
                                            float startAngle, float endAngle, Slider& s)
{
    auto* k = dynamic_cast<Knob*> (&s);
    const bool small = k != nullptr && k->small;
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const auto c = bounds.getCentre();
    const float body = small ? 16.0f : 34.0f;
    const float angle = startAngle + pos * (endAngle - startAngle);

    auto pt = [&] (float a, float r) { return Point<float> (c.x + r * std::sin (a), c.y - r * std::cos (a)); };

    // printed scale (ticks + numerals) on the panel
    if (! small)
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            g.setColour (Colour (0xffe9e6dc).withAlpha (i % 5 == 0 ? 0.95f : 0.7f));
            g.drawLine ({ pt (a, body + 6.0f), pt (a, body + (i % 5 == 0 ? 14.0f : 11.0f)) }, i % 5 == 0 ? 1.6f : 1.1f);
            g.setFont (fnt (10.0f, Font::bold));
            auto tp = pt (a, body + 21.0f);
            g.drawText (String (i), Rectangle<float> (16, 12).withCentre (tp), Justification::centred);
        }
    }
    else
    {
        for (int i = 0; i <= 10; ++i)
        {
            const float a = startAngle + (float) i / 10.0f * (endAngle - startAngle);
            g.setColour (Colour (0xffe9e6dc).withAlpha (0.8f));
            g.drawLine ({ pt (a, body + 4.0f), pt (a, body + (i % 5 == 0 ? 9.0f : 7.0f)) }, 1.0f);
        }
    }

    // shadow cast on the faceplate
    Path disc; disc.addEllipse (c.x - body, c.y - body, body * 2, body * 2);
    DropShadow (Colours::black.withAlpha (0.75f), small ? 8 : 16, { 2, small ? 3 : 6 }).drawForPath (g, disc);

    // dark skirt ring
    g.setGradientFill (ColourGradient (Colour (0xff3a3a3a), c.x - body, c.y - body, Colour (0xff0c0c0c), c.x + body, c.y + body, false));
    g.fillEllipse (c.x - body - 2.5f, c.y - body - 2.5f, body * 2 + 5.0f, body * 2 + 5.0f);

    // serrated grip edge
    {
        Path ridges;
        const int nRidges = small ? 36 : 60;
        for (int i = 0; i < nRidges; ++i)
        {
            const float a = (float) i / (float) nRidges * MathConstants<float>::twoPi;
            ridges.startNewSubPath (pt (a, body - 3.5f));
            ridges.lineTo (pt (a, body));
        }
        g.setColour (Colour (0xff8d8b84));
        g.fillEllipse (c.x - body, c.y - body, body * 2, body * 2);
        g.setColour (Colour (0xff302f2b).withAlpha (0.55f));
        g.strokePath (ridges, PathStrokeType (small ? 1.0f : 1.3f));
    }

    // satin white body (light from top-left)
    const float bodyIn = body - (small ? 3.0f : 4.5f);
    g.setGradientFill (ColourGradient (Colour (0xfff7f5ee), c.x - bodyIn * 0.8f, c.y - bodyIn * 0.9f,
                                       Colour (0xffb9b6ab), c.x + bodyIn * 0.9f, c.y + bodyIn, false));
    g.fillEllipse (c.x - bodyIn, c.y - bodyIn, bodyIn * 2, bodyIn * 2);
    g.setColour (Colours::white.withAlpha (0.55f));
    g.drawEllipse (c.x - bodyIn + 0.7f, c.y - bodyIn + 0.7f, bodyIn * 2 - 1.4f, bodyIn * 2 - 1.4f, 1.0f);

    // recessed grey cap
    const float cap = bodyIn * (small ? 0.62f : 0.66f);
    g.setGradientFill (ColourGradient (Colour (0xff8b8a85), c.x - cap, c.y - cap, Colour (0xffe8e6df), c.x + cap, c.y + cap, false));
    g.fillEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2);
    g.setColour (Colour (0xff2a2a28).withAlpha (0.7f));
    g.drawEllipse (c.x - cap, c.y - cap, cap * 2, cap * 2, 1.2f);

    // pointer
    Path ptr;
    ptr.startNewSubPath (pt (angle, small ? 3.0f : 8.0f));
    ptr.lineTo (pt (angle, bodyIn - (small ? 1.5f : 3.0f)));
    g.setColour (Colour (0xff141414));
    g.strokePath (ptr, PathStrokeType (small ? 2.2f : 3.6f, PathStrokeType::curved, PathStrokeType::rounded));
}

//==============================================================================
void ui::PanelButton::paintButton (Graphics& g, bool over, bool down)
{
    auto r = getLocalBounds().toFloat().reduced (1.5f);
    const bool on = getToggleState();

    DropShadow (Colours::black.withAlpha (0.7f), 5, { 0, 2 }).drawForPath (g, [&] { Path p; p.addRoundedRectangle (r, 4.0f); return p; }());

    g.setGradientFill (ColourGradient (down ? Colour (0xff1c1c1d) : Colour (0xff3b3c3e), r.getX(), r.getY(),
                                       Colour (0xff17181a), r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 4.0f);
    g.setColour (Colour (0xff555759).withAlpha (over ? 0.9f : 0.6f));
    g.drawRoundedRectangle (r, 4.0f, 1.0f);
    g.setColour (Colours::white.withAlpha (0.07f));
    g.drawLine (r.getX() + 4, r.getY() + 1.5f, r.getRight() - 4, r.getY() + 1.5f);

    // LED window
    auto led = Rectangle<float> (r.getCentreX() - 11.0f, r.getY() + 8.0f, 22.0f, 6.0f);
    g.setColour (Colour (0xff060606));
    g.fillRoundedRectangle (led.expanded (1.5f), 2.5f);
    if (on)
    {
        g.setColour (ledColour.withAlpha (0.20f));
        g.fillRoundedRectangle (led.expanded (5.0f, 4.0f), 5.0f);
        g.setColour (ledColour.withAlpha (0.28f));
        g.fillRoundedRectangle (led.expanded (2.5f, 2.0f), 3.5f);
        g.setGradientFill (ColourGradient (ledColour.brighter (0.7f), led.getCentreX(), led.getY(),
                                           ledColour, led.getCentreX(), led.getBottom(), false));
    }
    else
        g.setColour (ledColour.darker (2.4f).withAlpha (0.85f));
    g.fillRoundedRectangle (led, 2.0f);

    g.setColour (on ? Colour (0xfff2efe6) : Colour (0xffb9b6ab));
    g.setFont (fnt (11.5f, Font::bold));
    g.drawText (getButtonText(), getLocalBounds().withTrimmedTop (22).withTrimmedBottom (4), Justification::centred);
}

//==============================================================================
ui::ChoiceStrip::ChoiceStrip (RangedAudioParameter& p, const StringArray& labels, Colour led)
    : parameter (p),
      attachment (p, [this] (float v)
      {
          for (int i = 0; i < buttons.size(); ++i) buttons[i]->setToggleState (i == (int) std::round (v), dontSendNotification);
      })
{
    for (int i = 0; i < labels.size(); ++i)
    {
        auto* b = buttons.add (new PanelButton (labels[i], led));
        b->setClickingTogglesState (false);
        b->onClick = [this, i] { attachment.setValueAsCompleteGesture ((float) i); };
        addAndMakeVisible (b);
    }
    attachment.sendInitialUpdate();
}

void ui::ChoiceStrip::resized()
{
    const int n = buttons.size(), gap = 4;
    const int bw = (getWidth() - gap * (n - 1)) / n;
    for (int i = 0; i < n; ++i) buttons[i]->setBounds (i * (bw + gap), 0, bw, getHeight());
}

//==============================================================================
void ui::LedMeter::paint (Graphics& g)
{
    static const float grSteps[12] = { 1, 2, 3, 4, 6, 8, 10, 12, 15, 18, 20, 24 };
    auto b = getLocalBounds().toFloat();

    if (kind == GainReduction)
    {
        const float w = 16.0f, gap = 4.0f;
        for (int i = 0; i < 12; ++i)
        {
            auto r = Rectangle<float> (b.getX() + (float) i * (w + gap), b.getY() + 6.0f, w, 9.0f);
            const bool on = value >= grSteps[i];
            const Colour col = i >= 9 ? Colour (0xffff3b2a) : Colour (0xffffb02e);
            g.setColour (Colour (0xff060606)); g.fillRoundedRectangle (r.expanded (1.5f), 2.0f);
            if (on)
            {
                g.setColour (col.withAlpha (0.25f)); g.fillRoundedRectangle (r.expanded (3.5f, 3.0f), 4.0f);
                g.setGradientFill (ColourGradient (col.brighter (0.6f), r.getCentreX(), r.getY(), col, r.getCentreX(), r.getBottom(), false));
            }
            else g.setColour (col.darker (2.6f).withAlpha (0.9f));
            g.fillRoundedRectangle (r, 1.5f);

            g.setColour (Colour (0xffb9b6ab));
            g.setFont (fnt (9.5f, Font::bold));
            g.drawText (String ((int) grSteps[i]), r.translated (0, 14).expanded (6, 0), Justification::centred);
        }
    }
    else
    {
        const bool led1 = value >= 1.0f, led3 = value >= 3.0f;
        auto draw = [&] (Rectangle<float> r, Colour col, bool on, const String& label)
        {
            g.setColour (Colour (0xff060606)); g.fillEllipse (r.expanded (2.0f));
            if (on) { g.setColour (col.withAlpha (0.28f)); g.fillEllipse (r.expanded (7.0f));
                      g.setGradientFill (ColourGradient (col.brighter (0.8f), r.getCentreX(), r.getY(), col, r.getCentreX(), r.getBottom(), false)); }
            else g.setColour (col.darker (2.6f).withAlpha (0.9f));
            g.fillEllipse (r);
            g.setColour (Colour (0xffb9b6ab)); g.setFont (fnt (9.5f, Font::bold));
            g.drawText (label, r.translated (0, 12).expanded (16, 0), Justification::centred);
        };
        draw (Rectangle<float> (6.0f, 6.0f, 9.0f, 9.0f), Colour (0xffffc83a), led1, "1%");
        draw (Rectangle<float> (50.0f, 6.0f, 9.0f, 9.0f), Colour (0xffff3b2a), led3, "3%");
    }
}

//==============================================================================
class ViseEditor::Panel : public Component
{
public:
    explicit Panel (ViseProcessor& p)
        : proc (p),
          inputK (false), attackK (false), releaseK (false), outputK (false), mixK (true),
          ratio (*p.apvts.getParameter ("ratio"), { "1:1", "2:1", "3:1", "4:1", "6:1", "10:1", "20:1", "NUKE" }, Colour (0xffffb02e)),
          audio (*p.apvts.getParameter ("audio"), { "CLEAN", "DIST 2", "DIST 3" }, Colour (0xffffc83a)),
          hpB ("HP", Colour (0xff59e36b)), detHpB ("DET HP", Colour (0xff59e36b)),
          linkB ("LINK", Colour (0xff59e36b)), britB ("BRIT", Colour (0xffff7a2e)), inB ("IN", Colour (0xff59e36b)),
          grMeter (ui::LedMeter::GainReduction), thdMeter (ui::LedMeter::Distortion)
    {
        setSize (panelW(), panelH());
        setOpaque (true);

        auto setup = [this] (ui::Knob& k, const char* id, std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment>& a)
        {
            addAndMakeVisible (k);
            a = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.apvts, id, k);
        };
        setup (inputK, "input", aIn);  setup (attackK, "attack", aAtt);
        setup (releaseK, "release", aRel);  setup (outputK, "output", aOut);
        setup (mixK, "mix", aMix);
        mixK.setDoubleClickReturnValue (true, 100.0);
        mixK.setRotaryParameters (degreesToRadians (225.0f), degreesToRadians (495.0f), true);
        attackK.setDoubleClickReturnValue (true, 3.0);

        addAndMakeVisible (ratio);  addAndMakeVisible (audio);
        auto toggle = [this] (ui::PanelButton& b, const char* id, std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment>& a)
        {
            b.setClickingTogglesState (true);
            addAndMakeVisible (b);
            a = std::make_unique<AudioProcessorValueTreeState::ButtonAttachment> (proc.apvts, id, b);
        };
        toggle (hpB, "hp", aHp);  toggle (detHpB, "dethp", aDet);  toggle (linkB, "link", aLink);
        toggle (britB, "brit", aBrit);  toggle (inB, "in", aIn2);
        addAndMakeVisible (grMeter);  addAndMakeVisible (thdMeter);
    }

    static int panelW() { return ViseEditor::panelW; }
    static int panelH() { return ViseEditor::panelH; }

    void resized() override
    {
        const int ky = 172;
        auto place = [&] (ui::Knob& k, int cx, int cy, int size) { k.setBounds (Rectangle<int> (size, size).withCentre ({ cx, cy })); };
        place (inputK, 105, ky, 124);  place (attackK, 229, ky, 124);
        place (releaseK, 353, ky, 124);  place (outputK, 477, ky, 124);
        place (mixK, 985, 236, 52);

        ratio.setBounds (584, 66, 8 * 48 + 7 * 4, 46);
        inB.setBounds (1004, 66, 48, 46);
        audio.setBounds (584, 150, 3 * 48 + 2 * 4, 46);
        hpB.setBounds (748, 150, 48, 46);
        detHpB.setBounds (816, 150, 48, 46);
        linkB.setBounds (868, 150, 48, 46);
        britB.setBounds (920, 150, 48, 46);
        grMeter.setBounds (584, 220, 12 * 20, 34);
        thdMeter.setBounds (836, 220, 90, 34);
    }

    void paint (Graphics& g) override
    {
        if (bg.isNull()) bg = renderBackground();
        g.drawImage (bg, getLocalBounds().toFloat(), RectanglePlacement::stretchToFit);
    }

private:
    ViseProcessor& proc;
    Image bg;

    void groupLabel (Graphics& g, const String& text, float x1, float x2, float y) const
    {
        g.setFont (fnt (10.5f, Font::bold).withExtraKerningFactor (0.18f));
        const float tw = g.getCurrentFont().getStringWidthFloat (text) + 14.0f;
        const float cx = (x1 + x2) * 0.5f;
        g.setColour (Colour (0xffe9e6dc).withAlpha (0.85f));
        g.drawText (text, Rectangle<float> (tw, 14.0f).withCentre ({ cx, y }), Justification::centred);
        g.setColour (Colour (0xffe9e6dc).withAlpha (0.45f));
        g.drawHorizontalLine ((int) y, x1, cx - tw * 0.5f);
        g.drawHorizontalLine ((int) y, cx + tw * 0.5f, x2);
        g.drawVerticalLine ((int) x1, y, y + 5.0f);
        g.drawVerticalLine ((int) x2, y, y + 5.0f);
    }

    void screw (Graphics& g, float cx, float cy, float rot) const
    {
        g.setGradientFill (ColourGradient (Colour (0xffd8d8d4), cx - 6, cy - 6, Colour (0xff6c6c68), cx + 6, cy + 6, false));
        g.fillEllipse (cx - 6.5f, cy - 6.5f, 13.0f, 13.0f);
        g.setColour (Colours::black.withAlpha (0.55f));
        g.drawEllipse (cx - 6.5f, cy - 6.5f, 13.0f, 13.0f, 1.0f);
        Path slot; slot.addRectangle (-5.0f, -1.1f, 10.0f, 2.2f);
        slot.applyTransform (AffineTransform::rotation (rot).translated (cx, cy));
        g.setColour (Colour (0xff1c1c1b)); g.fillPath (slot);
    }

    Image renderBackground() const
    {
        const float W = (float) panelW(), H = (float) panelH();
        Image img (Image::ARGB, (int) W * 2, (int) H * 2, true);
        Graphics g (img);
        g.addTransform (AffineTransform::scale (2.0f));

        // rack ears: brushed dark aluminium
        g.setGradientFill (ColourGradient (Colour (0xff4a4b4d), 0, 0, Colour (0xff2a2b2d), 0, H, false));
        g.fillRect (0.0f, 0.0f, W, H);

        // faceplate
        const Rectangle<float> face (38.0f, 0.0f, W - 76.0f, H);
        g.setGradientFill (ColourGradient (Colour (0xff1e1f21), 0, 0, Colour (0xff111213), 0, H, false));
        g.fillRect (face);

        // brushing
        Random rng (7);
        for (float y = 0; y < H; y += 0.5f)
        {
            const float a = rng.nextFloat() * 0.05f;
            g.setColour ((rng.nextBool() ? Colours::white : Colours::black).withAlpha (a));
            g.drawHorizontalLine ((int) y, 0.0f, W);
        }
        for (int i = 0; i < 2600; ++i)    // speckle
        {
            g.setColour (Colours::white.withAlpha (rng.nextFloat() * 0.035f));
            g.fillRect (rng.nextFloat() * W, rng.nextFloat() * H, 1.0f + rng.nextFloat() * 6.0f, 0.5f);
        }

        // bevels
        g.setColour (Colours::white.withAlpha (0.10f)); g.drawVerticalLine (38, 0, H); g.drawHorizontalLine (1, 38, W - 38);
        g.setColour (Colours::black.withAlpha (0.8f));  g.drawVerticalLine (37, 0, H); g.drawVerticalLine ((int) W - 38, 0, H);
        g.setColour (Colours::black.withAlpha (0.5f));  g.drawHorizontalLine ((int) H - 1, 0, W);
        g.setColour (Colours::white.withAlpha (0.14f)); g.drawHorizontalLine (0, 0, W);

        // rack slots + screws
        for (float x : { 19.0f, W - 19.0f })
        {
            for (float y : { 52.0f, H - 52.0f })
            {
                Path slot; slot.addRoundedRectangle (x - 6.5f, y - 13.0f, 13.0f, 26.0f, 6.5f);
                g.setColour (Colour (0xff050505)); g.fillPath (slot);
                g.setColour (Colours::white.withAlpha (0.12f)); g.strokePath (slot, PathStrokeType (1.0f));
            }
        }
        const float rots[] = { 0.4f, 1.9f, 0.9f, 2.6f, 1.2f, 0.2f, 2.2f, 0.7f };
        int ri = 0;
        for (float x : { 56.0f, W - 56.0f }) for (float y : { 18.0f, H - 18.0f }) screw (g, x, y, rots[ri++]);

        // brand block
        g.setColour (Colour (0xfff2efe6));
        g.setFont (fnt (34.0f, Font::bold).withExtraKerningFactor (0.06f));
        g.drawText ("VISE-8", Rectangle<float> (62, 20, 300, 40), Justification::centredLeft);
        g.setFont (fnt (11.0f, Font::bold).withExtraKerningFactor (0.24f));
        g.setColour (Colour (0xffe9e6dc).withAlpha (0.85f));
        g.drawText ("KNEE COMPRESSOR / HARMONIC DISTORTION", Rectangle<float> (63, 56, 420, 14), Justification::centredLeft);
        g.setFont (fnt (10.0f, Font::bold).withExtraKerningFactor (0.3f));
        g.setColour (Colour (0xffe9e6dc).withAlpha (0.55f));
        g.drawText ("ANVIL DYNAMICS", Rectangle<float> (63, 72, 300, 14), Justification::centredLeft);
        g.drawLine (62, 92, 330, 92, 1.0f);

        // knob captions
        const char* names[] = { "INPUT", "ATTACK", "RELEASE", "OUTPUT" };
        const int cxs[] = { 105, 229, 353, 477 };
        for (int i = 0; i < 4; ++i)
        {
            g.setColour (Colour (0xfff2efe6));
            g.setFont (fnt (13.0f, Font::bold).withExtraKerningFactor (0.2f));
            g.drawText (names[i], Rectangle<float> (110, 16).withCentre ({ (float) cxs[i], 246.0f }), Justification::centred);
        }

        // right-hand silkscreen
        groupLabel (g, "RATIO", 584, 584 + 412, 52);
        g.setColour (Colour (0xffb9b6ab)); g.setFont (fnt (9.5f, Font::bold).withExtraKerningFactor (0.15f));
        g.drawText ("OPTO", Rectangle<float> (584 + 5 * 52, 113, 48, 12), Justification::centred);
        g.drawText ("BRICKWALL", Rectangle<float> (584 + 7 * 52 - 8, 113, 64, 12), Justification::centred);

        groupLabel (g, "AUDIO", 584, 796, 136);
        groupLabel (g, "DETECTOR", 816, 968, 136);
        groupLabel (g, "GAIN REDUCTION  dB", 584, 584 + 240, 212);
        groupLabel (g, "DISTORTION", 836, 926, 212);
        g.setColour (Colour (0xfff2efe6)); g.setFont (fnt (11.0f, Font::bold).withExtraKerningFactor (0.2f));
        g.drawText ("MIX", Rectangle<float> (60, 14).withCentre ({ 985.0f, 272.0f }), Justification::centred);
        g.setFont (fnt (10.0f, Font::bold));
        g.drawText ("0",   Rectangle<float> (24, 12).withCentre ({ 950.0f, 244.0f }), Justification::centred);
        g.drawText ("100", Rectangle<float> (30, 12).withCentre ({ 1024.0f, 244.0f }), Justification::centred);

        // power lamp
        g.setColour (Colour (0xff060606)); g.fillEllipse (320, 24, 12, 12);
        g.setColour (Colour (0xff59e36b).withAlpha (0.25f)); g.fillEllipse (315, 19, 22, 22);
        g.setGradientFill (ColourGradient (Colour (0xffb8ffc2), 326, 26, Colour (0xff2fbf48), 326, 34, false));
        g.fillEllipse (322, 26, 8, 8);

        // vignette to seat the controls into the plate
        g.setGradientFill (ColourGradient (Colours::transparentBlack, W * 0.5f, H * 0.5f, Colours::black.withAlpha (0.35f), 0, 0, true));
        g.fillRect (face);
        return img;
    }

    ui::Knob inputK, attackK, releaseK, outputK, mixK;
    ui::ChoiceStrip ratio, audio;
    ui::PanelButton hpB, detHpB, linkB, britB, inB;
public:
    ui::LedMeter grMeter, thdMeter;
private:
    std::unique_ptr<AudioProcessorValueTreeState::SliderAttachment> aIn, aAtt, aRel, aOut, aMix;
    std::unique_ptr<AudioProcessorValueTreeState::ButtonAttachment> aHp, aDet, aLink, aBrit, aIn2;
};

//==============================================================================
ViseEditor::ViseEditor (ViseProcessor& p) : AudioProcessorEditor (&p), processor (p)
{
    panel = std::make_unique<Panel> (p);
    addAndMakeVisible (*panel);
    setResizable (true, false);
    setResizeLimits (825, 217, 2200, 580);
    getConstrainer()->setFixedAspectRatio ((double) panelW / (double) panelH);
    setSize (panelW, panelH);
    startTimerHz (30);
}

ViseEditor::~ViseEditor() { stopTimer(); }

void ViseEditor::resized()
{
    panel->setTransform (AffineTransform::scale ((float) getWidth() / (float) panelW));
    panel->setBounds (0, 0, panelW, panelH);
}

void ViseEditor::timerCallback() { updateMeters(); }

void ViseEditor::updateMeters()
{
    panel->grMeter.setValue (processor.engine.meterGR.load());
    panel->thdMeter.setValue (processor.engine.meterThd.load());
}
