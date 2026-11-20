#pragma once

#include "AudioEngine.h"
#include "TrackTemplate.h"

#include <optional>

class MultitrackView;
class SurroundPad;

/** The session as a mixing desk: a strip for each track and bus, in the session's order, with its
    meter, volume fader, pan, mute, polarity switch and solo, and a master strip metering what goes
    to the outputs. In a 5.1 master each strip is wider, with a surround pad, width and LFE beside its fader.

    The controls edit the session as the track headers do (each change an undo step, a fader's
    drag one step); an automated volume or pan follows its envelope and can't be moved here.
    The surround pad, width and LFE likewise follow their envelopes, and with automation being
    written (setAutomationWriter) they stay live and record into them while the session plays.
    Right-click a strip to make it solo safe, change its colour, or save or apply a channel-strip
    preset (ChannelStrips, TrackTemplate::apply).

    A column at the left stores mixer scenes (MixScene): Store Scene keeps the mix as a new scene,
    and a button for each scene, listed under it, recalls it, each an undo step; right-click a
    scene's button to store the mix into it again, rename it or delete it. The column takes width,
    not height, so the strips keep their full height in the Multitrack workspace.
    Meters show peaks after each track's volume and pan, falling back over about half a second. */
class MixerView final : public juce::Component,
                        private juce::ChangeListener,
                        private juce::Timer
{
public:
    explicit MixerView (AudioEngine&);
    ~MixerView() override;

    static constexpr int stripWidth = 84;
    static constexpr int surroundStripWidth = 132;   // a strip's in a 5.1 master, with its surround pad beside the fader

    /** Strips for tracks and buses (the master strip not counted). */
    int getNumStrips() const noexcept               { return (int) strips.size(); }

    /** A strip's controls, for the track or bus with this id (null when there is none). */
    juce::Slider* findFader (juce::uint32 trackId) const;
    juce::Label* findReadout (juce::uint32 trackId) const;

    /** A volume typed into a strip's readout: a number from -60 to +12, with or without "dB", rounded to
        0.1 dB as the fader sets it. Anything else gives nothing. */
    static std::optional<float> parseVolume (const juce::String&);
    juce::Slider* findPan (juce::uint32 trackId) const;

    /** A strip's surround controls, shown in a 5.1 master: the point, the width (0 to 100) and
        the LFE (-61 for off, then -60 to +10 dB). */
    SurroundPad* findSurroundPad (juce::uint32 trackId) const;
    juce::Slider* findSurroundWidth (juce::uint32 trackId) const;
    juce::Slider* findLfe (juce::uint32 trackId) const;
    juce::Button* findMute (juce::uint32 trackId) const;
    juce::Button* findSolo (juce::uint32 trackId) const;
    juce::Button* findPolarity (juce::uint32 trackId) const;

    /** The menu a strip shows when right-clicked: Solo Safe and a Colour submenu (each an undo
        step), the channel-strip items (addChannelStripItems) and MIDI learn's; empty for an
        unknown id. */
    juce::PopupMenu getStripMenu (juce::uint32 trackId) const;

    /** The scene column: Store Scene, and a button that recalls each scene (null for none). */
    juce::Button* findStoreSceneButton() const noexcept   { return storeScene.get(); }
    juce::Button* findSceneButton (juce::uint32 sceneId) const;

    /** A scene button's right-click menu: Update Scene, Rename Scene... and Delete Scene; empty
        for an unknown id. */
    juce::PopupMenu getSceneMenu (juce::uint32 sceneId);

    /** Renames a scene, as an undo step; false when nothing changed. */
    bool renameScene (juce::uint32 sceneId, const juce::String& name);

    /** Where the strip menus save channel-strip presets and find them (the user's, as made). */
    void setChannelStrips (ChannelStrips::Store& store)   { channelStrips = &store; }

    /** Saves a track's or bus's channel strip (TrackTemplate::create, with its effects) as a preset
        under a name; false when it can't be written. */
    bool saveChannelStrip (juce::uint32 trackId, const juce::String& name);

    /** Adds a strip menu's channel-strip items for a track: Save Channel Strip... (asking for a
        name), and an Apply Channel Strip submenu of the saved presets and Other File.... Failures
        are shown in a message box. */
    static void addChannelStripItems (juce::PopupMenu&, AudioEngine&, ChannelStrips::Store&, juce::uint32 trackId);

    /** Asks for a name and saves the track's channel strip under it. */
    static void askToSaveChannelStrip (AudioEngine&, ChannelStrips::Store&, juce::uint32 trackId);

    /** Applies a preset to a track (AudioEngine::applyChannelStrip), saying in a message box what
        could not be applied. */
    static void applyChannelStrip (AudioEngine&, juce::uint32 trackId, const juce::XmlElement&, const juce::String& presetName);

    /** Adds MIDI learn's items for a track to its strip's menu (the app's MidiLearn). */
    std::function<void (juce::PopupMenu&, juce::uint32 trackId)> addMidiLearnMenu;

    /** The level a meter shows (gain, not dB): a track's or bus's, or with id 0 the master's
        (channels 0 to 5, L R C LFE Ls Rs, in a 5.1 master). */
    float getShownLevel (juce::uint32 trackId, int channel) const;

    /** How many channels the master's meter shows: 2, or 6 in a 5.1 master. */
    int getMasterMeterChannels() const;

    /** Takes the latest peaks into the meters (the view's timer does this 30 times a second). */
    void updateMeters();

    /** The multitrack view whose automation writing the surround controls follow: with writing on
        and the session playing, moving them records a pass into their envelopes (one undo step,
        "Write Surround Automation"). nullptr (as made) for none. */
    void setAutomationWriter (MultitrackView*);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class Meter;
    class Strip;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void timerCallback() override                   { updateMeters(); }
    void rebuild();
    void rebuildScenes (const SessionState&);
    Strip* findStrip (juce::uint32 trackId) const;

    AudioEngine& engine;
    juce::Viewport viewport;
    juce::Component content;
    std::vector<std::unique_ptr<Strip>> strips;
    std::unique_ptr<Strip> master;
    std::vector<std::pair<juce::uint32, bool>> shownTracks;   // id and bus-ness of each strip, to see when to rebuild
    int shownChannels = 2;                                    // the master's channels the strips are laid out for (wider past 2)
    MultitrackView* writer = nullptr;
    ChannelStrips::Store* channelStrips = &ChannelStrips::userStore();

    // The scene column: Store Scene, and the scenes' buttons in a list that scrolls.
    static constexpr int sceneColumnWidth = 104;
    static constexpr int sceneButtonHeight = 24;
    std::unique_ptr<juce::TextButton> storeScene;
    juce::Component sceneList;                                // declared first: the viewport shows it
    juce::Viewport sceneViewport;
    std::vector<std::pair<juce::uint32, std::unique_ptr<juce::TextButton>>> sceneButtons;
    std::vector<std::pair<juce::uint32, juce::String>> shownScenes;
    bool shownWriting = false;                                // the controls set up for writing automation

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MixerView)
};
