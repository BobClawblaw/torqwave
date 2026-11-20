#include "SequencePreview.h"
#include "AudioEngine.h"
#include "OfflineRender.h"
#include "TrackEffects.h"

//==============================================================================
/** The rendered pass, looped, after a wait of `delay` frames. Set and cleared on the message thread;
    the audio thread only reads, and plays silence for a block while it is being swapped. */
class SequencePreview::Loop final : public juce::AudioSource
{
public:
    void prepareToPlay (int, double) override {}
    void releaseResources() override {}

    void set (std::shared_ptr<const juce::AudioBuffer<float>> b, juce::int64 delay)
    {
        const juce::SpinLock::ScopedLockType sl (lock);
        buffer = std::move (b);
        position = -delay;
    }

    void clear()
    {
        std::shared_ptr<const juce::AudioBuffer<float>> old;
        {
            const juce::SpinLock::ScopedLockType sl (lock);
            old = std::move (buffer);
        }
    }

    void getNextAudioBlock (const juce::AudioSourceChannelInfo& info) override
    {
        info.clearActiveBufferRegion();
        const juce::SpinLock::ScopedTryLockType sl (lock);

        if (! sl.isLocked() || buffer == nullptr || buffer->getNumSamples() == 0 || buffer->getNumChannels() == 0)
            return;

        const auto length = (juce::int64) buffer->getNumSamples();

        for (int i = 0; i < info.numSamples;)
        {
            if (position < 0)
            {
                const auto wait = (int) juce::jmin (-position, (juce::int64) (info.numSamples - i));
                position += wait;
                i += wait;
                continue;
            }

            const auto at = (int) (position % length);
            const auto n = juce::jmin (info.numSamples - i, (int) length - at);

            for (int ch = 0; ch < juce::jmin (2, info.buffer->getNumChannels()); ++ch)
                info.buffer->addFrom (ch, info.startSample + i, *buffer, juce::jmin (ch, buffer->getNumChannels() - 1), at, n);

            position += n;
            i += n;
        }
    }

private:
    juce::SpinLock lock;
    std::shared_ptr<const juce::AudioBuffer<float>> buffer;
    juce::int64 position = 0;
};

//==============================================================================
SequencePreview::SequencePreview (AudioEngine& e) : engine (e), loop (std::make_unique<Loop>())
{
    player.setSource (loop.get());
    engine.deviceManager.addAudioCallback (&player);
}

SequencePreview::~SequencePreview()
{
    renderer.removeAllJobs (true, 20000);
    cancelPendingUpdate();
    engine.deviceManager.removeAudioCallback (&player);
    player.setSource (nullptr);
}

juce::AudioSource& SequencePreview::getSource()
{
    return *loop;
}

juce::int64 SequencePreview::framesToNextBar (const Metronome::TempoMap& map, juce::int64 frame)
{
    const auto bar = map.barAt (frame);
    const auto start = map.barFrame (bar);
    return start == frame ? 0 : map.barFrame (bar + 1) - frame;
}

juce::String SequencePreview::render (const SequenceFile::Sequence& q, double tempo, int beatsPerBar, double sampleRate,
                                      juce::AudioPluginFormatManager& formats, juce::AudioBuffer<float>& out)
{
    out.setSize (0, 0);

    if (SequenceFile::instrumentOf (q).isEmpty())
        return "It has no instrument of its own: dropped on an instrument track, it plays on that track's.";

    // A session of its own at the tempo, the sequence twice in a row on a track made from its voice.
    SessionState s;
    s.sampleRate = sampleRate;
    s.tempo = tempo;
    s.beatsPerBar = beatsPerBar;
    const auto placed = SequenceFile::place (s, q, -1, 0, false);
    const auto* track = s.findTrack (placed.trackId);

    if (placed.clipId == 0 || track == nullptr || track->noteClips.empty() || placed.effects == nullptr)
        return "It could not be placed.";

    const auto length = track->noteClips.front().length;
    SessionEdits::addNoteClip (s, placed.trackId, SequenceFile::toClip (q, s, length));

    std::map<juce::uint32, std::unique_ptr<juce::XmlElement>> chains;
    chains[placed.trackId] = std::make_unique<juce::XmlElement> (*placed.effects);

    // Some plug-ins must be deleted on the message thread, so they go there when done.
    auto inserts = std::make_shared<TrackEffects>();
    const juce::ScopeGuard release { [&inserts] { juce::MessageManager::callAsync ([doomed = std::move (inserts)] {}); } };
    const auto warnings = OfflineRender::loadTrackEffects (chains, formats, sampleRate, *inserts);

    if (inserts->getInstrument (placed.trackId) == nullptr)
        return SequenceFile::instrumentOf (q) + " could not be loaded" + (warnings.isNotEmpty() ? ": " + warnings.trim() : juce::String ("."));

    juce::AudioBuffer<float> mix;

    if (! OfflineRender::mixSession (s, { 0, 2 * length }, mix, inserts.get(), {}))
        return "It could not be rendered.";

    out.setSize (mix.getNumChannels(), (int) length);

    for (int ch = 0; ch < mix.getNumChannels(); ++ch)
        out.copyFrom (ch, 0, mix, ch, (int) length, (int) length);

    return {};
}

