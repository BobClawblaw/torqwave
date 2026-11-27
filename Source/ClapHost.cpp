#include "ClapHost.h"
#include <cstring>

namespace ClapHost
{
namespace
{
    /** Resolves the native shared-library path inside a .clap bundle (macOS) or file (Windows/Linux). */
    juce::File resolveModuleBinary (const juce::File& clapPath)
    {
       #if JUCE_MAC
        if (clapPath.isDirectory())
        {
            const auto macosDir = clapPath.getChildFile ("Contents/MacOS");
            if (macosDir.isDirectory())
            {
                const auto plist = clapPath.getChildFile ("Contents/Info.plist");
                if (plist.existsAsFile())
                {
                    if (const auto xml = juce::parseXML (plist))
                    {
                        if (const auto* dict = xml->getChildByName ("dict"))
                        {
                            for (int i = 0; i + 1 < dict->getNumChildElements(); ++i)
                            {
                                const auto* k = dict->getChildElement (i);
                                const auto* v = dict->getChildElement (i + 1);
                                if (k != nullptr && v != nullptr && k->hasTagName ("key")
                                    && k->getAllSubText().trim() == "CFBundleExecutable")
                                {
                                    const auto candidate = macosDir.getChildFile (v->getAllSubText().trim());
                                    if (candidate.existsAsFile())
                                        return candidate;
                                }
                            }
                        }
                    }
                }
                const auto stem = macosDir.getChildFile (clapPath.getFileNameWithoutExtension());
                if (stem.existsAsFile())
                    return stem;
                auto files = macosDir.findChildFiles (juce::File::findFiles, false);
                if (! files.isEmpty())
                    return files.getFirst();
            }
        }
       #endif
        return clapPath;
    }

    /** Ref-counted loaded CLAP module (calls entry->init once, entry->deinit on final release). */
    struct LoadedModule
    {
        juce::DynamicLibrary library;
        const clap_plugin_entry_t* entry = nullptr;
        bool initialized = false;
        bool ownsEntry = true;

        ~LoadedModule()
        {
            if (ownsEntry && initialized && entry != nullptr && entry->deinit != nullptr)
                entry->deinit();
        }
    };

    std::shared_ptr<LoadedModule> loadModule (const juce::File& clapPath, juce::String& error)
    {
        const auto bin = resolveModuleBinary (clapPath);
        if (! bin.existsAsFile())
        {
            error = "CLAP binary not found at " + clapPath.getFullPathName();
            return nullptr;
        }

        auto mod = std::make_shared<LoadedModule>();
        if (! mod->library.open (bin.getFullPathName()))
        {
            error = "Could not load CLAP library " + bin.getFullPathName();
            return nullptr;
        }

        mod->entry = static_cast<const clap_plugin_entry_t*> (mod->library.getFunction ("clap_entry"));
        if (mod->entry == nullptr)
        {
            error = "Missing clap_entry symbol in " + clapPath.getFileName();
            return nullptr;
        }

        if (! clap_version_is_compatible (mod->entry->clap_version))
        {
            error = "Incompatible CLAP ABI version in " + clapPath.getFileName();
            return nullptr;
        }

        if (mod->entry->init != nullptr)
        {
            if (! mod->entry->init (clapPath.getFullPathName().toRawUTF8()))
            {
                error = "clap_entry::init() failed for " + clapPath.getFileName();
                return nullptr;
            }
            mod->initialized = true;
        }

        return mod;
    }

    juce::String encodeIdentifier (const juce::String& filePath, const juce::String& pluginId)
    {
        return filePath + "|" + pluginId;
    }

    std::pair<juce::String, juce::String> decodeIdentifier (const juce::String& id)
    {
        const auto sep = id.lastIndexOfChar ('|');
        if (sep < 0)
            return { id, {} };
        return { id.substring (0, sep), id.substring (sep + 1) };
    }

    bool descriptorHasFeature (const clap_plugin_descriptor_t* desc, const char* feature) noexcept
    {
        if (desc == nullptr || desc->features == nullptr)
            return false;
        for (const char* const* f = desc->features; *f != nullptr; ++f)
            if (std::strcmp (*f, feature) == 0)
                return true;
        return false;
    }

