#include "PluginEditor.h"

using namespace juce;

static Font uiFont (float size, bool bold = true)
{
    return Font (FontOptions (size, bold ? Font::bold : Font::plain));
}

static void drawSpacedText (Graphics& g, const String& s, Rectangle<float> area, float size, Colour c,
                            Justification j = Justification::centred, float kerning = 0.12f)
{
    g.setColour (c);
    g.setFont (uiFont (size).withExtraKerningFactor (kerning));
    g.drawText (s, area, j, false);
}

static void cornerBrackets (Graphics& g, Rectangle<float> r, Colour c, float len = 14.f, float inset = 10.f)
{
    r = r.reduced (inset);
    Path p;
    p.startNewSubPath (r.getX(), r.getY() + len); p.lineTo (r.getX(), r.getY()); p.lineTo (r.getX() + len, r.getY());
    p.startNewSubPath (r.getRight() - len, r.getY()); p.lineTo (r.getRight(), r.getY()); p.lineTo (r.getRight(), r.getY() + len);
    p.startNewSubPath (r.getX(), r.getBottom() - len); p.lineTo (r.getX(), r.getBottom()); p.lineTo (r.getX() + len, r.getBottom());
    p.startNewSubPath (r.getRight() - len, r.getBottom()); p.lineTo (r.getRight(), r.getBottom()); p.lineTo (r.getRight(), r.getBottom() - len);
    g.setColour (c);
    g.strokePath (p, PathStrokeType (2.0f));
}

// ============================================================ knob look
void RaveLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float R = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 10.0f;
    const auto c = bounds.getCentre();
    const bool stepped = s.getInterval() >= 1.0 && s.getMaximum() <= 3.0;
    const bool bipolar = s.getMinimum() < 0;
    const float ang = a0 + pos * (a1 - a0);

    // purple glow
    g.setGradientFill (ColourGradient (rave::purple.withAlpha (0.35f), c.x, c.y, rave::purple.withAlpha (0.0f), c.x + R * 1.35f, c.y, true));
    g.fillEllipse (Rectangle<float> (R * 2.7f, R * 2.7f).withCentre (c));

    // track + value arc
    Path track; track.addCentredArc (c.x, c.y, R, R, 0, a0, a1, true);
    g.setColour (rave::border); g.strokePath (track, PathStrokeType (6.f, PathStrokeType::curved, PathStrokeType::rounded));
    Path arc;
    if (bipolar) { const float mid = (a0 + a1) / 2; arc.addCentredArc (c.x, c.y, R, R, 0, jmin (mid, ang), jmax (mid, ang), true); }
    else arc.addCentredArc (c.x, c.y, R, R, 0, a0, ang, true);
    g.setColour (rave::lime); g.strokePath (arc, PathStrokeType (6.f, PathStrokeType::curved, PathStrokeType::rounded));

    // silver ring (like the Kick Extractor icons) + dark body
    const float r2 = R - 11.f;
    g.setGradientFill (ColourGradient (Colour (0xffe9e6f2), c.x - r2, c.y - r2, Colour (0xff5d5670), c.x + r2, c.y + r2, false));
    g.fillEllipse (Rectangle<float> (r2 * 2, r2 * 2).withCentre (c));
    const float r3 = r2 - 3.f;
    g.setGradientFill (ColourGradient (Colour (0xff2a2038), c.x, c.y - r3, Colour (0xff120c1c), c.x, c.y + r3, false));
    g.fillEllipse (Rectangle<float> (r3 * 2, r3 * 2).withCentre (c));

    // pointer
    const Point<float> p1 = c.getPointOnCircumference (r3 * 0.35f, ang), p2 = c.getPointOnCircumference (r3 - 6.f, ang);
    g.setColour (rave::lime);
    g.drawLine ({ p1, p2 }, 4.f);
    g.fillEllipse (Rectangle<float> (6, 6).withCentre (p2));

    if (stepped) // 4 detents for SHAPE
        for (int i = 0; i < 4; ++i)
        {
            const float a = a0 + (float) i / 3.f * (a1 - a0);
            const bool on = i == roundToInt (s.getValue());
            g.setColour (on ? rave::lime : rave::purple);
            g.fillEllipse (Rectangle<float> (on ? 8.f : 6.f, on ? 8.f : 6.f).withCentre (c.getPointOnCircumference (R + 11.f, a)));
        }
}

