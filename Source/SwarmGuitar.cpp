#include "SwarmGuitar.h"

namespace BuiltInEffects
{
namespace
{
    constexpr double lowestHz = 20.0;
    constexpr double fadeSeconds = 0.020;      // a stolen or cut note's fade, 120 dB down over it
    constexpr float silent = 1.0e-6f;

    juce::uint32 hash (juce::uint32 x) noexcept
    {
        x ^= x >> 16;
        x *= 0x7feb352du;
        x ^= x >> 15;
        x *= 0x846ca68bu;
        x ^= x >> 16;
        return x;
    }

    /** -1 to 1, the same for the same number. */
    float scatter (juce::uint32 x) noexcept
    {
        return (float) (hash (x) >> 8) * (2.0f / 16777216.0f) - 1.0f;
    }

    double logCosh (double x) noexcept
    {
        x = std::abs (x);
        return x + std::log1p (std::exp (-2.0 * x)) - 0.6931471805599453;
    }

    struct Coefficients { float b0, b1, b2, a1, a2; };

    Coefficients lowPass (double hz, double q, double rate)
    {
        const auto w = juce::MathConstants<double>::twoPi * juce::jmin (hz, rate * 0.45) / rate, alpha = std::sin (w) / (2.0 * q), c = std::cos (w), a0 = 1.0 + alpha;
        return { (float) ((1.0 - c) * 0.5 / a0), (float) ((1.0 - c) / a0), (float) ((1.0 - c) * 0.5 / a0), (float) (-2.0 * c / a0), (float) ((1.0 - alpha) / a0) };
    }

    Coefficients highPass (double hz, double q, double rate)
    {
        const auto w = juce::MathConstants<double>::twoPi * hz / rate, alpha = std::sin (w) / (2.0 * q), c = std::cos (w), a0 = 1.0 + alpha;
        return { (float) ((1.0 + c) * 0.5 / a0), (float) (-(1.0 + c) / a0), (float) ((1.0 + c) * 0.5 / a0), (float) (-2.0 * c / a0), (float) ((1.0 - alpha) / a0) };
    }

    Coefficients peak (double hz, double q, double dB, double rate)
    {
        const auto w = juce::MathConstants<double>::twoPi * juce::jmin (hz, rate * 0.45) / rate, a = std::pow (10.0, dB / 40.0), alpha = std::sin (w) / (2.0 * q),
                   c = std::cos (w), a0 = 1.0 + alpha / a;
        return { (float) ((1.0 + alpha * a) / a0), (float) (-2.0 * c / a0), (float) ((1.0 - alpha * a) / a0), (float) (-2.0 * c / a0), (float) ((1.0 - alpha / a) / a0) };
    }