    juce::PluginDescription makeDescription (const juce::File& file, const clap_plugin_descriptor_t* desc)
    {
        juce::PluginDescription d;
        d.pluginFormatName = formatName;
        d.name = d.descriptiveName = (desc != nullptr && desc->name != nullptr) ? juce::String::fromUTF8 (desc->name) : file.getFileNameWithoutExtension();
        d.manufacturerName = (desc != nullptr && desc->vendor != nullptr && desc->vendor[0] != '\0') ? juce::String::fromUTF8 (desc->vendor) : "Unknown";
        d.version = (desc != nullptr && desc->version != nullptr) ? juce::String::fromUTF8 (desc->version) : "1.0";
        const auto clapId = (desc != nullptr && desc->id != nullptr) ? juce::String::fromUTF8 (desc->id) : d.name;
        d.fileOrIdentifier = encodeIdentifier (file.getFullPathName(), clapId);
        d.uniqueId = d.deprecatedUid = (int) clapId.hashCode();
        d.isInstrument = descriptorHasFeature (desc, CLAP_PLUGIN_FEATURE_INSTRUMENT);
        d.category = d.isInstrument ? "Instrument"
                   : descriptorHasFeature (desc, CLAP_PLUGIN_FEATURE_NOTE_EFFECT) ? "MIDI FX" : "Effect";
        d.numInputChannels = d.isInstrument ? 0 : 2;
        d.numOutputChannels = 2;
        return d;
    }

    //==========================================================================
    // CLAP event list adapters
    struct EventQueue
    {
        union AnyEvent
        {
            clap_event_header_t header;
            clap_event_note_t note;
            clap_event_midi_t midi;
            clap_event_param_value_t paramValue;
            clap_event_param_gesture_t gesture;
        };

        std::vector<AnyEvent> events;

        void clear() { events.clear(); }

        void addNote (uint16_t type, uint32_t sampleOffset, int16_t channel, int16_t key, double velocity)
        {
            AnyEvent e {};
            e.note.header.size = sizeof (clap_event_note_t);
            e.note.header.time = sampleOffset;
            e.note.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            e.note.header.type = type;
            e.note.header.flags = 0;
            e.note.note_id = -1;
            e.note.port_index = 0;
            e.note.channel = channel;
            e.note.key = key;
            e.note.velocity = velocity;
            events.push_back (e);
        }

        void addRawMidi (uint32_t sampleOffset, const uint8_t* bytes, int size)
        {
            if (size <= 0 || size > 3)
                return;
            AnyEvent e {};
            e.midi.header.size = sizeof (clap_event_midi_t);
            e.midi.header.time = sampleOffset;
            e.midi.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            e.midi.header.type = CLAP_EVENT_MIDI;
            e.midi.header.flags = 0;
            e.midi.port_index = 0;
            for (int i = 0; i < size; ++i)
                e.midi.data[i] = bytes[i];
            events.push_back (e);
        }

        void addParamValue (uint32_t sampleOffset, clap_id paramId, void* cookie, double plainValue)
        {
            AnyEvent e {};
            e.paramValue.header.size = sizeof (clap_event_param_value_t);
            e.paramValue.header.time = sampleOffset;
            e.paramValue.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            e.paramValue.header.type = CLAP_EVENT_PARAM_VALUE;
            e.paramValue.header.flags = 0;
            e.paramValue.param_id = paramId;
            e.paramValue.cookie = cookie;
            e.paramValue.note_id = -1;
            e.paramValue.port_index = -1;
            e.paramValue.channel = -1;
            e.paramValue.key = -1;
            e.paramValue.value = plainValue;
            events.push_back (e);
        }

        static uint32_t inSize (const clap_input_events_t* list) noexcept
        {
            return (uint32_t) static_cast<const EventQueue*> (list->ctx)->events.size();
        }

        static const clap_event_header_t* inGet (const clap_input_events_t* list, uint32_t index) noexcept
        {
            const auto* q = static_cast<const EventQueue*> (list->ctx);
            return index < q->events.size() ? &q->events[index].header : nullptr;
        }

        static bool outPush (const clap_output_events_t* list, const clap_event_header_t* ev) noexcept
        {
            if (ev == nullptr || ev->size > sizeof (AnyEvent))
                return false;
            auto* q = static_cast<EventQueue*> (list->ctx);
            AnyEvent copy {};
            std::memcpy (&copy, ev, ev->size);
            q->events.push_back (copy);
            return true;
        }

