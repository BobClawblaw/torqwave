#pragma once

#include <JuceHeader.h>
#include "AiClient.h"
#include "PluginCatalogue.h"
#include "PluginScanner.h"

/** Experimental: the plug-in catalogue's learning, and its window (Settings > AI > Plug-in
    Catalogue). Learning goes through the plug-ins Torqwave knows, one after another: it reads
    each one's facts (its parameters), in the scanner's worker process for third-party plug-ins
    so a crash costs only the worker, then has the language model in Settings > AI write what the
    plug-in does from those facts. What is learnt is kept, so a run that stops resumes where it
    left off, and Ask Torq reads it to choose plug-ins. */
namespace Ai
{
    class CatalogueLearner final : private juce::Thread
    {
    public:
        /** plugins: the plug-ins Torqwave knows, read on the message thread when a run starts.
            formats opens the built-in effects in this process; worker is the scanner executable
            (empty: this app). */
        CatalogueLearner (PluginCatalogue&, juce::AudioPluginFormatManager& formats, std::function<juce::Array<juce::PluginDescription>()> plugins,
                          std::function<Settings()> settings, juce::File worker = {});
        ~CatalogueLearner() override;

        /** Learns every plug-in not yet learnt. Facts only when the model isn't set up. */
        void start();
        void stop();
        bool isRunning() const                    { return isThreadRunning(); }

        struct Progress
        {
            int done = 0, total = 0, read = 0, written = 0, failed = 0;
            juce::String current;     // the plug-in being learnt
            juce::String message;     // how the last run ended, or the last problem
        };

        Progress getProgress() const;

        /** Which plug-ins are learnt: effects for now. Instruments come with multitrack. */
        static bool isLearnt (const juce::PluginDescription& d)    { return ! d.isInstrument; }

        /** The request that has the model write one plug-in's entry (for the tests). */
        static juce::Array<juce::var> requestFor (const juce::PluginDescription&, const juce::var& facts);

    private:
        void run() override;
        void report (const std::function<void (Progress&)>&);
        juce::var readFacts (const juce::PluginDescription&, juce::String& reason);

        PluginCatalogue& catalogue;
        juce::AudioPluginFormatManager& formats;
        std::function<juce::Array<juce::PluginDescription>()> getPlugins;
        std::function<Settings()> getSettings;
        const juce::File worker;

        juce::Array<juce::PluginDescription> queue;   // set before the thread starts
        Settings settings;                            // likewise
        juce::String key;                             // likewise
        std::unique_ptr<OutOfProcessScanner::Describer> describer;   // the run's worker, used on its thread

        mutable juce::CriticalSection lock;
        Progress progress;                            // guarded by lock
        juce::CriticalSection clientLock;
        Client* client = nullptr;                     // the request in progress, guarded by clientLock

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CatalogueLearner)
    };

    //==============================================================================
    /** The catalogue's window: every effect with what is known of it, Learn All and Stop, and
        the selected plug-in's entry to read, edit or forget. */
    class CatalogueView final : public juce::Component,
                                private juce::ListBoxModel,
                                private juce::Timer
    {
    public:
        CatalogueView (PluginCatalogue&, CatalogueLearner&, std::function<juce::Array<juce::PluginDescription>()> plugins,
                       std::function<Settings()> settings);
        ~CatalogueView() override;

        void resized() override;
        void paint (juce::Graphics&) override;

        /** For the tests: the rows shown, and select one. */
        int getNumShown() const                      { return (int) shown.size(); }
        void select (int row);
        juce::String getEntryText() const            { return entry.getText(); }

    private:
        struct Row
        {
            juce::PluginDescription description;      // the first format found
            juce::StringArray formats;
        };

        int getNumRows() override                    { return (int) shown.size(); }
        void paintListBoxItem (int, juce::Graphics&, int width, int height, bool selected) override;
        void selectedRowsChanged (int) override;
        void timerCallback() override;
        void rebuild();
        void showEntry();
        juce::String stateOf (const Row&) const;

        PluginCatalogue& catalogue;
        CatalogueLearner& learner;
        std::function<juce::Array<juce::PluginDescription>()> getPlugins;
        std::function<Settings()> getSettings;

        std::vector<Row> rows, shown;
        juce::Label status, about, written;
        juce::TextEditor search, entry;
        juce::ListBox list { {}, this };
        juce::TextButton learnButton { "Learn All" }, stopButton { "Stop" }, saveButton { "Save" }, forgetButton { "Forget" };
        int lastDone = -1;
        bool lastRunning = false;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CatalogueView)
    };
}
