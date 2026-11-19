#include "PluginEditor.h"
#include "BinaryData.h"

using namespace juce;

// ============================================================================
//  Fonts and helpers (same as Kick Extractor)
// ============================================================================
namespace look
{
namespace
{
struct Typefaces
{
    Typeface::Ptr oswald = Typeface::createSystemTypefaceFor (BinaryData::OswaldBold_ttf, BinaryData::OswaldBold_ttfSize);
    Typeface::Ptr monoBold = Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoBold_ttf, BinaryData::SpaceMonoBold_ttfSize);
    Typeface::Ptr monoRegular = Typeface::createSystemTypefaceFor (BinaryData::SpaceMonoRegular_ttf, BinaryData::SpaceMonoRegular_ttfSize);
};
}

Font heading (float height)
{
    static SharedResourcePointer<Typefaces> t;
    return Font (FontOptions (t->oswald).withHeight (height));
}

Font mono (float height, bool bold)
{
    static SharedResourcePointer<Typefaces> t;
    return Font (FontOptions (bold ? t->monoBold : t->monoRegular).withHeight (height));
}

void spaced (Graphics& g, const String& s, Rectangle<float> area, float height, Colour c, Justification j, float tracking, bool bold)
{
    auto f = mono (height, bold);
    f.setExtraKerningFactor (tracking);
    g.setFont (f);
    g.setColour (c);
    g.drawText (s, area, j, false);
}
}

using namespace look;

static void cornerBrackets (Graphics& g, Rectangle<float> r, Colour c, float len, float inset)
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

// ============================================================================
//  Knob
// ============================================================================
void RaveLookAndFeel::drawRotarySlider (Graphics& g, int x, int y, int w, int h, float pos, float a0, float a1, Slider& s)
{
    const auto bounds = Rectangle<float> ((float) x, (float) y, (float) w, (float) h);
    const float R = jmin (bounds.getWidth(), bounds.getHeight()) / 2.0f - 13.0f;
    const auto c = bounds.getCentre();
    const bool stepped = s.getInterval() >= 1.0 && s.getMaximum() <= 3.0;
    const bool bipolar = s.getMinimum() < 0;
    const float ang = a0 + pos * (a1 - a0);

    g.setGradientFill (ColourGradient (violet.withAlpha (0.30f), c.x, c.y, violet.withAlpha (0.0f), c.x + R * 1.3f, c.y, true));
    g.fillEllipse (Rectangle<float> (R * 2.6f, R * 2.6f).withCentre (c));

    Path track; track.addCentredArc (c.x, c.y, R, R, 0, a0, a1, true);
    g.setColour (line); g.strokePath (track, PathStrokeType (5.f, PathStrokeType::curved, PathStrokeType::rounded));
    Path arc;
    if (bipolar) { const float mid = (a0 + a1) / 2; arc.addCentredArc (c.x, c.y, R, R, 0, jmin (mid, ang), jmax (mid, ang), true); }
    else arc.addCentredArc (c.x, c.y, R, R, 0, a0, ang, true);
    g.setColour (lime); g.strokePath (arc, PathStrokeType (5.f, PathStrokeType::curved, PathStrokeType::rounded));

    const float r2 = R - 9.f;
    g.setGradientFill (ColourGradient (Colour (0xffe9e6f2), c.x - r2, c.y - r2, Colour (0xff5d5670), c.x + r2, c.y + r2, false));
    g.fillEllipse (Rectangle<float> (r2 * 2, r2 * 2).withCentre (c));
    const float r3 = r2 - 2.5f;
    g.setGradientFill (ColourGradient (Colour (0xff261a36), c.x, c.y - r3, Colour (0xff0e0716), c.x, c.y + r3, false));
    g.fillEllipse (Rectangle<float> (r3 * 2, r3 * 2).withCentre (c));

    const Point<float> p1 = c.getPointOnCircumference (r3 * 0.35f, ang), p2 = c.getPointOnCircumference (r3 - 5.f, ang);
    g.setColour (lime);
    g.drawLine ({ p1, p2 }, 3.5f);
    g.fillEllipse (Rectangle<float> (5, 5).withCentre (p2));

    if (stepped)
        for (int i = 0; i < 4; ++i)
        {
            const float a = a0 + (float) i / 3.f * (a1 - a0);
            const bool on = i == roundToInt (s.getValue());
            g.setColour (on ? lime : violet);
            const float d = on ? 7.f : 5.f;
            g.fillEllipse (Rectangle<float> (d, d).withCentre (c.getPointOnCircumference (R + 9.f, a)));
        }
}

