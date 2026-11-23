#include "DecryptView.h"
#include "SoundDrops.h"
#include "SignalConsoleView.h"
#include "Theme.h"

namespace SoundDrops
{
    std::function<bool (const juce::var&)> isSound;
    std::function<std::optional<Sound> (const juce::var&, double)> soundOf;
}

using namespace SignalConsoleDrawing;
using namespace Decrypt;

namespace
{
    const juce::StringArray& sectorLabels()
    {
        static const juce::StringArray n { "OSCILLATORS", "FILTER", "ENVELOPES", "MODULATION", "EFFECTS" };
        return n;
    }

    /** The signal's colour, the theme's second colour beside the accent. */
    juce::Colour signal()       { return Theme::warn; }

    /** How far out a lock is drawn, as a share of the scope's radius: 75 % on the inner ring (a third),
        25 % on the outer. */
    float reachOf (float lock)  { return juce::jlimit (0.2f, 1.04f, (100.0f - lock) / 75.0f); }

    juce::String timeText (double seconds)
    {
        const auto s = juce::roundToInt (seconds);
        return juce::String (s / 60) + ":" + juce::String (s % 60).paddedLeft ('0', 2);
    }

    /** The part a candidate's settings differ from the best's in most (by their share of each
        setting's range); the effects are never matched, so never it. */
    int partApart (const Patch& a, const Patch& b, Synth& synth)
    {
        std::array<float, SignalConsole::numParts> apart {};

        for (auto& [id, value] : a.values)
        {
            const auto part = SignalConsole::partOf (id);
            const auto other = b.values.find (id);
            auto* p = synth.parameter (id);

            if (part < 0 || part == SignalConsole::effects || other == b.values.end() || p == nullptr)
                continue;

            apart[(size_t) part] += std::abs (p->convertTo0to1 ((float) value) - p->convertTo0to1 ((float) other->second));
        }

        return (int) (std::max_element (apart.begin(), apart.begin() + SignalConsole::effects) - apart.begin());
    }
}

//==============================================================================
DecryptView::DecryptView (Synth& s) : synth (s)
{
    motion = ! SignalConsole::reduceMotion();
    setTooltip ("Decrypt: drop a recording of one note here and Torqwave Synth searches for a patch that plays like it");
    words.setTextToShowWhenEmpty ("What is it? (\"a reese\", \"match the body, not the click\")", Theme::faintText);
    words.setTooltip ("For Ask Torq's model, which suggests where to start and what to change; the search scores whatever it suggests and keeps only what sounds closer");
    words.setFont (mono (12.0f));
    addChildComponent (words);
}

DecryptView::~DecryptView()
{
    alive->store (false);
    renders.removeAllJobs (true, 4000);

    if (search != nullptr && ! restored)
        stop();
}

bool DecryptView::isInterestedInFileDrag (const juce::StringArray& files)
{
    return files.size() == 1 && juce::File (files[0]).hasFileExtension ("wav;aif;aiff;flac;ogg;mp3;m4a;caf");
}

void DecryptView::fileDragEnter (const juce::StringArray&, int, int)    { dragOver = true; repaint(); }
void DecryptView::fileDragExit (const juce::StringArray&)               { dragOver = false; repaint(); }

void DecryptView::filesDropped (const juce::StringArray& files, int, int)
{
    dragOver = false;
    decryptFile (juce::File (files[0]));
}

bool DecryptView::isInterestedInDragSource (const SourceDetails& d)   { return SoundDrops::accepts (d.description); }
void DecryptView::itemDragEnter (const SourceDetails&)                { dragOver = true; repaint(); }
void DecryptView::itemDragExit (const SourceDetails&)                 { dragOver = false; repaint(); }

void DecryptView::itemDropped (const SourceDetails& d)
{
    dragOver = false;

    // Ten seconds, as from a file: the note is found in it.
    if (const auto sound = SoundDrops::take (d.description, 10.0))
        decrypt (sound->audio, sound->sampleRate, sound->name);
    else
        problem = "That sound couldn't be read.";

    repaint();
}

bool DecryptView::decryptFile (const juce::File& file)
{
    juce::AudioFormatManager formats;
    formats.registerBasicFormats();
    const std::unique_ptr<juce::AudioFormatReader> reader (formats.createReaderFor (file));

    if (reader == nullptr || reader->lengthInSamples <= 0)
    {
        problem = file.getFileName() + " couldn't be read as audio.";
        repaint();
        return false;
    }

    // Up to ten seconds: the note is found in it, and four seconds of it from there are matched.
    const auto length = (int) juce::jmin (reader->lengthInSamples, (juce::int64) (reader->sampleRate * 10.0));
    juce::AudioBuffer<float> audio ((int) juce::jmin (2u, reader->numChannels), length);
    reader->read (&audio, 0, length, 0, true, audio.getNumChannels() > 1);
    return decrypt (audio, reader->sampleRate, file.getFileNameWithoutExtension());
}