    template <typename Filter>
    void set (Filter& f, Coefficients k)
    {
        f.b0 = k.b0; f.b1 = k.b1; f.b2 = k.b2; f.a1 = k.a1; f.a2 = k.a2;
    }
}

//==============================================================================
SwarmGuitarProcessor::SwarmGuitarProcessor()
    : BuiltInEffect (swarmGuitarDescription(), "TORQWAVE_SWARM_GUITAR", false, true)
{
    strings = addFloat ("strings", "Strings", { 1.0f, (float) maxStrings, 1.0f }, 6.0f);
    tuning = addChoice ("tuning", "Tuning", { "Unison", "Low Octave", "Three Octaves", "Fifths" }, 2);
    detune = addFloat ("detune", "Detune", { 0.0f, 50.0f }, 7.0f, "ct");
    strumTime = addFloat ("strum", "Strum", { 0.0f, 80.0f }, 9.0f, "ms");
    strumRate = addFloat ("strumRate", "Strum Rate", { 0.0f, 24.0f }, 0.0f, "Hz");
    layers = addFloat ("layers", "Layers", { 1.0f, (float) maxLayers, 1.0f }, 2.0f);
    width = addFloat ("width", "Width", { 0.0f, 100.0f }, 100.0f, "%");
    pick = addFloat ("pick", "Pick Position", { 5.0f, 50.0f }, 18.0f, "%");
    brightness = addFloat ("brightness", "Brightness", { 0.0f, 100.0f }, 60.0f, "%");
    sustain = addFloat ("sustain", "Sustain", { 0.3f, 20.0f, 0.0f, 0.5f }, 6.0f, "s");
    release = addFloat ("release", "Release", { 20.0f, 3000.0f, 0.0f, 0.4f }, 180.0f, "ms");
    amp = addChoice ("amp", "Amp", { "Fuzz", "Crunch", "None" }, 0);
    drive = addFloat ("drive", "Drive", { 0.0f, 100.0f }, 70.0f, "%");
    tone = addFloat ("tone", "Tone", { 800.0f, 12000.0f, 0.0f, 0.4f }, 4500.0f, "Hz");
    level = addFloat ("level", "Level", { -40.0f, 0.0f }, -12.0f, "dB");
}

int SwarmGuitarProcessor::stringCount() const    { return juce::jlimit (1, maxStrings, juce::roundToInt (strings->get())); }
int SwarmGuitarProcessor::layerCount() const     { return juce::jlimit (1, maxLayers, juce::roundToInt (layers->get())); }

int SwarmGuitarProcessor::octaveOf (int tuningIndex, int string, int count)
{
    const auto third = juce::jmax (1, count / 3);

    switch (tuningIndex)
    {
        case 1:     return count > 1 && string < third ? -12 : 0;
        case 2:     return count < 3 ? (string == 0 && count == 2 ? -12 : 0)
                                     : string < third ? -12 : string == count - 1 ? 12 : 0;
        case 3:     { static constexpr int steps[] = { 0, 7, 12 }; return steps[string % 3] + (string >= 3 && string % 3 == 0 ? 12 : 0) - (count > 3 ? 12 : 0); }
        default:    return 0;
    }
}

void SwarmGuitarProcessor::prepareToPlay (double sampleRate, int maximumBlockSize)
{
    rate = sampleRate;
    const auto size = juce::nextPowerOfTwo ((int) (rate / lowestHz) + 8);
    mask = size - 1;

    for (auto& v : voices)
        for (auto& layer : v.strings)
            for (auto& s : layer)
                s.line.assign ((size_t) size, 0.0f);

    for (auto& b : buses)
        b.assign ((size_t) juce::jmax (1, maximumBlockSize), 0.0f);

    reset();
}

void SwarmGuitarProcessor::reset()
{
    for (auto& v : voices)
    {
        free (v);

        // From the start again, so a part renders the same twice.
        for (auto& layer : v.strings)
            for (auto& s : layer)
            {
                s.write = 0;
                s.seed = s.plucks = 0;
            }
    }

    counter = 0;
    amps = {};
    bend = 0.0f;
    volume = expression = 1.0f;
    pedal = false;
    lastGain = -1.0f;
}

double SwarmGuitarProcessor::getTailLengthSeconds() const
{
    return sustain->get();
}

int SwarmGuitarProcessor::getNumSounding() const
{
    return (int) std::count_if (voices.begin(), voices.end(), [] (const Voice& v) { return v.note >= 0; });
}

void SwarmGuitarProcessor::free (Voice& v)
{
    v.note = -1;
    v.down = v.held = v.dying = false;
    v.gain = 1.0f;

    for (auto& layer : v.strings)
        for (auto& s : layer)
        {
            s.pluckIn = s.excite = -1;
            s.lowpass = s.exciteLowpass = 0.0f;
            std::fill (s.line.begin(), s.line.end(), 0.0f);
        }
}

void SwarmGuitarProcessor::tune (Voice& v)
{
    const auto count = stringCount();
    const auto ringing = v.down || v.held;
    const auto seconds = ringing ? (double) sustain->get() : release->get() * 0.001;
    const auto c = 0.3 + 0.0068 * brightness->get();

    for (int l = 0; l < maxLayers; ++l)
        for (int n = 0; n < maxStrings; ++n)
        {
            auto& s = v.strings[(size_t) l][(size_t) n];

            // Each string a little off, each guitar a little off the others, the same every time.
            const auto cents = detune->get() * (scatter ((juce::uint32) (l * 31 + n + 1)) + 0.6f * scatter ((juce::uint32) (1000 + l)));
            auto hz = 440.0 * std::pow (2.0, ((double) v.note + bend + (double) octaveOf (tuning->getIndex(), juce::jmin (n, count - 1), count) + cents * 0.01 - 69.0) / 12.0);

            while (hz < lowestHz)
                hz *= 2.0;

            while (hz > rate / 6.0)
                hz *= 0.5;

            // The loop is the delay line and its low-pass, whose delay at the string's pitch comes off the line's.
            const auto w = juce::MathConstants<double>::twoPi * hz / rate;
            const auto filterDelay = std::atan2 ((1.0 - c) * std::sin (w), 1.0 - (1.0 - c) * std::cos (w)) / w;

            s.hz = hz;
            s.delay = juce::jmax (1.0, rate / hz - filterDelay);
            s.coefficient = (float) c;
            s.loss = (float) std::pow (10.0, -3.0 / (hz * seconds * juce::jmin (1.0, std::sqrt (220.0 / hz))));
        }
}

void SwarmGuitarProcessor::strum (Voice& v, int startingIn, bool repeated)
{
    const auto count = stringCount();
    auto apart = strumTime->get() * 0.001 * rate;

    if (repeated && strumRate->get() > 0.0f)
        apart = juce::jmin (apart, 0.5 * rate / strumRate->get() / count);

    for (int l = 0; l < maxLayers; ++l)
    {
        // Each guitar after the first comes in up to 7 ms late, differently each strum.
        const auto late = l == 0 ? 0.0 : (0.5 + 0.5 * scatter (v.age * 977u + (juce::uint32) l * 131u + v.strings[(size_t) l][0].plucks)) * 0.007 * rate;

        for (int n = 0; n < count; ++n)
        {
            auto& s = v.strings[(size_t) l][(size_t) n];
            const auto order = v.upstroke ? count - 1 - n : n;
            const auto key = v.age * 7919u + (juce::uint32) (l * 64 + n) * 104729u + s.plucks * 31u;

            s.pluckIn = startingIn + juce::jmax (0, juce::roundToInt (late + apart * (order + 0.15 * scatter (key))));
            s.pluckLevel = v.velocity * (0.85f + 0.15f * scatter (key + 1));
        }
    }

    v.upstroke = ! v.upstroke;
    v.sinceTouch = -startingIn;
}

void SwarmGuitarProcessor::noteOn (int note, float velocity)
{
    // The same note again strums the guitar that is ringing.
    for (auto& v : voices)
        if (v.note == note && ! v.dying)
        {
            v.velocity = velocity;
            v.down = true;
            v.held = false;
            v.age = ++counter;
            v.upstroke = false;
            tune (v);
            strum (v, 0, false);
            v.played = 0;
            v.strumAt = strumRate->get() > 0.0f ? rate / strumRate->get() : 0.0;
            return;
        }

    // The oldest note gives way, fading, when six are sounding.
    Voice* oldest = nullptr;
    int live = 0;

    for (auto& v : voices)
        if (v.note >= 0 && ! v.dying)
        {
            ++live;

            if (oldest == nullptr || v.age < oldest->age)
                oldest = &v;
        }

    if (live >= maxNotes && oldest != nullptr)
    {
        oldest->dying = true;

        for (auto& layer : oldest->strings)
            for (auto& s : layer)
                s.pluckIn = -1;
    }

    Voice* use = nullptr;

    for (auto& v : voices)
        if (v.note < 0)
        {
            use = &v;
            break;
        }

    if (use == nullptr)
    {
        // Every spare is still fading: the one furthest gone is taken.
        for (auto& v : voices)
            if (v.dying && (use == nullptr || v.gain < use->gain))
                use = &v;

        if (use == nullptr)
            return;

        free (*use);
    }

    auto& v = *use;
    v.note = note;
    v.velocity = velocity;
    v.down = true;
    v.held = v.dying = false;
    v.gain = 1.0f;
    v.age = ++counter;
    v.upstroke = false;
    tune (v);
    strum (v, 0, false);
    v.played = 0;
    v.strumAt = strumRate->get() > 0.0f ? rate / strumRate->get() : 0.0;
}

void SwarmGuitarProcessor::noteOff (int note)
{
    for (auto& v : voices)
        if (v.note == note && v.down && ! v.dying)
        {
            v.down = false;
            v.held = pedal;

            if (! pedal)
            {
                v.sinceTouch = 0;
                tune (v);
            }
        }
}

void SwarmGuitarProcessor::allOff (bool fade)
{
    for (auto& v : voices)
    {
        if (v.note < 0)
            continue;

        if (fade)
        {
            v.dying = true;

            for (auto& layer : v.strings)
                for (auto& s : layer)
                    s.pluckIn = -1;
        }
        else if (! v.dying)
        {
            v.down = v.held = false;
            v.sinceTouch = 0;
            tune (v);
        }
    }

    if (! fade)
        pedal = false;
}

void SwarmGuitarProcessor::controller (const juce::MidiMessage& m)
{
    if (m.isPitchWheel())
    {
        bend = (float) (m.getPitchWheelValue() - 8192) / 8192.0f * 2.0f;

        for (auto& v : voices)
            if (v.note >= 0)
                tune (v);

        return;
    }

    const auto value = (float) m.getControllerValue() / 127.0f;

    switch (m.getControllerNumber())
    {
        case 7:     volume = value * value; break;
        case 11:    expression = value * value; break;

        case 64:
            pedal = m.getControllerValue() >= 64;

            if (! pedal)
                for (auto& v : voices)
                    if (v.note >= 0 && v.held)
                    {
                        v.held = false;
                        v.sinceTouch = 0;
                        tune (v);
                    }
            break;

        default:    break;
    }
}

float SwarmGuitarProcessor::tick (String& s) noexcept
{
    if (s.pluckIn >= 0 && s.pluckIn-- == 0)
    {
        // A pluck: a period of noise, less itself a little later (where along the string the pick is).
        s.period = juce::jmax (2, juce::roundToInt (rate / s.hz));
        s.lag = juce::jlimit (1, s.period - 1, juce::roundToInt (pick->get() * 0.01 * s.period));
        s.excite = 0;
        s.exciteLevel = s.pluckLevel;
        s.exciteCoefficient = juce::jlimit (0.05f, 1.0f, 0.12f + 0.0085f * brightness->get() * (0.4f + 0.6f * s.pluckLevel));
        s.seed = hash (s.seed + 0x9e3779b9u * ++s.plucks);
    }

    const auto at = (double) s.write - s.delay;
    const auto whole = (int) std::floor (at);
    const auto part = (float) (at - whole);
    const auto y = s.line[(size_t) (whole & mask)] * (1.0f - part) + s.line[(size_t) ((whole + 1) & mask)] * part;

    s.lowpass += s.coefficient * (y - s.lowpass);
    auto in = s.lowpass * s.loss;

    if (s.excite >= 0)
    {
        const auto i = s.excite++;
        const auto burst = (i < s.period ? scatter (s.seed + (juce::uint32) i) : 0.0f)
                         - (i >= s.lag && i - s.lag < s.period ? scatter (s.seed + (juce::uint32) (i - s.lag)) : 0.0f);
        s.exciteLowpass += s.exciteCoefficient * (burst - s.exciteLowpass);
        in += s.exciteLowpass * s.exciteLevel;

        if (i > s.period + s.lag + 96)
        {
            s.excite = -1;
            s.exciteLowpass = 0.0f;
        }
    }

    s.line[(size_t) s.write] = in;
    s.write = (s.write + 1) & mask;
    return y;
}

void SwarmGuitarProcessor::renderStrings (int from, int count)
{
    if (count <= 0)
        return;

    const auto guitars = layerCount(), perGuitar = stringCount();
    const auto fade = (float) std::pow (10.0, -6.0 / (fadeSeconds * rate));

    for (int l = 0; l < maxLayers; ++l)
        std::fill_n (buses[(size_t) l].begin() + from, count, 0.0f);

    for (auto& v : voices)
    {
        if (v.note < 0)
            continue;

        // Strum Rate: strummed again and again while the key is down.
        if (v.down && ! v.dying && strumRate->get() > 0.0f)
        {
            while (v.strumAt - (double) v.played < (double) count)
            {
                strum (v, juce::jmax (0, (int) (v.strumAt - (double) v.played)), true);
                v.strumAt += rate / strumRate->get();
            }
        }

        v.played += count;

        // To the sample: a fading note ends when it is 120 dB down, a ringing one when it has had its time,
        // wherever the blocks fall.
        const auto step = v.dying ? fade : 1.0f;
        const auto lasts = (juce::int64) (((v.down || v.held) ? 2.0 * sustain->get() : 2.0 * release->get() * 0.001 + 0.05) * rate);
        auto mine = count;
        auto ends = false;

        if (v.dying)
        {
            auto gain = v.gain;

            for (mine = 0; mine < count && gain >= silent; ++mine)
                gain *= step;

            ends = gain < silent;
        }
        else if (v.sinceTouch + count > lasts)
        {
            mine = (int) juce::jlimit ((juce::int64) 0, (juce::int64) count, lasts - v.sinceTouch);
            ends = true;
        }

        for (int l = 0; l < guitars; ++l)
        {
            auto* out = buses[(size_t) l].data() + from;

            for (int n = 0; n < perGuitar; ++n)
            {
                auto& s = v.strings[(size_t) l][(size_t) n];
                auto gain = v.gain;

                for (int i = 0; i < mine; ++i)
                {
                    out[i] += tick (s) * gain;
                    gain *= step;
                }
            }
        }

        if (v.dying)
            for (int i = 0; i < mine; ++i)
                v.gain *= step;

        v.sinceTouch += count;

        if (ends)
            free (v);
    }
}

double SwarmGuitarProcessor::shape (double x, double& before, double bias) noexcept
{
    // tanh, anti-aliased by its antiderivative: the mean of the curve between the last sample and this one.
    const auto lean = std::tanh (bias);
    const auto change = x - before;
    const auto y = std::abs (change) < 1.0e-5 ? std::tanh (0.5 * (x + before) + bias) - lean
                                               : ((logCosh (x + bias) - x * lean) - (logCosh (before + bias) - before * lean)) / change;
    before = x;
    return y;
}

void SwarmGuitarProcessor::setAmps()
{
    for (auto& a : amps)
    {
        set (a.lowCut, highPass (85.0, 0.7, rate));
        set (a.body, peak (140.0, 1.0, 3.0, rate));
        set (a.presence, peak (2400.0, 1.2, 4.0, rate));
        set (a.tone1, lowPass (tone->get(), 0.7, rate));
        set (a.tone2, lowPass (tone->get(), 0.7, rate));
    }
}

void SwarmGuitarProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto n = buffer.getNumSamples();
    buffer.clear();