        clap_input_events_t asInput() noexcept   { return { this, &inSize, &inGet }; }
        clap_output_events_t asOutput() noexcept { return { this, &outPush }; }
    };

    //==========================================================================
    class ClapPluginInstance final : public juce::AudioPluginInstance
    {
    public:
        class ClapParameter final : public juce::AudioProcessorParameterWithID
        {
        public:
            ClapParameter (ClapPluginInstance& o, const clap_param_info_t& info)
                : AudioProcessorParameterWithID (juce::ParameterID { juce::String ((juce::int64) info.id), 1 },
                                                 juce::String::fromUTF8 (info.name)),
                  owner (o), clapId (info.id), cookie (info.cookie),
                  minVal (info.min_value), maxVal (info.max_value), defVal (info.default_value),
                  currentPlain (info.default_value), stepped ((info.flags & CLAP_PARAM_IS_STEPPED) != 0)
            {
            }

            float getValue() const override
            {
                const auto span = maxVal - minVal;
                if (span <= 0.0)
                    return 0.0f;
                return (float) juce::jlimit (0.0, 1.0, (currentPlain.load (std::memory_order_relaxed) - minVal) / span);
            }

            void setValue (float newValue) override
            {
                const auto span = maxVal - minVal;
                auto plain = minVal + (double) juce::jlimit (0.0f, 1.0f, newValue) * span;
                if (stepped)
                    plain = std::round (plain);
                currentPlain.store (plain, std::memory_order_relaxed);
                owner.queueParamChange (clapId, cookie, plain);
            }

            float getDefaultValue() const override
            {
                const auto span = maxVal - minVal;
                return span > 0.0 ? (float) juce::jlimit (0.0, 1.0, (defVal - minVal) / span) : 0.0f;
            }

            juce::String getText (float v, int) const override
            {
                const auto plain = minVal + (double) v * (maxVal - minVal);
                if (owner.extParams != nullptr && owner.extParams->value_to_text != nullptr && owner.plugin != nullptr)
                {
                    char buf[128] {};
                    if (owner.extParams->value_to_text (owner.plugin, clapId, plain, buf, sizeof (buf)))
                        return juce::String::fromUTF8 (buf);
                }
                return juce::String (plain, stepped ? 0 : 2);
            }

            float getValueForText (const juce::String& text) const override
            {
                const auto span = maxVal - minVal;
                if (span <= 0.0)
                    return 0.0f;
                return (float) juce::jlimit (0.0, 1.0, (text.getDoubleValue() - minVal) / span);
            }

            void syncPlainFromPlugin (double plain) noexcept
            {
                currentPlain.store (juce::jlimit (minVal, maxVal, plain), std::memory_order_relaxed);
            }

            clap_id getClapId() const noexcept { return clapId; }

        private:
            ClapPluginInstance& owner;
            clap_id clapId;
            void* cookie;
            double minVal, maxVal, defVal;
            std::atomic<double> currentPlain;
            bool stepped = false;
        };

        ClapPluginInstance (std::shared_ptr<LoadedModule> mod, const clap_plugin_t* p, const juce::PluginDescription& d)
            : AudioPluginInstance (d.isInstrument
                  ? BusesProperties().withOutput ("Output", juce::AudioChannelSet::stereo())
                  : BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo())
                                     .withOutput ("Output", juce::AudioChannelSet::stereo())),
              module (std::move (mod)), plugin (p), desc (d)
        {
            if (plugin != nullptr && plugin->get_extension != nullptr)
            {
                extParams  = static_cast<const clap_plugin_params_t*>  (plugin->get_extension (plugin, CLAP_EXT_PARAMS));
                extState   = static_cast<const clap_plugin_state_t*>   (plugin->get_extension (plugin, CLAP_EXT_STATE));
                extLatency = static_cast<const clap_plugin_latency_t*> (plugin->get_extension (plugin, CLAP_EXT_LATENCY));
                extTail    = static_cast<const clap_plugin_tail_t*>    (plugin->get_extension (plugin, CLAP_EXT_TAIL));
                extGui     = static_cast<const clap_plugin_gui_t*>     (plugin->get_extension (plugin, CLAP_EXT_GUI));
            }

            if (extParams != nullptr && extParams->count != nullptr && extParams->get_info != nullptr)
            {
                const auto n = extParams->count (plugin);
                for (uint32_t i = 0; i < n; ++i)
                {
                    clap_param_info_t info {};
                    if (extParams->get_info (plugin, i, &info))
                    {
                        auto param = std::make_unique<ClapParameter> (*this, info);
                        if (extParams->get_value != nullptr)
                        {
                            double cur = info.default_value;
                            if (extParams->get_value (plugin, info.id, &cur))
                                param->syncPlainFromPlugin (cur);
                        }
                        paramMap[info.id] = param.get();
                        addHostedParameter (std::move (param));
                    }
                }
            }
        }