bool DecryptView::decrypt (const juce::AudioBuffer<float>& audio, double sampleRate, const juce::String& name)
{
    auto target = prepare (audio, sampleRate);

    if (target.problem.isNotEmpty())
    {
        problem = target.problem;
        repaint();
        return false;
    }

    // A search under way gives its patch back first.
    if (search != nullptr && ! restored)
        stop();

    clipName = name;
    passes = 0;
    chain.clear();
    begin (std::move (target), {});
    return true;
}

bool DecryptView::canGoAgain() const
{
    return search != nullptr && ! search->isRunning() && ! progress.best.empty();
}

bool DecryptView::again()
{
    if (! canGoAgain())
        return false;

    // From the one chosen, if one was: taken, or on the keys. Otherwise from all twelve.
    std::vector<SignalConsole::Patch> from;
    Link link;

    if (chosen.has_value())
    {
        from.push_back (*chosen);
        link = { *chosen, chosenLock, {} };
    }
    else if (juce::isPositiveAndBelow (onKeys, (int) progress.best.size()))
    {
        from.push_back (progress.best[(size_t) onKeys].patch);
        link = { progress.best[(size_t) onKeys].patch, progress.best[(size_t) onKeys].lock, {} };
    }
    else
    {
        for (auto& c : progress.best)
            from.push_back (c.patch);

        link = { progress.best.front().patch, progress.best.front().lock, {} };
    }

    // The chain keeps where this pass went on from, to hear beside what comes next.
    addLink (link.patch, link.lock);

    // Stop then gives back the patch from before the first pass, or the one taken (begin).
    auto target = search->getTarget();
    begin (std::move (target), std::move (from));
    return true;
}

void DecryptView::begin (Decrypt::Target target, std::vector<SignalConsole::Patch> resume)
{
    problem = {};
    ended = {};
    offer = {};
    onKeys = -1;
    hovered = -2;
    chosen.reset();

    // What Stop gives back: the patch the synth had before any of this. A pass carried on while a
    // candidate is still on the keys (not taken) keeps the patch from before it.
    if (search == nullptr || restored)
        before = SignalConsole::capture (synth);

    restored = false;

    if (resume.empty())
    {
        shownLock = 0.0f;
        places.clear();
        shownRadius.clear();
    }

    ++passes;
    Options options;
    options.factoryTablesOnly = tablesOnly;
    options.words = words.getText().trim();
    options.advisor = makeAdvisor();
    options.seed = (SignalConsole::addressOf (before) + (juce::uint32) (passes - 1) * 0x9e3779b9u) | 1u;   // other variations each pass
    options.resume = std::move (resume);
    options.untilSettled = true;        // it goes on while it comes closer, and says when it no longer does
    search = std::make_unique<Search> (std::move (target), std::move (options));
    progress = {};
    search->onChange = [this]
    {
        progress = search->getProgress();
        growChain();
        layout();
        repaint();

        if (progress.finished && ! restored)
            ended = progress.stopped ? "STOPPED" : progress.noCloser ? "NO CLOSER FOUND" : "LOCKED";
    };
    search->start();
    startTimerHz (30);
    repaint();
}

void DecryptView::stop()
{
    if (search == nullptr)
        return;

    search->stop();

    if (! restored)
        SignalConsole::apply (synth, before);

    restored = true;
    onKeys = -1;
    finish ("STOPPED");
}

void DecryptView::take()
{
    const auto* c = shown();

    if (search == nullptr || c == nullptr)
        return;

    search->stop();
    chosen = c->patch;
    chosenLock = c->lock;
    auto patch = c->patch;
    patch.name = "Node " + SignalConsole::addressText (c->address);
    SignalConsole::apply (synth, patch);
    restored = true;
    onKeys = -1;
    finish ("TAKEN");

    // Ask Torq's model may offer a better name, to use or refuse; the patch is named for its address until then.
    if (auto advisor = makeAdvisor())
    {
        Advisor::Brief brief;
        brief.target = describe (search->getTarget());
        brief.best = describeSettings (synth, patch);
        brief.words = words.getText().trim();
        brief.lock = c->lock;
        offerFor = c->address;
        renders.addJob ([advisor, brief, address = c->address, weak = std::weak_ptr<std::atomic<bool>> (alive), safe = juce::Component::SafePointer<DecryptView> (this)]
        {
            auto name = advisor->name (brief);

            if (name.name.trim().isEmpty())
                return;

            juce::MessageManager::callAsync ([name, address, weak, safe]
            {
                if (const auto a = weak.lock(); a != nullptr && a->load() && safe != nullptr && safe->offerFor == address)
                {
                    safe->offer = name;
                    safe->layout();
                    safe->repaint();
                }
            });
        });
    }
}

