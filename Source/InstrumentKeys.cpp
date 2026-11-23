#include "InstrumentKeys.h"
#include "BuiltInEffects.h"
#include "EffectPresets.h"
#include "Theme.h"
#include "WaveSynthFavourites.h"
#include "WaveSynth.h"
#include "DrumMachine.h"
#include "SwarmGuitar.h"
#include "Sampler.h"

namespace InstrumentKeys
{
std::function<void()> layoutChanged;
std::function<void()> auditionChanged;
std::function<double()> sessionTempo;

double auditionSpeed()
{
    const auto tempo = sessionTempo != nullptr ? sessionTempo() : 0.0;
    return tempo > 0.0 ? juce::jlimit (20.0, 999.0, tempo) / 125.0 : 1.0;
}

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

Kind kindFor (juce::AudioProcessor& p)
{
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

        if (is ({ "Bass", "Sub", "Reese", "Acid" }))                                            return Kind::bass;
        if (is ({ "Pad", "Strings", "Organ", "Chord", "Texture", "Vox", "Cinematic" }))        return Kind::chord;
        if (is ({ "Stab", "Brass" }))                                                          return Kind::stab;
        if (is ({ "Sequence", "Arp", "Drone", "Riser" }))                                      return Kind::held;
        if (is ({ "Perc", "Drum", "Impact", "FX", "Glitch", "Game" }))                         return Kind::hit;
        if (is ({ "Guitar" }))                                                                 return Kind::strings;

        return Kind::lead;
    }

    if (dynamic_cast<BuiltInEffects::BasicDrumsProcessor*> (&p) != nullptr)
        return Kind::kit;

    if (dynamic_cast<BuiltInEffects::SwarmGuitarProcessor*> (&p) != nullptr)
        return Kind::strings;

    // The sampler: a kit's beat for slices, hits for one-shots, a held chord for looped samples.
    if (auto* sampler = dynamic_cast<BuiltInEffects::SamplerProcessor*> (&p))
    {
        if (sampler->getSlicing().on)
            return Kind::kit;

        const auto zones = sampler->getZones();

        if (zones.empty())
            return Kind::other;

        if (std::all_of (zones.begin(), zones.end(), [] (const Sampler::Zone& z) { return z.oneShot; }))
            return Kind::hit;

        if (std::any_of (zones.begin(), zones.end(), [] (const Sampler::Zone& z) { return z.loop != 0; }))
            return Kind::chord;

        return Kind::lead;
    }

    return Kind::other;
}

namespace
{
    constexpr std::array<const char*, 9> kindNames { "bass", "chord", "stab", "lead", "held", "hit", "strings", "kit", "other" };
    std::array<int, kindNames.size()> chosenPatterns {};
}

int getPattern (Kind k)
{
    const auto i = chosenPatterns[(size_t) k];
    return juce::isPositiveAndBelow (i, (int) patternsFor (k).size()) ? i : 0;
}

void setPattern (Kind k, int index)
{
    chosenPatterns[(size_t) k] = juce::jlimit (0, (int) patternsFor (k).size() - 1, index);
}

juce::String getPatternChoices()
{
    juce::StringArray out;

    for (size_t i = 0; i < kindNames.size(); ++i)
        if (chosenPatterns[i] > 0)
            out.add (juce::String (kindNames[i]) + "=" + patternsFor ((Kind) i)[(size_t) getPattern ((Kind) i)].name);

    return out.joinIntoString (";");
}

void setPatternChoices (const juce::String& text)
{
    // By name, so a pattern added or moved in a later version keeps the one chosen.
    chosenPatterns.fill (0);

    for (auto& item : juce::StringArray::fromTokens (text, ";", ""))
        for (size_t i = 0; i < kindNames.size(); ++i)
            if (item.upToFirstOccurrenceOf ("=", false, false) == kindNames[i])
            {
                const auto& list = patternsFor ((Kind) i);

                for (size_t j = 0; j < list.size(); ++j)
                    if (list[j].name == item.fromFirstOccurrenceOf ("=", false, false))
                        chosenPatterns[i] = (int) j;
            }
}

const Pattern& patternFor (juce::AudioProcessor& p)
{
    const auto kind = kindFor (p);
    return patternsFor (kind)[(size_t) getPattern (kind)];
}

std::vector<PhraseNote> phraseFor (juce::AudioProcessor& p)
{
    return patternFor (p).notes;
}

//==============================================================================
void Auditioner::start (std::vector<PhraseNote> notes, bool loop, double roundMs)
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
    loopLength = roundMs > 0.0 ? roundMs : juce::jmax (2.0 * beat, std::ceil ((end + 200.0) / beat) * beat);
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

bool toggleFavourite (juce::AudioProcessor& p, bool* handled)
{
    auto* synth = dynamic_cast<BuiltInEffects::BasicSynthProcessor*> (&p);
    const auto known = synth != nullptr && std::any_of (WaveSynth::factoryPatches().begin(), WaveSynth::factoryPatches().end(),
                                                        [synth] (const WaveSynth::Patch& patch) { return patch.name == synth->getPatchName(); });

    if (handled != nullptr)
        *handled = known;

    return known && WaveSynth::Favourites::shared().toggle (synth->getPatchName());
}

juce::String stepSound (juce::AudioProcessor& p, juce::Component* editor, int by)
{
    if (by == 0)
        return {};

    // Torqwave Synth: through what its own arrows go through (every factory patch, or the favourites while they are browsed).
    if (auto* synth = dynamic_cast<BuiltInEffects::BasicSynthProcessor*> (&p))
    {
        const auto loaded = synth->stepPatch (by);
        return loaded >= 0 ? WaveSynth::factoryPatches()[(size_t) loaded].name
                           : synth->isBrowsingDecrypted() ? synth->getPatchName() : juce::String();   // one of the user's Decrypted patches
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
    const auto before = octave;
    octave = juce::jlimit (lowestOctave, highestOctave, o);
    showSettings();

    // A phrase playing moves with the keys: it starts again an octave up or down.
    if (octave != before && phraseFollows && auditioner.isPlaying())
        audition (phrase, true, phraseRound);
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
}

void Strip::audition (std::vector<PhraseNote> notes, bool followOctave, double roundMs)
{
    phrase = std::move (notes);
    phraseFollows = followOctave;
    phraseRound = roundMs;
    auto shifted = phrase;

    if (followOctave)
        for (auto& n : shifted)
            n.note = juce::jlimit (0, 127, n.note + 12 * (octave - 3));

    auditioner.start (std::move (shifted), getAuditionLoop(), phraseRound);
    lastTick = juce::Time::getMillisecondCounterHiRes();
    startTimerHz (60);
}

void Strip::timerCallback()
{
    const auto keys = playingKeys();
    const auto now = juce::Time::getMillisecondCounterHiRes();
    auditioner.advance ((now - lastTick) * auditionSpeed());    // the session's tempo, read as it goes
    lastTick = now;
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