// ============================================================ envelope display
EnvelopeDisplay::EnvelopeDisplay (KickDuckProcessor& p) : proc (p)
{
    lenParam = proc.params.getParameter ("length");
    setMouseCursor (MouseCursor::LeftRightResizeCursor);
}

void EnvelopeDisplay::pullMeter()
{
    KickDuckProcessor::MeterPoint pts[2048];
    const int n = proc.readMeter (pts, 2048);
    for (int i = 0; i < n; ++i)
    {
        const int col = jlimit (0, numCols - 1, (int) (pts[i].barPhase * numCols));
        if (lastCol >= 0 && col > lastCol && col - lastCol < numCols / 4)
            for (int c = lastCol + 1; c <= col; ++c) { inPk[(size_t) c] = pts[i].inPeak; outPk[(size_t) c] = pts[i].outPeak; }
        else { inPk[(size_t) col] = pts[i].inPeak; outPk[(size_t) col] = pts[i].outPeak; }
        lastCol = col;
    }
}

void EnvelopeDisplay::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    const bool playing = proc.hostPlaying.load();

    // card with lime glow (selected look from Kick Extractor)
    for (int i = 3; i >= 1; --i)
    {
        g.setColour (rave::lime.withAlpha (0.05f * (float) i));
        g.drawRoundedRectangle (b.reduced (3.f - (float) i), 18.f, 2.f * (float) i);
    }
    g.setGradientFill (ColourGradient (Colour (0xff141a08), 0, b.getY(), Colour (0xff0d0916), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b.reduced (3), 16.f);
    g.setColour (rave::lime);
    g.drawRoundedRectangle (b.reduced (3), 16.f, 2.f);
    cornerBrackets (g, b.reduced (3), rave::lime, 16.f, 10.f);

    auto area = b.reduced (34.f, 26.f).withTrimmedBottom (14.f);
    const float x0 = area.getX(), w = area.getWidth(), top = area.getY(), h = area.getHeight();
    const float mid = top + h / 2;
    const int beats = jmax (1, proc.beatsPerBar.load());

    // grid
    for (int i = 0; i <= beats * 4; ++i)
    {
        const float gx = x0 + (float) i / (float) (beats * 4) * w;
        g.setColour (rave::border.withAlpha (i % 4 == 0 ? 0.9f : 0.35f));
        g.drawVerticalLine ((int) gx, top, top + h);
        if (i % 4 == 0 && i < beats * 4)
        {
            g.setColour (rave::purple);
            g.fillRoundedRectangle (gx, top + h + 7.f, 7.f, 7.f, 1.5f);
        }
    }
    g.setColour (rave::border); g.drawHorizontalLine ((int) mid, x0, x0 + w);

    // waveform: input (silver) behind, output (lime) in front
    const float sx = w / (float) numCols;
    for (int c = 0; c < numCols; ++c)
    {
        const float a = jmin (1.f, inPk[(size_t) c]) * h / 2, o = jmin (1.f, outPk[(size_t) c]) * h / 2;
        if (a > 0.3f) { g.setColour (rave::silver.withAlpha (0.16f)); g.fillRect (x0 + (float) c * sx, mid - a, sx + 0.4f, 2 * a); }
        if (o > 0.3f) { g.setColour (rave::lime.withAlpha (0.55f)); g.fillRect (x0 + (float) c * sx, mid - o, sx + 0.4f, 2 * o); }
    }

    // envelope (final gain incl. MIX) over one bar
    const auto shape = proc.currentShape();
    const double mix = proc.currentMix01(), bl = 60.0 / proc.hostBpm.load();
    Path env, fill;
    for (int px = 0; px <= (int) w; ++px)
    {
        double pos = (double) px / w * beats, xx = pos - std::floor (pos);
        if (px == (int) w) xx = 0.99999;
        const float yy = top + h - (float) kickduck::gain (xx, bl, shape, mix) * h;
        if (px == 0) { env.startNewSubPath (x0, yy); fill.startNewSubPath (x0, top + h); fill.lineTo (x0, yy); }
        else { env.lineTo (x0 + (float) px, yy); fill.lineTo (x0 + (float) px, yy); }
    }
    fill.lineTo (x0 + w, top + h); fill.closeSubPath();
    g.setGradientFill (ColourGradient (Colours::white.withAlpha (0.10f), 0, top, Colours::white.withAlpha (0.0f), 0, top + h, false));
    g.fillPath (fill);
    g.setColour (Colours::white.withAlpha (0.95f));
    g.strokePath (env, PathStrokeType (2.2f, PathStrokeType::curved, PathStrokeType::rounded));

    // playhead
    const float bp = proc.barPhase.load();
    if (playing && bp >= 0)
    {
        const float hx = x0 + bp * w;
        double bx = bp * beats; bx -= std::floor (bx);
        g.setColour (rave::lime.withAlpha (0.9f)); g.drawVerticalLine ((int) hx, top, top + h);
        g.setColour (Colours::white);
        g.fillEllipse (Rectangle<float> (9, 9).withCentre ({ hx, top + h - (float) kickduck::gain (bx, bl, shape, mix) * h }));
    }

    drawSpacedText (g, playing ? "DRAG LEFT / RIGHT = LENGTH" : "PRESS PLAY TO SEE THE WAVEFORM  ·  DRAG = LENGTH",
                    { x0, b.getBottom() - 30.f, w, 16.f }, 10.f, rave::dim, Justification::centredRight);
}

