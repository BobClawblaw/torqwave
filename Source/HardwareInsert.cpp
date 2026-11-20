#include "HardwareInsert.h"
#include "Theme.h"

namespace HardwareInsert
{
namespace
{
    bool validPair (int first) noexcept   { return first >= 0 && first < maxChannels - 1 && first % 2 == 0; }

    juce::String pairName (int first)     { return juce::String (first + 1) + "/" + juce::String (first + 2); }

    /** Runs the inner callback inside the Port's cycle. */
    class Wrapper final : public juce::AudioIODeviceCallback
    {
    public:
        Wrapper (Port& p, juce::AudioIODeviceCallback& i) : port (p), inner (i) {}

        void audioDeviceIOCallbackWithContext (const float* const* inputs, int numInputs, float* const* outputs, int numOutputs,
                                               int numSamples, const juce::AudioIODeviceCallbackContext& context) override
        {
            port.beginCycle (inputs, numInputs, numSamples);
            inner.audioDeviceIOCallbackWithContext (inputs, numInputs, outputs, numOutputs, numSamples, context);
            port.endCycle (outputs, numOutputs, numSamples);
        }

        void audioDeviceAboutToStart (juce::AudioIODevice* device) override
        {
            port.deviceStarting (device->getCurrentSampleRate(), device->getActiveInputChannels(), device->getActiveOutputChannels(),
                                 device->getInputLatencyInSamples() + device->getOutputLatencyInSamples());
            inner.audioDeviceAboutToStart (device);
        }

        void audioDeviceStopped() override
        {
            port.deviceStopped();
            inner.audioDeviceStopped();
        }

        void audioDeviceError (const juce::String& message) override   { inner.audioDeviceError (message); }

