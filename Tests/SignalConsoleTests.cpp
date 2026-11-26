#include <JuceHeader.h>
#include "../Source/SignalConsole.h"
#include "../Source/SignalConsoleView.h"
#include "../Source/DecryptView.h"
#include "../Source/WaveSynthEditor.h"
#include "../Source/Theme.h"
#include "TestUtils.h"
#include "SignalConsoleTests.h"

namespace
{
    using TestUtils::check;
    using namespace SignalConsole;

    Patch factory (Synth& synth, const juce::String& name)
    {
        synth.loadPatch (name);
        return capture (synth);
    }

    bool sameValues (const Patch& a, const Patch& b, const std::function<bool (const juce::String&)>& only = {})
    {
        for (auto& [id, value] : a.values)
            if ((only == nullptr || only (id)) && ! juce::exactlyEqual (value, b.values.at (id)))
                return false;

        return a.tables == b.tables;
    }

    void testNodes()
    {
        Synth synth;
        Renderer renderer;
        const auto held = factory (synth, "Abyss Reese");
        Settings settings;
        settings.reach = 0.5f;
        settings.seed = 7;

        // The same node, Reach and seed: the same twelve. Another seed: others.
        const auto first = nodesAround (held, settings, renderer), again = nodesAround (held, settings, renderer);
        settings.seed = 8;
        const auto other = nodesAround (held, settings, renderer);
        settings.seed = 7;
        auto same = first.size() == numNodes && again.size() == numNodes, different = false, moved = true, levelled = true;
        std::array<int, numParts> perPart {};
        float nearest = 1.0f, furthest = 0.0f, worstLevel = 0.0f;
        const auto heldNode = nodeOf (held, renderer);

        for (size_t i = 0; i < first.size() && same; ++i)
        {
            same = same && sameValues (first[i].patch, again[i].patch) && first[i].address == again[i].address && first[i].part == again[i].part;
            different = different || ! sameValues (first[i].patch, other[i].patch);
            moved = moved && ! sameValues (first[i].patch, held) && first[i].address != heldNode.address && ! first[i].changes.empty();
            ++perPart[(size_t) first[i].part];
            nearest = juce::jmin (nearest, first[i].distance);
            furthest = juce::jmax (furthest, first[i].distance);
            const auto off = std::abs (juce::Decibels::gainToDecibels (first[i].signature.peak / heldNode.signature.peak));
            worstLevel = juce::jmax (worstLevel, off);
            levelled = levelled && off < 1.5f;
        }

        check (same && different && moved,
               "console: the same node, Reach and seed give the same twelve nodes (settings, addresses and parts), another seed gives others, and each of the twelve differs from the node held");
        check (levelled && juce::approximatelyEqual (furthest, 0.5f) && std::abs (nearest - 0.15f) < 0.001f && *std::min_element (perPart.begin(), perPart.end()) >= 1,
               "console: at Reach 0.5 the twelve lie from " + juce::String (nearest, 2) + " to " + juce::String (furthest, 2) + " away, in every part (" + juce::String (perPart[0]) + " oscillators, "
                   + juce::String (perPart[1]) + " filter, " + juce::String (perPart[2]) + " envelopes, " + juce::String (perPart[3]) + " modulation, " + juce::String (perPart[4])
                   + " effects), each levelled to the node held (the furthest off " + juce::String (worstLevel, 2) + " dB)");

        // A firewall: that part's settings are the node held's in every node, and no node lands in it.
        auto held2 = true, elsewhere = true;

        for (int part = 0; part < numParts; ++part)
        {
            settings.firewall = {};
            settings.firewall[(size_t) part] = true;

            for (auto& n : nodesAround (held, settings, renderer))
            {
                held2 = held2 && sameValues (n.patch, held, [part] (const juce::String& id) { return partOf (id) == part; });
                elsewhere = elsewhere && n.part != part;
            }
        }

        settings.firewall = {};
        check (held2 && elsewhere, "console: a firewalled part's settings are unchanged in every node, and no node lies in its sector (each of the five parts in turn)");

        // No reach: every node is the node held.
        settings.reach = 0.0f;
        auto all = true;

        for (auto& n : nodesAround (held, settings, renderer))
            all = all && sameValues (n.patch, held) && n.address == heldNode.address && juce::exactlyEqual (n.distance, 0.0f);

        check (all, "console: at Reach 0 every node is the node held");

        // The settings the console never moves.
        settings.reach = 1.0f;
        auto kept = true;

        for (auto& n : nodesAround (held, settings, renderer))
            kept = kept && sameValues (n.patch, held, [] (const juce::String& id) { return partOf (id) < 0 && id != "ampLevel"; });

        check (kept && partOf ("ampLevel") < 0 && partOf ("glideTime") < 0 && partOf ("macro2") < 0 && partOf ("osc2EnvDecay") == envelopes && partOf ("fxReverbMix") == effects,
               "console: at full Reach no node moves the pan, velocity, voice mode, glide, bend or the macros (the amp's level is set by levelling alone)");

        // A bridge: the twelve along the line to a second patch, each a step nearer it.
        settings.bridge = std::make_shared<Patch> (factory (synth, "Cold Bell"));
        const auto line = nodesAround (held, settings, renderer);
        const auto apart = [&] (const Patch& p, const Patch& to)
        {
            double sum = 0.0;

            for (auto& [id, value] : p.values)
                if (auto* q = synth.parameter (id); q != nullptr && partOf (id) >= 0)
                    sum += std::abs (q->convertTo0to1 ((float) value) - q->convertTo0to1 ((float) to.values.at (id)));

            return sum;
        };
        auto nearer = line.size() == numNodes;

        for (size_t i = 1; i < line.size(); ++i)
            nearer = nearer && apart (line[i].patch, *settings.bridge) < apart (line[i - 1].patch, *settings.bridge) && line[i].distance > line[i - 1].distance;

        check (nearer && apart (line.front().patch, held) < apart (line.back().patch, held),
               "console: bridged to a second patch, each of the twelve is a step nearer to it than the one before (from " + juce::String (apart (line.front().patch, *settings.bridge), 1)
                   + " to " + juce::String (apart (line.back().patch, *settings.bridge), 1) + " of the settings' ranges away)");

        // Addresses: the same for the same settings, another when one changes; four hex digits.
        auto changed = held;
        changed.values["filter1Cutoff"] += 1.0;
        check (addressOf (held) == addressOf (factory (synth, "Abyss Reese")) && addressOf (changed) != addressOf (held) && addressText (addressOf (held)).length() == 4
                   && addressText (addressOf (held)).containsOnly ("0123456789ABCDEF"),
               "console: a patch's address (" + addressText (addressOf (held)) + " for Abyss Reese) is the same for the same settings and another when a setting moves by 1 Hz");
    }