void DecryptView::useOfferedName (bool use)
{
    if (use && offer.name.isNotEmpty())
    {
        synth.setPatchName (offer.name.trim().substring (0, 40));

        for (int m = 0; m < WaveSynth::numMacros; ++m)
            if (offer.macros[(size_t) m].trim().isNotEmpty())
                synth.setMacroName (m, offer.macros[(size_t) m].trim().substring (0, 16));
    }

    offer = {};
    layout();
    repaint();
}

void DecryptView::finish (const juce::String& how)
{
    ended = how;
    stopTimer();
    progress = search != nullptr ? search->getProgress() : Progress();
    layout();
    repaint();
}

const Candidate* DecryptView::shown() const
{
    const auto& best = progress.best;

    if (juce::isPositiveAndBelow (hovered, (int) best.size()))
        return &best[(size_t) hovered];

    if (juce::isPositiveAndBelow (onKeys, (int) best.size()))
        return &best[(size_t) onKeys];

    return best.empty() ? nullptr : &best.front();
}

void DecryptView::hear (int candidate)
{
    if (search == nullptr)
        return;

    auto& console = synth.getConsole();

    if (candidate < 0)
    {
        console.play (search->getTarget().audio, rate);
        return;
    }

    if (juce::isPositiveAndBelow (candidate, (int) progress.best.size()))
        hearPatch (progress.best[(size_t) candidate].patch);
}

void DecryptView::addLink (const SignalConsole::Patch& patch, float lock)
{
    if (! chain.empty() && SignalConsole::addressOf (chain.back().patch) == SignalConsole::addressOf (patch))
        return;

    chain.push_back ({ patch, lock, {} });
}

void DecryptView::growChain()
{
    // A step each time the best comes a point nearer (and the first best, and the last when it ends).
    if (progress.best.empty())
        return;

    const auto& best = progress.best.front();

    if (chain.empty() || best.lock >= chain.back().lock + 1.0f || (progress.finished && ! progress.stopped && best.lock > chain.back().lock))
        addLink (best.patch, best.lock);
}

void DecryptView::playChain()
{
    if (chainPlayer.isTimerRunning() || search == nullptr || chain.empty())
    {
        chainPlayer.stopTimer();
        repaint();
        return;
    }

    chainPlayer.next = -1;
    chainPlayer.timerCallback();
    repaint();
}

void DecryptView::ChainPlayer::timerCallback()
{
    if (view.search == nullptr || next >= (int) view.chain.size())
    {
        stopTimer();
        view.repaint();
        return;
    }

    if (next < 0)
        view.hear (-1);
    else
        view.hearPatch (view.chain[(size_t) next].patch);

    ++next;
    // Each for as long as the note, and a breath.
    startTimer (juce::roundToInt (1000.0 * (double) view.search->getTarget().audio.getNumSamples() / rate) + 350);
    view.repaint();
}

void DecryptView::goBackTo (int index)
{
    if (search == nullptr || ! juce::isPositiveAndBelow (index, (int) chain.size()))
        return;

    chainPlayer.stopTimer();
    search->stop();
    const auto link = chain[(size_t) index];
    chain.resize ((size_t) index + 1);

    if (restored)       // Stop or Take had settled it: what the synth holds now is what Stop gives back
        before = SignalConsole::capture (synth);

    auto patch = link.patch;
    patch.name = "Node " + SignalConsole::addressText (SignalConsole::addressOf (link.patch));
    SignalConsole::apply (synth, patch);
    chosen = link.patch;
    chosenLock = link.lock;
    restored = false;
    onKeys = -1;
    finish ("STEP " + juce::String (index + 1));
}

void DecryptView::hearPatch (const SignalConsole::Patch& heard)
{
    if (search == nullptr)
        return;

    // Rendered off the message thread, as the search renders it, then played.
    renders.removeAllJobs (false, 0);
    // A copy of the target: Again or a new clip replaces the search while a render may still run.
    renders.addJob ([patch = heard, target = std::make_shared<const Target> (search->getTarget()), safe = juce::Component::SafePointer<DecryptView> (this),
                     weak = std::weak_ptr<std::atomic<bool>> (alive)]
    {
        Synth player;
        player.setPlayConfigDetails (0, 2, rate, 512);
        player.setNonRealtime (true);
        auto audio = std::make_shared<juce::AudioBuffer<float>> (render (player, patch, *target));
        const auto peak = audio->getMagnitude (0, audio->getNumSamples());

        if (peak > 1.0e-6f)
            audio->applyGain (0.5f / peak);   // as loud as the signal is played

        juce::MessageManager::callAsync ([audio, safe, weak]
        {
            if (const auto a = weak.lock(); a != nullptr && a->load() && safe != nullptr)
                safe->synth.getConsole().play (*audio, rate);
        });
    });
}