    for (auto& b : buses)
        if ((int) b.size() < n)
            b.resize ((size_t) n);

    for (auto& v : voices)
        if (v.note >= 0)
            tune (v);

    setAmps();

    int done = 0;

    for (const auto meta : midi)
    {
        const auto at = juce::jlimit (done, n, meta.samplePosition);
        renderStrings (done, at - done);
        done = at;

        const auto m = meta.getMessage();

        if (m.isNoteOn())
            noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isNoteOff())
            noteOff (m.getNoteNumber());
        else if (m.isAllSoundOff())
            allOff (true);
        else if (m.isAllNotesOff())
            allOff (false);
        else if (m.isController() || m.isPitchWheel())
            controller (m);
    }

    renderStrings (done, n - done);
    midi.clear();

    // Each guitar through its own amp, then to its place across the field.
    const auto guitars = layerCount();
    const auto kind = amp->getIndex();
    const auto d = drive->get() * 0.01;
    const auto first = kind == 0 ? juce::Decibels::decibelsToGain (12.0 + 42.0 * d) : juce::Decibels::decibelsToGain (30.0 * d);
    const auto second = 2.0 + 4.0 * d;
    const auto tight = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * (kind == 0 ? 110.0 : 60.0) / rate));
    const auto between = (float) (1.0 - std::exp (-juce::MathConstants<double>::twoPi * 40.0 / rate));
    const auto gain = juce::Decibels::decibelsToGain (level->get()) * volume * expression * std::sqrt (2.0f / (float) guitars);
    const auto gainFrom = lastGain < 0.0f ? gain : lastGain;
    const auto channels = buffer.getNumChannels();

    for (int l = 0; l < guitars; ++l)
    {
        auto& a = amps[(size_t) l];
        auto* x = buses[(size_t) l].data();
        const auto place = guitars == 1 ? 0.0f : (2.0f * (float) l / (float) (guitars - 1) - 1.0f) * width->get() * 0.01f;
        const auto angle = (place + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
        const auto left = std::cos (angle), right = std::sin (angle);

        for (int i = 0; i < n; ++i)
        {
            auto y = x[i] * 0.5f;

            if (kind != 2)
            {
                a.tight += tight * (y - a.tight);
                auto stage = shape ((double) (y - a.tight) * first, a.before1, kind == 0 ? 0.35 : 0.1);

                a.between += between * ((float) stage - a.between);
                stage -= a.between;

                if (kind == 0)
                    stage = shape (stage * second, a.before2, 0.0);

                y = (float) stage * 0.7f;
                y = a.tone2.process (a.tone1.process (a.presence.process (a.body.process (a.lowCut.process (y)))));
            }

            y *= gainFrom + (gain - gainFrom) * (float) (i + 1) / (float) n;

            if (channels >= 2)
            {
                buffer.addSample (0, i, y * left);
                buffer.addSample (1, i, y * right);
            }
            else if (channels == 1)
            {
                buffer.addSample (0, i, y * 0.5f * (left + right));
            }
        }
    }

    lastGain = gain;
}
}
