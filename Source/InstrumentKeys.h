#pragma once

#include <JuceHeader.h>

namespace EffectPresets { class Bar; }

/** Playing an instrument from its own window: the computer's keys as a piano, a strip of keys to
    click, and the next and previous sound on the arrow keys, so a sound can be heard and changed
    without leaving the window or reaching for a MIDI keyboard. */
namespace InstrumentKeys
{
    /** Which of the computer's keys play. Q row, as Cubase's on-screen keyboard has it: Q W E R T Y
        U I O P the white keys from C, the digits above them (2 3 5 6 7 9 0) the black. A row, as
        Logic and Live have it: A S D F G H J K L ; ' the white keys, W E T Y U O P the black. */
    enum class Layout { qRow, aRow };

    Layout getLayout();
    void setLayout (Layout);
    juce::String keyOf (Layout);              // "q" or "a", for the settings
    Layout layoutOf (const juce::String&);

    /** Called when the layout is changed from a strip, for the choice to be kept. */
    extern std::function<void()> layoutChanged;

    /** The keys that play in the layout chosen, in order from C. */
    juce::String playingKeys();

    /** The note a typed key plays at an octave (octave 3: the first key is C3, 48; middle C is C4,
        60), or -1 for a key that plays nothing or a note out of range. Upper and lower case are
        the same key. */
    int noteFor (juce::juce_wchar key, int octave);

    /** The next (by 1) or previous (by -1) sound of an instrument, loaded: Torqwave Synth's factory
        patch after the one it is on (round the ends of the list; through its favourites alone
        while its browser is on them, as its own arrows go), any other instrument's preset
        through the presets row in its window (`editor`). The name of what was loaded; empty if
        the instrument has nothing to step through. */
    juce::String stepSound (juce::AudioProcessor&, juce::Component* editor, int by);

    /** Marks the sound an instrument is on as a favourite, or unmarks it: Torqwave Synth's factory
        patch. True if it is a favourite now; nothing happens (false) for an instrument without
        favourites or a sound that is no factory patch. `handled` says whether there was one to mark. */
    bool toggleFavourite (juce::AudioProcessor&, bool* handled = nullptr);

    /** The presets row in an instrument's window, if it has one. */
    EffectPresets::Bar* presetsRow (juce::Component& editor);

    /** Audition: with it on, a sound chosen in an instrument's window (the arrows, the list, the
        arrow keys) is played at once, by a short phrase that suits it. One switch for every
        instrument, kept in the settings. */
    bool getAudition();
    void setAudition (bool);

    /** Loop: with it on, an audition phrase goes round until Stop is pressed, the sound is changed
        (the new sound's phrase takes over), or Audition or Loop is turned off. Kept in the settings. */
    bool getAuditionLoop();
    void setAuditionLoop (bool);
    extern std::function<void()> auditionChanged;

    /** A note of an audition phrase: when it starts and how long it is held, in milliseconds. */
    struct PhraseNote
    {
        int note = 60;
        double startMs = 0.0, lengthMs = 300.0;
        float velocity = 0.8f;
    };

    /** The kinds of sound that have their own audition patterns. Torqwave Synth's patches by their
        category: basses (bass, sub, reese, acid), chords (pads, strings, organs, chords, textures,
        vox, cinematic), stabs (and brass), leads (and plucks, bells, keys, chip, anything else),
        held sounds (sequences, arps, drones, risers: they need time to move) and hits (perc, drum,
        impact, FX, glitch, game). Basic Drums is a kit, StringSwarm strings; any other instrument other. */
    enum class Kind { bass, chord, stab, lead, held, hit, strings, kit, other };
    Kind kindFor (juce::AudioProcessor&);

    /** An audition pattern: its name in the menu, its notes, and how long a round is when it loops
        (0: the phrase and a breath, to the next whole beat). */
    struct Pattern
    {
        juce::String name;
        std::vector<PhraseNote> notes;
        double lengthMs = 0.0;
    };

    /** The patterns a kind of sound can be auditioned by, at 125 BPM and written for the keys at
        octave 3 (AuditionPatterns.cpp): a dozen for basses, chords, stabs, leads and kits, eight for
        held sounds, hits and strings. The first of each is the phrase it had before there was a choice. */
    const std::vector<Pattern>& patternsFor (Kind);

    /** The pattern chosen for a kind (an index into patternsFor), and choosing one: one choice per
        kind for every window, kept in the settings as `getPatternChoices`' text. */
    int getPattern (Kind);
    void setPattern (Kind, int index);
    juce::String getPatternChoices();
    void setPatternChoices (const juce::String&);

