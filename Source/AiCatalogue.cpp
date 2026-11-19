#include "AiCatalogue.h"
#include "AiKeychain.h"
#include "Theme.h"
#include <set>

namespace Ai
{
namespace
{
    juce::var message (const juce::String& role, const juce::String& content)
    {
        auto m = std::make_unique<juce::DynamicObject>();
        m->setProperty ("role", role);
        m->setProperty ("content", content);
        return juce::var (m.release());
    }

    juce::String keyFor (const Settings& s)
    {
        const auto key = AiKeychain::load (s.keyAccount().toStdString());
        return key ? juce::String::fromUTF8 (key->c_str(), (int) key->size()) : juce::String();
    }

    /** One entry for each plug-in, whatever its format, as the understanding is kept. */
    juce::String sameKey (const juce::PluginDescription& d)    { return (d.manufacturerName.trim() + "|" + d.name.trim()).toLowerCase(); }

    constexpr int promptParameters = 150;   // the most parameters shown to the model
}

//==============================================================================
CatalogueLearner::CatalogueLearner (PluginCatalogue& c, juce::AudioPluginFormatManager& f, std::function<juce::Array<juce::PluginDescription>()> plugins,
                                    std::function<Settings()> s, juce::File w)
    : juce::Thread ("Plug-in catalogue"), catalogue (c), formats (f), getPlugins (std::move (plugins)), getSettings (std::move (s)), worker (std::move (w))
{
}

CatalogueLearner::~CatalogueLearner()
{
    stop();
}

void CatalogueLearner::start()
{
    if (isThreadRunning())
        return;

    queue.clearQuick();

    for (auto& d : getPlugins())
        if (isLearnt (d))
            queue.add (d);

    // Built-in effects first (quick, and in every list), then by name, so formats of one plug-in sit together.
    std::stable_sort (queue.begin(), queue.end(), [] (const juce::PluginDescription& a, const juce::PluginDescription& b)
    {
        const auto builtInA = a.pluginFormatName == "Torqwave", builtInB = b.pluginFormatName == "Torqwave";
        return builtInA != builtInB ? builtInA : a.name.compareIgnoreCase (b.name) < 0;
    });

    settings = getSettings();
    key = settings.isOn() && settings.model.isNotEmpty() ? keyFor (settings) : juce::String();

    {
        const juce::ScopedLock sl (lock);
        progress = {};
        progress.total = queue.size();
    }

    startThread();
}

void CatalogueLearner::stop()
{
    signalThreadShouldExit();

    {
        const juce::ScopedLock sl (clientLock);

        if (client != nullptr)
            client->cancel();
    }

    stopThread (30000);
}

CatalogueLearner::Progress CatalogueLearner::getProgress() const
{
    const juce::ScopedLock sl (lock);
    return progress;
}

void CatalogueLearner::report (const std::function<void (Progress&)>& change)
{
    const juce::ScopedLock sl (lock);
    change (progress);
}

juce::var CatalogueLearner::readFacts (const juce::PluginDescription& d, juce::String& reason)
{
    // Torqwave's own effects are its own code, opened here; the rest in the worker.
    if (d.pluginFormatName == "Torqwave")
    {
        auto plugin = formats.createPluginInstance (d, 48000.0, 512, reason);
        return plugin != nullptr ? PluginCatalogue::factsOf (*plugin) : juce::var();
    }

    if (describer == nullptr)
        describer = std::make_unique<OutOfProcessScanner::Describer> (worker);

    return describer->describe (d, reason, [this] { return threadShouldExit(); });
}

juce::Array<juce::var> CatalogueLearner::requestFor (const juce::PluginDescription& d, const juce::var& facts)
{
    juce::Array<juce::var> parameters;

    if (auto* list = facts["parameters"].getArray())
        for (auto& p : *list)
            if (parameters.size() < promptParameters)
                parameters.add (p);

    auto shown = std::make_unique<juce::DynamicObject>();
    shown->setProperty ("name", d.name);
    shown->setProperty ("maker", d.manufacturerName);
    shown->setProperty ("format", d.pluginFormatName == "Torqwave" ? juce::String ("Torqwave's built-in effect") : d.pluginFormatName);
    shown->setProperty ("category", d.category);
    shown->setProperty ("inputs", facts["inputs"]);
    shown->setProperty ("outputs", facts["outputs"]);
    shown->setProperty ("parameterCount", facts["parameterCount"]);
    shown->setProperty ("parameters", parameters);

    if (facts.hasProperty ("programs"))
    {
        shown->setProperty ("presets", facts["programs"]);
        shown->setProperty ("presetCount", facts["programCount"]);
    }

    return { message ("system",
                      "You write entries for the plug-in catalogue of Torqwave, an audio editor. An assistant reads them to choose "
                      "an effect for a job and to set it through its parameters. Write from the facts given and from what you know "
                      "of this plug-in. Plain text, no headings or markdown, at most 150 words: first one sentence on what it is and "
                      "what it does to the sound; then what it is good for; then the parameters that matter most for common jobs, by "
                      "their names here, with useful settings, and any presets that suit common jobs. If neither the facts nor what "
                      "you know make clear what the plug-in does to the sound, write only \"" + juce::String (PluginCatalogue::notKnown)
                      + "\" and one sentence on what can be told from the facts, with no settings: a guess would mislead."),
             message ("user", juce::JSON::toString (juce::var (shown.release()), true)) };
}

void CatalogueLearner::run()
{
    const auto useModel = settings.isOn() && settings.model.isNotEmpty();
    std::set<juce::String> writtenThisRun;
    juce::String ended;

    for (int i = 0; i < queue.size() && ! threadShouldExit(); ++i)
    {
        const auto& d = queue.getReference (i);
        report ([&] (Progress& p) { p.current = d.name + " (" + (d.pluginFormatName == "Torqwave" ? juce::String ("built-in") : d.pluginFormatName) + ")"; });

        if (! catalogue.hasCurrentFacts (d))
        {
            juce::String reason;
            const auto facts = readFacts (d, reason);

            if (threadShouldExit())
                break;

            if (facts.isObject())
            {
                catalogue.setFacts (d, facts);
                report ([] (Progress& p) { ++p.read; });
            }
            else
            {
                catalogue.setFactsFailed (d, reason);
                report ([&] (Progress& p) { ++p.failed; p.message = d.name + " couldn't be read: " + reason; });
            }
        }

        const auto facts = catalogue.getFacts (d);

        if (useModel && facts.isObject() && ! facts.hasProperty ("error") && catalogue.needsUnderstanding (d)
            && writtenThisRun.count (sameKey (d)) == 0)
        {
            Client c (settings, key);

            {
                const juce::ScopedLock sl (clientLock);
                client = &c;
            }

            const auto reply = threadShouldExit() ? Reply {} : c.chat (juce::var (requestFor (d, facts)), juce::var (juce::Array<juce::var>()));

            {
                const juce::ScopedLock sl (clientLock);
                client = nullptr;
            }

            if (threadShouldExit())
                break;

            if (reply.error.isNotEmpty())
            {
                // A server that can't be reached won't be for the next one either.
                if (reply.status == 0)
                {
                    ended = reply.error + " Learning stopped.";
                    break;
                }

                report ([&] (Progress& p) { p.message = d.name + ": " + reply.error; });
            }
            else if (const auto text = reply.content.trim(); text.isNotEmpty())
            {
                catalogue.setUnderstanding (d, { text, settings.model, d.version, juce::Time::getCurrentTime() });
                writtenThisRun.insert (sameKey (d));
                report ([] (Progress& p) { ++p.written; });
            }
        }

        report ([] (Progress& p) { ++p.done; });

        if (i % 10 == 9)
            catalogue.saveIfNeeded();
    }

    describer = nullptr;   // ends the worker with the run
    catalogue.saveIfNeeded();

    report ([&] (Progress& p)
    {
        p.current = {};
        p.message = ended.isNotEmpty() ? ended
                  : threadShouldExit() ? juce::String ("Stopped; Learn All goes on from here.")
                  : "Done: " + juce::String (p.read) + " read, " + juce::String (p.written) + " described, " + juce::String (p.failed) + " couldn't be opened."
                        + (useModel ? juce::String() : juce::String (" Set up a model above to have them described."));
    });
}

//==============================================================================
CatalogueView::CatalogueView (PluginCatalogue& c, CatalogueLearner& l, std::function<juce::Array<juce::PluginDescription>()> plugins,
                              std::function<Settings()> s)
    : catalogue (c), learner (l), getPlugins (std::move (plugins)), getSettings (std::move (s))
{
    for (auto* b : { &learnButton, &stopButton, &saveButton, &forgetButton })
        addAndMakeVisible (b);

    for (auto* label : { &status, &about, &written })
    {
        addAndMakeVisible (label);
        label->setColour (juce::Label::textColourId, label == &about ? Theme::textStrong : Theme::dimText);
    }

    about.setFont (juce::FontOptions (15.0f, juce::Font::bold));
    status.setJustificationType (juce::Justification::centredLeft);

    addAndMakeVisible (search);
    search.setTextToShowWhenEmpty ("Search plug-ins", Theme::faintText);
    search.onTextChange = [this] { rebuild(); };

    addAndMakeVisible (list);
    list.setRowHeight (24);
    list.setColour (juce::ListBox::backgroundColourId, Theme::surface);

    addAndMakeVisible (entry);
    entry.setMultiLine (true, true);
    entry.setReturnKeyStartsNewLine (true);
    entry.setTextToShowWhenEmpty ("Nothing written yet. Learn All has the model write it, or write it here and Save.", Theme::faintText);

    learnButton.setTooltip ("Read every effect's parameters and have the model in Settings > AI describe each one; what is already known is kept");
    stopButton.setTooltip ("Stop learning; Learn All goes on from where it stopped");
    saveButton.setTooltip ("Keep this text as the plug-in's entry, written by you; learning won't replace it");
    forgetButton.setTooltip ("Drop what is known of this plug-in, so Learn All reads and describes it again");

    learnButton.onClick = [this] { learner.start(); timerCallback(); };
    stopButton.onClick = [this] { learner.stop(); timerCallback(); };

    saveButton.onClick = [this]
    {
        if (const auto row = list.getSelectedRow(); juce::isPositiveAndBelow (row, (int) shown.size()))
        {
            catalogue.setUnderstanding (shown[(size_t) row].description, { entry.getText().trim(), "you", shown[(size_t) row].description.version,
                                                                         juce::Time::getCurrentTime() });
            catalogue.saveIfNeeded();
            showEntry();
            list.repaint();
        }
    };

    forgetButton.onClick = [this]
    {
        if (const auto row = list.getSelectedRow(); juce::isPositiveAndBelow (row, (int) shown.size()))
        {
            catalogue.forget (shown[(size_t) row].description);
            catalogue.saveIfNeeded();
            showEntry();
            list.repaint();
        }
    };

    rebuild();
    timerCallback();
    startTimerHz (4);
}

CatalogueView::~CatalogueView()
{
    stopTimer();
}

void CatalogueView::rebuild()
{
    rows.clear();
    std::map<juce::String, size_t> index;

    for (auto& d : getPlugins())
    {
        if (! CatalogueLearner::isLearnt (d))
            continue;

        const auto key = sameKey (d);

        if (auto found = index.find (key); found != index.end())
        {
            rows[found->second].formats.addIfNotAlreadyThere (d.pluginFormatName);
            continue;
        }

        index[key] = rows.size();
        rows.push_back ({ d, { d.pluginFormatName } });
    }

    std::sort (rows.begin(), rows.end(), [] (const Row& a, const Row& b) { return a.description.name.compareIgnoreCase (b.description.name) < 0; });

    const auto wanted = search.getText().trim();
    shown.clear();

    for (auto& r : rows)
        if (wanted.isEmpty() || r.description.name.containsIgnoreCase (wanted) || r.description.manufacturerName.containsIgnoreCase (wanted)
            || r.description.category.containsIgnoreCase (wanted) || catalogue.summaryOf (r.description).containsIgnoreCase (wanted))
            shown.push_back (r);

    list.updateContent();
    list.deselectAllRows();
    list.repaint();
    showEntry();
}

juce::String CatalogueView::stateOf (const Row& r) const
{
    if (const auto u = catalogue.getUnderstanding (r.description))
        return u->text.startsWith (PluginCatalogue::notKnown) ? "not known" : u->shipped ? "from Torqwave" : "described";

    const auto facts = catalogue.getFacts (r.description);

    if (facts.hasProperty ("error"))
        return "couldn't be opened";

    return facts.isObject() ? "parameters read" : "not learnt yet";
}

void CatalogueView::paintListBoxItem (int row, juce::Graphics& g, int width, int height, bool selected)
{
    if (! juce::isPositiveAndBelow (row, (int) shown.size()))
        return;

    const auto& r = shown[(size_t) row];

    if (selected)
        g.fillAll (Theme::selection);

    const auto state = stateOf (r);
    g.setFont (juce::FontOptions (13.0f));
    g.setColour (state == "described" || state == "from Torqwave" ? Theme::good : state == "couldn't be opened" ? Theme::danger
               : state == "not known" ? Theme::warn : Theme::dimText);
    g.drawText (state, width - 130, 0, 122, height, juce::Justification::centredRight, true);

    g.setColour (Theme::textStrong);
    g.drawText (r.description.name, 8, 0, width / 2, height, juce::Justification::centredLeft, true);
    g.setColour (Theme::dimText);
    g.drawText (r.description.manufacturerName + "  " + r.formats.joinIntoString (", ").replace ("Torqwave", "built-in"),
                width / 2 + 8, 0, width / 2 - 146, height, juce::Justification::centredLeft, true);
}

void CatalogueView::selectedRowsChanged (int)
{
    showEntry();
}

void CatalogueView::select (int row)
{
    list.selectRow (row);
    showEntry();
}

void CatalogueView::showEntry()
{
    const auto row = list.getSelectedRow();
    const auto valid = juce::isPositiveAndBelow (row, (int) shown.size());
    saveButton.setEnabled (valid);
    forgetButton.setEnabled (valid);
    entry.setEnabled (valid);

    if (! valid)
    {
        about.setText ({}, juce::dontSendNotification);
        written.setText ("Choose a plug-in to read or edit its entry.", juce::dontSendNotification);
        entry.setText ({}, false);
        return;
    }

    const auto& r = shown[(size_t) row];
    about.setText (r.description.name + " (" + r.description.manufacturerName + ")", juce::dontSendNotification);
    const auto u = catalogue.getUnderstanding (r.description);
    entry.setText (u ? u->text : juce::String(), false);

    const auto facts = catalogue.getFacts (r.description);
    juce::String line;

    if (u && u->shipped)
        line << "From Torqwave's catalogue: written by " << u->model << ", reviewed " << u->when.toString (true, false) << "; Save keeps your own. ";
    else if (u)
        line << (u->model == "you" ? juce::String ("Written by you") : "Written by " + u->model) << ", " << u->when.toString (true, false) << ". ";

    if (facts.hasProperty ("error"))
        line << "Its parameters couldn't be read: " << facts["error"].toString();
    else if (facts.isObject())
        line << (int) facts["parameterCount"] << " parameters read.";
    else
        line << "Its parameters haven't been read.";

    written.setText (line, juce::dontSendNotification);
}

void CatalogueView::timerCallback()
{
    const auto running = learner.isRunning();
    const auto p = learner.getProgress();
    learnButton.setEnabled (! running);
    stopButton.setEnabled (running);

    int described = 0;

    for (auto& r : rows)
        described += catalogue.getUnderstanding (r.description) ? 1 : 0;

    const auto counts = juce::String (catalogue.getNumFacts()) + " plug-ins read, " + juce::String (described) + " of " + juce::String ((int) rows.size())
                        + " described.";
    const auto s = getSettings();
    juce::String text;

    if (running)
        text << "Learning " << (p.done + 1) << " of " << p.total << ": " << p.current << "    " << counts;
    else
        text << counts << "  " << (p.message.isNotEmpty() ? p.message
                                   : s.isOn() && s.model.isNotEmpty() ? "Learn All reads and describes what isn't known yet with " + s.model + "."
                                   : juce::String ("Ask Torq isn't set up, so Learn All only reads parameters."));

    status.setText (text, juce::dontSendNotification);

    // Rows change state as learning goes; the open entry too, unless it is being edited.
    if (p.done != lastDone || running != lastRunning)
    {
        lastDone = p.done;
        lastRunning = running;
        list.repaint();

        if (! entry.hasKeyboardFocus (true))
            showEntry();
    }
}

void CatalogueView::paint (juce::Graphics& g)
{
    g.fillAll (Theme::base);
}

void CatalogueView::resized()
{
    auto r = getLocalBounds().reduced (12);

    auto top = r.removeFromTop (28);
    stopButton.setBounds (top.removeFromRight (80));
    top.removeFromRight (6);
    learnButton.setBounds (top.removeFromRight (100));
    top.removeFromRight (10);
    search.setBounds (top);
    r.removeFromTop (6);
    status.setBounds (r.removeFromTop (36));
    r.removeFromTop (4);

    auto bottom = r.removeFromBottom (juce::jmax (170, r.getHeight() * 2 / 5));
    list.setBounds (r);

    bottom.removeFromTop (8);
    about.setBounds (bottom.removeFromTop (24));
    written.setBounds (bottom.removeFromTop (20));
    auto buttons = bottom.removeFromBottom (28);
    forgetButton.setBounds (buttons.removeFromRight (90));
    buttons.removeFromRight (6);
    saveButton.setBounds (buttons.removeFromRight (90));
    bottom.removeFromBottom (6);
    entry.setBounds (bottom);
}
}
