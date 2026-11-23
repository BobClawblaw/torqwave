#pragma once

#include "SignalConsole.h"

/** The console's drawing, shared by its views (Trace's scope and Decrypt's). */
namespace SignalConsoleDrawing
{
    constexpr float tau = juce::MathConstants<float>::twoPi;

    juce::Font mono (float size, bool bold = false);

    /** Text in the monospaced face, placed by `where` round (x, y). */
    void text (juce::Graphics&, const juce::String&, float x, float y, float size, juce::Colour,
               juce::Justification where = juce::Justification::centredLeft, bool bold = false);

    /** A sound's signature: a spoke for each band of its spectrum round a ring, longer where it is stronger. */
    void drawSignature (juce::Graphics&, juce::Point<float> centre, float radius, const SignalConsole::Signature&, juce::Colour, float alpha, bool glow);

    /** A chip: a label in a rounded box, tinted when it is on. */
    void drawChip (juce::Graphics&, juce::Rectangle<float>, const juce::String& label, juce::Colour, bool filled, juce::Colour labelColour);

    /** The angle of a part's sector's middle: oscillators at twelve o'clock, then clockwise. */
    float sectorMiddle (int part);
}

/** The signal console's look, Trace: a scope round the node held. Five sectors, one for each part
    of a patch (oscillators, filter, envelopes, modulation, effects); each of the twelve nodes
    within reach sits in the sector of the part that changed most, as far from the middle as it is
    from the node held. Every node is drawn as its sound's signature (a ring of 48 spokes, each a
    band of its spectrum) with its address under it. A firewalled sector is hatched and closed by
    an arc. A line sweeps the scope slowly; it is the only thing that moves at rest, and it stays
    still when the system asks for less motion.

    Click a node to hear it, double-click it to jump to it. The panel at the right shows the node
    under the pointer: its signature, address, the part that changed, the settings that moved most,
    Hear and Jump, and the route. Under the scope: a firewall for each part, Reach, and Rescan. */
class SignalConsoleView final : public juce::Component,
                                public juce::SettableTooltipClient,
                                private juce::Timer
{
public:
    explicit SignalConsoleView (SignalConsole::Synth&);
    ~SignalConsoleView() override;

    void paint (juce::Graphics&) override;
    void resized() override;
    void mouseMove (const juce::MouseEvent&) override;
    void mouseExit (const juce::MouseEvent&) override;
    void mouseDown (const juce::MouseEvent&) override;
    void mouseDrag (const juce::MouseEvent&) override;
    void mouseDoubleClick (const juce::MouseEvent&) override;
    void visibilityChanged() override;
    juce::String getTooltip() override;

    /** For tests: whether anything moves (off under Reduce Motion, or when told), where a node is
        drawn (0 to 11, -1 the node held), and the node the panel shows (-1 the node held). */
    void setMotion (bool allowed)                                 { motion = allowed; }
    bool isMoving() const noexcept                                { return motion && moving; }
    juce::Point<float> nodeCentre (int node) const;
    int shownNode() const noexcept                                { return hovered >= 0 ? hovered : selected; }
    /** The firewall chip's, Reach's, Rescan's, Hear's and Jump's places, and a route entry's. */
    juce::Rectangle<float> firewallBounds (int part) const        { return firewallChips[(size_t) part]; }
    juce::Rectangle<float> reachBounds() const                    { return reachArea; }
    juce::Rectangle<float> rescanBounds() const                   { return rescanChip; }
    juce::Rectangle<float> hearBounds() const                     { return hearChip; }
    juce::Rectangle<float> jumpBounds() const                     { return jumpChip; }
    juce::Rectangle<float> routeBounds (int entry) const;

private:
    void timerCallback() override;
    void layoutNodes();
    int nodeAt (juce::Point<float>) const;
    void drawScope (juce::Graphics&);
    void drawPanel (juce::Graphics&);
    void drawControls (juce::Graphics&);

    SignalConsole::Console& console;
    juce::Point<float> centre;
    float radius = 100.0f;
    juce::Rectangle<float> panel, reachArea, rescanChip, hearChip, jumpChip;
    std::array<juce::Rectangle<float>, SignalConsole::numParts> firewallChips;
    std::vector<juce::Point<float>> places;                        // the twelve, where they are drawn
    std::vector<juce::Rectangle<float>> routeRows;
    std::vector<int> routeEntries;                                 // the route entry each row shows
    int hovered = -2, selected = -1;                               // -2 nothing under the pointer, -1 the node held
    float sweep = 0.0f, reachAtDragStart = 0.0f;
    bool draggingReach = false, motion = true, moving = false;
    juce::uint32 heardAt = 0, jumpedAt = 0, lastSync = 0;
    int heard = -2;
    juce::Point<float> jumpedFrom;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SignalConsoleView)
};
