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

    /** Asked by the Apply button: render the rack into the audio, as Effects > Apply Effects Rack does. */
    std::function<void()> onApply;

    /** The Apply button, on while the rack holds an effect. */
    juce::Button& getApplyButton()                 { return applyButton; }

    /** Adds an effect at the end of the rack (as + Add Effect does) and, once it is in, asks for
        its window through onEditSlot, so a new effect is in front without a click on Edit. A
        failure to load is shown. */
    void addEffect (const juce::PluginDescription&);

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    class SlotRow;

    void changeListenerCallback (juce::ChangeBroadcaster*) override;
    void rebuildRows();
    void showAddMenu();

    AudioEngine& engine;
    juce::Label title { {}, "Effects Rack" };
    juce::TextButton addButton { "+ Add Effect" };
    juce::TextButton applyButton { "Apply" };
    juce::OwnedArray<SlotRow> rows;
    std::vector<std::pair<const void*, bool>> shown;   // each row's plug-in and whether it is on

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (RackView)
};