    private:
        Port& port;
        juce::AudioIODeviceCallback& inner;
    };
}

//==============================================================================
Port::Port()
{
    for (auto& ring : inputs)
        ring.assign ((size_t) ringSize, 0.0f);

    for (auto& slot : slots)
        for (auto& ring : slot.ring)
            ring.assign ((size_t) ringSize, 0.0f);

    inputIndex.fill (-1);
    outputIndex.fill (-1);
}

Port& Port::shared()
{
    static Port port;
    return port;
}

std::unique_ptr<juce::AudioIODeviceCallback> Port::wrap (juce::AudioIODeviceCallback& inner)
{
    return std::make_unique<Wrapper> (*this, inner);
}

void Port::deviceStarting (double rate, const juce::BigInteger& activeInputs, const juce::BigInteger& activeOutputs, int reportedRoundTrip)
{
    // A cycle's buffers are the active channels in order.
    const auto index = [] (const juce::BigInteger& active, std::array<int, maxChannels>& into)
    {
        int next = 0;

        for (int ch = 0; ch < maxChannels; ++ch)
            into[(size_t) ch] = active[ch] ? next++ : -1;
    };

    index (activeInputs, inputIndex);
    index (activeOutputs, outputIndex);
    reported = juce::jmax (0, reportedRoundTrip);
    deviceRate = rate;
}

void Port::deviceStopped()
{
    deviceRate = 0.0;
    inCycle = false;

    // A ping going when the device stops finds nothing.
    if (pingState.load() == PingState::waiting)
    {
        pingAsked = false;
        pingState = PingState::failed;
    }
}

void Port::beginCycle (const float* const* in, int numInputs, int numFrames)
{
    const auto start = nextFrame;

    for (int ch = 0; ch < maxChannels; ++ch)
    {
        const auto index = inputIndex[(size_t) ch];
        const auto* source = juce::isPositiveAndBelow (index, numInputs) ? in[index] : nullptr;
        auto* ring = inputs[(size_t) ch].data();

        for (int i = 0; i < numFrames; ++i)
            ring[(start + i) & mask] = source != nullptr ? source[i] : 0.0f;
    }

    // A ping's click coming back: the loudest frame of the input pair after it went out.
    if (pingState.load() == PingState::waiting && pingAt >= 0)
    {
        const auto first = pingInput.load();

        for (int i = 0; i < numFrames; ++i)
        {
            const auto frame = start + i;

            if (frame <= pingAt)
                continue;

            if (frame - pingAt > maxRoundTrip)
            {
                pingResult = pingPeak >= pingThreshold ? (int) (pingPeakAt - pingAt) : -1;
                pingAsked = false;
                pingState = pingPeak >= pingThreshold ? PingState::done : PingState::failed;
                break;
            }

            const auto level = juce::jmax (std::abs (inputs[(size_t) first][(size_t) (frame & mask)]),
                                           std::abs (inputs[(size_t) first + 1][(size_t) (frame & mask)]));

            if (level > pingPeak)
            {
                pingPeak = level;
                pingPeakAt = frame;
            }
        }
    }

    cycleStart = start;
    cycleEnd = start + numFrames;
    cycleThread = juce::Thread::getCurrentThreadId();
    ++cycle;
    inCycle = true;
}

void Port::endCycle (float* const* outputs, int numOutputs, int numFrames)
{
    inCycle = false;
    const auto start = cycleStart.load();
    const auto pinging = pingState.load() == PingState::waiting;
    const auto pingOut = pinging ? pingOutput.load() : -1;

    // Each insert's sends for this cycle's frames go out, and leave its ring.
    for (auto& slot : slots)
    {
        if (slot.owner.load() == nullptr)
            continue;

        const auto first = slot.outputFirst.load();

        for (int side = 0; side < 2; ++side)
        {
            auto* ring = slot.ring[(size_t) side].data();
            const auto index = validPair (first) && first != pingOut ? outputIndex[(size_t) (first + side)] : -1;
            auto* dest = juce::isPositiveAndBelow (index, numOutputs) ? outputs[index] : nullptr;

            for (int i = 0; i < numFrames; ++i)
            {
                auto& value = ring[(start + i) & mask];

                if (dest != nullptr)
                    dest[i] += value;

                value = 0.0f;
            }
        }
    }

    // A ping asked for: its click goes out at this cycle's first frame.
    if (pinging && pingAsked.load() && pingAt < 0)
    {
        for (int side = 0; side < 2; ++side)
            if (const auto index = outputIndex[(size_t) (pingOut + side)]; juce::isPositiveAndBelow (index, numOutputs) && outputs[index] != nullptr)
                outputs[index][0] += pingLevel;

        pingAt = start;
        pingPeak = 0.0f;
        pingPeakAt = -1;
    }

    nextFrame = start + numFrames;
}

bool Port::startPing (int outputFirst, int inputFirst)
{
    if (pingState.load() == PingState::waiting || deviceRate.load() <= 0.0 || ! validPair (outputFirst) || ! validPair (inputFirst)
        || outputIndex[(size_t) outputFirst] < 0 || outputIndex[(size_t) outputFirst + 1] < 0
        || inputIndex[(size_t) inputFirst] < 0 || inputIndex[(size_t) inputFirst + 1] < 0)
        return false;

    pingOutput = outputFirst;
    pingInput = inputFirst;
    pingResult = -1;
    pingAt = -1;
    pingAsked = true;
    pingState = PingState::waiting;
    return true;
}

void Port::clearPing()
{
    if (pingState.load() != PingState::waiting)
        pingState = PingState::idle;
}

int Port::acquire (const void* owner)
{
    for (int i = 0; i < maxInserts; ++i)
    {
        const void* none = nullptr;

        if (slots[(size_t) i].owner.compare_exchange_strong (none, owner))
        {
            clearSends (i);
            return i;
        }
    }

    return -1;
}

void Port::release (int slot)
{
    if (juce::isPositiveAndBelow (slot, maxInserts))
    {
        slots[(size_t) slot].outputFirst = -1;
        slots[(size_t) slot].owner = nullptr;
    }
}

bool Port::isLive (double rate) const noexcept
{
    return inCycle.load() && cycleThread.load() == juce::Thread::getCurrentThreadId() && juce::exactlyEqual (deviceRate.load(), rate);
}

void Port::send (int slot, int outputFirst, juce::int64 frame, float left, float right) noexcept
{
    // Frames already gone out, or so far ahead the ring would wrap onto ones waiting, are dropped.
    const auto start = cycleStart.load();

    if (! juce::isPositiveAndBelow (slot, maxInserts) || frame < start || frame - start >= ringSize / 2)
        return;

    auto& s = slots[(size_t) slot];
    s.outputFirst = validPair (outputFirst) ? outputFirst : -1;
    s.ring[0][(size_t) (frame & mask)] += left;
    s.ring[1][(size_t) (frame & mask)] += right;
}

void Port::clearSends (int slot) noexcept
{
    if (juce::isPositiveAndBelow (slot, maxInserts))
        for (auto& ring : slots[(size_t) slot].ring)
            std::fill (ring.begin(), ring.end(), 0.0f);
}

std::pair<float, float> Port::received (int inputFirst, juce::int64 frame) const noexcept
{
    const auto end = cycleEnd.load();

    if (! validPair (inputFirst) || frame >= end || frame < end - ringSize / 2)
        return { 0.0f, 0.0f };

    return { inputs[(size_t) inputFirst][(size_t) (frame & mask)], inputs[(size_t) inputFirst + 1][(size_t) (frame & mask)] };
}

bool Port::isPinging (int outputFirst, int inputFirst) const noexcept
{
    return pingState.load() == PingState::waiting && (pingOutput.load() == outputFirst || pingInput.load() == inputFirst);
}

//==============================================================================
Processor::Processor (Port& p)
    : BuiltInEffect (description(), "HARDWAREINSERT"), port (p)
{
    sendDb = addFloat ("send", "Send Level", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    returnDb = addFloat ("return", "Return Level", { -24.0f, 24.0f, 0.1f }, 0.0f, "dB");
    updateLatency();
}

Processor::~Processor()
{
    port.release (slot);
}

juce::PluginDescription Processor::description()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = "Hardware Insert";
    d.pluginFormatName = BuiltInEffects::formatName;
    d.category = "Routing";
    d.manufacturerName = "Torqwave";
    d.version = "1";
    d.fileOrIdentifier = identifier;
    d.uniqueId = d.deprecatedUid = 0x53424849;   // "SBHI"
    d.numInputChannels = d.numOutputChannels = 2;
    return d;
}

void Processor::setPairs (int output, int input)
{
    output = validPair (output) ? output : 0;
    input = validPair (input) ? input : 0;

    if (output == outputFirst.load() && input == inputFirst.load())
        return;

    outputFirst = output;
    inputFirst = input;

    if (onPairsChanged != nullptr)
        onPairsChanged();
}

bool Processor::ping()
{
    pinging = port.startPing (outputFirst.load(), inputFirst.load());
    return pinging;
}

Port::PingState Processor::collectPing()
{
    if (! pinging)
        return Port::PingState::idle;

    const auto state = port.getPingState();

    if (state == Port::PingState::waiting)
        return state;

    pinging = false;

    if (state == Port::PingState::done)
    {
        measured = port.getPingResult();
        measuredRate = port.getDeviceRate();
        updateLatency();
    }

    port.clearPing();
    return state;
}

void Processor::updateLatency()
{
    // The measured round trip at this rate, else the device's own estimate.
    const auto roundTrip = measured >= 0 && juce::exactlyEqual (measuredRate, rate) ? measured : port.getReportedRoundTrip();
    setLatencySamples (juce::jlimit (0, Port::ringSize - 1, roundTrip + Port::guard));
}

void Processor::addToState (juce::XmlElement& xml) const
{
    xml.setAttribute ("output", outputFirst.load());
    xml.setAttribute ("input", inputFirst.load());

    if (measured >= 0)
    {
        xml.setAttribute ("roundTrip", measured);
        xml.setAttribute ("roundTripRate", measuredRate);
    }
}

void Processor::readFromState (const juce::XmlElement& xml)
{
    setPairs (xml.getIntAttribute ("output", 0), xml.getIntAttribute ("input", 0));
    measured = juce::jlimit (-1, Port::maxRoundTrip, xml.getIntAttribute ("roundTrip", -1));
    measuredRate = xml.getDoubleAttribute ("roundTripRate");
    updateLatency();
}

void Processor::prepareToPlay (double sampleRate, int)
{
    rate = sampleRate;
    delay.setSize (2, Port::ringSize);
    updateLatency();
    reset();
}

void Processor::reset()
{
    delay.clear();
    delayWrite = 0;
    running = false;
}

void Processor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    const auto numFrames = buffer.getNumSamples();
    const auto channels = juce::jmin (2, buffer.getNumChannels());
    const auto latency = getLatencySamples();