//==============================================================================
void DecryptView::timerCallback()
{
    if (search == nullptr)
        return;

    // The figures run toward the search's own; the sweep turns while it searches.
    const auto target = progress.best.empty() ? 0.0f : progress.best.front().lock;
    shownLock = motion ? shownLock + (target - shownLock) * 0.18f : target;

    if (std::abs (shownLock - target) < 0.05f)
        shownLock = target;

    if (motion && search->isRunning())
        sweep = std::fmod (sweep + 0.02f, tau);

    auto drifting = false;

    for (size_t i = 0; i < shownRadius.size() && i < progress.best.size(); ++i)
    {
        const auto want = reachOf (progress.best[i].lock);
        shownRadius[i] = motion ? shownRadius[i] + (want - shownRadius[i]) * 0.15f : want;
        drifting = drifting || std::abs (shownRadius[i] - want) > 0.002f;
    }

    if (drifting)
        layout();

    if (! search->isRunning() && ! drifting && juce::exactlyEqual (shownLock, target))
        stopTimer();

    repaint();
}

void DecryptView::resized()
{
    const auto b = getLocalBounds().toFloat();
    panel = b.withTrimmedLeft (b.getWidth() - 320.0f).reduced (4.0f, 8.0f);
    const auto scope = b.withTrimmedRight (328.0f);
    radius = juce::jmin ((scope.getWidth() - 110.0f) * 0.5f, (scope.getHeight() - 54.0f - 64.0f - 24.0f) * 0.5f);
    centre = { scope.getCentreX(), 58.0f + radius };
    const auto y = b.getBottom() - 30.0f;
    chooseChip = { scope.getX() + 24.0f, y - 12.0f, 150.0f, 24.0f };
    tablesChip = { chooseChip.getRight() + 10.0f, y - 12.0f, 230.0f, 24.0f };
    words.setBounds (juce::Rectangle<float> (tablesChip.getRight() + 12.0f, y - 12.0f, juce::jmax (80.0f, scope.getRight() - tablesChip.getRight() - 36.0f), 24.0f).toNearestInt());
    words.setVisible (makeAdvisor() != nullptr);
    layout();
}

void DecryptView::layout()
{
    const auto& best = progress.best;
    const auto k = radius / 236.0f;

    if (shownRadius.size() != best.size())
    {
        // New candidates start at the scope's edge and drift in; with less motion they are placed at once.
        shownRadius.resize (best.size(), 1.04f);

        if (! motion)
            for (size_t i = 0; i < best.size(); ++i)
                shownRadius[i] = reachOf (best[i].lock);
    }

    // Each in the sector of the part it differs most from the best in; the best in the part least settled.
    sectorOf.assign (best.size(), 0);

    if (! best.empty())
    {
        sectorOf[0] = (int) (std::min_element (progress.settled.begin(), progress.settled.begin() + SignalConsole::effects) - progress.settled.begin());

        for (size_t i = 1; i < best.size(); ++i)
            sectorOf[i] = partApart (best[i].patch, best[0].patch, synth);
    }

    places.assign (best.size(), centre);
    std::array<int, SignalConsole::numParts> inSector {}, placed {};

    for (auto s : sectorOf)
        ++inSector[(size_t) s];

    for (size_t i = 0; i < best.size(); ++i)
    {
        const auto s = sectorOf[i];
        const auto count = inSector[(size_t) s], n = placed[(size_t) s]++;
        const auto angle = sectorMiddle (s) + (((float) n + 0.5f) / (float) count - 0.5f) * 0.8f * tau / (float) SignalConsole::numParts;
        const auto r = juce::jmax (radius * shownRadius[i], 52.0f * k + 20.0f * k);
        places[i] = centre + juce::Point<float> (std::cos (angle), std::sin (angle)) * r;
    }

    // The offer of a name, under the panel's figures.
    useChip = refuseChip = {};

    if (offer.name.isNotEmpty())
    {
        useChip = { panel.getX() + 16.0f, panel.getBottom() - 92.0f, 64.0f, 24.0f };
        refuseChip = { useChip.getRight() + 8.0f, useChip.getY(), 64.0f, 24.0f };
    }

    // The chain, above the buttons: its last seven links, oldest first.
    {
        playChip = chain.empty() ? juce::Rectangle<float>() : juce::Rectangle<float> (panel.getX() + 16.0f, panel.getBottom() - 76.0f, 44.0f, 22.0f);
        const auto shownLinks = juce::jmin (6, (int) chain.size());
        const auto first = (int) chain.size() - shownLinks;

        for (int i = 0; i < (int) chain.size(); ++i)
            chain[(size_t) i].at = i < first ? juce::Rectangle<float>()
                                             : juce::Rectangle<float> (playChip.getRight() + 6.0f + (float) (i - first) * 39.0f, panel.getBottom() - 76.0f, 36.0f, 22.0f);

        if (! chain.empty() && offer.name.isNotEmpty())
        {
            useChip.translate (0.0f, -34.0f);
            refuseChip.translate (0.0f, -34.0f);
        }
    }

    stopChip = { panel.getX() + 16.0f, panel.getBottom() - 44.0f, 84.0f, 26.0f };
    takeChip = { stopChip.getRight() + 8.0f, stopChip.getY(), 108.0f, 26.0f };
    againChip = { takeChip.getRight() + 8.0f, stopChip.getY(), 72.0f, 26.0f };
}

