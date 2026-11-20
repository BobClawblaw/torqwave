#pragma once

#include "AudioEngine.h"
#include "ClipEffects.h"

/** A clip's effects (Clip Effects...): its pitch shift and its chain, each change an undo step
    of the session. A row a slot: its name, Edit (its window), On (bypass), Up and Down, and Remove.
    Add Effect... offers the effects (not instruments) known to the app.

    An effect's window works on an instance of its own: what is changed there goes into the chain
    (as the slot's state) once it rests for a moment, and when the window closes; the clip is then
    rendered again in the background. The status line says whether the render is there yet. */
class ClipEffectsPanel final : public juce::Component,
                               private juce::ChangeListener,
                               private juce::Timer
{
public:
    ClipEffectsPanel (AudioEngine&, juce::uint32 clipId);
    ~ClipEffectsPanel() override;

    juce::uint32 getClipId() const noexcept     { return clipId; }

    /** The changes the panel makes, each an undo step (false when nothing changed). */
    bool setSemitones (double);
    bool addEffect (const juce::PluginDescription&);
    bool removeEffect (int slot);
    bool setBypassed (int slot, bool);
    bool moveEffect (int from, int to);

    /** Opens a slot's window (asynchronously: the plug-in is made first). */
    void editEffect (int slot);

    /** The rows shown, as text ("Reverb (off)"), and the status line. */
    juce::StringArray describeRows() const;
    juce::String getStatus() const              { return status.getText(); }

    /** The buttons of a row: 0 Edit, 1 On, 2 Up, 3 Down, 4 Remove. */
    juce::Button* rowButton (int slot, int which);

    void resized() override;
    void paint (juce::Graphics&) override;

    static constexpr int rowHeight = 28;

private:
    class EffectWindow;

    struct Row
    {
        juce::Label name;
        juce::TextButton edit { "Edit" }, up { "Up" }, down { "Down" }, remove { "Remove" };
        juce::ToggleButton on { "On" };
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override;
    const SessionClip* clip() const;                             // as last shown
    const SessionClip* clipIn (const SessionState&) const;
    bool apply (const juce::String& description, double semitones, const juce::String& chain, bool merge = false);
    void refresh();
    void showAddMenu();
    void storeWindowState();

    AudioEngine& engine;
    const juce::uint32 clipId;
    std::shared_ptr<const SessionState> state;

    juce::Label title, pitchLabel { {}, "Pitch (semitones)" }, status;
    juce::Slider pitch;
    juce::TextButton add { "Add Effect..." };
    std::vector<std::unique_ptr<Row>> rows;

    std::unique_ptr<EffectWindow> window;   // the slot being edited, if any
    int windowSlot = -1;
    juce::MemoryBlock windowState;          // what the chain last took from it

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClipEffectsPanel)
};