struct SequencePreview::Job final : public juce::ThreadPoolJob
{
    Job (SequencePreview& p, juce::File f, SequenceFile::Sequence s, juce::String k, double t, int b, double r)
        : juce::ThreadPoolJob ("Sequence preview"), owner (&p), formats (p.engine.pluginFormatManager),
          file (std::move (f)), q (std::move (s)), key (std::move (k)), tempo (t), beatsPerBar (b), rate (r) {}

    JobStatus runJob() override
    {
        auto audio = std::make_shared<juce::AudioBuffer<float>>();
        const auto failed = render (q, tempo, beatsPerBar, rate, formats, *audio);

        if (auto* p = owner.get())
        {
            const juce::ScopedLock sl (p->pendingLock);
            p->pending.push_back ([weak = owner, f = file, k = key, a = std::shared_ptr<const juce::AudioBuffer<float>> (audio), failed, t = tempo]
            {
                if (auto* self = weak.get())
                    self->rendered (f, k, a, failed, t);
            });
            p->triggerAsyncUpdate();
        }

        return jobHasFinished;
    }

    juce::WeakReference<SequencePreview> owner;
    juce::AudioPluginFormatManager& formats;
    juce::File file;
    SequenceFile::Sequence q;
    juce::String key;
    double tempo;
    int beatsPerBar;
    double rate;
};

void SequencePreview::handleAsyncUpdate()
{
    std::vector<std::function<void()>> taken;
    {
        const juce::ScopedLock sl (pendingLock);
        taken.swap (pending);
    }

    for (auto& f : taken)
        f();
}

void SequencePreview::play (const juce::File& file)
{
    stop();
    current = file;
    error.clear();

    juce::String loadError;
    const auto q = SequenceFile::load (file, loadError);

    if (! q.has_value())
    {
        error = loadError;

        if (onChange != nullptr)
            onChange();

        return;
    }

    const auto session = engine.session.getState();   // held: the map refers to its tempo changes
    const auto map = session->tempoMap();
    const auto rate = engine.getOutputSampleRate() > 0.0 ? engine.getOutputSampleRate() : session->sampleRate;
    const auto frame = engine.getPlayer().getPositionFrames();
    const auto tempo = map.tempoAt (frame);
    const auto beatsPerBar = map.beatsPerBarAt (frame);
    const auto key = file.getFullPathName() + "|" + juce::String (file.getLastModificationTime().toMilliseconds()) + "|"
                     + juce::String (tempo, 4) + "|" + juce::String (beatsPerBar) + "|" + juce::String (rate);

    playingAt = juce::String (tempo, juce::exactlyEqual (tempo, std::round (tempo)) ? 0 : 2) + " BPM";

    for (auto& [k, audio] : cache)
        if (k == key)
        {
            start (audio);
            return;
        }

    state = State::rendering;
    renderer.addJob (new Job (*this, file, *q, key, tempo, beatsPerBar, rate), true);

    if (onChange != nullptr)
        onChange();
}

void SequencePreview::rendered (const juce::File& file, juce::String key, std::shared_ptr<const juce::AudioBuffer<float>> audio, juce::String failed, double)
{
    if (failed.isEmpty())
    {
        cache.emplace_back (std::move (key), audio);

        while (cache.size() > 8)
            cache.erase (cache.begin());
    }

    // Only the sequence asked for last is played.
    if (file != current || state != State::rendering)
        return;

    if (failed.isNotEmpty())
    {
        error = failed;
        state = State::stopped;

        if (onChange != nullptr)
            onChange();

        return;
    }

    start (std::move (audio));
}

void SequencePreview::start (std::shared_ptr<const juce::AudioBuffer<float>> audio)
{
    // In time with the session: from its next bar while it plays.
    juce::int64 delay = 0;
    auto& transport = engine.getPlayer();

    if (transport.isPlaying())
    {
        const auto session = engine.session.getState();
        const auto rate = engine.getOutputSampleRate();
        const auto frames = framesToNextBar (session->tempoMap(), transport.getPositionFrames());
        delay = rate > 0.0 && session->sampleRate > 0.0 ? (juce::int64) std::llround ((double) frames * rate / session->sampleRate) : frames;
    }

    loop->set (std::move (audio), delay);
    state = State::playing;

    if (onChange != nullptr)
        onChange();
}

void SequencePreview::stop()
{
    loop->clear();
    const auto changed = state != State::stopped;
    state = State::stopped;

    if (changed && onChange != nullptr)
        onChange();
}

juce::String SequencePreview::describe() const
{
    const auto name = current.getFileNameWithoutExtension();

    switch (state)
    {
        case State::rendering: return "Rendering " + name + "...";
        case State::playing:   return name + " at " + playingAt;
        case State::stopped:   break;
    }

    return error;
}