juce::Point<float> DecryptView::candidateCentre (int index) const
{
    return juce::isPositiveAndBelow (index, (int) places.size()) ? places[(size_t) index] : centre;
}

int DecryptView::candidateAt (juce::Point<float> p) const
{
    const auto k = radius / 236.0f;

    for (int i = 0; i < (int) places.size(); ++i)
        if (p.getDistanceFrom (places[(size_t) i]) < 22.0f * k + 4.0f)
            return i;

    return p.getDistanceFrom (centre) < 46.0f * k ? -1 : -2;
}

void DecryptView::mouseMove (const juce::MouseEvent& e)
{
    const auto now = candidateAt (e.position);

    if (now != hovered)
    {
        hovered = now;
        repaint();
    }
}

void DecryptView::mouseExit (const juce::MouseEvent&)
{
    hovered = -2;
    repaint();
}

void DecryptView::mouseDown (const juce::MouseEvent& e)
{
    const auto p = e.position;

    if (chooseChip.contains (p))
    {
        chooser = std::make_unique<juce::FileChooser> ("Choose one note to decrypt", juce::File(), "*.wav;*.aif;*.aiff;*.flac;*.ogg;*.mp3;*.m4a;*.caf");
        chooser->launchAsync (juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles,
                              [safe = juce::Component::SafePointer<DecryptView> (this)] (const juce::FileChooser& c)
                              {
                                  if (safe != nullptr && c.getResult().existsAsFile())
                                      safe->decryptFile (c.getResult());
                              });
        return;
    }

    if (tablesChip.contains (p))
    {
        tablesOnly = ! tablesOnly;
        repaint();
        return;
    }

    if (useChip.contains (p) || refuseChip.contains (p))
    {
        useOfferedName (useChip.contains (p));
        return;
    }

    if (playChip.contains (p) && ! chain.empty())
    {
        playChain();
        return;
    }

    for (auto& link : chain)
        if (link.at.contains (p))
        {
            chainPlayer.stopTimer();
            hearPatch (link.patch);
            return;
        }

    if (againChip.contains (p) && canGoAgain())
    {
        again();
        return;
    }

    if (search != nullptr && ! restored && stopChip.contains (p))
    {
        stop();
        return;
    }

    if (search != nullptr && ! restored && takeChip.contains (p))
    {
        take();
        return;
    }

    const auto at = candidateAt (p);

    if (at >= -1)
        hear (at);
}

void DecryptView::mouseDoubleClick (const juce::MouseEvent& e)
{
    for (int i = 0; i < (int) chain.size(); ++i)
        if (chain[(size_t) i].at.contains (e.position))
        {
            goBackTo (i);
            return;
        }

    const auto at = candidateAt (e.position);

    // On the keys: the synth plays the candidate until Stop gives the patch back or Take keeps it.
    if (search != nullptr && ! restored && juce::isPositiveAndBelow (at, (int) progress.best.size()))
    {
        auto patch = progress.best[(size_t) at].patch;
        patch.name = "Node " + SignalConsole::addressText (progress.best[(size_t) at].address);
        SignalConsole::apply (synth, patch);
        onKeys = at;
        repaint();
    }
}

juce::String DecryptView::getTooltip()
{
    const auto p = getMouseXYRelative().toFloat();

    if (chooseChip.contains (p))   return "Choose a recording of one note to decrypt";
    if (tablesChip.contains (p))   return "Tables only from the factory: the patch is made of the synth's own waves, never given the clip's own sampled table";
    for (size_t i = 0; i < chain.size(); ++i)
        if (chain[i].at.contains (p))
            return "Step " + juce::String ((int) i + 1) + " of the chain, lock " + juce::String (juce::roundToInt (chain[i].lock))
                   + " %: click to hear it, double-click to go back to it (it goes on the keys, and Again goes on from it)";

    if (playChip.contains (p) && ! chain.empty())
        return "Play the chain: the signal, then each step in turn, to hear the patch closing in. Click again to stop";

    if (againChip.contains (p))    return "Again: search on from the candidate you chose (taken, or double-clicked onto the keys), or from all twelve if none, in smaller steps. Press it as often as it keeps getting closer";
    if (stopChip.contains (p))     return "Stop: end the search and give the synth back the patch it had";
    if (takeChip.contains (p))     return "Take: keep the candidate shown as the synth's patch";

    const auto at = candidateAt (p);

    if (at == -1)                  return "The signal: click to hear it";
    if (at >= 0)                   return "A candidate: click to hear it, double-click to play it on the keys";

    return SettableTooltipClient::getTooltip();
}