    /** The pattern a sound is auditioned by now: its kind's chosen one. */
    const Pattern& patternFor (juce::AudioProcessor&);

    /** Its notes. */
    std::vector<PhraseNote> phraseFor (juce::AudioProcessor&);

    /** Plays a phrase over time: `advance` moves it on by so many milliseconds, sending each note's
        start and end to `on` and `off` as they come due. Starting a phrase ends the one playing. */
    class Auditioner
    {
    public:
        std::function<void (int note, float velocity)> on;
        std::function<void (int note)> off;

        /** `loop`: the phrase goes round until stopped, each round as long as the phrase and a
            breath after it, to the next whole beat at 125 BPM (two beats at least), or `roundMs`
            when it is given (a pattern of whole bars). */
        void start (std::vector<PhraseNote>, bool loop = false, double roundMs = 0.0);
        void stop();
        bool isLooping() const noexcept         { return looping && ! phrase.empty(); }
        double getLoopLengthMs() const noexcept { return loopLength; }
        void advance (double milliseconds);
        bool isPlaying() const noexcept         { return ! phrase.empty(); }

    private:
        struct Playing
        {
            PhraseNote note;
            bool started = false, ended = false;
        };

        std::vector<Playing> phrase;
        double at = 0.0, loopLength = 0.0;
        bool looping = false;
    };

    /** The strip along the bottom of an instrument's window: five octaves of keys to click, what
        the computer's keys play shown on them, and the octave and velocity they play at.
        Z and X move the octave down and up, C and V the velocity. Notes go to `send`; `aboutToPlay`
        is called before a note is sent (so the window's track can be made the one that sounds).
        A button on it changes which keys play (Q row or A row), for every strip. */
    class Strip final : public juce::Component,
                        private juce::MidiKeyboardState::Listener,
                        private juce::Timer
    {
    public:
        explicit Strip (std::function<void (const juce::MidiMessage&)> send);
        ~Strip() override;

        static constexpr int height = 64, lowestOctave = 0, highestOctave = 7, maxKeys = 18;

        std::function<void()> aboutToPlay;

        /** A key going down or coming up, as the window hears it. True if it is one of the strip's
            (a playing key, or Z, X, C or V going down). */
        bool keyDown (juce::juce_wchar key);
        bool keyUp (juce::juce_wchar key);

        /** Reads which playing keys are held from the keyboard itself: for a window's
            keyStateChanged. With `enabled` false (the window in the background, a number being
            typed) every key counts as up. True if a playing key is down. */
        bool readKeys (bool enabled);

        /** Lets go of every note: the window going to the background. */
        void allUp();

        /** Plays a phrase on the strip's keys (they light as it goes): a sound just chosen. Ends
            the one before, and is ended by a key played by hand. It is written for octave 3 and
            plays as many octaves up or down as the keys are set from it (Z and X), moving with them
            while it plays; `followOctave` false keeps it where it is (a drum kit's notes). */
        void audition (std::vector<PhraseNote>, bool followOctave = true, double roundMs = 0.0);
        void stopAudition();
        bool isAuditioning() const noexcept     { return auditioner.isPlaying(); }

        /** The keys' state: what is played into it (noteOn, noteOff) is sent on like a key's note. */
        juce::MidiKeyboardState& getState() noexcept   { return state; }

        int getOctave() const noexcept          { return octave; }
        int getVelocity() const noexcept        { return velocity; }
        void setOctave (int);
        void setVelocity (int);

        void resized() override;
        void paint (juce::Graphics&) override;

    private:
        void handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float velocity) override;
        void handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float velocity) override;
        void showSettings();
        void timerCallback() override;

        std::function<void (const juce::MidiMessage&)> sendMessage;
        juce::MidiKeyboardState state;
        juce::MidiKeyboardComponent keyboard { state, juce::MidiKeyboardComponent::horizontalKeyboard };
        juce::Label settings, hint;
        juce::TextButton layoutButton;
        Auditioner auditioner;
        std::vector<PhraseNote> phrase;         // the last phrase as written, before the octave moves it
        bool phraseFollows = true;
        double phraseRound = 0.0;
        double lastTick = 0.0;
        std::array<int, maxKeys> sounding;      // the note each playing key sounds, or -1
        int octave = 3, velocity = 100;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Strip)
    };
}
