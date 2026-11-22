#include "InstrumentKeys.h"
#include "BuiltInEffects.h"
#include "EffectPresets.h"
#include "Theme.h"
#include "WaveSynth.h"
#include "DrumMachine.h"
#include "SwarmGuitar.h"

namespace InstrumentKeys
{
std::function<void()> layoutChanged;
std::function<void()> auditionChanged;

namespace
{
    Layout layout = Layout::qRow;
    bool auditionOn = true;
    bool auditionLoop = false;

    int indexOfKey (juce::juce_wchar key)
    {
        return playingKeys().indexOfChar (juce::CharacterFunctions::toLowerCase (key));
    }

    EffectPresets::Bar* presetsRowIn (juce::Component& c)
    {
        if (auto* bar = dynamic_cast<EffectPresets::Bar*> (&c))
            return bar;

        for (auto* child : c.getChildren())
            if (auto* bar = presetsRowIn (*child))
                return bar;

        return nullptr;
    }
}

Layout getLayout()                              { return layout; }
void setLayout (Layout l)                       { layout = l; }
juce::String keyOf (Layout l)                   { return l == Layout::aRow ? "a" : "q"; }
Layout layoutOf (const juce::String& key)       { return key == "a" ? Layout::aRow : Layout::qRow; }
juce::String playingKeys()                      { return layout == Layout::aRow ? "awsedftgyhujkolp;'" : "q2w3er5t6y7ui9o0p"; }

bool getAudition()                              { return auditionOn; }
void setAudition (bool on)                      { auditionOn = on; }
bool getAuditionLoop()                          { return auditionLoop; }
void setAuditionLoop (bool on)                  { auditionLoop = on; }

std::vector<PhraseNote> phraseFor (juce::AudioProcessor& p)
{
    using Notes = std::vector<PhraseNote>;
    const auto chord = [] (std::initializer_list<int> notes, double start, double length, float velocity = 0.8f)
    {
        Notes out;

        for (auto n : notes)
            out.push_back ({ n, start, length, velocity });

        return out;
    };
    const auto join = [] (Notes a, const Notes& b) { a.insert (a.end(), b.begin(), b.end()); return a; };

    if (auto* synth = dynamic_cast<BuiltInEffects::BasicSynthProcessor*> (&p))
    {
        juce::String category;

        for (auto& patch : WaveSynth::factoryPatches())
            if (patch.name == synth->getPatchName())
                category = patch.category;

        const auto is = [&category] (std::initializer_list<const char*> names)
        {
            for (auto* n : names)
                if (category == n)
                    return true;

            return false;
        };

        // A bass line: the root, the root, the octave, the root, in sixteenths at 125 BPM.
        if (is ({ "Bass", "Sub", "Reese", "Acid" }))
            return { { 36, 0.0, 100.0, 0.9f }, { 36, 120.0, 100.0, 0.7f }, { 48, 240.0, 100.0, 0.8f }, { 36, 360.0, 220.0, 0.9f } };

        if (is ({ "Pad", "Strings", "Organ", "Chord", "Texture", "Vox", "Cinematic" }))
            return chord ({ 48, 55, 60, 63 }, 0.0, 1600.0, 0.75f);

        if (is ({ "Stab", "Brass" }))
            return join (chord ({ 48, 60, 63, 67 }, 0.0, 160.0, 0.9f), chord ({ 48, 60, 63, 67 }, 360.0, 260.0, 0.8f));

        if (is ({ "Sequence", "Arp", "Drone", "Riser" }))
            return { { 48, 0.0, 2000.0, 0.85f } };

        if (is ({ "Perc", "Drum", "Impact", "FX", "Glitch", "Game" }))
            return { { 60, 0.0, 120.0, 0.9f }, { 60, 380.0, 120.0, 0.7f } };

        if (is ({ "Guitar" }))
            return { { 40, 0.0, 1200.0, 0.85f } };

        // Leads, plucks, bells, keys, chip and anything else: a minor figure upward from middle C.
        return { { 60, 0.0, 150.0, 0.85f }, { 63, 170.0, 150.0, 0.75f }, { 67, 340.0, 150.0, 0.8f }, { 72, 510.0, 420.0, 0.9f } };
    }

    if (dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (&p) != nullptr)
        return { { 36, 0.0, 60.0, 0.95f }, { 42, 125.0, 40.0, 0.6f }, { 38, 250.0, 60.0, 0.9f }, { 42, 375.0, 40.0, 0.6f }, { 36, 500.0, 60.0, 0.9f }, { 39, 750.0, 60.0, 0.85f } };

    if (dynamic_cast<BuiltInEffects::SwarmGuitarProcessor*> (&p) != nullptr)
        return { { 40, 0.0, 1500.0, 0.85f } };

    return { { 60, 0.0, 700.0, 0.8f } };
}

//==============================================================================
void Auditioner::start (std::vector<PhraseNote> notes, bool loop)
{
    stop();
    auto end = 0.0;

    for (auto& n : notes)
    {
        phrase.push_back ({ n, false, false });
        end = juce::jmax (end, n.startMs + n.lengthMs);
    }

    // A round: the phrase and a breath, to the next whole beat at 125 BPM, two beats at least.
    constexpr double beat = 480.0;
    loopLength = juce::jmax (2.0 * beat, std::ceil ((end + 200.0) / beat) * beat);
    looping = loop;
    at = 0.0;
    advance (0.0);
}

void Auditioner::stop()
{
    for (auto& p : phrase)
        if (p.started && ! p.ended && off != nullptr)
            off (p.note.note);

    phrase.clear();
    looping = false;
}

void Auditioner::advance (double milliseconds)
{
    if (phrase.empty())
        return;

    at += milliseconds;

    // Ends before starts, so a note repeated at the moment the one before ends is heard as two.
    const auto play = [this]
    {
        auto left = false;

        for (auto& p : phrase)
            if (p.started && ! p.ended && at >= p.note.startMs + p.note.lengthMs)
            {
                p.ended = true;

                if (off != nullptr)
                    off (p.note.note);
            }

        for (auto& p : phrase)
        {
            if (! p.started && at >= p.note.startMs)
            {
                p.started = true;

                if (on != nullptr)
                    on (p.note.note, p.note.velocity);
            }

            left = left || ! p.ended;
        }

        return left;
    };

    const auto left = play();

    if (looping)
    {
        // Round again: everything of this round ended, the next begun from where the round's length falls.
        if (at >= loopLength)
        {
            for (auto& p : phrase)
            {
                if (p.started && ! p.ended && off != nullptr)
                    off (p.note.note);

                p.started = p.ended = false;
            }

            at = std::fmod (at, loopLength);
            play();
        }
    }
    else if (! left)
    {
        phrase.clear();
    }
}

int noteFor (juce::juce_wchar key, int octave)
{
    const auto index = indexOfKey (key);
    const auto note = 12 * (octave + 1) + index;
    return index >= 0 && juce::isPositiveAndBelow (note, 128) ? note : -1;
}

EffectPresets::Bar* presetsRow (juce::Component& editor)       { return presetsRowIn (editor); }

juce::String stepSound (juce::AudioProcessor& p, juce::Component* editor, int by)
{
    if (by == 0)
        return {};

    if (auto* synth = dynamic_cast<BuiltInEffects::BasicSynthProcessor*> (&p))
    {
        const auto& patches = WaveSynth::factoryPatches();
        const auto count = (int) patches.size();

        if (count == 0)
            return {};

        // From the patch it is on; a sound that is no factory patch (an old session's, a saved preset) from the list's start.
        auto at = -1;

        for (int i = 0; i < count; ++i)
            if (patches[(size_t) i].name == synth->getPatchName())
                at = i;

        const auto next = at < 0 ? (by > 0 ? 0 : count - 1) : (((at + by) % count) + count) % count;
        synth->loadPatch (next);
        return patches[(size_t) next].name;
    }

    if (editor != nullptr)
        if (auto* bar = presetsRowIn (*editor); bar != nullptr && bar->step (by))
            return bar->getChosen();

    return {};
}

//==============================================================================
Strip::Strip (std::function<void (const juce::MidiMessage&)> send) : sendMessage (std::move (send))
{
    sounding.fill (-1);
    auditioner.on = [this] (int note, float v) { state.noteOn (1, note, v); };
    auditioner.off = [this] (int note) { state.noteOff (1, note, 0.0f); };
    keyboard.setAvailableRange (24, 108);
    keyboard.setOctaveForMiddleC (4);
    keyboard.clearKeyMappings();               // the window hears the keys, so they play wherever the keyboard focus is
    keyboard.setWantsKeyboardFocus (false);
    state.addListener (this);
    addAndMakeVisible (keyboard);

    for (auto* l : { &settings, &hint })
    {
        l->setFont (juce::Font (juce::FontOptions (11.5f)));
        l->setInterceptsMouseClicks (false, false);
        addAndMakeVisible (*l);
    }

    Theme::follow (settings, juce::Label::textColourId, Theme::text);
    Theme::follow (hint, juce::Label::textColourId, Theme::dimText);
    hint.setJustificationType (juce::Justification::centredRight);
    layoutButton.setTooltip ("Which of the computer's keys play: the Q row (Q W E R T Y U I, with 2 3 5 6 7 for the black keys) or the A row (A S D F G H J K L, with W E T Y U)");
    layoutButton.setWantsKeyboardFocus (false);
    layoutButton.onClick = [this]
    {
        allUp();
        setLayout (getLayout() == Layout::qRow ? Layout::aRow : Layout::qRow);

        if (layoutChanged != nullptr)
            layoutChanged();

        showSettings();
    };
    addAndMakeVisible (layoutButton);

    loopButton.setClickingTogglesState (true);
    loopButton.setTooltip ("Loop the audition: the phrase goes round until you press Stop or choose another sound");
    stopButton.setTooltip ("Stop the audition that is playing");

    for (auto* b : { &loopButton, &stopButton })
    {
        b->setWantsKeyboardFocus (false);
        addAndMakeVisible (*b);
    }

    loopButton.onClick = [this]
    {
        setAuditionLoop (loopButton.getToggleState());

        if (auditionChanged != nullptr)
            auditionChanged();

        if (! getAuditionLoop())
            stopAudition();
    };
    stopButton.onClick = [this] { stopAudition(); };
    showAudition();
    showSettings();
    setSize (600, height);
}

Strip::~Strip()
{
    state.removeListener (this);
}

void Strip::showSettings()
{
    const auto q = getLayout() == Layout::qRow;
    settings.setText ("Keys play from C" + juce::String (octave) + " at velocity " + juce::String (velocity), juce::dontSendNotification);
    layoutButton.setButtonText (q ? "Q row" : "A row");
    hint.setText (juce::String (q ? "Play: Q to P, 2 3 5 6 7 9 0" : "Play: A to ', W E T Y U O P")
                      + juce::String::fromUTF8 ("   Octave: Z X   Velocity: C V   Sound: \xe2\x86\x90 \xe2\x86\x92"), juce::dontSendNotification);
}

void Strip::setOctave (int o)
{
    octave = juce::jlimit (lowestOctave, highestOctave, o);
    showSettings();
}

void Strip::setVelocity (int v)
{
    velocity = juce::jlimit (1, 127, v);
    keyboard.setVelocity ((float) velocity / 127.0f, false);
    showSettings();
}

bool Strip::keyDown (juce::juce_wchar key)
{
    const auto lower = juce::CharacterFunctions::toLowerCase (key);

    if (lower == 'z' || lower == 'x')
    {
        setOctave (octave + (lower == 'x' ? 1 : -1));
        return true;
    }

    if (lower == 'c' || lower == 'v')
    {
        // Down to the step below, up to the step above: the usual 100 is one of them, and the top is 127.
        auto next = velocity;

        for (auto step : { 20, 36, 52, 68, 84, 100, 116, 127 })
        {
            if (lower == 'c' && step < velocity)
                next = step;

            if (lower == 'v' && step > velocity && next == velocity)
                next = step;
        }

        setVelocity (next);
        return true;
    }

    const auto index = indexOfKey (key);

    if (index < 0)
        return false;

    if (sounding[(size_t) index] < 0)
        if (const auto note = noteFor (key, octave); note >= 0)
        {
            if (! auditioner.isLooping())
                auditioner.stop();
            sounding[(size_t) index] = note;
            state.noteOn (1, note, (float) velocity / 127.0f);
            lastTick = juce::Time::getMillisecondCounterHiRes();
            startTimerHz (60);
        }

    return true;
}

bool Strip::keyUp (juce::juce_wchar key)
{
    const auto index = indexOfKey (key);

    if (index < 0)
        return false;

    // The note it started, whatever the octave is now.
    if (const auto note = sounding[(size_t) index]; note >= 0)
    {
        sounding[(size_t) index] = -1;
        state.noteOff (1, note, 0.0f);
    }

    return true;
}

bool Strip::readKeys (bool enabled)
{
    const auto keys = playingKeys();
    auto any = false;

    for (int i = 0; i < keys.length(); ++i)
    {
        const auto key = keys[i];
        const auto down = enabled && juce::KeyPress::isKeyCurrentlyDown ((int) key);

        if (down)
            keyDown (key);
        else
            keyUp (key);

        any = any || down;
    }

    return any;
}

void Strip::stopAudition()
{
    auditioner.stop();
    showAudition();
}

void Strip::showAudition()
{
    loopButton.setToggleState (getAuditionLoop(), juce::dontSendNotification);
    stopButton.setEnabled (auditioner.isPlaying());
}

void Strip::audition (std::vector<PhraseNote> notes)
{
    auditioner.start (std::move (notes), getAuditionLoop());
    showAudition();
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void Strip::timerCallback()
{
    const auto keys = playingKeys();
    const auto now = juce::Time::getMillisecondCounterHiRes();
    auditioner.advance (now - lastTick);
    lastTick = now;
    stopButton.setEnabled (auditioner.isPlaying());
    // A key's release can go unheard (the window losing the keyboard mid-note): a key no longer down lets go of its note.
    auto any = false;

    for (int i = 0; i < keys.length(); ++i)
        if (sounding[(size_t) i] >= 0)
        {
            if (juce::KeyPress::isKeyCurrentlyDown ((int) keys[i]))
                any = true;
            else
                keyUp (keys[i]);
        }

    if (! any && ! auditioner.isPlaying())
        stopTimer();
}

void Strip::allUp()
{
    const auto keys = playingKeys();
    auditioner.stop();
    for (int i = 0; i < keys.length(); ++i)
        keyUp (keys[i]);

    state.allNotesOff (1);
}

void Strip::handleNoteOn (juce::MidiKeyboardState*, int channel, int note, float v)
{
    if (aboutToPlay != nullptr)
        aboutToPlay();

    if (sendMessage != nullptr)
        sendMessage (juce::MidiMessage::noteOn (channel, note, v));
}

void Strip::handleNoteOff (juce::MidiKeyboardState*, int channel, int note, float v)
{
    if (sendMessage != nullptr)
        sendMessage (juce::MidiMessage::noteOff (channel, note, v));
}

void Strip::resized()
{
    auto area = getLocalBounds().reduced (6, 2);
    auto top = area.removeFromTop (18);
    settings.setBounds (top.removeFromLeft (juce::jmin (220, top.getWidth() / 2)));
    layoutButton.setBounds (top.removeFromLeft (52).reduced (0, 1));
    top.removeFromLeft (10);
    loopButton.setBounds (top.removeFromLeft (46).reduced (0, 1));
    top.removeFromLeft (2);
    stopButton.setBounds (top.removeFromLeft (44).reduced (0, 1));
    hint.setBounds (top);
    keyboard.setKeyWidth ((float) area.getWidth() / 50.0f);    // C1 to C8: fifty white keys across the window
    keyboard.setBounds (area.withTrimmedTop (2));
}

void Strip::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
    g.setColour (Theme::line);
    g.fillRect (0, 0, getWidth(), 1);
}
}