    void testEveryNodeSounds()
    {
        // Round 50 factory patches spread through the list: every node sounds, none clips, and each differs from the node held.
        const auto& all = WaveSynth::factoryPatches();
        Synth synth;
        Renderer renderer;
        Settings settings;
        settings.reach = 0.7f;
        int nodes = 0, silent = 0, clipped = 0, same = 0;
        float quietest = 1.0f, loudest = 0.0f, furthestUnder = 0.0f;
        juce::String quietestName;
        juce::StringArray sameNames;
        const auto started = juce::Time::getMillisecondCounterHiRes();

        for (int k = 0; k < 50; ++k)
        {
            const auto index = 1 + k * ((int) all.size() - 1) / 50;
            synth.loadPatch (index);
            const auto held = capture (synth);
            settings.seed = (juce::uint32) (k + 1);
            const auto heldPeak = renderer.measure (held).peak;

            for (auto& n : nodesAround (held, settings, renderer))
            {
                ++nodes;
                const auto peak = renderer.measure (n.patch).peak;
                // Silent: under -40 dB and over 3 dB under the node held (a slow riser is under -40 dB itself in its first second,
                // and the amp's level only goes up to +6 dB).
                if (peak < 0.01f && peak < heldPeak * 0.708f) ++silent;
                if (heldPeak > 0.0f) furthestUnder = juce::jmin (furthestUnder, juce::Decibels::gainToDecibels (peak / heldPeak));
                if (peak > 1.0f) ++clipped;
                if (n.changes.empty()) { ++same; sameNames.addIfNotAlreadyThere (all[(size_t) index].name); }
                if (peak < quietest) { quietest = peak; quietestName = all[(size_t) index].name; }
                loudest = juce::jmax (loudest, peak);
            }
        }

        const auto seconds = (juce::Time::getMillisecondCounterHiRes() - started) / 1000.0;
        check (nodes == 600 && silent == 0 && clipped == 0 && same == 0,
               "console: all 600 nodes round 50 factory patches at Reach 0.7 sound (the quietest peaks at " + juce::String (juce::Decibels::gainToDecibels (quietest), 1) + " dB, round "
                   + quietestName + "; the furthest under the node it lies round, " + juce::String (-furthestUnder, 1) + " dB, where the amp's level stops at +6 dB) and none clips (the loudest at " + juce::String (juce::Decibels::gainToDecibels (loudest), 1)
                   + " dB); " + (same == 0 ? juce::String ("every one differs") : juce::String (same) + " do not differ") + " from the node held"
                   + (silent > 0 ? ", " + juce::String (silent) + " silent" : juce::String()) + (clipped > 0 ? ", " + juce::String (clipped) + " clipped" : juce::String())
                   + (sameNames.isEmpty() ? juce::String() : " (round " + sameNames.joinIntoString (", ") + ")") + "; a patch's twelve take about " + juce::String (juce::roundToInt (seconds / 50.0 * 1000.0 * 0.5)) + " ms to make");
    }