        ~ClapPluginInstance() override
        {
            if (plugin != nullptr)
            {
                if (processing)
                {
                    plugin->stop_processing (plugin);
                    processing = false;
                }
                if (activated)
                {
                    plugin->deactivate (plugin);
                    activated = false;
                }
                plugin->destroy (plugin);
                plugin = nullptr;
            }
        }

        void fillInPluginDescription (juce::PluginDescription& d) const override { d = desc; }
        const juce::String getName() const override { return desc.name; }
        bool acceptsMidi() const override  { return desc.isInstrument || desc.category == "MIDI FX"; }
        bool producesMidi() const override { return desc.category == "MIDI FX"; }
        double getTailLengthSeconds() const override
        {
            if (extTail != nullptr && extTail->get != nullptr && plugin != nullptr && currentRate > 0.0)
            {
                const auto t = extTail->get (plugin);
                return t == UINT32_MAX ? 0.0 : (double) t / currentRate;
            }
            return 0.0;
        }

        bool isBusesLayoutSupported (const BusesLayout& layout) const override
        {
            const auto out = layout.getMainOutputChannelSet();
            if (desc.isInstrument)
                return layout.getMainInputChannelSet().isDisabled() && (out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo());
            const auto in = layout.getMainInputChannelSet();
            return in == out && (in == juce::AudioChannelSet::mono() || in == juce::AudioChannelSet::stereo());
        }

        void prepareToPlay (double sampleRate, int maxBlockSize) override
        {
            if (plugin == nullptr)
                return;

            currentRate = juce::jmax (8000.0, sampleRate);
            const auto block = (uint32_t) juce::jmax (1, maxBlockSize);

            if (processing) { plugin->stop_processing (plugin); processing = false; }
            if (activated)  { plugin->deactivate (plugin); activated = false; }

            if (plugin->activate (plugin, currentRate, 1, block))
            {
                activated = true;
                processing = plugin->start_processing (plugin);
            }

            if (extLatency != nullptr && extLatency->get != nullptr)
                setLatencySamples ((int) extLatency->get (plugin));

            inCopy.setSize (2, (int) block);
            outCopy.setSize (2, (int) block);
        }

        void releaseResources() override
        {
            if (plugin != nullptr)
            {
                if (processing) { plugin->stop_processing (plugin); processing = false; }
                if (activated)  { plugin->deactivate (plugin); activated = false; }
            }
        }

        void reset() override
        {
            if (plugin != nullptr && plugin->reset != nullptr && activated)
                plugin->reset (plugin);
        }

