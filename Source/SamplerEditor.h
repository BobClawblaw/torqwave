#pragma once

#include "Sampler.h"
#include "SoundControls.h"
#include "EffectPresets.h"

/** Torqwave Sampler's window, laid out as the sound flows. Along the top, Keys or Slices and the
    presets row. Under it the waveform of the selected zone's sample with handles for the part
    played, the loop and its crossfade, and, playing slices, the cuts between them; a line shows
    where each sounding voice is. Then the key map: the zones over the keys, each across the
    velocities that play it, with its root marked, or a pad for each slice. Then the selected
    zone's settings, and the sound's: the amp envelope, the filter and its envelope, the LFO and
    the voice settings. A sound dropped on it (or chosen by a double-click on the empty waveform)
    becomes the selected zone's sample, or the first zone, or the slices. */
class SamplerEditor final : public juce::AudioProcessorEditor,
                            public juce::FileDragAndDropTarget,
                            public juce::DragAndDropTarget,
                            private juce::Timer
{
public:
    explicit SamplerEditor (BuiltInEffects::SamplerProcessor&);
    ~SamplerEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

    bool isInterestedInFileDrag (const juce::StringArray& files) override;
    void filesDropped (const juce::StringArray& files, int x, int y) override;

    /** An open file dragged from the Files panel (SoundDrops). */
    bool isInterestedInDragSource (const SourceDetails&) override;
    void itemDropped (const SourceDetails&) override;

    /** Gives it several files at once, as dropping them together does. */
    bool loadFiles (const juce::Array<juce::File>&, int atKey = -1);

    static constexpr int windowWidth = 940, windowHeight = 746;

    //==============================================================================
    /** The waveform, with its handles. */
    class Wave final : public juce::Component
    {
    public:
        explicit Wave (SamplerEditor&);

        enum class Handle { none, start, end, loopStart, loopEnd, crossfade, cut };

        /** The sample it shows: a new one is shown whole. */
        void setSample (const Sampler::SamplePtr&);

        /** The frames of the sample shown across its width. */
        juce::Range<double> getView() const noexcept                 { return view; }
        void setView (juce::Range<double>);
        void showAll();

        float xOf (juce::int64 frame) const;
        juce::int64 frameAt (float x) const;

        /** The handle under a point (within 5 pixels), and for a cut, which (1 is between the first
            two slices). */
        Handle handleAt (juce::Point<float>, int& cut) const;

        /** What dragging a handle to a frame does: the part's ends, the loop's ends (each kept in
            order and inside the part), the crossfade (the frames from where it is dragged to the
            loop's end) or a cut between two slices. */
        void dragTo (Handle, int cut, juce::int64 frame);

        /** Playing slices: a cut added at a frame (the slice there becomes two, the keys after it
            moving up), and a cut taken away (its two slices becoming one). False when it can't. */
        bool addCut (juce::int64 frame);
        bool removeCut (int cut);

        /** The frames of the sample shown where a line is drawn for a sounding voice: the frame
            each reads next. */
        std::vector<juce::int64> playingFrames() const;

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;
        void mouseMove (const juce::MouseEvent&) override;
        void mouseDoubleClick (const juce::MouseEvent&) override;
        void mouseWheelMove (const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
        void mouseMagnify (const juce::MouseEvent&, float scale) override;

    private:
        void zoom (double by, float aroundX);

        SamplerEditor& editor;
        juce::Range<double> view;
        Sampler::SamplePtr shown;                    // the sample the view and the peaks are for
        std::vector<float> lows, highs;              // its lowest and highest over every 256 frames
        Handle dragging = Handle::none;
        int draggingCut = 0;
    };

    /** The zones over the keys, or the slices' pads. */
    class KeyMap final : public juce::Component
    {
    public:
        explicit KeyMap (SamplerEditor&);

        enum class Edge { none, low, high, top, bottom };

        /** Where a zone is drawn (its keys across, its velocities up), and a slice's pad. */
        juce::Rectangle<float> zoneBounds (int zone) const;
        juce::Rectangle<float> padBounds (int slice) const;

        int keyAt (float x) const;
        int velocityAt (float y) const;

        /** The zone at a point (the selected one first, where zones overlap), or -1. */
        int zoneAt (juce::Point<float>) const;

        /** What dragging a zone's edge does: its lowest or highest key, or its highest or lowest
            velocity, kept in order. */
        void dragEdge (int zone, Edge, int value);

        /** Whether a slice's pad is lit (a voice is playing it). */
        bool isLit (int slice) const;

        void paint (juce::Graphics&) override;
        void mouseDown (const juce::MouseEvent&) override;
        void mouseDrag (const juce::MouseEvent&) override;
        void mouseUp (const juce::MouseEvent&) override;

    private:
        juce::Rectangle<float> keysArea() const;
        juce::Rectangle<float> zonesArea() const;

        SamplerEditor& editor;
        Edge dragging = Edge::none;
        int draggingZone = -1, playing = -1;
    };

    //==============================================================================
    // For the tests, and for the window's own parts.

    /** A knob by its name ("Cutoff", "Root"), or nullptr. */
    SoundControls::Knob* knob (const juce::String& name);

    /** The zones as the window last read them, and the one selected (an index into them; -1, none). */
    const std::vector<Sampler::Zone>& getZones() const noexcept     { return zoneList; }
    int getSelected() const noexcept                                 { return selectedZone; }
    void select (int zone);

    /** Whether it shows slices rather than zones across the keys. */
    bool showsSlices() const                                         { return sampler.getSlicing().on; }

    Wave& getWave() noexcept                                         { return *wave; }
    KeyMap& getKeyMap() noexcept                                     { return *keyMap; }

    /** The line under the title: the selected zone's sample, or what is wrong. */
    juce::String getStatus() const                                   { return status; }

    /** Reads the instrument again now, as its timer does. */
    void refreshNow()                                                { timerCallback(); }

    /** Gives it a sound from a file, as a drop does. False, with the reason in its status, when it can't. */
    bool loadFile (const juce::File&, int atKey = -1);

    /** Changes the selected zone and gives the zones back to the instrument. */
    void changeSelected (const std::function<void (Sampler::Zone&)>&);

    /** Gives changed zones to the instrument, keeping the selection. */
    void setZones (std::vector<Sampler::Zone>);

    /** The buttons, as a click does. */
    void findPitch();
    void findLoop();
    void removeZone();
    void sliceAgain();
    void setSlices (bool on);

    BuiltInEffects::SamplerProcessor& getSampler() noexcept          { return sampler; }

private:
    void timerCallback() override;
    void readZones();
    void showZone();
    void say (const juce::String&);
    void chooseFile();
    void locate();
    std::vector<SoundControls::Knob*> allKnobs();

    BuiltInEffects::SamplerProcessor& sampler;
    std::vector<Sampler::Zone> zoneList;
    int selectedZone = -1;
    juce::uint32 seenVersion = 0;
    bool showing = false;                // setting the zone's controls from the zone, not from the user
    juce::String status;

    SoundControls::Panel wavePanel { "Sample" }, keysPanel { "Keys" }, zonePanel { "Zone" }, ampPanel { "Amp" }, filterPanel { "Filter" },
                         lfoPanel { "LFO" }, voicePanel { "Voice" };
    std::unique_ptr<Wave> wave;
    std::unique_ptr<KeyMap> keyMap;

    SoundControls::StepSelector mode, loopMode, choke, sliceSource, sliceParts, filterType, lfoShape, lfoSync, voices;
    SoundControls::Knob root { "Root" }, tune { "Tune" }, zoneLevel { "Zone Level" }, zonePan { "Zone Pan" }, crossfade { "Crossfade" },
                        sensitivity { "Sensitivity" };
    SoundControls::LedToggle reverse { "Reverse" }, oneShot { "One-Shot" }, loopInRelease { "Loop in Release" }, cutEachOther { "Cut Each Other" };
    juce::TextButton open { "Open..." }, pitchButton { "Find Pitch" }, loopButton { "Find Loop" }, locateButton { "Locate..." },
                     removeButton { "Remove Zone" }, sliceButton { "Slice Again" };

    using Small = SoundControls::Knob::Size;
    SoundControls::Knob attack { "Attack", Small::small }, decay { "Decay", Small::small }, sustain { "Sustain", Small::small },
                        release { "Release", Small::small }, velocity { "Velocity", Small::small };
    SoundControls::Knob cutoff { "Cutoff", Small::small }, resonance { "Reso", Small::small }, keyTrack { "Key", Small::small },
                        filterEnv { "Env", Small::small }, filterAttack { "F Attack", Small::small }, filterDecay { "F Decay", Small::small },
                        filterSustain { "F Sustain", Small::small }, filterRelease { "F Release", Small::small }, velocityToCutoff { "Vel", Small::small };
    SoundControls::Knob lfoRate { "Rate", Small::small }, lfoFade { "Fade In", Small::small }, lfoPitch { "To Pitch", Small::small },
                        lfoFilter { "To Filter", Small::small }, lfoLevel { "To Level", Small::small }, lfoPan { "To Pan", Small::small };
    SoundControls::Knob level { "Level", Small::small }, pan { "Pan", Small::small }, transpose { "Transpose", Small::small },
                        fine { "Fine", Small::small }, bendRange { "Bend", Small::small }, glide { "Glide", Small::small },
                        wheel { "Wheel", Small::small };

    juce::Label statusLine;
    std::unique_ptr<juce::FileChooser> chooser;
    EffectPresets::Bar presets;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SamplerEditor)
};