    juce::AudioBuffer<float> run (Synth& synth, int samples, int note = -1)
    {
        juce::AudioBuffer<float> out (2, samples), block (2, 512);

        for (int at = 0; at < samples; at += 512)
        {
            const auto n = juce::jmin (512, samples - at);
            juce::MidiBuffer midi;
            if (at == 0 && note >= 0) midi.addEvent (juce::MidiMessage::noteOn (1, note, 0.8f), 0);
            block.setSize (2, n, false, false, true);
            block.clear();
            synth.processBlock (block, midi);
            for (int ch = 0; ch < 2; ++ch) out.copyFrom (ch, at, block, ch, 0, n);
        }

        return out;
    }

    void testConsole()
    {
        Synth synth;
        synth.setPlayConfigDetails (0, 2, 48000.0, 512);
        synth.prepareToPlay (48000.0, 512);
        synth.loadPatch ("Machine Sequence");
        auto& console = synth.getConsole();
        int changes = 0;
        console.onChange = [&changes] { ++changes; };
        console.scanNow();
        const auto start = capture (synth);
        const auto startAddress = console.getHeld().address;

        // Hearing a node plays it without touching the patch held; nothing sounds before.
        const auto before = run (synth, 4800);
        console.hear (3);
        const auto heard = run (synth, 48000);
        const auto untouched = addressOf (capture (synth)) == startAddress;
        check (console.getNodes().size() == numNodes && console.getRoute().size() == 1 && before.getMagnitude (0, 4800) == 0.0f && heard.getMagnitude (0, 24000) > 0.01f
                   && heard.getMagnitude (0, 43200, 4800) < heard.getMagnitude (0, 24000) && untouched,
               "console: hearing a node plays it for a moment through the synth's output (peaking at " + juce::String (juce::Decibels::gainToDecibels (heard.getMagnitude (0, 24000)), 1)
                   + " dB) and leaves the patch held as it was");

        // A jump makes the node the synth's patch and the node held, and adds it to the route.
        const auto target = console.getNodes()[5];
        console.jump (5);
        console.scanNow();
        const auto jumped = capture (synth);
        auto landed = true;

        for (auto& [id, value] : target.patch.values)
            if (auto* p = synth.parameter (id))
                landed = landed && std::abs (p->convertTo0to1 ((float) value) - p->convertTo0to1 ((float) jumped.values.at (id))) < 1.0e-4f;

        check (landed && console.getRoute().size() == 2 && console.getRoutePosition() == 1 && console.getHeld().address == addressOf (jumped) && console.getNodes().size() == numNodes
                   && synth.getPatchName() == "Node " + addressText (target.address) && changes >= 3,
               "console: a jump sets the synth's knobs to the node's, names the patch " + synth.getPatchName() + ", holds it and maps twelve more round it");

        // Two more jumps, then a click on the route's first entry: those settings exactly.
        console.jump (0);
        console.scanNow();
        console.jump (7);
        console.scanNow();
        console.goTo (0);
        const auto back = capture (synth);
        auto exact = back.values.size() == start.values.size();

        for (auto& [id, value] : start.values)
            exact = exact && juce::exactlyEqual (value, back.values.at (id));

        console.goTo (2);
        check (exact && console.getRoute().size() == 4 && addressOf (back) == startAddress && console.getRoutePosition() == 2 && addressOf (capture (synth)) == console.getRoute()[2].address,
               "console: after three jumps a click on the route's first entry returns its settings exactly, and the route keeps all four to go forward again");

        // A knob turned outside the console: the patch as it is becomes the node held, a new entry on the route.
        console.scanNow();
        const auto stillSame = ! console.sync();
        auto* cutoff = synth.parameter ("filter1Cutoff");
        cutoff->setValueNotifyingHost (cutoff->getValue() * 0.5f);
        const auto took = console.sync();
        check (stillSame && took && console.getRoute().size() == 5 && console.getHeld().address == addressOf (capture (synth)),
               "console: a knob turned by hand makes the patch as it is the node held, a new entry on the route; nothing changed leaves it alone");

        // Firewalls, Reach and Rescan from the console.
        console.setFirewall (filter, true);
        console.setReach (1.0f);
        console.scanNow();
        auto walled = true;
        const auto heldNow = console.getHeld().patch;
        std::vector<juce::uint32> addresses;

        for (auto& n : console.getNodes())
        {
            addresses.push_back (n.address);
            walled = walled && n.part != filter;

            for (auto& [id, value] : n.patch.values)
                if (partOf (id) == filter)
                    walled = walled && juce::exactlyEqual (value, heldNow.values.at (id));
        }

        console.rescan();
        console.scanNow();
        auto others = false;

        for (size_t i = 0; i < console.getNodes().size(); ++i)
            others = others || console.getNodes()[i].address != addresses[i];

        check (walled && others, "console: with the filter firewalled no node moves it or lies in its sector, and Rescan gives another twelve");
        console.onChange = nullptr;
    }