        void processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi) override
        {
            const auto numSamples = buffer.getNumSamples();
            if (plugin == nullptr || ! activated || numSamples <= 0)
                return;

            if (! processing)
                processing = plugin->start_processing (plugin);

            if (inCopy.getNumSamples() < numSamples)
            {
                inCopy.setSize (2, numSamples, false, false, true);
                outCopy.setSize (2, numSamples, false, false, true);
            }

            const auto numCh = juce::jmin (2, buffer.getNumChannels());
            inCopy.clear();
            outCopy.clear();
            for (int c = 0; c < numCh; ++c)
                inCopy.copyFrom (c, 0, buffer, c, 0, numSamples);
            if (numCh == 1)
                inCopy.copyFrom (1, 0, buffer, 0, 0, numSamples);

            inEvents.clear();
            outEvents.clear();

            // Flush queued host parameter changes
            {
                const juce::SpinLock::ScopedLockType sl (pendingParamLock);
                for (const auto& pc : pendingParams)
                    inEvents.addParamValue (0, pc.id, pc.cookie, pc.plain);
                pendingParams.clear();
            }

            // Convert incoming JUCE MIDI to CLAP events
            for (const auto meta : midi)
            {
                const auto msg = meta.getMessage();
                const auto pos = (uint32_t) juce::jlimit (0, numSamples - 1, meta.samplePosition);
                if (msg.isNoteOn())
                    inEvents.addNote (CLAP_EVENT_NOTE_ON, pos, (int16_t) (msg.getChannel() - 1), (int16_t) msg.getNoteNumber(), (double) msg.getFloatVelocity());
                else if (msg.isNoteOff())
                    inEvents.addNote (CLAP_EVENT_NOTE_OFF, pos, (int16_t) (msg.getChannel() - 1), (int16_t) msg.getNoteNumber(), (double) msg.getFloatVelocity());
                else if (msg.getRawDataSize() > 0 && msg.getRawDataSize() <= 3)
                    inEvents.addRawMidi (pos, msg.getRawData(), msg.getRawDataSize());
            }

            clap_event_transport_t transport {};
            transport.header.size = sizeof (clap_event_transport_t);
            transport.header.space_id = CLAP_CORE_EVENT_SPACE_ID;
            transport.header.type = CLAP_EVENT_TRANSPORT;
            transport.tempo = 120.0;
            if (auto* ph = getPlayHead())
            {
                if (const auto pos = ph->getPosition())
                {
                    if (const auto b = pos->getBpm(); b.hasValue())
                    {
                        transport.tempo = *b;
                        transport.flags |= CLAP_TRANSPORT_HAS_TEMPO;
                    }
                    if (pos->getIsPlaying())
                        transport.flags |= CLAP_TRANSPORT_IS_PLAYING;
                    if (const auto ppq = pos->getPpqPosition(); ppq.hasValue())
                    {
                        transport.song_pos_beats = (clap_beattime) std::llround (*ppq * (double) CLAP_BEATTIME_FACTOR);
                        transport.flags |= CLAP_TRANSPORT_HAS_BEATS_TIMELINE;
                    }
                }
            }

            float* inPtrs[2] = { inCopy.getWritePointer (0), inCopy.getWritePointer (1) };
            float* outPtrs[2] = { outCopy.getWritePointer (0), outCopy.getWritePointer (1) };

            clap_audio_buffer_t inAudio {};
            inAudio.data32 = inPtrs;
            inAudio.channel_count = 2;

            clap_audio_buffer_t outAudio {};
            outAudio.data32 = outPtrs;
            outAudio.channel_count = 2;

            auto inList = inEvents.asInput();
            auto outList = outEvents.asOutput();

            clap_process_t proc {};
            proc.steady_time = steadyTime;
            proc.frames_count = (uint32_t) numSamples;
            proc.transport = &transport;
            proc.audio_inputs = desc.isInstrument ? nullptr : &inAudio;
            proc.audio_inputs_count = desc.isInstrument ? 0u : 1u;
            proc.audio_outputs = &outAudio;
            proc.audio_outputs_count = 1u;
            proc.in_events = &inList;
            proc.out_events = &outList;

            plugin->process (plugin, &proc);
            steadyTime += numSamples;

            for (int c = 0; c < numCh; ++c)
                buffer.copyFrom (c, 0, outCopy, c, 0, numSamples);

            // Convert CLAP output events back to MIDI / parameter updates
            if (! outEvents.events.empty())
            {
                if (producesMidi())
                    midi.clear();

                for (const auto& ev : outEvents.events)
                {
                    if (ev.header.space_id != CLAP_CORE_EVENT_SPACE_ID)
                        continue;
                    if (ev.header.type == CLAP_EVENT_PARAM_VALUE)
                    {
                        if (const auto it = paramMap.find (ev.paramValue.param_id); it != paramMap.end())
                            it->second->syncPlainFromPlugin (ev.paramValue.value);
                    }
                    else if (producesMidi())
                    {
                        const auto pos = (int) juce::jlimit (0u, (uint32_t) (numSamples - 1), ev.header.time);
                        if (ev.header.type == CLAP_EVENT_NOTE_ON)
                            midi.addEvent (juce::MidiMessage::noteOn (ev.note.channel + 1, ev.note.key, (float) ev.note.velocity), pos);
                        else if (ev.header.type == CLAP_EVENT_NOTE_OFF)
                            midi.addEvent (juce::MidiMessage::noteOff (ev.note.channel + 1, ev.note.key, (float) ev.note.velocity), pos);
                        else if (ev.header.type == CLAP_EVENT_MIDI)
                            midi.addEvent (ev.midi.data, 3, pos);
                    }
                }
            }
        }

