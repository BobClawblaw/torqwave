#pragma once

#include "AudioEngine.h"

/** The effect rack: one row per plug-in slot, processed top to bottom. */
class RackView final : public juce::Component,
                       private juce::ChangeListener
{
public:
    explicit RackView (AudioEngine& engine);
    ~RackView() override;

    std::function<void (int slotIndex)> onEditSlot;

    /** In a session: asked to show a master effect's window. */
    std::function<void (juce::AudioPluginInstance&)> onEditPlugin;

    /** Asked by the Apply button: render the rack into the audio, as Effects > Apply Effects Rack does. */
    std::function<void()> onApply;

    /** The Apply button, on while the rack holds an effect. */
    juce::Button& getApplyButton()                 { return applyButton; }

    /** Adds an effect at the end of the rack (as + Add Effect does) and, once it is in, asks for
        its window through onEditSlot, so a new effect is in front without a click on Edit. A
        failure to load is shown. */
    void addEffect (const juce::PluginDescription&);

    /** In a session the panel shows the master bus's effects: the Master track's chain, which the
        whole mix goes through once, after the tracks and buses and before the master volume (the
        editor's rack is left out of a session's playback). It is named for that, and its Apply
        button (which renders into the open file) is hidden. */
    void setMasterBus (bool);

    /** The name over the rows: "Effects Rack", or "Master Bus Effects" in a session. */
    juce::String getHeading() const               { return title.getText(); }

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class SlotRow;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuildRows();
    void showAddMenu();

    // The chain shown: the session's master track's in a session that has one, else the rack.
    juce::uint32 masterId() const;
    int numSlots() const;
    juce::AudioPluginInstance* pluginAt (int) const;
    bool isBypassed (int) const;
    void setBypassed (int, bool);
    void move (int from, int to);
    void remove (int);
    void edit (int);

    AudioEngine& engine;
    juce::Label title { {}, "Effects Rack" };
    juce::TextButton addButton { "+ Add Effect" };
    juce::TextButton applyButton { "Apply" };
    juce::OwnedArray<SlotRow> rows;
    std::vector<std::pair<const void*, bool>> shown;
    bool masterBus = false;   // each row's plug-in and whether it is on

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackView)
};