    /** Whether a component and everything it sits in, up to the window, is set to show. */
    bool shows (const juce::Component* c)
    {
        for (; c != nullptr; c = c->getParentComponent())
            if (! c->isVisible())
                return c->getParentComponent() == nullptr;   // the window itself isn't on the screen in a test

        return true;
    }

    void testView()
    {
        Theme::apply (Theme::make ("Dark", "Blue"));
        Synth synth;
        synth.setPlayConfigDetails (0, 2, 48000.0, 512);
        synth.prepareToPlay (48000.0, 512);
        synth.loadPatch ("Abyss Reese");
        std::unique_ptr<juce::AudioProcessorEditor> made (synth.createEditorAndMakeActive());
        auto* window = dynamic_cast<WaveSynthEditor*> (made.get());

        if (window == nullptr || window->getConsoleView() == nullptr)
        {
            check (false, "console: the synth's window has a Trace view");
            return;
        }

        auto& view = *window->getConsoleView();
        auto& console = synth.getConsole();
        view.setMotion (false);
        window->showView (1);
        console.scanNow();
        const auto shown = shows (&view) && window->knobFor ("filter1Cutoff") != nullptr && ! shows (window->knobFor ("filter1Cutoff"));
        auto drawn = true;

        for (auto scale : { 0.7f, 1.0f, 1.6f })
        {
            window->setSize (juce::roundToInt (WaveSynthEditor::designWidth * scale), juce::roundToInt (WaveSynthEditor::designHeight * scale));
            const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
            drawn = drawn && image.getWidth() == window->getWidth() * 2;
        }

        window->setSize (WaveSynthEditor::designWidth, WaveSynthEditor::designHeight);

        // Every node is drawn inside the scope, clear of the others, in its part's sector.
        auto placed = true;
        float closest = 1000.0f;

        for (int i = 0; i < numNodes; ++i)
        {
            const auto p = view.nodeCentre (i), c = view.nodeCentre (-1);
            auto angle = std::atan2 (p.y - c.y, p.x - c.x) + juce::MathConstants<float>::halfPi;   // 0 at twelve o'clock, clockwise
            while (angle < -juce::MathConstants<float>::pi / (float) numParts) angle += juce::MathConstants<float>::twoPi;
            const auto sector = (int) ((angle + juce::MathConstants<float>::pi / (float) numParts) / (juce::MathConstants<float>::twoPi / (float) numParts)) % numParts;
            placed = placed && sector == console.getNodes()[(size_t) i].part && view.getLocalBounds().toFloat().contains (p);

            for (int j = i + 1; j < numNodes; ++j)
                closest = juce::jmin (closest, p.getDistanceFrom (view.nodeCentre (j)));
        }

        // A click on a node selects and hears it; the panel's Jump jumps; a click on the route goes back; a firewall chip closes a sector.
        const auto click = [&view] (juce::Point<float> at, int clicks = 1)
        {
            const auto event = juce::MouseEvent (juce::Desktop::getInstance().getMainMouseSource(), at, {}, 0.5f, 0, 0, 0, 0, &view, &view, juce::Time::getCurrentTime(), at, juce::Time::getCurrentTime(), clicks, false);
            if (clicks == 2) view.mouseDoubleClick (event); else view.mouseDown (event);
        };
        const auto first = console.getHeld().address;
        click (view.nodeCentre (4));
        const auto selected = view.shownNode() == 4;
        const auto target = console.getNodes()[4].address;
        juce::ignoreUnused (target);
        (void) window->createComponentSnapshot (window->getLocalBounds(), true, 1.0f);   // the panel lays out its chips as it draws
        click (view.jumpBounds().getCentre());
        console.scanNow();
        const auto jumped = console.getRoute().size() == 2 && console.getHeld().address != first;
        click (view.nodeCentre (2), 2);
        console.scanNow();
        const auto twice = console.getRoute().size() == 3;
        (void) window->createComponentSnapshot (window->getLocalBounds(), true, 1.0f);
        click (view.routeBounds (0).getCentre());
        const auto wentBack = console.getRoutePosition() == 0 && console.getHeld().address == first;
        click (view.firewallBounds (envelopes).getCentre());
        console.scanNow();
        auto closed = console.getSettings().firewall[envelopes];

        for (auto& n : console.getNodes())
            closed = closed && n.part != envelopes;

        check (shown && drawn && placed && closest > 20.0f,
               "console: the Trace view takes the knobs' place and draws at 70 %, 100 % and 160 %; each of the twelve nodes is drawn in its part's sector, the nearest two "
                   + juce::String (juce::roundToInt (closest)) + " points apart");
        check (selected && jumped && twice && wentBack && closed && ! view.isMoving(),
               "console: a click on a node shows it in the panel, Jump and a double-click jump (the route grows to three), a click on the route's first entry goes back to it, and a firewall chip closes its sector; with motion off nothing moves");

        // A node's signature and address follow its settings: the same for the same, others when a setting moves.
        Renderer renderer;
        auto patch = capture (synth);
        const auto one = nodeOf (patch, renderer), same = nodeOf (patch, renderer);
        patch.values["filter1Cutoff"] = 300.0;
        const auto darker = nodeOf (patch, renderer);
        check (one.address == same.address && one.signature.band == same.signature.band && darker.address != one.address && darker.signature.band != one.signature.band
                   && darker.signature.brightness < one.signature.brightness,
               "console: a node's signature and address are the same for the same settings and change when a setting does (the filter closed to 300 Hz: brightness "
                   + juce::String (one.signature.brightness, 2) + " to " + juce::String (darker.signature.brightness, 2) + ")");
        window->showView (0);
        check (! shows (&view) && shows (window->knobFor ("filter1Cutoff")) && ! shows (window->knobFor ("osc1EnvAttack")), "console: Knobs brings the synth's controls back, each page showing what it showed");

        // The window offers Decrypt, not Trace: the header's second page shows Decrypt's screen and hides the scope.
        window->showView (2);
        const auto decryptShown = window->getDecryptView() != nullptr && shows (window->getDecryptView()) && ! shows (&view) && ! shows (window->knobFor ("filter1Cutoff"));
        window->showView (0);
        check (decryptShown, "console: the synth's window offers Decrypt in Trace's place (Trace's scope is not offered, as 0.20.0 withdrew it)");
    }
}