void EnvelopeDisplay::mouseDown (const MouseEvent&)
{
    dragStartLen = lenParam->convertFrom0to1 (lenParam->getValue());
    lenParam->beginChangeGesture();
}
void EnvelopeDisplay::mouseDrag (const MouseEvent& e)
{
    const float sens = e.mods.isShiftDown() ? 1600.f : 400.f;
    const float v = jlimit (0.f, 100.f, dragStartLen + (float) e.getDistanceFromDragStartX() * 100.f / sens);
    lenParam->setValueNotifyingHost (lenParam->convertTo0to1 (v));
}
void EnvelopeDisplay::mouseUp (const MouseEvent&) { lenParam->endChangeGesture(); }
void EnvelopeDisplay::mouseDoubleClick (const MouseEvent&)
{
    lenParam->beginChangeGesture();
    lenParam->setValueNotifyingHost (lenParam->getDefaultValue());
    lenParam->endChangeGesture();
}

// ============================================================ knob cards
KnobCard::KnobCard (KickDuckProcessor& p, const String& paramId, const String& t, bool isStepped)
    : proc (p), pid (paramId), title (t), stepped (isStepped)
{
    slider.setSliderStyle (Slider::RotaryVerticalDrag);
    slider.setTextBoxStyle (Slider::NoTextBox, true, 0, 0);
    slider.setRotaryParameters (MathConstants<float>::pi * 1.25f, MathConstants<float>::pi * 2.75f, true);
    slider.setMouseDragSensitivity (stepped ? 120 : 220);
    addAndMakeVisible (slider);
    attachment = std::make_unique<AudioProcessorValueTreeState::SliderAttachment> (proc.params, pid, slider);
    if (auto* prm = proc.params.getParameter (pid))
        slider.setDoubleClickReturnValue (true, prm->convertFrom0to1 (prm->getDefaultValue()));
    slider.onValueChange = [this] { repaint(); };
    slider.addMouseListener (this, false);
}

void KnobCard::resized()
{
    auto r = getLocalBounds().reduced (14);
    r.removeFromBottom (34);
    slider.setBounds (r.withSizeKeepingCentre (jmin (r.getWidth(), r.getHeight()), jmin (r.getWidth(), r.getHeight())));
}

void KnobCard::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat().reduced (3);
    const bool hot = slider.isMouseOverOrDragging (true) || isMouseOver (true);
    g.setGradientFill (ColourGradient (hot ? rave::cardOn : rave::card, 0, b.getY(), Colour (0xff0e0918), 0, b.getBottom(), false));
    g.fillRoundedRectangle (b, 16.f);
    g.setColour (hot ? rave::lime : rave::border);
    g.drawRoundedRectangle (b, 16.f, hot ? 2.f : 1.2f);
    g.setColour (rave::border.withAlpha (0.7f));
    g.drawHorizontalLine ((int) b.getY() + 10, b.getX() + 24, b.getRight() - 24);
    g.drawHorizontalLine ((int) b.getBottom() - 10, b.getX() + 24, b.getRight() - 24);
    if (hot) cornerBrackets (g, b, rave::lime, 12.f, 9.f);

    String value;
    if (auto* prm = proc.params.getParameter (pid)) value = prm->getCurrentValueAsText();
    auto label = b.withTop (b.getBottom() - 46.f).withTrimmedBottom (14.f);
    drawSpacedText (g, title, label.removeFromTop (16.f), 11.5f, hot ? rave::lime : rave::text);
    drawSpacedText (g, value, label, 12.5f, hot ? rave::lime : rave::silver);
}