//==============================================================================
void DecryptView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base.darker (0.25f));
    g.setColour (Theme::overlay.withAlpha (0.06f));

    for (float x = 14.0f; x < (float) getWidth(); x += 28.0f)
        for (float y = 14.0f; y < (float) getHeight(); y += 28.0f)
            g.fillRect (x, y, 1.5f, 1.5f);

    // What Decrypt is for, in a line.
    text (g, "DECRYPT", 24.0f, 20.0f, 12.0f, signal(), juce::Justification::centredLeft, true);
    text (g, "finds a Torqwave Synth patch that plays like a recording of one note. Drop the note on the scope.", 92.0f, 20.0f, 12.0f, Theme::dimText);

    drawScope (g);
    drawControls (g);
    drawPanel (g);
}

void DecryptView::drawScope (juce::Graphics& g)
{
    const auto k = radius / 236.0f;
    const auto& best = progress.best;
    const auto running = search != nullptr && search->isRunning();

    // The rings: 75, 50 and 25 % lock, and the edge.
    for (auto lock : { 75.0f, 50.0f, 25.0f })
    {
        const auto r = radius * reachOf (lock);
        g.setColour (Theme::line);
        g.drawEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f, 1.0f);
        text (g, juce::String ((int) lock) + "%", centre.x + 6.0f, centre.y - r - 9.0f, 11.0f, Theme::dimText);
    }

    g.setColour (Theme::lineStrong);
    g.drawEllipse (centre.x - radius * 1.04f, centre.y - radius * 1.04f, radius * 2.08f, radius * 2.08f, 1.0f);

    // The sectors and their names; the effects are off while matching.
    for (int part = 0; part < SignalConsole::numParts; ++part)
    {
        const auto from = sectorMiddle (part) - tau / (2.0f * (float) SignalConsole::numParts);
        g.setColour (Theme::line);
        g.drawLine (centre.x + std::cos (from) * radius * 0.25f, centre.y + std::sin (from) * radius * 0.25f,
                    centre.x + std::cos (from) * radius * 1.04f, centre.y + std::sin (from) * radius * 1.04f, 1.0f);
        const auto middle = sectorMiddle (part);
        const auto off = part == SignalConsole::effects;
        text (g, sectorLabels()[part], centre.x + std::cos (middle) * (radius + 26.0f), centre.y + std::sin (middle) * (radius + 19.0f), 11.5f,
              off ? Theme::faintText : Theme::dimText, juce::Justification::centred, true);
    }

    // The sweep, while it searches.
    if (running || search == nullptr)
    {
        const auto at = motion && running ? sweep : -tau * 0.25f;
        juce::Path wedge;
        wedge.addPieSegment (centre.x - radius, centre.y - radius, radius * 2.0f, radius * 2.0f, at - 0.5f + tau * 0.25f, at + tau * 0.25f, 0.0f);
        g.setColour (signal().withAlpha (0.07f));
        g.fillPath (wedge);
        g.setColour (signal().withAlpha (running ? 0.6f : 0.25f));
        g.drawLine (centre.x, centre.y, centre.x + std::cos (at) * radius, centre.y + std::sin (at) * radius, 1.0f);
    }

    if (search == nullptr)
    {
        // Nothing to crack yet: a place to drop it.
        juce::Path ring, dashed;
        const auto r = 46.0f * k;
        ring.addEllipse (centre.x - r, centre.y - r, r * 2.0f, r * 2.0f);
        const float dashes[] = { 4.0f, 5.0f };
        juce::PathStrokeType (1.5f).createDashedStroke (dashed, ring, dashes, 2);
        g.setColour ((dragOver ? signal() : Theme::lineStrong).withAlpha (dragOver ? 1.0f : 0.8f));
        g.fillPath (dashed);
        text (g, dragOver ? "RELEASE TO DECRYPT" : "DROP ONE NOTE", centre.x, centre.y, 12.0f, dragOver ? signal() : Theme::dimText, juce::Justification::centred, true);
        text (g, "held or plucked, up to 4 s", centre.x, centre.y + r + 16.0f, 11.0f, Theme::faintText, juce::Justification::centred);

        if (problem.isNotEmpty())
            text (g, problem, centre.x, centre.y + r + 36.0f, 11.5f, Theme::danger, juce::Justification::centred);

        return;
    }

    // The twelve: a dashed line toward the middle, the signature, the lock under it.
    const auto nodeRadius = 15.0f * k;
    const auto* show = shown();

    for (size_t i = 0; i < best.size() && i < places.size(); ++i)
    {
        const auto p = places[i];
        const auto toward = (centre - p) / juce::jmax (1.0f, p.getDistanceFrom (centre));
        juce::Path link, dashed;
        link.startNewSubPath (p + toward * (nodeRadius + 4.0f));
        link.lineTo (p + toward * (nodeRadius + 34.0f * k));
        const float dashes[] = { 2.0f, 5.0f };
        juce::PathStrokeType (1.0f).createDashedStroke (dashed, link, dashes, 2);
        g.setColour (signal().withAlpha (0.35f));
        g.fillPath (dashed);

        const auto on = &best[i] == show;
        drawSignature (g, p, nodeRadius, best[i].signature, on ? Theme::accent : Theme::accent.interpolatedWith (Theme::text, 0.55f), on ? 1.0f : 0.8f, on);

        if (on)
        {
            g.setColour (Theme::textStrong);
            g.drawEllipse (p.x - nodeRadius - 6.0f, p.y - nodeRadius - 6.0f, (nodeRadius + 6.0f) * 2.0f, (nodeRadius + 6.0f) * 2.0f, 1.5f);
        }

        if ((int) i == onKeys)
            text (g, "ON KEYS", p.x, p.y - nodeRadius - 12.0f, 9.5f, Theme::accent, juce::Justification::centred, true);

        text (g, juce::String (juce::roundToInt (best[i].lock)) + "%", p.x, p.y + nodeRadius + 12.0f, 11.0f, on ? Theme::textStrong : Theme::dimText, juce::Justification::centred, on);
    }

    // The signal in the middle, the candidate shown laid over it, and the bands where they agree lit.
    const auto& target = search->getTarget().signature;
    const auto r = 40.0f * k;
    drawSignature (g, centre, r, target, signal(), 1.0f, true);

    if (show != nullptr)
    {
        drawSignature (g, centre, r, show->signature, Theme::accent, 0.7f, false);
        juce::Path agree;

        for (int b = 0; b < SignalConsole::Signature::bands * 2; ++b)
        {
            const auto band = b < SignalConsole::Signature::bands ? b : SignalConsole::Signature::bands * 2 - 1 - b;
            const auto t = target.band[(size_t) band], c = show->signature.band[(size_t) band];

            if (std::abs (t - c) > 0.06f || t < 0.1f)
                continue;

            const auto a = ((float) b + 0.5f) / (float) (SignalConsole::Signature::bands * 2) * tau - tau * 0.25f;
            const auto reach = r * (0.5f + 0.5f * t);
            agree.startNewSubPath (centre.x + std::cos (a) * r * 0.5f, centre.y + std::sin (a) * r * 0.5f);
            agree.lineTo (centre.x + std::cos (a) * reach, centre.y + std::sin (a) * reach);
        }

        g.setColour (Theme::good);
        g.strokePath (agree, juce::PathStrokeType (2.0f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }
}

void DecryptView::drawControls (juce::Graphics& g)
{
    drawChip (g, chooseChip, "Decrypt a Sound...", Theme::lineStrong, false, Theme::text);
    drawChip (g, tablesChip, juce::String (tablesOnly ? "[x]" : "[ ]") + " Tables only from the factory", tablesOnly ? Theme::accent : Theme::lineStrong, tablesOnly,
              tablesOnly ? Theme::textStrong : Theme::dimText);
}

void DecryptView::drawPanel (juce::Graphics& g)
{
    g.setColour (Theme::inset);
    g.fillRoundedRectangle (panel, 8.0f);
    g.setColour (Theme::line);
    g.drawRoundedRectangle (panel, 8.0f, 1.0f);
    const auto x = panel.getX() + 16.0f, right = panel.getRight() - 16.0f, width = right - x;
    auto y = panel.getY() + 22.0f;

    if (search == nullptr)
    {
        text (g, "DECRYPT", x, y, 12.0f, signal(), juce::Justification::centredLeft, true);
        y += 30.0f;
        const juce::StringArray lines {
            "Drop a recording of one note,",
            "held or plucked, up to 4 s.",
            "",
            "Its pitch is found and it is",
            "trimmed and levelled. The synth",
            "then plays thousands of patches",
            "at that note and keeps the",
            "nearest, by their spectra over",
            "time and their loudness.",
            "",
            "The lock is how near: 100 % is",
            "the same sound, 0 % 24 dB apart.",
            "A sound this synth made is",
            "found again; anything else",
            "gives a patch in its spirit:",
            "close for synths, loose for",
            "voices, drums and samples.",
            "",
            "Effects are off while it",
            "matches; one note, no chords."
        };

        for (auto& line : lines)
        {
            text (g, line, x, y, 11.5f, Theme::dimText);
            y += 17.0f;
        }

        return;
    }

    const auto running = search->isRunning();
    const auto* c = shown();
    const auto heading = running ? juce::String ("DECRYPTING") : ended.isNotEmpty() ? ended : juce::String ("LOCKED");
    text (g, heading, x, y, 12.0f, signal(), juce::Justification::centredLeft, true);
    text (g, progress.stage + "  " + timeText (progress.seconds), right, y, 11.0f, Theme::dimText, juce::Justification::centredRight);
    y += 24.0f;

    // The candidate shown: its signature, lock, address and where it came from.
    if (c != nullptr)
    {
        drawSignature (g, { x + 34.0f, y + 40.0f }, 34.0f, c->signature, Theme::accent, 1.0f, true);
        const auto lock = c == &progress.best.front() ? shownLock : c->lock;
        text (g, juce::String (juce::roundToInt (lock)) + "%", x + 80.0f, y + 26.0f, 30.0f, Theme::textStrong, juce::Justification::centredLeft, true);
        text (g, "lock   node " + SignalConsole::addressText (c->address), x + 80.0f, y + 52.0f, 11.0f, Theme::dimText);
        text (g, "from " + c->from, x + 80.0f, y + 68.0f, 10.5f, Theme::faintText);
        y += 96.0f;
        text (g, "tone " + juce::String (c->distance.tone, 1) + " dB   loudness " + juce::String (c->distance.loudness, 1) + " dB apart", x, y, 10.5f, Theme::dimText);
        y += 24.0f;
    }
    else
    {
        text (g, "rendering the factory patches...", x, y + 20.0f, 11.0f, Theme::dimText);
        y += 120.0f;
    }

    // A bar for each part as the search settles it.
    for (int part = 0; part < SignalConsole::numParts; ++part)
    {
        const auto off = part == SignalConsole::effects;
        const auto settled = off ? 0.0f : progress.settled[(size_t) part];
        text (g, SignalConsole::partNames()[part].toLowerCase() + (off ? " (off while matching)" : ""), x, y, 11.5f, off ? Theme::faintText : Theme::text);

        if (! off)
            text (g, juce::String (juce::roundToInt (settled * 100.0f)) + "%", right, y, 10.5f, Theme::dimText, juce::Justification::centredRight);

        g.setColour (Theme::line);
        g.fillRect (x, y + 10.0f, width, 4.0f);
        g.setColour (settled > 0.85f ? Theme::good : signal());
        g.fillRect (x, y + 10.0f, width * settled, 4.0f);
        y += 32.0f;
    }

    // What the model proposed, the newest last.
    y += 4.0f;

    for (int i = juce::jmax (0, progress.model.size() - 3); i < progress.model.size(); ++i)
    {
        const auto& line = progress.model[i];
        text (g, line.substring (0, 44), x, y, 10.5f, line.contains (": kept") ? Theme::good : Theme::accentLight);
        y += 16.0f;
    }

    const auto& t = search->getTarget();
    text (g, "signal  " + (t.pitchHz > 0.0 ? noteName (t.note) : juce::String ("no pitch")) + "  " + juce::String (t.audio.getNumSamples() / rate, 2) + " s  dry"
             + (clipName.isNotEmpty() ? "  " + clipName.substring (0, 18) : juce::String()), x, juce::jmax (y + 8.0f, panel.getBottom() - 118.0f), 10.5f, Theme::dimText);

    if (offer.name.isNotEmpty())
    {
        text (g, "Name it " + offer.name.quoted() + (offer.category.isNotEmpty() ? " (" + offer.category + ")?" : "?"), x, useChip.getY() - 22.0f, 11.0f, Theme::accentLight);
        drawChip (g, useChip, "Use", Theme::good, false, Theme::text);
        drawChip (g, refuseChip, "No", Theme::lineStrong, false, Theme::dimText);
    }

    if (! restored)
    {
        drawChip (g, stopChip, "Stop", Theme::lineStrong, false, Theme::text);
        drawChip (g, takeChip, c != nullptr ? "Take " + juce::String (juce::roundToInt (c->lock)) + "%" : juce::String ("Take"), Theme::good, true, Theme::textStrong);
    }

    if (canGoAgain())
        drawChip (g, againChip, "Again", Theme::accent, true, Theme::textStrong);

    for (size_t i = 0; i < chain.size(); ++i)
        if (! chain[i].at.isEmpty())
            drawChip (g, chain[i].at, juce::String (juce::roundToInt (chain[i].lock)) + "%",
                      chainPlayer.isTimerRunning() && (int) i == chainPlayer.next - 1 ? Theme::accent : Theme::lineStrong, false, Theme::dimText);

    if (! chain.empty())
    {
        const auto playing = chainPlayer.isTimerRunning();
        drawChip (g, playChip, playing ? "Stop" : "Play", Theme::accent, playing, Theme::text);
        text (g, juce::String ((int) chain.size()) + (chain.size() == 1 ? " step" : " steps") + " so far: click one to hear it, double-click to go back to it",
              x, panel.getBottom() - 88.0f, 10.0f, Theme::faintText);
    }
}