// ============================================================================
//  Envelope display
// ============================================================================
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

    // selected-card look from Kick Extractor: lime frame, soft glow, corner brackets
    for (int i = 5; i > 0; --i)
    {
        g.setColour (lime.withAlpha (0.03f));
        g.fillRoundedRectangle (b.reduced (6.0f - (float) i), 18.0f + (float) i * 0.5f);
    }
    auto card = b.reduced (6.0f);
    g.setGradientFill (ColourGradient (Colour (0xff1d2a0c), card.getX(), card.getY(), limeDeep, card.getX(), card.getBottom(), false));
    g.fillRoundedRectangle (card, 18.0f);
    g.setColour (Colour (0xff0b0613).withAlpha (0.55f));
    g.fillRoundedRectangle (card.reduced (3.0f), 16.0f);
    g.setColour (lime);
    g.drawRoundedRectangle (card.reduced (1.0f), 18.0f, 2.5f);
    cornerBrackets (g, card, lime, 13.f, 9.f);

    auto area = card.reduced (30.f, 16.f).withTrimmedBottom (14.f);
    const float x0 = area.getX(), w = area.getWidth(), top = area.getY(), h = area.getHeight();
    const float mid = top + h / 2;
    const int beats = jmax (1, proc.beatsPerBar.load());

    for (int i = 0; i <= beats * 4; ++i)
    {
        const float gx = x0 + (float) i / (float) (beats * 4) * w;
        g.setColour (line.withAlpha (i % 4 == 0 ? 1.0f : 0.45f));
        g.drawVerticalLine ((int) gx, top, top + h);
    }
    g.setColour (line); g.drawHorizontalLine ((int) mid, x0, x0 + w);

    const float sx = w / (float) numCols;
    for (int c = 0; c < numCols; ++c)
    {
        const float a = jmin (1.f, inPk[(size_t) c]) * h / 2, o = jmin (1.f, outPk[(size_t) c]) * h / 2;
        if (a > 0.3f) { g.setColour (white.withAlpha (0.14f)); g.fillRect (x0 + (float) c * sx, mid - a, sx + 0.4f, 2 * a); }
        if (o > 0.3f) { g.setColour (lime.withAlpha (0.55f)); g.fillRect (x0 + (float) c * sx, mid - o, sx + 0.4f, 2 * o); }
    }

    const auto shape = proc.currentShape();
    const double mix = proc.currentMix01(), bl = 60.0 / proc.hostBpm.load();
    Path env;
    for (int px = 0; px <= (int) w; ++px)
    {
        double pos = (double) px / w * beats, xx = pos - std::floor (pos);
        if (px == (int) w) xx = 0.99999;
        const float yy = top + h - (float) kickduck::gain (xx, bl, shape, mix) * h;
        if (px == 0) env.startNewSubPath (x0, yy); else env.lineTo (x0 + (float) px, yy);
    }
    g.setColour (white.withAlpha (0.22f));
    g.strokePath (env, PathStrokeType (6.f, PathStrokeType::curved, PathStrokeType::rounded));
    g.setColour (white);
    g.strokePath (env, PathStrokeType (2.f, PathStrokeType::curved, PathStrokeType::rounded));

    const float bp = proc.barPhase.load();
    if (playing && bp >= 0)
    {
        const float hx = x0 + bp * w;
        double bx = bp * beats; bx -= std::floor (bx);
        g.setColour (white.withAlpha (0.7f)); g.drawVerticalLine ((int) hx, top - 4.f, top + h + 4.f);
        g.setColour (lime);
        g.fillEllipse (Rectangle<float> (8, 8).withCentre ({ hx, top + h - (float) kickduck::gain (bx, bl, shape, mix) * h }));
    }

    for (int k = 0; k < beats; ++k)
        spaced (g, "KICK", { x0 + (float) k / (float) beats * w - 2.f, top + h + 3.f, 60.f, 12.f }, 8.5f, grey, Justification::centredLeft, 0.1f, false);
    spaced (g, playing ? "DRAG LEFT / RIGHT = LENGTH" : "PRESS PLAY TO SEE THE WAVEFORM", { x0, top + h + 3.f, w, 12.f }, 8.5f, grey,
            Justification::centredRight, 0.1f, false);
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

// ============================================================================
//  Knob cards (same card language as Kick Extractor's mode buttons)
// ============================================================================
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
    auto r = getLocalBounds().reduced (6);
    slider.setBounds (r.removeFromLeft (r.getHeight()).reduced (2));
}