void runSignalConsoleTests()
{
    testNodes();
    testEveryNodeSounds();
    testConsole();
    testView();
}

void saveSignalConsoleWindow (const juce::File& folder)
{
    folder.createDirectory();
    Synth synth;
    synth.setPlayConfigDetails (0, 2, 48000.0, 512);
    synth.prepareToPlay (48000.0, 512);
    synth.loadPatch ("Abyss Reese");
    std::unique_ptr<juce::AudioProcessorEditor> made (synth.createEditorAndMakeActive());
    auto* window = dynamic_cast<WaveSynthEditor*> (made.get());

    if (window == nullptr || window->getConsoleView() == nullptr)
        return;

    auto& console = synth.getConsole();
    console.setReach (0.6f);
    console.scanNow();
    console.jump (2);
    console.scanNow();
    console.jump (9);
    console.setFirewall (filter, true);
    console.scanNow();
    window->getConsoleView()->setMotion (false);
    window->showView (1);
    console.scanNow();

    for (auto theme : { "Dark", "Light" })
    {
        Theme::apply (Theme::make (theme, "Blue"));
        const auto image = window->createComponentSnapshot (window->getLocalBounds(), true, 2.0f);
        auto file = folder.getChildFile (juce::String ("console-") + theme + ".png");
        file.deleteFile();
        juce::FileOutputStream out (file);
        juce::PNGImageFormat().writeImageToStream (image, out);
        std::cout << file.getFullPathName() << std::endl;
    }
}
