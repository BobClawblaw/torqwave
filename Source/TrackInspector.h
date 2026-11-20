#pragma once

#include "AudioEngine.h"

/** The selected track at a glance, as Cubase's inspector: its name (type a new one), what it is,
    its volume, pan, mute and solo, where it plays (the master or a bus), its instrument, its
    inserts and its sends with their levels and pre-fader switches. Changes are undo steps as in
    the mixer (a slider's drag one step) and follow the track's group. A click on the instrument
    or an insert opens its window; effects are added and ordered in the Effects Rack.

    Follows the session and the selection, so undo and edits elsewhere show here at once. */
class TrackInspector final : public juce::Component,
                             private juce::ChangeListener,
                             private juce::Timer
{
public:
    /** trackToShow gives the selected track's id (0 for none); asked a few times a second. */
    TrackInspector (AudioEngine&, std::function<juce::uint32()> trackToShow);
    ~TrackInspector() override;

    /** Opens a plug-in's window (the instrument's or an insert's). */
    std::function<void (juce::AudioProcessor&)> showEditor;

    juce::uint32 getShownTrack() const noexcept     { return shownTrack; }

    // The controls, for typing into and clicking (as the tests do).
    juce::TextEditor& getNameEditor()               { return name; }
    juce::Slider& getVolume()                       { return volume; }
    juce::Slider& getPan()                          { return pan; }
    juce::ToggleButton& getMute()                   { return mute; }
    juce::ToggleButton& getSolo()                   { return solo; }
    juce::ComboBox& getOutput()                     { return output; }
    int getNumInserts() const noexcept              { return inserts.size(); }
    juce::TextButton& getInsert (int i)             { return *inserts[i]; }
    juce::TextButton& getInstrumentButton()         { return instrument; }
    int getNumSends() const noexcept                { return (int) sends.size(); }
    juce::Slider& getSendLevel (int i)              { return sends[(size_t) i]->level; }
    juce::ToggleButton& getSendPre (int i)          { return sends[(size_t) i]->pre; }

    /** What the panel says beside the track's name: "Audio track", "Instrument track", "Bus", "Folder". */
    juce::String getKindText() const                { return kind.getText(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    struct Send
    {
        juce::uint32 busId = 0;
        juce::Label name;
        juce::Slider level;
        juce::ToggleButton pre { "Pre" };
    };

    void changeListenerCallback (juce::ChangeBroadcaster*) override  { refresh(); }
    void timerCallback() override                                   { refresh(); }

    /** Shows the selected track; rebuilds the insert and send rows when they have changed. */
    void refresh();

    /** Changes the shown track (and its group) as one undo step; merged with the last for a drag. */
    void edit (const juce::String& description, bool merge, const std::function<void (SessionTrack&)>& change);

    /** What the insert and send rows were built for: the plug-ins' names and states, the sends' buses. */
    juce::String layoutKey (const SessionState&) const;

    AudioEngine& engine;
    std::function<juce::uint32()> trackToShow;
    juce::uint32 shownTrack = 0;
    juce::String shownLayout;

    juce::Label empty { {}, "No track selected" }, kind;
    juce::TextEditor name;
    juce::Label volumeLabel { {}, "Volume" }, panLabel { {}, "Pan" }, outputLabel { {}, "Output" };
    juce::Slider volume, pan;
    juce::ToggleButton mute { "Mute" }, solo { "Solo" };
    juce::ComboBox output;
    std::vector<juce::uint32> outputIds;   // the bus each output item plays into (0: the master)
    juce::Label instrumentLabel { {}, "Instrument" }, insertsLabel { {}, "Inserts" }, sendsLabel { {}, "Sends" };
    juce::TextButton instrument;
    juce::OwnedArray<juce::TextButton> inserts;
    juce::Label noInserts { {}, "None (add them in the Effects Rack)" }, noSends { {}, "None" };
    std::vector<std::unique_ptr<Send>> sends;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (TrackInspector)
};