        juce::AudioProcessorEditor* createEditor() override { return new juce::GenericAudioProcessorEditor (*this); }
        bool hasEditor() const override { return true; }

        int getNumPrograms() override { return 1; }
        int getCurrentProgram() override { return 0; }
        void setCurrentProgram (int) override {}
        const juce::String getProgramName (int) override { return {}; }
        void changeProgramName (int, const juce::String&) override {}

        void getStateInformation (juce::MemoryBlock& dest) override
        {
            if (extState == nullptr || extState->save == nullptr || plugin == nullptr)
                return;

            struct Writer { juce::MemoryBlock* out; };
            Writer w { &dest };
            clap_ostream_t stream {};
            stream.ctx = &w;
            stream.write = [] (const clap_ostream_t* s, const void* buf, uint64_t size) -> int64_t
            {
                static_cast<Writer*> (s->ctx)->out->append (buf, (size_t) size);
                return (int64_t) size;
            };
            extState->save (plugin, &stream);
        }

        void setStateInformation (const void* data, int sizeInBytes) override
        {
            if (extState == nullptr || extState->load == nullptr || plugin == nullptr || data == nullptr || sizeInBytes <= 0)
                return;

            struct Reader { const uint8_t* ptr; size_t size, pos; };
            Reader r { static_cast<const uint8_t*> (data), (size_t) sizeInBytes, 0 };
            clap_istream_t stream {};
            stream.ctx = &r;
            stream.read = [] (const clap_istream_t* s, void* buf, uint64_t size) -> int64_t
            {
                auto* rd = static_cast<Reader*> (s->ctx);
                const auto avail = rd->size - rd->pos;
                const auto take = (size_t) std::min<uint64_t> (size, avail);
                if (take > 0)
                {
                    std::memcpy (buf, rd->ptr + rd->pos, take);
                    rd->pos += take;
                }
                return (int64_t) take;
            };

            if (extState->load (plugin, &stream) && extParams != nullptr && extParams->get_value != nullptr)
            {
                for (auto& [cid, param] : paramMap)
                {
                    double cur = 0.0;
                    if (extParams->get_value (plugin, cid, &cur))
                        param->syncPlainFromPlugin (cur);
                }
            }
        }

        void queueParamChange (clap_id id, void* cookie, double plain)
        {
            const juce::SpinLock::ScopedLockType sl (pendingParamLock);
            pendingParams.push_back ({ id, cookie, plain });
        }

        static clap_host_t makeHost (void* hostData) noexcept
        {
            clap_host_t h {};
            h.clap_version = CLAP_VERSION;
            h.host_data = hostData;
            h.name = "Torqwave";
            h.vendor = "Torqwave";
            h.url = "https://bobclawblaw.github.io/torqwave/";
            h.version = "0.23.0";
            h.get_extension = [] (const clap_host_t*, const char*) -> const void* { return nullptr; };
            h.request_restart = [] (const clap_host_t*) {};
            h.request_process = [] (const clap_host_t*) {};
            h.request_callback = [] (const clap_host_t*) {};
            return h;
        }

    private:
        struct ParamChange { clap_id id; void* cookie; double plain; };

        std::shared_ptr<LoadedModule> module;
        const clap_plugin_t* plugin = nullptr;
        juce::PluginDescription desc;

        const clap_plugin_params_t*  extParams  = nullptr;
        const clap_plugin_state_t*   extState   = nullptr;
        const clap_plugin_latency_t* extLatency = nullptr;
        const clap_plugin_tail_t*    extTail    = nullptr;
        const clap_plugin_gui_t*     extGui     = nullptr;

        bool activated = false, processing = false;
        double currentRate = 48000.0;
        int64_t steadyTime = 0;

        juce::AudioBuffer<float> inCopy, outCopy;
        EventQueue inEvents, outEvents;
        std::map<clap_id, ClapParameter*> paramMap;
        juce::SpinLock pendingParamLock;
        std::vector<ParamChange> pendingParams;

        JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ClapPluginInstance)
    };

    // Host struct kept alive inside a small holder alongside the plugin
    static clap_host_t& sharedHost() noexcept
    {
        static clap_host_t h = ClapPluginInstance::makeHost (nullptr);
        return h;
    }

    std::unique_ptr<juce::AudioPluginInstance> instantiateFromFactory (std::shared_ptr<LoadedModule> mod,
                                                                       const clap_plugin_factory_t* factory,
                                                                       const juce::File& file,
                                                                       const juce::String& wantedId,
                                                                       double sampleRate, int blockSize,
                                                                       juce::String& error)
    {
        if (factory == nullptr || factory->get_plugin_count == nullptr || factory->get_plugin_descriptor == nullptr || factory->create_plugin == nullptr)
        {
            error = "CLAP factory missing required callbacks.";
            return nullptr;
        }

        const auto count = factory->get_plugin_count (factory);
        const clap_plugin_descriptor_t* chosen = nullptr;

        for (uint32_t i = 0; i < count; ++i)
        {
            const auto* d = factory->get_plugin_descriptor (factory, i);
            if (d == nullptr || d->id == nullptr)
                continue;
            if (wantedId.isEmpty() || wantedId == juce::String::fromUTF8 (d->id))
            {
                chosen = d;
                break;
            }
        }

        if (chosen == nullptr)
        {
            error = "CLAP plug-in ID not found: " + wantedId;
            return nullptr;
        }

        const auto* plugin = factory->create_plugin (factory, &sharedHost(), chosen->id);
        if (plugin == nullptr)
        {
            error = "CLAP factory failed to create plug-in " + juce::String::fromUTF8 (chosen->id);
            return nullptr;
        }

        if (plugin->init != nullptr && ! plugin->init (plugin))
        {
            if (plugin->destroy != nullptr)
                plugin->destroy (plugin);
            error = "CLAP plugin::init() failed for " + juce::String::fromUTF8 (chosen->id);
            return nullptr;
        }

        auto inst = std::make_unique<ClapPluginInstance> (std::move (mod), plugin, makeDescription (file, chosen));
        inst->setRateAndBufferSizeDetails (sampleRate, blockSize);
        return inst;
    }

    //==========================================================================
    class ClapPluginFormat final : public juce::AudioPluginFormat
    {
    public:
        juce::String getName() const override { return formatName; }

        void findAllTypesForFile (juce::OwnedArray<juce::PluginDescription>& results, const juce::String& fileOrId) override
        {
            const auto [path, wantedId] = decodeIdentifier (fileOrId);
            const juce::File f (path);
            juce::String err;
            auto mod = loadModule (f, err);
            if (mod == nullptr || mod->entry == nullptr || mod->entry->get_factory == nullptr)
                return;

            const auto* factory = static_cast<const clap_plugin_factory_t*> (mod->entry->get_factory (CLAP_PLUGIN_FACTORY_ID));
            if (factory == nullptr || factory->get_plugin_count == nullptr || factory->get_plugin_descriptor == nullptr)
                return;

            const auto count = factory->get_plugin_count (factory);
            for (uint32_t i = 0; i < count; ++i)
            {
                const auto* desc = factory->get_plugin_descriptor (factory, i);
                if (desc == nullptr)
                    continue;
                if (wantedId.isEmpty() || (desc->id != nullptr && wantedId == juce::String::fromUTF8 (desc->id)))
                    results.add (new juce::PluginDescription (makeDescription (f, desc)));
            }
        }

        bool fileMightContainThisPluginType (const juce::String& fileOrId) override
        {
            const auto [path, _] = decodeIdentifier (fileOrId);
            const juce::File f (path);
            return f.hasFileExtension (".clap") && f.exists();
        }

        juce::String getNameOfPluginFromIdentifier (const juce::String& fileOrId) override
        {
            const auto [path, pid] = decodeIdentifier (fileOrId);
            return pid.isNotEmpty() ? pid : juce::File (path).getFileNameWithoutExtension();
        }

        bool pluginNeedsRescanning (const juce::PluginDescription&) override { return false; }

        bool doesPluginStillExist (const juce::PluginDescription& d) override
        {
            const auto [path, _] = decodeIdentifier (d.fileOrIdentifier);
            return juce::File (path).exists();
        }

        bool canScanForPlugins() const override { return true; }
        bool isTrivialToScan() const override   { return false; }

        juce::StringArray searchPathsForPlugins (const juce::FileSearchPath& dirs, bool recursive, bool) override
        {
            juce::StringArray found;
            for (int i = 0; i < dirs.getNumPaths(); ++i)
            {
                const auto dir = dirs[i];
                if (! dir.isDirectory())
                    continue;
                // On macOS .clap is a bundle directory; on Windows/Linux it's a file.
                const int what = juce::File::findFilesAndDirectories;
                for (const auto& entry : dir.findChildFiles (what, recursive, "*.clap"))
                    found.addIfNotAlreadyThere (entry.getFullPathName());
            }
            return found;
        }

        juce::FileSearchPath getDefaultLocationsToSearch() override { return defaultSearchPaths(); }
        bool requiresUnblockedMessageThreadDuringCreation (const juce::PluginDescription&) const override { return false; }

    protected:
        void createPluginInstance (const juce::PluginDescription& d, double sampleRate, int blockSize, PluginCreationCallback done) override
        {
            const auto [path, wantedId] = decodeIdentifier (d.fileOrIdentifier);
            const juce::File f (path);
            juce::String err;
            auto mod = loadModule (f, err);
            if (mod == nullptr)
            {
                done (nullptr, err);
                return;
            }

            const auto* factory = mod->entry->get_factory != nullptr
                ? static_cast<const clap_plugin_factory_t*> (mod->entry->get_factory (CLAP_PLUGIN_FACTORY_ID))
                : nullptr;
            auto inst = instantiateFromFactory (std::move (mod), factory, f, wantedId, sampleRate, blockSize, err);
            done (std::move (inst), err);
        }
    };
}