    if (port.isLive (rate) && slot < 0)
        slot = port.acquire (this);

    if (! port.isLive (rate) || slot < 0 || channels == 0 || delay.getNumSamples() < Port::ringSize)
    {
        // Away from the hardware: the audio passes through, as late as the insert would make it.
        running = false;
        live = false;

        if (delay.getNumSamples() < Port::ringSize)   // not prepared
            return;

        for (int ch = 0; ch < channels; ++ch)
        {
            auto* data = buffer.getWritePointer (ch);
            auto* ring = delay.getWritePointer (ch);

            for (int i = 0; i < numFrames; ++i)
            {
                const auto at = (delayWrite + i) % Port::ringSize;
                ring[at] = data[i];
                data[i] = ring[(at - latency + Port::ringSize) % Port::ringSize];
            }
        }

        delayWrite = (delayWrite + numFrames) % Port::ringSize;
        return;
    }

    live = true;

    // The first block of a cycle: still in step with the device, or start again at its first frame.
    if (const auto cycle = port.getCycle(); ! running || cycle != lastCycle)
    {
        const auto start = port.getCycleStart();

        if (! running || position < start || position > start + Port::guard)
        {
            position = start;
            silentUntil = start + latency;
            port.clearSends (slot);
        }

        lastCycle = cycle;
        running = true;
    }