void KnobCard::paint (Graphics& g)
{
    auto r = getLocalBounds().toFloat().reduced (2.0f);
    const bool hot = slider.isMouseOverOrDragging (true) || isMouseOver (true);

    g.setGradientFill (ColourGradient (plum.brighter (hot ? 0.25f : 0.08f), r.getX(), r.getY(), plum, r.getX(), r.getBottom(), false));
    g.fillRoundedRectangle (r, 18.0f);
    g.setColour (hot ? lime.withAlpha (0.7f) : line);
    g.drawRoundedRectangle (r.reduced (0.5f), 18.0f, hot ? 1.7f : 1.2f);

    String value;
    if (auto* prm = proc.params.getParameter (pid)) value = prm->getCurrentValueAsText();
    const float tx = (float) slider.getRight() + 4.0f, tw = r.getRight() - tx - 12.0f;
    spaced (g, title, { tx, r.getCentreY() - 30.f, tw, 16.f }, 11.f, lavender, Justification::centredLeft, 0.18f, true);
    g.setFont (heading (30.f));
    g.setColour (hot ? lime : white);
    g.drawText (value, Rectangle<float> (tx, r.getCentreY() - 12.f, tw, 38.f), Justification::centredLeft, false);
}

// ============================================================================
//  Editor
// ============================================================================
KickDuckEditor::KickDuckEditor (KickDuckProcessor& p)
    : AudioProcessorEditor (&p), proc (p), display (p),
      mixCard (p, "mix", "MIX", false), lengthCard (p, "length", "LENGTH", false),
      curveCard (p, "curve", "CURVE", false), shapeCard (p, "shape", "SHAPE", true)
{
    setLookAndFeel (&lnf);
    logo = ImageCache::getFromMemory (BinaryData::logo_png, BinaryData::logo_pngSize);

    panel.addAndMakeVisible (display);
    for (auto* c : { &mixCard, &lengthCard, &curveCard, &shapeCard }) panel.addAndMakeVisible (c);
    addAndMakeVisible (panel);

    // resizable exactly like Kick Extractor: fixed aspect, 60..200 %, size remembered
    const int saved = proc.getSavedWidth();
    restoring = true;
    setResizable (true, true);
    setResizeLimits (baseW * 6 / 10, baseH * 6 / 10, baseW * 2, baseH * 2);
    getConstrainer()->setFixedAspectRatio ((double) baseW / baseH);
    const int w = saved > 0 ? jlimit (baseW * 6 / 10, baseW * 2, saved) : baseW;
    setSize (w, roundToInt (w * (double) baseH / baseW));
    restoring = false;

    startTimerHz (60);
}

KickDuckEditor::~KickDuckEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void KickDuckEditor::timerCallback()
{
    display.pullMeter();
    display.repaint();
}

void KickDuckEditor::paint (Graphics& g) { g.fillAll (night); }

void KickDuckEditor::resized()
{
    panel.setBounds (0, 0, baseW, baseH);
    panel.setTransform (AffineTransform::scale ((float) getWidth() / (float) baseW));
    display.setBounds (24, 112, 912, 166);
    int i = 0;
    for (auto* c : { &mixCard, &lengthCard, &curveCard, &shapeCard })
        c->setBounds (30 + (i++) * 230, 288, 210, 112);
    if (! restoring) proc.setSavedWidth (getWidth());
}

void KickDuckEditor::paintPanel (Graphics& g)
{
    const auto area = Rectangle<float> (0, 0, (float) baseW, (float) baseH);

    // background: identical to Kick Extractor
    g.setColour (night);
    g.fillRect (area);
    g.setGradientFill (ColourGradient (Colour (0x3e4d107a), 480.0f, 220.0f, Colour (0x004d107a), 480.0f, 410.0f, true));
    g.fillRect (area);

    // header: same logo, same position and size as Kick Extractor
    if (logo.isValid())
    {
        const float h = 62.0f;
        const float w = h * (float) logo.getWidth() / (float) logo.getHeight();
        g.drawImage (logo, Rectangle<float> (32.0f, 26.0f, w, h), RectanglePlacement::xLeft);
    }

    // product name: lime first word + white second word, Oswald 56, right aligned at 32 px
    g.setFont (heading (56.0f));
    const String first ("INSTANT"), second (" PUMP");
    const float wSecond = GlyphArrangement::getStringWidth (g.getCurrentFont(), second);
    const float wFirst = GlyphArrangement::getStringWidth (g.getCurrentFont(), first);
    const float right = (float) baseW - 32.0f;
    g.setColour (white);
    g.drawText (second, Rectangle<float> (right - wSecond, 24.0f, wSecond, 64.0f), Justification::centredRight, false);
    g.setColour (lime);
    g.drawText (first, Rectangle<float> (right - wSecond - wFirst - 4.0f, 24.0f, wFirst + 4.0f, 64.0f), Justification::centredRight, false);
}