std::unique_ptr<juce::AudioPluginFormat> createFormat()
{
    return std::make_unique<ClapPluginFormat>();
}

juce::FileSearchPath defaultSearchPaths()
{
    juce::FileSearchPath paths;
   #if JUCE_MAC
    paths.add (juce::File ("/Library/Audio/Plug-Ins/CLAP"));
    paths.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile ("Library/Audio/Plug-Ins/CLAP"));
   #elif JUCE_WINDOWS
    const auto common = juce::SystemStats::getEnvironmentVariable ("CommonProgramFiles", "C:\\Program Files\\Common Files");
    paths.add (juce::File (common).getChildFile ("CLAP"));
    const auto local = juce::SystemStats::getEnvironmentVariable ("LOCALAPPDATA", {});
    if (local.isNotEmpty())
        paths.add (juce::File (local).getChildFile ("Programs\\Common\\CLAP"));
   #else
    paths.add (juce::File ("/usr/lib/clap"));
    paths.add (juce::File ("/usr/local/lib/clap"));
    paths.add (juce::File::getSpecialLocation (juce::File::userHomeDirectory).getChildFile (".clap"));
   #endif

    const auto env = juce::SystemStats::getEnvironmentVariable ("CLAP_PATH", {});
    if (env.isNotEmpty())
    {
        const auto sep =
           #if JUCE_WINDOWS
            ";";
           #else
            ":";
           #endif
        for (const auto& p : juce::StringArray::fromTokens (env, sep, {}))
            if (p.trim().isNotEmpty())
                paths.addIfNotAlreadyThere (juce::File (p.trim()));
    }
    return paths;
}

std::unique_ptr<juce::AudioPluginInstance> createFromEntry (const clap_plugin_entry_t* entry,
                                                            const juce::String& pluginId,
                                                            double sampleRate, int maxBlockSize,
                                                            juce::String& error)
{
    if (entry == nullptr || entry->get_factory == nullptr)
    {
        error = "Invalid CLAP entry point.";
        return nullptr;
    }

    auto mod = std::make_shared<LoadedModule>();
    mod->entry = entry;
    mod->ownsEntry = false;
    const auto* factory = static_cast<const clap_plugin_factory_t*> (entry->get_factory (CLAP_PLUGIN_FACTORY_ID));
    return instantiateFromFactory (std::move (mod), factory, juce::File(), pluginId, sampleRate, maxBlockSize, error);
}
}