// ============================================================ editor
KickDuckEditor::KickDuckEditor (KickDuckProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p),
      mixCard (p, "mix", "MIX", false), lengthCard (p, "length", "LENGTH", false),
      curveCard (p, "curve", "CURVE", false), shapeCard (p, "shape", "SHAPE", true)
{
    setLookAndFeel (&lnf);
    addAndMakeVisible (display);
    for (auto* c : { &mixCard, &lengthCard, &curveCard, &shapeCard }) addAndMakeVisible (c);
    setSize (920, 600);
    startTimerHz (60);
}

KickDuckEditor::~KickDuckEditor() { setLookAndFeel (nullptr); }

void KickDuckEditor::timerCallback()
{
    display.pullMeter();
    display.repaint();
}

void KickDuckEditor::paint (Graphics& g)
{
    auto b = getLocalBounds().toFloat();
    g.setGradientFill (ColourGradient (rave::bg0, 0, 0, rave::bg1, 0, b.getBottom(), false));
    g.fillAll();
    // soft glows
    g.setGradientFill (ColourGradient (rave::purple.withAlpha (0.22f), b.getRight() - 140, b.getBottom() - 60,
                                       rave::purple.withAlpha (0.0f), b.getRight() - 140 - 420, b.getBottom() - 60, true));
    g.fillRect (b);
    g.setGradientFill (ColourGradient (rave::lime.withAlpha (0.10f), 140, 150, rave::lime.withAlpha (0.0f), 140 + 300, 150, true));
    g.fillRect (b);

    // logo text (left) + product name (right)
    auto head = b.removeFromTop (92.f).reduced (32.f, 18.f);
    auto logo = head.removeFromLeft (260.f);
    g.setColour (rave::text);
    g.setFont (uiFont (40.f).withExtraKerningFactor (0.02f).withHorizontalScale (1.05f));
    g.drawText ("RAVE", logo.removeFromTop (40.f), Justification::bottomLeft, false);
    drawSpacedText (g, "A C A D E M Y", logo.removeFromTop (18.f), 13.f, rave::purple, Justification::topLeft, 0.18f);

    g.setFont (uiFont (36.f).withHorizontalScale (0.92f));
    const String a = "KICK ", c = "DUCK";
    GlyphArrangement ga; ga.addLineOfText (g.getCurrentFont(), a + c, 0, 0);
    const float tw = ga.getBoundingBox (0, -1, true).getWidth();
    auto name = head.withLeft (head.getRight() - tw - 4.f);
    g.setColour (rave::lime); g.drawText (a, name, Justification::centredLeft, false);
    GlyphArrangement ga2; ga2.addLineOfText (g.getCurrentFont(), a, 0, 0);
    g.setColour (rave::text); g.drawText (c, name.withTrimmedLeft (ga2.getBoundingBox (0, -1, true).getWidth()), Justification::centredLeft, false);

    // main rounded panel
    auto panel = getLocalBounds().toFloat().withTrimmedTop (92.f).reduced (20.f, 0.f).withTrimmedBottom (20.f);
    g.setColour (rave::panel.withAlpha (0.85f));
    g.fillRoundedRectangle (panel, 26.f);
    g.setGradientFill (ColourGradient (Colour (0xff3a2a55), panel.getX(), panel.getY(), Colour (0xff1c1429), panel.getRight(), panel.getBottom(), false));
    g.drawRoundedRectangle (panel, 26.f, 2.f);
}

void KickDuckEditor::resized()
{
    auto panel = getLocalBounds().withTrimmedTop (92).reduced (20, 0).withTrimmedBottom (20).reduced (18);
    display.setBounds (panel.removeFromTop (262));
    panel.removeFromTop (16);
    const int gap = 14, cw = (panel.getWidth() - 3 * gap) / 4;
    for (auto* c : { &mixCard, &lengthCard, &curveCard, &shapeCard })
    {
        c->setBounds (panel.removeFromLeft (cw));
        panel.removeFromLeft (gap);
    }
}