    const auto out = outputFirst.load(), in = inputFirst.load();
    const auto quiet = port.isPinging (out, in);
    const auto sendGain = juce::Decibels::decibelsToGain (sendDb->get());
    const auto returnGain = juce::Decibels::decibelsToGain (returnDb->get());
    auto* left = buffer.getWritePointer (0);
    auto* right = buffer.getWritePointer (channels - 1);

    for (int i = 0; i < numFrames; ++i, ++position)
    {
        if (! quiet)
            port.send (slot, out, position, left[i] * sendGain, right[i] * sendGain);

        const auto back = quiet || position < silentUntil ? std::pair { 0.0f, 0.0f } : port.received (in, position - Port::guard);
        left[i] = back.first * returnGain;

        if (channels > 1)
            right[i] = back.second * returnGain;
    }
}

//==============================================================================
namespace
{
    /** The insert's window: its pairs, its levels, a ping and the latency it reports. */
    class Editor final : public juce::AudioProcessorEditor,
                         private juce::Timer
    {
    public:
        explicit Editor (Processor& p) : AudioProcessorEditor (p), insert (p), levels (p)
        {
            for (auto* box : { &output, &input })
            {
                for (int pair = 0; pair < numPairs; ++pair)
                    box->addItem (pairName (2 * pair), pair + 1);

                box->onChange = [this] { insert.setPairs (2 * (output.getSelectedId() - 1), 2 * (input.getSelectedId() - 1)); };
                addAndMakeVisible (*box);
            }

            output.setSelectedId (insert.getOutputFirst() / 2 + 1, juce::dontSendNotification);
            input.setSelectedId (insert.getInputFirst() / 2 + 1, juce::dontSendNotification);
            output.setTooltip ("The interface outputs it sends to");
            input.setTooltip ("The interface inputs it comes back on");

            for (auto* label : { &outputLabel, &inputLabel })
                addAndMakeVisible (*label);

            pingButton.setTooltip ("Send a click out of the outputs and time it coming back on the inputs: the round trip delay compensation uses");
            pingButton.onClick = [this]
            {
                if (! insert.ping())
                    status = "Can't ping: the audio device isn't running, or those outputs or inputs aren't on.";

                showLatency();
            };
            addAndMakeVisible (pingButton);
            addAndMakeVisible (latencyLabel);
            addAndMakeVisible (levels);
            showLatency();
            setSize (420, 140 + levels.getHeight());
            startTimerHz (10);
        }

        void paint (juce::Graphics& g) override
        {
            g.fillAll (Theme::surface);
        }

        void resized() override
        {
            auto r = getLocalBounds().reduced (10);
            auto row = r.removeFromTop (26);
            outputLabel.setBounds (row.removeFromLeft (60));
            output.setBounds (row.removeFromLeft (90));
            row.removeFromLeft (20);
            inputLabel.setBounds (row.removeFromLeft (50));
            input.setBounds (row.removeFromLeft (90));
            r.removeFromTop (8);
            row = r.removeFromTop (26);
            pingButton.setBounds (row.removeFromLeft (70));
            row.removeFromLeft (10);
            latencyLabel.setBounds (row.withHeight (50));
            r.removeFromTop (60);
            levels.setBounds (r);
        }

    private:
        void timerCallback() override
        {
            if (const auto was = insert.collectPing(); was == Port::PingState::done)
                status = {};
            else if (was == Port::PingState::failed)
                status = "Nothing came back: check the cables and the pairs.";

            showLatency();
        }

        void showLatency()
        {
            const auto measured = insert.getMeasuredRoundTrip();
            const auto ms = 1000.0 * insert.getLatencySamples() / juce::jmax (1.0, insert.getSampleRate());
            auto text = (measured >= 0 ? "Round trip " + juce::String (measured) + " frames, measured" : juce::String ("Round trip not measured: estimated"))
                        + "; latency " + juce::String (insert.getLatencySamples()) + " frames (" + juce::String (ms, 2) + " ms)."
                        + (insert.isLive() ? "" : " Not playing through the hardware now.");

            if (status.isNotEmpty())
                text = status;

            latencyLabel.setText (text, juce::dontSendNotification);
        }

        Processor& insert;
        juce::ComboBox output, input;
        juce::Label outputLabel { {}, "Output" }, inputLabel { {}, "Input" }, latencyLabel;
        juce::TextButton pingButton { "Ping" };
        juce::GenericAudioProcessorEditor levels;
        juce::String status;
    };
}

juce::AudioProcessorEditor* Processor::createEditor()
{
    return new Editor (*this);
}
}
