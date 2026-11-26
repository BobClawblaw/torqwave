#include <JuceHeader.h>
#include "../Source/InstrumentKeys.h"
#include "../Source/BuiltInEffects.h"
#include "../Source/EffectPresets.h"
#include "../Source/WaveSynth.h"
#include "../Source/SwarmGuitar.h"
#include "../Source/DrumMachine.h"
#include "../Source/WaveSynthFavourites.h"
#include "../Source/WaveSynthEditor.h"
#include "TestUtils.h"
#include "InstrumentKeysTests.h"

void runInstrumentKeysTests()
{
    using TestUtils::check;
    using namespace InstrumentKeys;

    // The Q row, as it starts: Q is C, 2 the C sharp above, on to P's E an octave and a third up; A plays nothing.
    const auto startsOnQ = getLayout() == Layout::qRow;
    setLayout (Layout::qRow);
    check (startsOnQ && noteFor ('q', 3) == 48 && noteFor ('2', 3) == 49 && noteFor ('w', 3) == 50 && noteFor ('i', 3) == 60 && noteFor ('p', 3) == 64
               && noteFor ('a', 3) == -1 && noteFor ('1', 3) == -1 && noteFor ('4', 3) == -1 && noteFor ('z', 3) == -1
               && layoutOf (keyOf (Layout::aRow)) == Layout::aRow && layoutOf (keyOf (Layout::qRow)) == Layout::qRow && layoutOf ("") == Layout::qRow,
           "instrument keys: the Q row plays to begin with: Q W E R T Y U I O P the white keys from C (C3, 48, at octave 3), 2 3 5 6 7 9 0 the black; 1, 4, 8 and the A row play nothing");
    setLayout (Layout::aRow);

    // The keys: A is C, W the C sharp above, and on along the rows to the apostrophe's F, an octave and a fourth up.
    check (noteFor ('a', 3) == 48 && noteFor ('A', 3) == 48 && noteFor ('w', 3) == 49 && noteFor ('s', 3) == 50 && noteFor ('k', 3) == 60
               && noteFor ('\'', 3) == 65 && noteFor ('a', 4) == 60 && noteFor ('q', 3) == -1 && noteFor ('z', 3) == -1 && noteFor ('\'', 9) == -1,
           "instrument keys: A to the apostrophe play from C (C3, 48, at octave 3; middle C on K or at octave 4), W E T Y U O P the black keys; other keys play nothing");

    std::vector<juce::MidiMessage> sent;
    int asked = 0;
    Strip strip ([&] (const juce::MidiMessage& m) { sent.push_back (m); });
    strip.aboutToPlay = [&] { ++asked; };

    // A key down plays once however long it is held, and lets go of the note it started though the octave has moved.
    const auto took = strip.keyDown ('a') && strip.keyDown ('a') && strip.keyDown ('x') && strip.getOctave() == 4 && strip.keyUp ('a');
    check (took && sent.size() == 2 && sent[0].isNoteOn() && sent[0].getNoteNumber() == 48 && sent[0].getVelocity() == 100
               && sent[1].isNoteOff() && sent[1].getNoteNumber() == 48 && asked == 1,
           "instrument keys: a key held plays its note once, at velocity 100, and ends that note when let go, though X has moved the octave up meanwhile; "
           "the window is asked to route the note before it is sent");

    // Z and X: the octave, within its range. C and V: the velocity, by steps, 100 and 127 among them.
    sent.clear();
    strip.setOctave (3);
    strip.keyDown ('z');
    const auto down = strip.getOctave() == 2;

    for (int i = 0; i < 12; ++i)
        strip.keyDown ('z');

    const auto lowest = strip.getOctave() == Strip::lowestOctave;

    for (int i = 0; i < 12; ++i)
        strip.keyDown ('x');

    const auto highest = strip.getOctave() == Strip::highestOctave;
    strip.setOctave (3);
    strip.keyDown ('v');
    const auto louder = strip.getVelocity() == 116;
    strip.keyDown ('v');
    strip.keyDown ('v');
    const auto loudest = strip.getVelocity() == 127;
    strip.keyDown ('c');
    strip.keyDown ('c');
    const auto back = strip.getVelocity() == 100;

    for (int i = 0; i < 12; ++i)
        strip.keyDown ('c');

    const auto softest = strip.getVelocity() == 20;
    strip.keyDown ('d');
    check (down && lowest && highest && louder && loudest && back && softest && sent.size() == 1 && sent[0].getNoteNumber() == 52 && sent[0].getVelocity() == 20,
           "instrument keys: Z and X move the octave down and up (0 to 7), C and V the velocity (20 to 127, through 100), and the next note plays there");

    // The window going behind: every note held is let go.
    strip.keyUp ('d');
    sent.clear();
    strip.setVelocity (100);
    strip.keyDown ('a');
    strip.keyDown ('g');
    strip.allUp();
    int ons = 0, offs = 0;

    for (auto& m : sent)
        (m.isNoteOn() ? ons : offs) += 1;

    strip.keyUp ('a');
    check (ons == 2 && offs == 2 && sent.size() == 4, "instrument keys: letting go of everything ends both notes held, once each");

    // The next and previous sound. Torqwave Synth: its factory patches in their order, round the ends.
    {
        BuiltInEffects::BasicSynthProcessor synth;
        const auto& patches = WaveSynth::factoryPatches();
        synth.loadPatch (0);
        const auto next = stepSound (synth, nullptr, 1);
        const auto again = stepSound (synth, nullptr, 1);
        const auto previous = stepSound (synth, nullptr, -1);
        synth.loadPatch (0);
        const auto last = stepSound (synth, nullptr, -1);
        const auto first = stepSound (synth, nullptr, 1);

        // Any other instrument: through the presets row in its window.
        BuiltInEffects::SwarmGuitarProcessor guitar;
        std::unique_ptr<juce::AudioProcessorEditor> editor (guitar.createEditor());
        const auto& made = EffectPresets::factory (BuiltInEffects::swarmGuitarId);
        const auto one = stepSound (guitar, editor.get(), 1);
        const auto two = stepSound (guitar, editor.get(), 1);
        const auto backToOne = stepSound (guitar, editor.get(), -1);
        const auto known = [&] (const juce::String& name) { return std::any_of (made.begin(), made.end(), [&] (const EffectPresets::Factory& f) { return f.name == name; }); };

        check (next == patches[1].name && again == patches[2].name && previous == patches[1].name
                   && last == patches[patches.size() - 1].name && first == patches[0].name && synth.getPatchName() == patches[0].name
                   && known (one) && known (two) && one != two && backToOne == one && stepSound (guitar, nullptr, 1).isEmpty(),
               "instrument keys: the next and previous sound: Torqwave Synth's factory patches in order (" + next + ", " + again + ", back to " + previous
                   + "), round from the first to the last (" + last + ") and on to the first; StringSwarm's presets through its window's presets row (" + one + ", " + two + ")");
    }

    // Favourites: Torqwave Synth's patches marked by name in a file, listed by name, and what the arrows go through while they are browsed.
    {
        const auto file = juce::File::getSpecialLocation (juce::File::tempDirectory).getChildFile ("torqwave-test-synth-favourites.txt");
        file.deleteFile();
        WaveSynth::Favourites::useFile (file);
        auto& favourites = WaveSynth::Favourites::shared();
        BuiltInEffects::BasicSynthProcessor synth;
        synth.loadPatch ("Warehouse Sub");
        auto handled = false;
        const auto marked = toggleFavourite (synth, &handled) && handled && favourites.contains ("Warehouse Sub");
        favourites.set ("Cold Bell", true);
        favourites.set ("Abyss Reese", true);
        favourites.set ("No Such Patch", true);
        const auto& patches = WaveSynth::factoryPatches();
        const auto treeIndexOf = [&patches] (const juce::String& name)
        {
            for (size_t i = 0; i < patches.size(); ++i)
                if (patches[i].name == name)
                    return (int) i;
            return -1;
        };
        juce::StringArray treeOrder;
        for (auto i : favourites.inTreeOrder())
            treeOrder.add (patches[(size_t) i].name);
        const auto listed = favourites.names() == juce::StringArray { "Abyss Reese", "Cold Bell", "Warehouse Sub" } && favourites.indices().size() == 3
                            && treeOrder == juce::StringArray { "Warehouse Sub", "Abyss Reese", "Cold Bell" };

        // The arrows: every patch until the favourites are browsed, then the favourites alone in the tree's order (Bass, Reese, Bell), round their ends.
        const auto everyPatch = synth.browseList().size() == patches.size();
        synth.setBrowsingFavourites (true);
        const auto one = stepSound (synth, nullptr, 1), two = stepSound (synth, nullptr, 1), three = stepSound (synth, nullptr, 1), before = stepSound (synth, nullptr, -1);
        synth.loadPatch ("Acid Corrosion");   // not a favourite: the arrows start from the favourites' first, or last
        const auto fromOutside = stepSound (synth, nullptr, 1), fromOutsideBack = (synth.loadPatch ("Acid Corrosion"), stepSound (synth, nullptr, -1));
        const auto stepped = one == "Abyss Reese" && two == "Cold Bell" && three == "Warehouse Sub" && before == "Cold Bell" && fromOutside == "Warehouse Sub" && fromOutsideBack == "Cold Bell";

        // The menu: a sub-menu for each category that holds a favourite, in the tree's order, the patch it is on ticked with its category.
        auto anyTicked = false;
        const auto menu = WaveSynthEditor::favouritesMenu ("Abyss Reese", true, 1, anyTicked);
        juce::StringArray menuCategories, menuItems, tickedCategories;
        for (juce::PopupMenu::MenuItemIterator it (menu); it.next();)
        {
            auto& item = it.getItem();
            menuCategories.add (item.text);
            if (item.isTicked) tickedCategories.add (item.text);
            if (item.subMenu != nullptr)
                for (juce::PopupMenu::MenuItemIterator sub (*item.subMenu); sub.next();)
                    menuItems.add (sub.getItem().text + (sub.getItem().isTicked ? "*" : "") + "=" + juce::String (sub.getItem().itemID - 1));
        }
        const auto inTree = anyTicked && menuCategories == juce::StringArray { "Bass", "Reese", "Bell" } && tickedCategories == juce::StringArray { "Reese" }
                            && menuItems == juce::StringArray { "Warehouse Sub=" + juce::String (treeIndexOf ("Warehouse Sub")), "Abyss Reese*=" + juce::String (treeIndexOf ("Abyss Reese")),
                                                                "Cold Bell=" + juce::String (treeIndexOf ("Cold Bell")) };

        // The file: read again it holds the same; a name that is no factory patch any more is dropped.
        file.appendText ("A Patch That Was Renamed\n");
        const WaveSynth::Favourites again (file);
        const auto kept = again.names() == favourites.names() && file.loadFileAsString().contains ("A Patch That Was Renamed") && ! again.contains ("A Patch That Was Renamed");

        // A patch taken out as a near-copy: its favourite is the one that stayed (Foundry Throb and Basalt Pulse were Foundry Pump's, Tunnel Sub Warehouse Sub's).
        const auto oldFile = file.getSiblingFile ("torqwave-test-synth-favourites-old.txt");
        oldFile.replaceWithText ("Foundry Throb\nBasalt Pulse\nTunnel Sub\n");
        const WaveSynth::Favourites old (oldFile);
        const auto merged = treeIndexOf ("Foundry Throb") < 0 && treeIndexOf ("Tunnel Sub") < 0 && old.names() == juce::StringArray { "Foundry Pump", "Warehouse Sub" };
        oldFile.deleteFile();

        // Renamed patches (style names taken out in 0.22.0): found under their old names when loaded, in favourites and in a saved state.
        BuiltInEffects::BasicSynthProcessor renamed;
        const auto loadedOld = renamed.loadPatch ("EBM Sequence Bass") && renamed.getPatchName() == "Machine Sequence";
        renamed.setPresetTexts ({ { "patch", "Body Music" } });
        const auto savedOld = renamed.getPatchName() == "Iron Motion";
        oldFile.replaceWithText ("Body Music\nEBM Sequence Bass\nDeep Bass\n");
        const WaveSynth::Favourites oldNames (oldFile);
        oldFile.deleteFile();
        const auto renamedFound = loadedOld && savedOld && treeIndexOf ("Body Music") < 0 && treeIndexOf ("Iron Motion") > 0
                                  && oldNames.names() == juce::StringArray { "Iron Motion", "Machine Sequence" };

        // Unmarked, it leaves the list; with none left the arrows go through every patch again; a sound that is no factory patch has no heart.
        synth.loadPatch ("Cold Bell");
        const auto unmarked = ! toggleFavourite (synth) && favourites.names() == juce::StringArray { "Abyss Reese", "Warehouse Sub" };
        favourites.set ("Abyss Reese", false);
        favourites.set ("Warehouse Sub", false);
        const auto noneLeft = favourites.names().isEmpty() && synth.browseList().size() == patches.size() && file.loadFileAsString().trim().isEmpty();
        synth.setPatchName ("Something of My Own");
        auto none = true;
        const auto notMarked = ! toggleFavourite (synth, &none) && ! none && favourites.names().isEmpty();

        // The window: its heart marks the patch it is on, and its arrows go through the same list as the keys.
        synth.setBrowsingFavourites (false);
        synth.loadPatch ("Minor Stab");
        std::unique_ptr<juce::AudioProcessorEditor> made (synth.createEditorAndMakeActive());
        auto* window = dynamic_cast<WaveSynthEditor*> (made.get());
        auto inWindow = window != nullptr;

        if (window != nullptr)
        {
            window->toggleFavourite();
            inWindow = favourites.contains ("Minor Stab") && window->getFavouriteBounds().getWidth() > 10 && window->getLocalBounds().contains (window->getFavouriteBounds());
            window->toggleFavourite();
            inWindow = inWindow && ! favourites.contains ("Minor Stab");
            const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 1.0f);
            inWindow = inWindow && image.isValid();
        }

        made.reset();
        WaveSynth::Favourites::useFile (WaveSynth::Favourites::defaultFile());
        file.deleteFile();

        check (marked && listed && kept && unmarked && noneLeft && notMarked,
               "synth favourites: a factory patch is marked and unmarked by name, listed by name and in the patch tree's order (Warehouse Sub, Abyss Reese, Cold Bell: Bass, Reese, Bell), kept in a file that reads back the same and drops a name that is no patch any more; "
               "a sound that is no factory patch can't be marked");
        check (renamedFound, "synth: a patch renamed in 0.22.0 is found under its old name, loaded, in a saved state and in favourites (" + oldNames.names().joinIntoString (", ") + ")");
        check (merged, "synth favourites: one marked on a patch taken out as a near-copy is the patch that stayed (" + old.names().joinIntoString (", ") + ")");
        check (inTree, "synth favourites: the Favourites menu shows them under their categories in the tree's order (" + menuCategories.joinIntoString (", ")
                           + ": " + menuItems.joinIntoString (", ") + "), the patch it is on ticked with its category");
        check (everyPatch && stepped && inWindow,
               "synth favourites: the arrows go through every patch until the favourites are browsed, then through the favourites alone in the tree's order, round their ends ("
                   + one + ", " + two + ", " + three + "); the window's heart marks the patch it is on");
    }

    // Audition: a phrase for each kind of sound, played over time on the keys.
    {
        BuiltInEffects::BasicSynthProcessor synth;
        const auto phraseOf = [&] (const juce::String& patchName) { synth.loadPatch (patchName); return phraseFor (synth); };
        const auto bass = phraseOf ("Warehouse Sub"), pad = phraseOf ("Grey Sky"), stab = phraseOf ("Minor Stab"), lead = phraseOf ("Cold Saw Lead");
        const auto topNote = [] (const std::vector<PhraseNote>& p) { int top = 0; for (auto& n : p) top = juce::jmax (top, n.note); return top; };
        const auto together = [] (const std::vector<PhraseNote>& p) { int n = 0; for (auto& x : p) if (juce::exactlyEqual (x.startMs, 0.0)) ++n; return n; };
        BuiltInEffects::BasicDrumsProcessor drums;
        BuiltInEffects::SwarmGuitarProcessor guitar;
        const auto kit = phraseFor (drums), strum = phraseFor (guitar);

        check (bass.size() == 4 && topNote (bass) <= 48 && together (pad) == 4 && pad[0].lengthMs > 1000.0 && together (stab) == 4 && stab.size() == 8
                   && lead.size() == 4 && together (lead) == 1 && lead[0].note == 60 && kit.size() == 6 && kit[0].note == 36 && kit[2].note == 38
                   && strum.size() == 1 && strum[0].note == 40 && strum[0].lengthMs >= 1000.0,
               "audition: a bass plays a line under C3, a pad a chord held over a second, a stab its chord twice, a lead a rising figure from middle C; "
               "Basic Drums a kick, hat and snare; StringSwarm a low E held");

        // The patterns: a dozen for basses, chords, stabs and leads, twenty for kits, eight for the rest; each plays, fits its round and is in range.
        {
            auto counts = true, fit = true, named = true;
            int total = 0;

            for (int k = 0; k <= (int) Kind::other; ++k)
            {
                const auto& list = patternsFor ((Kind) k);
                const auto want = (Kind) k == Kind::kit ? 20 : k <= (int) Kind::lead ? 12 : ((Kind) k == Kind::other ? 1 : 8);
                counts = counts && (int) list.size() == want;
                juce::StringArray names;

                for (auto& pattern : list)
                {
                    auto end = 0.0;

                    for (auto& n : pattern.notes)
                    {
                        end = juce::jmax (end, n.startMs + n.lengthMs);
                        fit = fit && n.note >= 24 && n.note <= 96 && n.lengthMs > 0.0 && n.velocity > 0.0f && n.velocity <= 1.0f;
                    }

                    fit = fit && ! pattern.notes.empty() && (pattern.lengthMs <= 0.0 || (end <= pattern.lengthMs + 40.0 && std::fmod (pattern.lengthMs, 1920.0) < 0.001));
                    named = named && pattern.name.isNotEmpty() && ! names.contains (pattern.name);
                    names.add (pattern.name);
                    ++total;
                }
            }

            // A pattern chosen is what the sound plays, is kept by name, and an unknown name leaves the first.
            setPattern (Kind::bass, 3);
            const auto galloping = phraseOf ("Warehouse Sub").size() == patternsFor (Kind::bass)[3].notes.size() && patternFor (synth).name == "EBM Gallop";
            const auto saved = getPatternChoices();
            setPatternChoices ({});
            const auto cleared = getPattern (Kind::bass) == 0;
            setPatternChoices (saved);
            const auto restored = getPattern (Kind::bass) == 3 && saved == "bass=EBM Gallop";
            setPatternChoices ("bass=No Such Pattern;kit=Four on the Floor");
            const auto unknown = getPattern (Kind::bass) == 0 && getPattern (Kind::kit) == 1;
            setPatternChoices ({});

            // A whole-bar pattern loops on its bars.
            Auditioner looper;
            looper.start (patternsFor (Kind::bass)[1].notes, true, patternsFor (Kind::bass)[1].lengthMs);
            const auto onBars = juce::exactlyEqual (looper.getLoopLengthMs(), 1920.0);
            looper.stop();

            check (counts && fit && named && galloping && cleared && restored && unknown && onBars && kindFor (drums) == Kind::kit && kindFor (guitar) == Kind::strings,
                   "audition patterns: " + juce::String (total) + " in all, a dozen for basses, chords, stabs and leads, twenty for kits and eight for held sounds, hits and strings, "
                   "each named once, in range and inside its round of whole bars; the one chosen plays, is kept by name, and an unknown name falls back to the first");
        }

        // Over time: each note starts and ends when it is due, a new phrase ends the one playing, and a key played by hand ends it.
        std::vector<juce::String> heard;
        Auditioner player;
        player.on = [&] (int note, float) { heard.push_back ("on " + juce::String (note)); };
        player.off = [&] (int note) { heard.push_back ("off " + juce::String (note)); };
        player.start ({ { 60, 0.0, 100.0, 0.8f }, { 64, 100.0, 100.0, 0.8f } });
        const auto atOnce = heard.size() == 1 && heard[0] == "on 60";
        player.advance (99.0);
        const auto held = heard.size() == 1;
        player.advance (1.0);
        const auto second = heard.size() == 3 && heard[1] == "off 60" && heard[2] == "on 64" && player.isPlaying();
        player.advance (100.0);
        const auto done = heard.size() == 4 && heard[3] == "off 64" && ! player.isPlaying();
        heard.clear();
        player.start ({ { 48, 0.0, 500.0, 0.8f } });
        player.start ({ { 50, 0.0, 500.0, 0.8f } });
        const auto replaced = heard.size() == 3 && heard[1] == "off 48" && heard[2] == "on 50";
        player.stop();
        const auto stopped = heard.size() == 4 && heard[3] == "off 50" && ! player.isPlaying();

        sent.clear();
        strip.audition ({ { 48, 0.0, 5000.0, 0.8f } });
        const auto sounding = strip.isAuditioning() && sent.size() == 1 && sent[0].isNoteOn() && sent[0].getNoteNumber() == 48;
        strip.setOctave (3);
        strip.keyDown ('a');
        const auto byHand = ! strip.isAuditioning() && sent.size() == 3 && sent[1].isNoteOff() && sent[1].getNoteNumber() == 48 && sent[2].isNoteOn();
        strip.keyUp ('a');

        // Looping: the phrase goes round, each round two beats at least and a whole number of beats, until it is stopped.
        heard.clear();
        player.start ({ { 60, 0.0, 100.0, 0.8f }, { 64, 300.0, 100.0, 0.8f } }, true);
        const auto roundLength = juce::exactlyEqual (player.getLoopLengthMs(), 960.0);
        player.advance (500.0);
        const auto firstRound = heard.size() == 3 && heard[2] == "on 64" && player.isLooping();
        player.advance (460.0);
        const auto again = heard.size() == 5 && heard[4] == "on 60";
        player.advance (960.0);
        const auto third = heard.size() == 9 && heard[8] == "on 60" && player.isPlaying();
        player.stop();
        const auto loopStopped = heard.size() == 10 && heard[9] == "off 60" && ! player.isPlaying() && ! player.isLooping();
        player.start ({ { 48, 0.0, 1700.0, 0.8f } }, true);
        const auto longRound = juce::exactlyEqual (player.getLoopLengthMs(), 1920.0);
        player.stop();

        // On the strip: with Loop on a key played by hand does not end the phrase; Stop does.
        setAuditionLoop (true);
        sent.clear();
        strip.audition ({ { 48, 0.0, 300.0, 0.8f } });
        strip.keyDown ('a');
        strip.keyUp ('a');
        const auto playedOver = strip.isAuditioning();
        strip.stopAudition();
        const auto stoppedByHand = ! strip.isAuditioning();

        // The octave: a phrase plays as far from octave 3 as the keys are, and moves with them while it plays; a kit's does not.
        strip.setOctave (4);
        sent.clear();
        strip.audition ({ { 48, 0.0, 300.0, 0.8f } });
        const auto up = sent.size() == 1 && sent[0].isNoteOn() && sent[0].getNoteNumber() == 60;
        strip.setOctave (2);
        const auto moved = sent.size() == 3 && sent[1].isNoteOff() && sent[1].getNoteNumber() == 60 && sent[2].isNoteOn() && sent[2].getNoteNumber() == 36;
        strip.stopAudition();
        sent.clear();
        strip.audition ({ { 36, 0.0, 300.0, 0.8f } }, false);
        strip.setOctave (5);
        const auto kitStays = sent.size() == 1 && sent[0].getNoteNumber() == 36 && strip.isAuditioning();
        strip.stopAudition();
        strip.setOctave (3);
        setAuditionLoop (false);

        // The session's tempo: an audition plays at it (written at 125 BPM), 125 with no session.
        const auto noSession = juce::exactlyEqual (auditionSpeed(), 1.0);
        sessionTempo = [] { return 250.0; };
        const auto doubled = juce::exactlyEqual (auditionSpeed(), 2.0);
        sessionTempo = [] { return 0.0; };
        const auto none = juce::exactlyEqual (auditionSpeed(), 1.0);
        sessionTempo = nullptr;

        check (noSession && doubled && none, "audition: it plays at the session's tempo (twice as fast at 250 BPM), at 125 BPM with no session");

        check (up && moved && kitStays,
               "audition: a phrase plays an octave up with the keys at octave 4, moves to octave 2 with them while it loops, and a kit's phrase stays where it is");

        check (roundLength && firstRound && again && third && loopStopped && longRound && playedOver && stoppedByHand,
               "audition: looping, a phrase goes round every 960 ms (1,920 for a long one: whole beats at 125 BPM) until stopped; "
               "a key played by hand leaves a looping phrase going, and Stop ends it");

        const auto wasOn = getAudition();
        setAudition (false);
        const auto off = ! getAudition();
        setAudition (true);

        check (atOnce && held && second && done && replaced && stopped && sounding && byHand && wasOn && off,
               "audition: a phrase's notes start and end when due (the second as the first ends), a new phrase ends the one playing, stopping ends its notes, "
               "and a key played by hand ends it; it is on to begin with and can be turned off");
    }

    setLayout (Layout::qRow);
}
