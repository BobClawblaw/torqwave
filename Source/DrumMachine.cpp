#include "DrumMachine.h"
#include "DrumMachineEditor.h"

namespace DrumMachine
{
namespace
{
    constexpr double twoPi = juce::MathConstants<double>::twoPi;

    /** Each sound's prefix, name, pad note, decay as it starts (ms to fall 60 dB), and the trim in
        dB that sets its peak at full velocity and 0 dB where it sits in a kit (the kick near 0 dB,
        snares and toms a little under, hats, cymbals and percussion 4 to 8 dB under). */
    struct Info
    {
        const char* id;
        const char* name;
        int pad;
        float decayMs;
        float trimDb;
    };

    constexpr std::array<Info, (size_t) numSounds> infos {{
        { "kick", "Kick", 36, 550.0f, -0.7f },           { "rim", "Rim", 37, 90.0f, -6.4f },
        { "snare", "Snare", 38, 320.0f, -7.6f },         { "clap", "Clap", 39, 420.0f, -3.6f },
        { "closedHat", "Closed Hat", 42, 90.0f, -1.4f }, { "pedalHat", "Pedal Hat", 44, 70.0f, -2.9f },
        { "openHat", "Open Hat", 46, 650.0f, -5.8f },    { "lowTom", "Low Tom", 41, 750.0f, -4.6f },
        { "midTom", "Mid Tom", 45, 650.0f, -4.2f },      { "highTom", "High Tom", 48, 550.0f, -4.5f },
        { "crash", "Crash", 49, 2600.0f, -8.0f },        { "ride", "Ride", 51, 3000.0f, -11.0f },
        { "cowbell", "Cowbell", 56, 600.0f, -6.4f },     { "shaker", "Shaker", 70, 170.0f, -6.7f },
        { "china", "China", 52, 2200.0f, -6.6f },        { "rideBell", "Ride Bell", 53, 2600.0f, -6.7f },
        { "tambourine", "Tambourine", 54, 700.0f, -6.5f }, { "splash", "Splash", 55, 1000.0f, -6.7f },
        { "vibraslap", "Vibraslap", 58, 1600.0f, -6.0f }, { "bongo", "Bongos", 60, 450.0f, -5.0f },
        { "muteConga", "Mute Conga", 62, 200.0f, -4.5f }, { "openConga", "Congas", 63, 1000.0f, -4.5f },
        { "timbale", "Timbales", 65, 1100.0f, -7.1f },   { "agogo", "Agogo", 67, 1500.0f, -4.6f },
        { "whistle", "Whistle", 72, 600.0f, -2.0f },     { "guiro", "Guiro", 74, 400.0f, -8.0f },
        { "claves", "Claves", 75, 170.0f, -4.3f },       { "woodBlock", "Wood Block", 76, 220.0f, -1.5f },
        { "cuica", "Cuica", 79, 800.0f, -1.9f },         { "triangle", "Triangle", 81, 4000.0f, -4.2f },
    }};

    /** A state-variable filter, its coefficients set once per hit (Zavalishin's, stable at any setting). */
    struct Svf
    {
        float k = 1.0f, a1 = 0.0f, a2 = 0.0f, a3 = 0.0f, ic1 = 0.0f, ic2 = 0.0f;
        float lp = 0.0f, bp = 0.0f, hp = 0.0f;

        void set (double hz, double q, double rate)
        {
            const auto g = (float) std::tan (juce::MathConstants<double>::pi * juce::jlimit (10.0, rate * 0.45, hz) / rate);
            k = (float) (1.0 / q);
            a1 = 1.0f / (1.0f + g * (g + k));
            a2 = g * a1;
            a3 = g * a2;
            ic1 = ic2 = 0.0f;
        }

        void run (float v0)
        {
            const auto v3 = v0 - ic2;
            const auto v1 = a1 * ic1 + a2 * v3;
            const auto v2 = ic2 + a2 * ic1 + a3 * v3;
            ic1 = 2.0f * v1 - ic1;
            ic2 = 2.0f * v2 - ic2;
            lp = v2;
            bp = v1;
            hp = v0 - k * v1 - v2;
        }

        float lowPass (float x)   { run (x); return lp; }
        float bandPass (float x)  { run (x); return bp; }
        float highPass (float x)  { run (x); return hp; }
    };

    /** An exponential decay, one multiply a sample. */
    struct Decay
    {
        float v = 1.0f, k = 0.0f;

        void set (double seconds, double rate)
        {
            v = 1.0f;
            k = (float) std::exp (-1.0 / (juce::jmax (1.0e-4, seconds) * rate));
        }

        float next()   { const auto r = v; v *= k; return r; }
    };

    float noise (juce::uint32& state)
    {
        state = state * 1664525u + 1013904223u;
        return (float) (state >> 8) / (float) (1u << 23) - 1.0f;
    }

    float sine (double& phase, double hz, double rate)
    {
        const auto s = (float) std::sin (twoPi * phase);
        phase += hz / rate;
        phase -= std::floor (phase);
        return s;
    }

    double polyBlep (double t, double dt)
    {
        if (t < dt)
        {
            t /= dt;
            return t + t - t * t - 1.0;
        }

        if (t > 1.0 - dt)
        {
            t = (t - 1.0) / dt;
            return t * t + t + t + 1.0;
        }

        return 0.0;
    }

    /** A band-limited square (its steps smoothed with polyBLEP, so the metal doesn't alias as much). */
    float square (double& phase, double hz, double rate)
    {
        const auto dt = juce::jmin (0.5, hz / rate);
        auto v = phase < 0.5 ? 1.0 : -1.0;
        v += polyBlep (phase, dt);
        auto half = phase + 0.5;
        half -= std::floor (half);
        v -= polyBlep (half, dt);
        phase += dt;
        phase -= std::floor (phase);
        return (float) v;
    }

    // The 808's six square oscillators, the metal of its hats and cymbals.
    constexpr std::array<double, 6> metalHz { 205.3, 304.4, 369.6, 522.7, 540.0, 800.0 };

    double semis (double st)   { return std::exp2 (st / 12.0); }

    /** The sounds whose recipe has no use of its own for tone: for them tone tilts the top end. */
    bool tilts (Sound s)
    {
        return s == Sound::vibraslap || s == Sound::timbale || s == Sound::agogo || s == Sound::whistle || s == Sound::guiro
            || s == Sound::claves || s == Sound::woodBlock || s == Sound::cuica || s == Sound::triangle || s == Sound::rideBell;
    }
}

//==============================================================================
std::optional<Sound> soundFor (int note)
{
    switch (note)
    {
        case 35: case 36:   return Sound::kick;
        case 37:            return Sound::rim;
        case 38: case 40:   return Sound::snare;
        case 39:            return Sound::clap;
        case 42:            return Sound::closedHat;
        case 44:            return Sound::pedalHat;
        case 46:            return Sound::openHat;
        case 41: case 43:   return Sound::lowTom;
        case 45: case 47:   return Sound::midTom;
        case 48: case 50:   return Sound::highTom;
        case 49: case 57:   return Sound::crash;
        case 51: case 59:   return Sound::ride;
        case 56:            return Sound::cowbell;
        case 69: case 70:   return Sound::shaker;
        case 52:            return Sound::china;
        case 53:            return Sound::rideBell;
        case 54:            return Sound::tambourine;
        case 55:            return Sound::splash;
        case 58:            return Sound::vibraslap;
        case 60: case 61:   return Sound::bongo;
        case 62:            return Sound::muteConga;
        case 63: case 64:   return Sound::openConga;
        case 65: case 66:   return Sound::timbale;
        case 67: case 68:   return Sound::agogo;
        case 71: case 72:   return Sound::whistle;
        case 73: case 74:   return Sound::guiro;
        case 75:            return Sound::claves;
        case 76: case 77:   return Sound::woodBlock;
        case 78: case 79:   return Sound::cuica;
        case 80: case 81:   return Sound::triangle;
        default:            return std::nullopt;
    }
}

int padNote (Sound s)              { return infos[(size_t) s].pad; }
juce::String nameOf (Sound s)      { return infos[(size_t) s].name; }
juce::String idOf (Sound s)        { return infos[(size_t) s].id; }

int chokeGroup (int note)
{
    switch (note)
    {
        case 42: case 44: case 46:   return 1;
        case 78: case 79:            return 2;
        case 80: case 81:            return 3;
        default:                     return 0;
    }
}

juce::StringArray kickCharacterNames()   { return { "Boom", "Punch", "Hard" }; }
juce::StringArray typeNames()            { return { "Classic", "Modern" }; }

bool hasTypes (Sound s)
{
    switch (s)
    {
        case Sound::kick: case Sound::snare: case Sound::clap: case Sound::closedHat: case Sound::pedalHat: case Sound::openHat:
        case Sound::lowTom: case Sound::midTom: case Sound::highTom: case Sound::crash: case Sound::ride: case Sound::shaker:
        case Sound::china: case Sound::tambourine: case Sound::splash: case Sound::bongo: case Sound::muteConga: case Sound::openConga:
        case Sound::agogo: case Sound::whistle: case Sound::triangle:
            return true;
        case Sound::rim: case Sound::cowbell: case Sound::rideBell: case Sound::vibraslap: case Sound::timbale: case Sound::guiro:
        case Sound::claves: case Sound::woodBlock: case Sound::cuica: case Sound::count:
            return false;
    }
}
juce::StringArray filterNames()          { return { "Off", "Low-pass", "Band-pass", "High-pass" }; }
juce::StringArray delayTimeNames()       { return { "1/16", "1/8T", "1/8", "1/8 dotted", "1/4", "1/4 dotted" }; }

double delayBeats (int index)
{
    static constexpr double beats[] { 0.25, 1.0 / 3.0, 0.5, 0.75, 1.0, 1.5 };
    return beats[juce::jlimit (0, 5, index)];
}

Settings defaultsFor (Sound s)
{
    Settings d;
    d.decay = infos[(size_t) s].decayMs;

    // A drummer's spread, from the audience: hats a little right, the ride left, toms across.
    const auto pan = [s]() -> float
    {
        if (s == Sound::closedHat || s == Sound::pedalHat || s == Sound::openHat)   return 15.0f;
        if (s == Sound::ride || s == Sound::rideBell)                               return -20.0f;
        if (s == Sound::crash || s == Sound::splash)                                return 18.0f;
        if (s == Sound::china)                                                      return -25.0f;
        if (s == Sound::lowTom)                                                     return -18.0f;
        if (s == Sound::highTom)                                                    return 18.0f;
        if (s == Sound::bongo || s == Sound::timbale)                               return 25.0f;
        if (s == Sound::openConga || s == Sound::muteConga)                         return -25.0f;
        if (s == Sound::shaker || s == Sound::tambourine)                           return -12.0f;
        if (s == Sound::cowbell || s == Sound::agogo)                               return 10.0f;
        return 0.0f;
    };

    d.pan = pan();
    return d;
}

KitSettings defaultKit()
{
    KitSettings k;

    for (int i = 0; i < numSounds; ++i)
        k.sounds[(size_t) i] = defaultsFor ((Sound) i);

    return k;
}

const std::vector<Kit>& kits()
{
    static const auto all = []
    {
        std::vector<Kit> list;

        const auto make = [&list] (const char* name, const char* description, auto&& change)
        {
            Kit kit { name, description, defaultKit() };
            change (kit.settings, [&kit] (Sound s) -> Settings& { return kit.settings.sounds[(size_t) s]; });
            list.push_back (kit);
        };

        make ("Standard", "Every sound as it starts: a 909-style punch kick, clean, nothing on the bus.", [] (KitSettings&, auto&&) {});

        make ("Dark Techno", "A low, long kick with drive for rumbles, dark snares and claps, crisp hats, the bus glued.", [] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            s (Sound::kick) = { -2.0f, 750.0f, -25.0f, 0.0f, 0.0f, 35.0f };
            s (Sound::snare) = { -3.0f, 260.0f, -30.0f, -2.0f, 0.0f, 20.0f };
            s (Sound::clap) = { -2.0f, 620.0f, -20.0f, -1.0f, 0.0f, 15.0f };
            s (Sound::closedHat) = { 0.0f, 70.0f, 15.0f, -3.0f, 12.0f, 0.0f };
            s (Sound::openHat) = { 0.0f, 520.0f, 10.0f, -4.0f, 12.0f, 0.0f };
            s (Sound::ride) = { -2.0f, 3200.0f, -10.0f, -4.0f, -20.0f, 0.0f };
            s (Sound::rim) = { -3.0f, 110.0f, 20.0f, -3.0f, -10.0f, 25.0f };
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t) = { -4.0f, 900.0f, -20.0f, -2.0f, s (t).pan, 30.0f };
            s (Sound::clap).reverb = 30.0f;
            s (Sound::snare).reverb = 15.0f;
            s (Sound::rim).delay = 25.0f;
            s (Sound::openHat).reverb = 10.0f;
            s (Sound::ride).reverb = 10.0f;
            k.drive = 25.0f;
            k.glue = 40.0f;
            k.level = -5.0f;
            k.reverbSize = 60.0f;
            k.delayTime = 3;
            k.feedback = 45.0f;
        });

        make ("Industrial", "A hard, overdriven kick, noise snares, metal that bites: everything through drive.", [] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::hard;
            s (Sound::kick) = { 0.0f, 450.0f, 30.0f, -1.0f, 0.0f, 70.0f };
            s (Sound::snare) = { -2.0f, 380.0f, 40.0f, -2.0f, 0.0f, 60.0f };
            s (Sound::clap) = { -4.0f, 520.0f, 0.0f, -2.0f, 0.0f, 45.0f };
            s (Sound::closedHat) = { -3.0f, 80.0f, 30.0f, -3.0f, 15.0f, 35.0f };
            s (Sound::openHat) = { -3.0f, 480.0f, 30.0f, -4.0f, 15.0f, 35.0f };
            s (Sound::cowbell) = { -5.0f, 450.0f, 20.0f, -3.0f, 10.0f, 55.0f };
            s (Sound::china) = { -2.0f, 1800.0f, 10.0f, -3.0f, -25.0f, 50.0f };
            s (Sound::rim) = { -5.0f, 140.0f, 30.0f, -2.0f, -10.0f, 60.0f };
            s (Sound::tambourine).drive = 40.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t) = { -5.0f, 800.0f, 20.0f, -1.0f, s (t).pan, 55.0f };
            s (Sound::snare).crush = 30.0f;
            s (Sound::snare).reverb = 20.0f;
            s (Sound::clap).crush = 20.0f;
            s (Sound::clap).reverb = 25.0f;
            s (Sound::closedHat).crush = 25.0f;
            s (Sound::openHat).crush = 25.0f;
            s (Sound::cowbell).delay = 30.0f;
            s (Sound::china).reverb = 20.0f;
            s (Sound::rim).crush = 40.0f;
            s (Sound::rim).delay = 30.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t).reverb = 15.0f;
            k.drive = 55.0f;
            k.glue = 55.0f;
            k.level = -4.0f;
            k.reverbSize = 45.0f;
            k.delayTime = 0;
            k.feedback = 40.0f;
        });

        make ("EBM Machine", "Tight drum-machine body music: a short punchy kick, big gated snares, near-flat hats.", [] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            s (Sound::kick) = { 1.0f, 340.0f, 10.0f, 0.0f, 0.0f, 20.0f };
            s (Sound::snare) = { 0.0f, 470.0f, 20.0f, -1.0f, 0.0f, 25.0f };
            s (Sound::clap) = { 0.0f, 520.0f, 10.0f, -1.0f, 0.0f, 10.0f };
            s (Sound::closedHat) = { 2.0f, 50.0f, 10.0f, -4.0f, 10.0f, 0.0f };
            s (Sound::openHat) = { 2.0f, 380.0f, 10.0f, -5.0f, 10.0f, 0.0f };
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t) = { -2.0f, 600.0f, 0.0f, -2.0f, s (t).pan, 15.0f };
            s (Sound::snare).reverb = 35.0f;
            s (Sound::clap).reverb = 30.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t).reverb = 20.0f;
            k.drive = 15.0f;
            k.glue = 50.0f;
            k.level = -7.0f;
            k.reverbSize = 70.0f;
            k.delayTime = 2;
            k.feedback = 30.0f;
        });

        make ("Electro", "An 808 in spirit: a long booming kick, snappy snare, sine toms and the cowbell.", [] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::boom;
            s (Sound::kick) = { -1.0f, 1400.0f, -30.0f, 0.0f, 0.0f, 10.0f };
            s (Sound::snare) = { 4.0f, 250.0f, -10.0f, -2.0f, 0.0f, 0.0f };
            s (Sound::clap) = { 1.0f, 380.0f, 10.0f, -1.0f, 0.0f, 0.0f };
            s (Sound::closedHat) = { 0.0f, 60.0f, -10.0f, -3.0f, 15.0f, 0.0f };
            s (Sound::openHat) = { 0.0f, 450.0f, -10.0f, -4.0f, 15.0f, 0.0f };
            s (Sound::cowbell) = { 0.0f, 700.0f, 0.0f, -2.0f, 10.0f, 0.0f };
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t) = { 2.0f, 800.0f, -40.0f, -1.0f, s (t).pan, 0.0f };
            s (Sound::clap).reverb = 20.0f;
            s (Sound::cowbell).delay = 20.0f;
            s (Sound::rim).delay = 20.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t).reverb = 15.0f;
            k.drive = 10.0f;
            k.glue = 30.0f;
            k.level = -6.0f;
            k.delayTime = 1;
            k.feedback = 35.0f;
        });

        make ("Breaks", "Crunchy and squashed: a tight kick, a cracking snare, bright hats, the bus driven and glued hard.", [] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            s (Sound::kick) = { 2.0f, 380.0f, 40.0f, 0.0f, 0.0f, 30.0f };
            s (Sound::snare) = { 2.0f, 280.0f, 30.0f, 0.0f, 0.0f, 30.0f };
            s (Sound::closedHat) = { 1.0f, 100.0f, 20.0f, -4.0f, 20.0f, 10.0f };
            s (Sound::openHat) = { 1.0f, 600.0f, 20.0f, -5.0f, 20.0f, 10.0f };
            s (Sound::ride) = { 0.0f, 2600.0f, 10.0f, -5.0f, -25.0f, 0.0f };
            s (Sound::kick).crush = 15.0f;
            s (Sound::snare).crush = 20.0f;
            s (Sound::snare).reverb = 10.0f;
            s (Sound::closedHat).crush = 20.0f;
            s (Sound::openHat).crush = 20.0f;
            k.drive = 30.0f;
            k.glue = 70.0f;
            k.level = -5.0f;
            k.reverbSize = 35.0f;
            k.delayTime = 2;
            k.feedback = 25.0f;
        });

        make ("Industrial Rock", "A played kit pushed hard, as on the industrial rock records: a beater on the kick, a ringing snare, big toms, cymbals spread wide, the room squashed.", [] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            s (Sound::kick) = { 3.0f, 420.0f, 35.0f, 0.0f, 0.0f, 25.0f };
            s (Sound::snare) = { 1.0f, 420.0f, 15.0f, -1.0f, 0.0f, 30.0f };
            s (Sound::closedHat) = { -1.0f, 110.0f, 5.0f, -4.0f, 30.0f, 10.0f };
            s (Sound::pedalHat) = { -1.0f, 80.0f, 0.0f, -4.0f, 30.0f, 10.0f };
            s (Sound::openHat) = { -1.0f, 800.0f, 5.0f, -5.0f, 30.0f, 10.0f };
            s (Sound::lowTom) = { -2.0f, 1100.0f, 30.0f, -1.0f, -35.0f, 20.0f };
            s (Sound::midTom) = { -2.0f, 950.0f, 30.0f, -1.0f, 0.0f, 20.0f };
            s (Sound::highTom) = { -2.0f, 850.0f, 30.0f, -1.0f, 25.0f, 20.0f };
            s (Sound::crash) = { -1.0f, 3200.0f, 0.0f, -3.0f, 35.0f, 0.0f };
            s (Sound::china) = { -1.0f, 2400.0f, 0.0f, -3.0f, -30.0f, 20.0f };
            s (Sound::ride) = { -1.0f, 3600.0f, 0.0f, -4.0f, -35.0f, 0.0f };
            s (Sound::snare).reverb = 30.0f;
            s (Sound::crash).reverb = 15.0f;
            s (Sound::kick).reverb = 5.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t).reverb = 25.0f;
            k.drive = 20.0f;
            k.glue = 60.0f;
            k.level = -6.0f;
            k.reverbSize = 65.0f;
            k.delayTime = 4;
            k.feedback = 25.0f;
        });

        // The kits of Modern sounds, after the seven that were there before them. Each starts from
        // every sound's Modern type, with hats that last long enough to be heard as hats.
        const auto modern = [&make] (const char* name, const char* description, auto&& change)
        {
            make (name, description, [&change] (KitSettings& k, auto&& s)
            {
                for (int i = 0; i < numSounds; ++i)
                    if (hasTypes ((Sound) i))
                        s ((Sound) i).type = 1;

                s (Sound::closedHat).decay = 160.0f;
                s (Sound::pedalHat).decay = 120.0f;
                change (k, s);
            });
        };

        // A sound's first six settings, leaving its type and effects as they are.
        const auto set = [] (Settings& to, float tune, float decay, float tone, float level, float pan, float drive)
        {
            to.tune = tune;
            to.decay = decay;
            to.tone = tone;
            to.level = level;
            to.pan = pan;
            to.drive = drive;
        };

        modern ("Modern", "Every sound that has a Modern type on it, clean: nothing on the bus.", [] (KitSettings&, auto&&) {});

        modern ("Warehouse", "Dark techno on the Modern sounds: a low kick that leaves room, a dark clap in a room, hats and a ride that carry, the bus glued.", [&set] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            set (s (Sound::kick), -2.0f, 440.0f, -10.0f, -2.0f, 0.0f, 25.0f);
            set (s (Sound::snare), -2.0f, 280.0f, -10.0f, -3.0f, 0.0f, 15.0f);
            set (s (Sound::clap), -1.0f, 480.0f, 0.0f, -1.0f, 0.0f, 10.0f);
            set (s (Sound::closedHat), 0.0f, 130.0f, 5.0f, 0.0f, 12.0f, 0.0f);
            set (s (Sound::openHat), 0.0f, 420.0f, 5.0f, -1.0f, 12.0f, 0.0f);
            set (s (Sound::ride), -2.0f, 2400.0f, -5.0f, -3.0f, -20.0f, 0.0f);
            set (s (Sound::rim), -3.0f, 110.0f, 20.0f, -4.0f, -10.0f, 25.0f);
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                set (s (t), -4.0f, 800.0f, -10.0f, -3.0f, s (t).pan, 25.0f);
            s (Sound::clap).reverb = 25.0f;
            s (Sound::snare).reverb = 12.0f;
            s (Sound::rim).delay = 22.0f;
            s (Sound::openHat).reverb = 8.0f;
            s (Sound::ride).reverb = 8.0f;
            k.drive = 18.0f;
            k.glue = 35.0f;
            k.level = -5.0f;
            k.reverbSize = 60.0f;
            k.delayTime = 3;
            k.feedback = 40.0f;
        });

        modern ("Body Music", "EBM on the Modern sounds: a short kick with a beater, a big snare in a room, flat tight hats, toms for the fills.", [&set] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            set (s (Sound::kick), 1.0f, 360.0f, 15.0f, 0.0f, 0.0f, 20.0f);
            set (s (Sound::snare), 0.0f, 420.0f, 15.0f, -2.0f, 0.0f, 25.0f);
            set (s (Sound::clap), 0.0f, 480.0f, 10.0f, -4.0f, 0.0f, 10.0f);
            set (s (Sound::closedHat), 2.0f, 110.0f, 10.0f, -4.0f, 10.0f, 0.0f);
            set (s (Sound::openHat), 2.0f, 340.0f, 10.0f, -5.0f, 10.0f, 0.0f);
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                set (s (t), -2.0f, 600.0f, 10.0f, -3.0f, s (t).pan, 15.0f);
            s (Sound::snare).reverb = 30.0f;
            s (Sound::clap).reverb = 25.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t).reverb = 18.0f;
            k.drive = 12.0f;
            k.glue = 45.0f;
            k.level = -6.0f;
            k.reverbSize = 68.0f;
            k.delayTime = 2;
            k.feedback = 30.0f;
        });

        modern ("Foundry", "Industrial on the Modern sounds: a hard driven kick, a crushed snare, hats and a china with grit, toms that pound.", [&set] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::hard;
            set (s (Sound::kick), 0.0f, 420.0f, 30.0f, -1.0f, 0.0f, 55.0f);
            set (s (Sound::snare), -2.0f, 360.0f, 35.0f, -3.0f, 0.0f, 50.0f);
            set (s (Sound::clap), -4.0f, 500.0f, 5.0f, -4.0f, 0.0f, 35.0f);
            set (s (Sound::closedHat), -3.0f, 120.0f, 20.0f, -5.0f, 15.0f, 30.0f);
            set (s (Sound::openHat), -3.0f, 440.0f, 20.0f, -6.0f, 15.0f, 30.0f);
            set (s (Sound::cowbell), -5.0f, 450.0f, 20.0f, -5.0f, 10.0f, 55.0f);
            set (s (Sound::china), -2.0f, 1800.0f, 5.0f, -5.0f, -25.0f, 35.0f);
            set (s (Sound::rim), -5.0f, 140.0f, 30.0f, -4.0f, -10.0f, 55.0f);
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                set (s (t), -5.0f, 800.0f, 20.0f, -2.0f, s (t).pan, 45.0f);
            s (Sound::snare).crush = 20.0f;
            s (Sound::snare).reverb = 18.0f;
            s (Sound::clap).crush = 12.0f;
            s (Sound::clap).reverb = 22.0f;
            s (Sound::closedHat).crush = 18.0f;
            s (Sound::openHat).crush = 18.0f;
            s (Sound::cowbell).delay = 28.0f;
            s (Sound::china).reverb = 18.0f;
            s (Sound::rim).crush = 35.0f;
            s (Sound::rim).delay = 28.0f;
            for (auto t : { Sound::lowTom, Sound::midTom, Sound::highTom })
                s (t).reverb = 15.0f;
            k.drive = 40.0f;
            k.glue = 50.0f;
            k.level = -5.0f;
            k.reverbSize = 45.0f;
            k.delayTime = 0;
            k.feedback = 38.0f;
        });

        modern ("Broken Beat", "Breaks on the Modern sounds: a tight kick, a snare with a crack and ghost notes that sit under it, bright hats, the bus driven and glued.", [&set] (KitSettings& k, auto&& s)
        {
            k.kick = KickCharacter::punch;
            set (s (Sound::kick), 2.0f, 340.0f, 30.0f, 0.0f, 0.0f, 25.0f);
            set (s (Sound::snare), 2.0f, 300.0f, 25.0f, -1.0f, 0.0f, 25.0f);
            set (s (Sound::closedHat), 1.0f, 140.0f, 15.0f, -4.0f, 20.0f, 0.0f);
            set (s (Sound::openHat), 1.0f, 520.0f, 15.0f, -5.0f, 20.0f, 0.0f);
            set (s (Sound::ride), 0.0f, 2400.0f, 5.0f, -6.0f, -25.0f, 0.0f);
            s (Sound::snare).reverb = 10.0f;
            k.drive = 22.0f;
            k.glue = 60.0f;
            k.level = -5.0f;
            k.reverbSize = 35.0f;
            k.delayTime = 2;
            k.feedback = 25.0f;
        });

        return list;
    }();

    return all;
}

int kitIndex (const juce::String& name)
{
    const auto& all = kits();

    for (size_t i = 0; i < all.size(); ++i)
        if (all[i].name == name)
            return (int) i;

    return -1;
}

//==============================================================================
struct Engine::Hit
{
    bool active = false;
    Sound sound = Sound::kick;
    int note = 0;
    float gainL = 1.0f, gainR = 1.0f;     // level, pan and velocity
    float drive = 0.0f, driveNorm = 1.0f;
    float tilt = 0.0f, tiltK = 0.0f, tiltL = 0.0f, tiltR = 0.0f;
    int filter = 0;                       // its effects: filter type, 0 off
    float filterGain = 1.0f;
    Svf filterL, filterR;
    float crushStep = 0.0f, holdInc = 1.0f, holdPhase = 1.0f, heldL = 0.0f, heldR = 0.0f;   // crushStep 0: no crush
    float sendReverb = 0.0f, sendDelay = 0.0f;
    juce::int64 age = 0, length = 0;
    int rise = 1, tail = 1;               // samples of its onset ramp and of its last fade
    int fade = 0, fadeLength = 0;         // when cut: samples left of its fade out
    double rate = 48000.0;
    double T = 1.0, S = 1.0, u = 0.0;     // its tune as a ratio, decay as a ratio of the sound's own, and tone, -1 to 1
    bool modern = false;                  // its type: the Modern model, not the Classic
    double vel = 1.0;                     // how hard it is struck, 0 to 1: a Modern sound's own use of it
    KickCharacter kick = KickCharacter::punch;
    std::array<double, 14> ph {};
    std::array<double, 6> hz {};
    std::array<Svf, 10> f {};
    std::array<Decay, 8> e {};
    std::array<double, 10> v {};
    juce::uint32 n1 = 1, n2 = 2, n3 = 3;
};

namespace
{
    using Hit = Engine::Hit;

    /** The toms, bongos and congas: a skin falling in pitch, a second mode, and a stick's strike. */
    void startMembrane (Hit& h, double base, double drop, double dropTime, double strikeHz, double strikeTime, double strike, double mode2, double decaySeconds)
    {
        h.v[0] = base;
        h.v[1] = drop;
        h.v[2] = strike;
        h.v[3] = mode2;
        h.e[0].set (decaySeconds / 6.9, h.rate);
        h.e[1].set (dropTime, h.rate);
        h.e[2].set (0.55 * decaySeconds / 6.9, h.rate);
        h.e[3].set (strikeTime, h.rate);
        h.f[0].set (strikeHz, 1.0, h.rate);
        h.length = (juce::int64) (decaySeconds * h.rate);
    }

    float membrane (Hit& h)
    {
        const auto hz = h.v[0] * (1.0 + h.v[1] * h.e[1].next());
        const auto body = sine (h.ph[0], hz, h.rate) * h.e[0].next() + (float) h.v[3] * sine (h.ph[1], hz * 1.58, h.rate) * h.e[2].next();
        const auto strike = h.f[0].bandPass (noise (h.n1)) * h.e[3].next() * (float) h.v[2];
        return 0.9f * body + 2.0f * strike;
    }

    /** The cymbals: six squares a side, ring-modulated in pairs, through a band-pass and a
        high-pass, with noise; the right side tuned a hair apart, so they are wide. */
    void startCymbal (Hit& h, double scale, double bandHz, double bandQ, double highHz, double noiseHz, double noiseMix, double noiseLonger, double seconds)
    {
        for (size_t i = 0; i < 6; ++i)
            h.hz[i] = metalHz[i] * scale * h.T;

        for (size_t i = 0; i < 12; ++i)
            h.ph[i] = (double) ((h.n3 = h.n3 * 1664525u + 1013904223u) >> 8) / (double) (1u << 24);

        const auto bright = std::exp2 (0.5 * h.u);
        h.f[0].set (bandHz * bright, bandQ, h.rate);
        h.f[1].set (bandHz * bright * 1.03, bandQ, h.rate);
        h.f[2].set (highHz * bright, 0.7, h.rate);
        h.f[3].set (highHz * bright * 1.03, 0.7, h.rate);
        h.f[4].set (noiseHz * bright, 0.7, h.rate);
        h.f[5].set (noiseHz * bright, 0.7, h.rate);
        h.v[0] = noiseMix;
        h.e[0].set (seconds / 6.9, h.rate);
        h.e[1].set (noiseLonger * seconds / 6.9, h.rate);
        h.e[2].set (0.015, h.rate);
        h.length = (juce::int64) (juce::jmax (1.0, noiseLonger) * seconds * h.rate);
    }

    void cymbal (Hit& h, float& l, float& r, float shape)
    {
        const auto side = [&h] (size_t first, double detune)
        {
            std::array<float, 6> s {};

            for (size_t i = 0; i < 6; ++i)
                s[i] = square (h.ph[first + i], h.hz[i] * detune, h.rate);

            return (s[0] * s[1] + s[2] * s[3] + s[4] * s[5]) / 3.0f;
        };

        const auto metal = h.e[0].next(), hiss = h.e[1].next(), crack = h.e[2].next();
        auto ml = h.f[2].highPass (h.f[0].bandPass (side (0, 1.0)));
        auto mr = h.f[3].highPass (h.f[1].bandPass (side (6, 1.012)));
        const auto nl = h.f[4].highPass (noise (h.n1)), nr = h.f[5].highPass (noise (h.n2));
        const auto mix = (float) h.v[0];
        l = (1.0f - mix) * ml * (metal + 0.5f * crack) * 2.2f + mix * nl * (hiss + 0.6f * crack);
        r = (1.0f - mix) * mr * (metal + 0.5f * crack) * 2.2f + mix * nr * (hiss + 0.6f * crack);

        if (shape > 0.0f)
        {
            l = std::tanh (shape * l) / std::tanh (shape);
            r = std::tanh (shape * r) / std::tanh (shape);
        }
    }

    //==============================================================================
    // The Modern type of a sound: a second model beside the first, which is left exactly as it was.

    /** The dB that puts a Modern sound's peak where its Classic one is, at full velocity and as it starts. */
    float modernTrimDb (Sound s)
    {
        switch (s)
        {
            case Sound::kick:        return -3.1f;
            case Sound::snare:       return 4.8f;
            case Sound::clap:        return -1.0f;
            case Sound::closedHat:   return -0.8f;
            case Sound::pedalHat:    return -3.4f;
            case Sound::openHat:     return -1.6f;
            case Sound::lowTom:      return 0.1f;
            case Sound::midTom:      return -0.4f;
            case Sound::highTom:     return -1.2f;
            case Sound::crash:       return 1.5f;
            case Sound::ride:        return -0.2f;
            case Sound::shaker:      return 5.6f;
            case Sound::china:       return 0.0f;
            case Sound::tambourine:  return 4.0f;
            case Sound::splash:      return 1.5f;
            case Sound::bongo:       return 0.0f;
            case Sound::muteConga:   return 0.0f;
            case Sound::openConga:   return 0.0f;
            case Sound::agogo:       return -1.7f;
            case Sound::whistle:     return -6.3f;
            case Sound::triangle:    return -1.9f;
            case Sound::rim: case Sound::cowbell: case Sound::rideBell: case Sound::vibraslap: case Sound::timbale: case Sound::guiro:
            case Sound::claves: case Sound::woodBlock: case Sound::cuica: case Sound::count:
                break;
        }

        return 0.0f;
    }

    /** A skin: its fundamental falling in pitch (further the harder it is struck), two more of a
        drumhead's modes that die sooner, the stick's strike, and for the toms the thud of the
        shell; pushed a little into a soft clip, so there is more to it than a sine. */
    void startSkin (Hit& h, double base, double drop, double dropTime, double strikeHz, double strikeTime, double strike, double mode2, double decaySeconds,
                    double shape, double thud)
    {
        h.v[0] = base;
        h.v[1] = drop * (0.5 + 0.5 * h.vel);
        h.v[2] = strike * (0.4 + 0.6 * h.vel);
        h.v[3] = mode2;
        h.v[4] = shape;
        h.v[5] = thud * (0.4 + 0.6 * h.vel);
        h.e[0].set (decaySeconds / 6.9, h.rate);
        h.e[1].set (dropTime, h.rate);
        h.e[2].set (0.55 * decaySeconds / 6.9, h.rate);
        h.e[3].set (strikeTime, h.rate);
        h.e[4].set (0.3 * decaySeconds / 6.9, h.rate);
        h.e[5].set (0.012, h.rate);
        h.f[0].set (strikeHz, 1.0, h.rate);
        h.f[1].set (base * 4.5, 0.9, h.rate);
        h.length = (juce::int64) (decaySeconds * h.rate);
    }

    float skin (Hit& h)
    {
        const auto hz = h.v[0] * (1.0 + h.v[1] * h.e[1].next());
        auto body = sine (h.ph[0], hz, h.rate) * h.e[0].next() + (float) h.v[3] * (sine (h.ph[1], hz * 1.59, h.rate) * h.e[2].next()
                                                                                   + 0.5f * sine (h.ph[2], hz * 2.14, h.rate) * h.e[4].next());
        const auto shape = (float) h.v[4];
        body = std::tanh (shape * body) / std::tanh (shape);
        const auto hit = noise (h.n1);
        const auto strike = h.f[0].bandPass (hit) * h.e[3].next() * (float) h.v[2];
        const auto thud = h.f[1].bandPass (hit) * h.e[5].next() * (float) h.v[5];
        return 0.9f * body + 2.0f * strike + thud;
    }

    /** A cymbal in two bands, as a cymbal is heard: the low one ringing on, the high one bright at
        the strike and gone sooner, so it darkens as it fades; over a wash of noise that swells in
        after the stick and shimmers, each side in its own time. */
    void startMetal (Hit& h, double scale, double lowHz, double lowQ, double highHz, double noiseHz, double noiseMix, double noiseLonger, double seconds,
                     double highShare, double stick)
    {
        for (size_t i = 0; i < 6; ++i)
            h.hz[i] = metalHz[i] * scale * h.T;

        for (size_t i = 0; i < 12; ++i)
            h.ph[i] = (double) ((h.n3 = h.n3 * 1664525u + 1013904223u) >> 8) / (double) (1u << 24);

        const auto bright = std::exp2 (0.5 * h.u);
        h.f[0].set (lowHz * bright, lowQ, h.rate);
        h.f[1].set (lowHz * bright * 1.03, lowQ, h.rate);
        h.f[2].set (highHz * bright, 0.7, h.rate);
        h.f[3].set (highHz * bright * 1.03, 0.7, h.rate);
        h.f[4].set (noiseHz * bright, 0.7, h.rate);
        h.f[5].set (noiseHz * bright, 0.7, h.rate);
        h.f[8].set (lowHz * bright * 0.5, 0.7, h.rate);    // the squares' own low notes kept out of the ring
        h.f[9].set (lowHz * bright * 0.5, 0.7, h.rate);
        h.v[0] = noiseMix;
        h.v[1] = stick * (0.3 + 0.7 * h.vel);
        h.v[2] = (double) ((h.n3 = h.n3 * 1664525u + 1013904223u) >> 8) / (double) (1u << 24);   // where the shimmer starts
        h.v[5] = 1.0 - std::exp (-twoPi * juce::jmin (0.45 * h.rate, 3500.0 * bright) / h.rate);     // the wash's top: from 14 kHz
        h.v[6] = 1.0 - std::exp (-twoPi * juce::jmin (0.45 * h.rate, 14000.0 * bright) / h.rate);    // at the strike down to 3.5
        h.e[0].set (seconds / 6.9, h.rate);
        h.e[1].set (noiseLonger * seconds / 6.9, h.rate);
        h.e[2].set (0.015, h.rate);
        h.e[3].set (highShare * seconds / 6.9, h.rate);
        h.e[4].set (0.008, h.rate);
        h.length = (juce::int64) (juce::jmax (1.0, noiseLonger) * seconds * h.rate);
    }

    void metal (Hit& h, float& l, float& r, float shape)
    {
        const auto side = [&h] (size_t first, double detune)
        {
            std::array<float, 6> s {};

            for (size_t i = 0; i < 6; ++i)
                s[i] = square (h.ph[first + i], h.hz[i] * detune, h.rate);

            return (s[0] * s[1] + s[2] * s[3] + s[4] * s[5] + s[0] * s[3] + s[1] * s[4] + s[2] * s[5]) / 6.0f;
        };

        const auto t = (double) h.age / h.rate;
        const auto ring = h.e[0].next(), hiss = h.e[1].next(), crack = h.e[2].next(), sheen = h.e[3].next();
        const auto swell = 1.0f - 0.7f * h.e[4].next();
        const auto metalL = side (0, 1.0), metalR = side (6, 1.012);
        const auto stick = (float) h.v[1];
        const auto ml = 1.5f * h.f[8].highPass (h.f[0].bandPass (metalL)) * ring + h.f[2].highPass (metalL) * (sheen + stick * crack);
        const auto mr = 1.5f * h.f[9].highPass (h.f[1].bandPass (metalR)) * ring + h.f[3].highPass (metalR) * (sheen + stick * crack);
        const auto shimmerL = (float) (1.0 + 0.22 * std::sin (twoPi * (5.3 * t + h.v[2])));
        const auto shimmerR = (float) (1.0 + 0.22 * std::sin (twoPi * (4.1 * t + 2.0 * h.v[2])));
        const auto top = h.v[5] + (h.v[6] - h.v[5]) * std::sqrt ((double) sheen);
        h.v[3] += top * ((double) h.f[4].highPass (noise (h.n1)) - h.v[3]);
        h.v[4] += top * ((double) h.f[5].highPass (noise (h.n2)) - h.v[4]);
        const auto nl = (float) h.v[3], nr = (float) h.v[4];
        const auto mix = (float) h.v[0];
        l = (1.0f - mix) * ml * 2.2f + mix * nl * (hiss * swell * shimmerL + 0.6f * stick * crack);
        r = (1.0f - mix) * mr * 2.2f + mix * nr * (hiss * swell * shimmerR + 0.6f * stick * crack);

        if (shape > 0.0f)
        {
            l = std::tanh (shape * l) / std::tanh (shape);
            r = std::tanh (shape * r) / std::tanh (shape);
        }
    }

    /** Sets a Modern hit going (its tune, tone, decay and velocity already in it); its onset ramp in seconds. */
    double startModern (Hit& h, int note, double D)
    {
        const auto rate = h.rate, u = h.u, T = h.T, S = h.S, vel = h.vel;

        switch (h.sound)
        {
            case Sound::kick:
            {
                // The Classic kick's three characters, each with a beater and shell of its own; struck
                // softer its pitch sweeps less and the beater is hardly heard, so ghost notes rumble.
                const auto character = (int) h.kick;
                const auto low = note == 35;
                const auto decay = D * (low ? 1.15 : 1.0);
                static constexpr double bases[] { 47.0, 52.0, 55.0 }, holds[] { 0.02, 0.012, 0.008 }, shapes[] { 1.1, 1.8, 4.0 }, clicks[] { 0.7, 1.5, 2.2 };
                static constexpr double drop1[] { 1.4, 3.2, 6.0 }, time1[] { 0.006, 0.004, 0.0025 }, drop2[] { 0.8, 1.1, 1.8 }, time2[] { 0.05, 0.028, 0.02 };
                static constexpr double biases[] { 0.2, 0.4, 0.5 }, shells[] { 0.45, 0.9, 1.2 };
                const auto hold = holds[character] * juce::jmin (1.0, S);
                h.v[0] = bases[character] * T * (low ? semis (-2.0) : 1.0);
                h.v[1] = drop1[character] * (0.35 + 0.65 * vel);
                h.v[2] = drop2[character] * (0.6 + 0.4 * vel);
                h.v[3] = shapes[character];
                h.v[4] = clicks[character] * (0.5 + 0.5 * u) * vel;
                h.v[5] = hold * rate;
                h.v[6] = biases[character];
                h.v[7] = 1.0 - std::exp (-twoPi * 20.0 / rate);
                h.v[9] = shells[character] * vel;
                h.e[0].set (juce::jmax (0.005, decay - hold) / 6.9, rate);
                h.e[1].set (time1[character], rate);
                h.e[2].set (time2[character], rate);
                h.e[3].set (0.003, rate);
                h.e[4].set (0.01, rate);
                h.f[0].set (700.0 * std::exp2 ((u + 1.0) * 2.5), 0.707, rate);
                h.f[1].set (3800.0 * std::sqrt (T), 0.8, rate);
                h.f[2].set (220.0 * T, 1.2, rate);
                h.length = (juce::int64) ((hold + decay) * rate);
                return 0.0002;
            }
            case Sound::snare:
            {
                const auto electric = note == 40;
                const auto t = T * (electric ? semis (3.0) : 1.0);
                const auto tone = juce::jlimit (-1.0, 1.0, u + (electric ? 0.3 : 0.0));
                h.v[0] = 185.0 * t;
                h.v[1] = 330.0 * t;
                h.v[2] = (electric ? 0.35 : 0.48) - 0.18 * tone;
                h.v[3] = 2.2 * (1.0 + 0.4 * tone);
                h.v[4] = 0.15 + 0.35 * vel;                               // how far the head's pitch bends at the strike
                h.v[5] = 2.2 * (1.0 + 0.2 * tone) * (0.6 + 0.4 * vel);    // the crack
                h.v[6] = 0.5 * vel;                                       // the snap
                h.e[0].set (0.06 * S, rate);
                h.e[1].set (0.04 * S, rate);
                h.e[2].set (0.01, rate);
                h.e[3].set (D / 6.9, rate);
                h.e[4].set (0.003, rate);
                h.e[5].set (0.006, rate);
                h.e[6].set (0.5 * D / 6.9, rate);
                h.f[0].set (350.0, 0.7, rate);
                h.f[1].set (5200.0 * std::exp2 (1.2 * tone), 0.5, rate);
                h.f[2].set (2500.0, 0.7, rate);
                h.f[3].set (1500.0 * std::sqrt (t) * std::exp2 (0.5 * tone), 0.8, rate);
                h.length = (juce::int64) (juce::jmax (D, 0.35 * S) * rate);
                return 0.0003;
            }
            case Sound::clap:
            {
                const double offsets[] { 0.0, 0.0085, 0.0175, 0.028 };

                for (size_t k = 0; k < 4; ++k)
                    h.v[k] = offsets[k];

                h.v[4] = D / 6.9;
                h.v[5] = 0.16 * S;
                h.v[6] = 0.55 * (1.0 + 0.5 * u);
                const auto hz = 1150.0 * T * std::exp2 (0.6 * u);
                h.f[0].set (hz, 1.8, rate);
                h.f[1].set (400.0, 0.7, rate);
                h.f[2].set (hz * 0.7, 0.8, rate);
                h.f[3].set (hz * 0.75, 0.8, rate);
                h.f[4].set (hz * 2.4, 0.7, rate);
                h.f[5].set (hz * 2.4, 0.7, rate);
                h.length = (juce::int64) ((0.028 + juce::jmax (D, 0.16 * S * 6.9)) * rate);
                return 0.0004;
            }
            case Sound::closedHat: case Sound::pedalHat: case Sound::openHat:
            {
                for (size_t i = 0; i < 6; ++i)
                {
                    h.hz[i] = metalHz[i] * T;
                    h.ph[i] = (double) ((h.n3 = h.n3 * 1664525u + 1013904223u) >> 8) / (double) (1u << 24);
                }

                // Struck softer a hat is shorter too.
                const auto bright = std::exp2 (0.5 * u);
                const auto decay = D * (0.7 + 0.3 * vel);
                h.f[0].set (6800.0 * bright, 1.4, rate);
                h.f[1].set (8600.0 * bright, 0.7, rate);
                h.f[2].set (6200.0 * bright, 0.7, rate);
                h.f[3].set (12500.0 * bright, 0.6, rate);
                h.f[4].set (4500.0 * bright, 0.7, rate);
                h.v[1] = 0.45 + 0.15 * u;
                h.e[0].set (decay / 6.9, rate);
                h.e[1].set (0.004, rate);
                h.e[2].set (0.6 * decay / 6.9, rate);
                h.length = (juce::int64) (decay * rate);
                return h.sound == Sound::pedalHat ? 0.0015 : 0.0003;
            }
            case Sound::lowTom: case Sound::midTom: case Sound::highTom:
            {
                const auto base = (note == 41 ? 80.0 : note == 43 ? 98.0 : note == 45 ? 117.0 : note == 47 ? 139.0 : note == 48 ? 165.0 : 196.0) * T;
                startSkin (h, base, 0.35, 0.05, 2500.0 * std::exp2 (u), 0.004, 0.4 + 0.25 * u, 0.3, D, 2.2, 1.6);
                return 0.00025;
            }
            case Sound::bongo:
                startSkin (h, (note == 60 ? 420.0 : 310.0) * T, 0.2, 0.015, 2500.0 * std::exp2 (u), 0.006, 0.25 + 0.15 * u, 0.15, D * (note == 61 ? 1.2 : 1.0), 0.7, 0.0);
                return 0.00025;
            case Sound::muteConga:
                startSkin (h, 340.0 * T, 0.15, 0.01, 1800.0 * std::exp2 (u), 0.008, 0.35 + 0.2 * u, 0.1, D, 0.7, 0.0);
                return 0.00025;
            case Sound::openConga:
                startSkin (h, (note == 63 ? 330.0 : 225.0) * T, 0.12, 0.02, 1500.0 * std::exp2 (u), 0.006, 0.25 + 0.15 * u, 0.12, D * (note == 64 ? 1.15 : 1.0), 0.7, 0.0);
                return 0.00025;
            case Sound::crash:
                startMetal (h, note == 57 ? 1.35 * semis (2.0) : 1.35, 3600.0, 0.7, 6800.0, 3800.0, 0.55, 1.2, D, 0.4, 0.6);
                return 0.001;
            case Sound::splash:
                startMetal (h, 1.7, 4800.0, 0.7, 7500.0, 5000.0, 0.5, 1.1, D, 0.5, 0.6);
                return 0.0008;
            case Sound::china:
                startMetal (h, 1.05, 2400.0, 0.8, 5000.0, 3000.0, 0.45, 1.1, D, 0.5, 0.5);
                return 0.0008;
            case Sound::ride:
                startMetal (h, note == 59 ? 1.55 * semis (-1.5) : 1.55, 4200.0, 1.4, 7000.0, 5000.0, 0.12, 1.0, D, 0.25, 1.0);
                return 0.0005;
            case Sound::shaker:
            {
                const auto cabasa = note == 69;
                const auto decay = D * (cabasa ? 1.5 : 1.0);
                h.e[0].set (decay / 6.9, rate);
                h.f[0].set ((cabasa ? 5200.0 : 4000.0) * T * std::exp2 (0.6 * u), 0.7, rate);
                h.f[1].set (10500.0 * std::exp2 (0.5 * u), 0.6, rate);
                h.length = (juce::int64) (decay * rate);
                return 0.005;
            }
            case Sound::tambourine:
                for (size_t i = 0; i < 6; ++i)
                    h.hz[i] = metalHz[i] * 3.1 * T;

                h.e[0].set (D / 6.9, rate);
                h.f[0].set (5200.0 * std::exp2 (0.5 * u), 0.7, rate);
                h.f[1].set (12000.0 * std::exp2 (0.5 * u), 0.6, rate);
                h.length = (juce::int64) (D * rate);
                return 0.0005;
            case Sound::agogo:
                h.v[0] = (note == 67 ? 950.0 : 700.0) * T;
                h.e[0].set (D / 6.9, rate);
                h.e[1].set (0.45 * D / 6.9, rate);
                h.e[2].set (0.25 * D / 6.9, rate);
                h.e[3].set (0.003, rate);
                h.f[0].set (h.v[0] * 3.0, 1.0, rate);
                h.length = (juce::int64) (D * rate);
                return 0.0003;
            case Sound::whistle:
                h.v[0] = (note == 71 ? 0.09 : 0.45) * S;
                h.f[0].set (2600.0 * T, 6.0, rate);
                h.length = (juce::int64) ((h.v[0] + 0.15) * rate);
                return 0.0;   // its blow rises over 10 ms of its own
            case Sound::triangle:
            {
                const auto decay = D * (note == 80 ? 0.025 : 1.0);
                h.e[0].set (decay / 6.9, rate);
                h.e[1].set (0.6 * decay / 6.9, rate);
                h.e[2].set (0.5 * decay / 6.9, rate);
                h.e[3].set (0.002, rate);
                h.f[0].set (6000.0, 0.7, rate);
                h.length = (juce::int64) (decay * rate);
                return 0.00025;
            }
            case Sound::rim: case Sound::cowbell: case Sound::rideBell: case Sound::vibraslap: case Sound::timbale: case Sound::guiro:
            case Sound::claves: case Sound::woodBlock: case Sound::cuica: case Sound::count:
                break;
        }

        jassertfalse;   // a sound with no Modern type
        return 0.00025;
    }

    /** One sample of a Modern hit, before its level, pan, drive, onset and end. */
    void voiceModern (Hit& h, float& l, float& r)
    {
        const auto t = (double) h.age / h.rate;
        float s = 0.0f;

        switch (h.sound)
        {
            case Sound::kick:
            {
                const auto hz = h.v[0] * (1.0 + h.v[1] * h.e[1].next() + h.v[2] * h.e[2].next());
                const auto body = sine (h.ph[0], hz, h.rate);
                const auto amp = h.age < (juce::int64) h.v[5] ? 1.0f : h.e[0].next();

                // Clipped off centre, so the body has even harmonics as well as odd (what is heard of a
                // kick on small speakers); what that leaves under 20 Hz is taken off again.
                const auto shape = (float) h.v[3], bias = (float) h.v[6] * amp;
                auto driven = (std::tanh (shape * body * amp + bias) - std::tanh (bias)) / std::tanh (shape);
                h.v[8] += h.v[7] * ((double) driven - h.v[8]);
                driven = h.f[0].lowPass (driven - (float) h.v[8]);

                // The beater: a burst of noise and a knock, then a moment of the shell.
                const auto hit = noise (h.n1);
                const auto click = (0.9f * h.f[1].bandPass (hit) + 0.4f * sine (h.ph[1], 1700.0 * h.T, h.rate)) * h.e[3].next() * (float) h.v[4];
                const auto shell = h.f[2].bandPass (hit) * h.e[4].next() * (float) h.v[9];
                s = 0.9f * driven + click + shell;
                break;
            }
            case Sound::snare:
            {
                // The head: two modes bent down from the strike, pushed into a soft clip. The wires: noise in
                // two bands, a crack in the middle that is over sooner and the sizzle above it. The stick's snap.
                const auto m = 1.0 + h.v[4] * h.e[2].next();
                auto body = sine (h.ph[0], h.v[0] * m, h.rate) * h.e[0].next() + 0.55f * sine (h.ph[1], h.v[1] * m, h.rate) * h.e[1].next();
                body = std::tanh (1.6f * body) * 1.085f;
                const auto wires = h.f[0].highPass (noise (h.n1));
                const auto burst = h.e[5].next();
                const auto crack = h.f[3].bandPass (wires) * (h.e[6].next() + 0.6f * burst);
                const auto sizzle = h.f[1].bandPass (wires) * (0.8f * h.e[3].next() + 0.6f * burst);
                const auto snap = h.f[2].highPass (noise (h.n2)) * h.e[4].next();
                s = body * (float) h.v[2] + sizzle * (float) h.v[3] + crack * (float) h.v[5] + (float) h.v[6] * snap;
                s = std::tanh (1.2f * s) * 1.2f;
                break;
            }
            case Sound::clap:
            {
                // The Classic clap's four bursts, tail and room, and over them the hands' own brightness
                // above the band, a different noise each side, so the clap is wide.
                float bursts = 0.0f;

                for (size_t k = 0; k < 4; ++k)
                    if (const auto dt = t - h.v[k]; dt >= 0.0)
                        bursts += (float) (juce::jmin (1.0, dt / 0.0004) * std::exp (-dt / 0.0028));

                const auto dt = t - h.v[3];
                const auto tail = dt >= 0.0 ? (float) (0.55 * juce::jmin (1.0, dt / 0.002) * std::exp (-dt / h.v[4])) : 0.0f;
                const auto clap = h.f[0].bandPass (h.f[1].highPass (noise (h.n1))) * (bursts + tail) * 2.6f;
                const auto room = (float) ((1.0 - std::exp (-t / 0.01)) * std::exp (-t / h.v[5])) * 0.9f;
                const auto nl = noise (h.n2), nr = noise (h.n3);
                const auto air = (bursts + 0.5f * tail) * (float) h.v[6];
                l = clap + h.f[2].bandPass (nl) * room + h.f[4].highPass (nl) * air;
                r = clap + h.f[3].bandPass (nr) * room + h.f[5].highPass (nr) * air;
                return;
            }
            case Sound::closedHat: case Sound::pedalHat: case Sound::openHat:
            {
                // The metal in two bands, its clang dying sooner than its sheen, and noise for the air; all
                // rolled off above, as a cymbal is, and with a tick where the stick lands.
                float squares = 0.0f;

                for (size_t i = 0; i < 6; ++i)
                    squares += square (h.ph[i], h.hz[i], h.rate);

                squares /= 6.0f;
                const auto ring = h.e[0].next(), tick = h.e[1].next();
                const auto clang = h.f[4].highPass (h.f[0].bandPass (squares)) * (h.e[2].next() + 0.5f * tick);
                const auto sheen = h.f[1].highPass (squares) * (ring + 0.5f * tick);
                const auto air = h.f[2].highPass (noise (h.n1)) * (float) h.v[1] * (ring + 0.3f * tick);
                s = h.f[3].lowPass (1.3f * clang + 1.1f * sheen + air) * 2.0f;
                break;
            }
            case Sound::lowTom: case Sound::midTom: case Sound::highTom:
            case Sound::bongo: case Sound::muteConga: case Sound::openConga:
                s = skin (h);
                break;
            case Sound::crash: case Sound::splash: case Sound::ride:
                metal (h, l, r, 0.0f);
                return;
            case Sound::china:
                metal (h, l, r, 2.2f);
                return;
            case Sound::shaker:
                s = 1.1f * h.f[1].lowPass (h.f[0].highPass (noise (h.n1))) * h.e[0].next();
                break;
            case Sound::tambourine:
            {
                float squares = 0.0f;

                for (size_t i = 0; i < 6; ++i)
                    squares += square (h.ph[i], h.hz[i], h.rate);

                float shakes = 0.0f;

                for (auto at : { 0.0, 0.007, 0.015 })
                    if (const auto dt = t - at; dt >= 0.0)
                        shakes += (float) (juce::jmin (1.0, dt / 0.0005) * std::exp (-dt / 0.006));

                const auto tail = (float) juce::jmin (1.0, t / 0.002) * h.e[0].next();
                s = 1.6f * h.f[1].lowPass (h.f[0].highPass (0.55f * squares / 6.0f + 0.45f * noise (h.n1))) * (0.6f * shakes + 0.7f * tail);
                break;
            }
            case Sound::agogo:
                s = 0.6f * sine (h.ph[0], h.v[0], h.rate) * h.e[0].next() + 0.3f * sine (h.ph[1], h.v[0] * 2.65, h.rate) * h.e[1].next()
                  + 0.15f * sine (h.ph[2], h.v[0] * 4.3, h.rate) * h.e[2].next() + 0.5f * h.f[0].bandPass (noise (h.n1)) * h.e[3].next();
                break;
            case Sound::whistle:
            {
                // The pea's flutter in pitch and in level, an overtone, and the breath round the note.
                const auto blow = h.v[0];
                const auto env = t < 0.01 ? t / 0.01 : (t < blow ? 1.0 : std::exp (-(t - blow) / 0.02));
                const auto flutter = std::sin (twoPi * 18.0 * t);
                const auto hz = 2600.0 * h.T * (1.0 + 0.02 * flutter) * (1.0 - 0.04 * (1.0 - h.vel));   // blown softer it is a little flat
                s = (0.5f * sine (h.ph[0], hz, h.rate) + 0.08f * sine (h.ph[1], 2.0 * hz, h.rate) + 0.35f * h.f[0].bandPass (noise (h.n1)))
                  * (float) (env * (0.85 + 0.15 * flutter));
                break;
            }
            case Sound::triangle:
                s = 0.45f * sine (h.ph[0], 4200.0 * h.T, h.rate) * h.e[0].next() + 0.2f * sine (h.ph[1], 4200.0 * 2.76 * h.T, h.rate) * h.e[1].next()
                  + 0.12f * sine (h.ph[2], 4200.0 * 1.52 * h.T, h.rate) * h.e[2].next() + 0.3f * h.f[0].highPass (noise (h.n1)) * h.e[3].next();
                break;
            case Sound::rim: case Sound::cowbell: case Sound::rideBell: case Sound::vibraslap: case Sound::timbale: case Sound::guiro:
            case Sound::claves: case Sound::woodBlock: case Sound::cuica: case Sound::count:
                break;
        }

        l = r = s;
    }

    /** One sample of a hit, before its level, pan, drive, onset and end. */
    void voice (Hit& h, float& l, float& r)
    {
        if (h.modern)
        {
            voiceModern (h, l, r);
            return;
        }

        const auto t = (double) h.age / h.rate;
        float s = 0.0f;
        l = r = 0.0f;

        switch (h.sound)
        {
            case Sound::kick:
            {
                const auto hz = h.v[0] * (1.0 + h.v[1] * h.e[1].next() + h.v[2] * h.e[2].next());
                const auto body = sine (h.ph[0], hz, h.rate);
                const auto amp = h.age < (juce::int64) h.v[5] ? 1.0f : h.e[0].next();
                const auto shape = (float) h.v[3];
                const auto driven = h.f[0].lowPass (std::tanh (shape * body * amp) / std::tanh (shape));
                const auto click = (0.8f * h.f[1].bandPass (noise (h.n1)) + 0.4f * sine (h.ph[1], 1700.0 * h.T, h.rate)) * h.e[3].next() * (float) h.v[4];
                s = 0.9f * driven + click;
                break;
            }
            case Sound::snare:
            {
                const auto m = 1.0 + 0.15 * h.e[2].next();
                const auto body = sine (h.ph[0], h.v[0] * m, h.rate) * h.e[0].next() + 0.55f * sine (h.ph[1], h.v[1] * m, h.rate) * h.e[1].next();
                const auto wires = h.f[1].bandPass (h.f[0].highPass (noise (h.n1))) * (0.8f * h.e[3].next() + 0.6f * h.e[5].next());
                const auto snap = h.f[2].highPass (noise (h.n2)) * h.e[4].next();
                s = body * (float) h.v[2] + wires * (float) h.v[3] + 0.5f * snap;
                break;
            }
            case Sound::clap:
            {
                // Four bursts a few ms apart, then the tail, each burst rising over 0.4 ms; and a short room, different each side.
                float bursts = 0.0f;

                for (size_t k = 0; k < 4; ++k)
                    if (const auto dt = t - h.v[k]; dt >= 0.0)
                        bursts += (float) (juce::jmin (1.0, dt / 0.0004) * std::exp (-dt / 0.0028));

                const auto dt = t - h.v[3];
                const auto tail = dt >= 0.0 ? (float) (0.55 * juce::jmin (1.0, dt / 0.002) * std::exp (-dt / h.v[4])) : 0.0f;
                const auto clap = h.f[0].bandPass (h.f[1].highPass (noise (h.n1))) * (bursts + tail) * 2.6f;
                const auto room = (float) ((1.0 - std::exp (-t / 0.01)) * std::exp (-t / h.v[5])) * 0.9f;
                l = clap + h.f[2].bandPass (noise (h.n2)) * room;
                r = clap + h.f[3].bandPass (noise (h.n3)) * room;
                return;
            }
            case Sound::closedHat: case Sound::pedalHat: case Sound::openHat:
            {
                float metal = 0.0f;

                for (size_t i = 0; i < 6; ++i)
                    metal += square (h.ph[i], h.hz[i], h.rate);

                const auto x = h.f[1].highPass (h.f[0].bandPass (metal / 6.0f)) * 1.4f + h.f[2].highPass (noise (h.n1)) * (float) h.v[1];
                const auto env = h.e[0].next() + (h.sound == Sound::openHat ? 0.3f * h.e[1].next() : 0.0f);
                s = x * env * 2.0f;
                break;
            }
            case Sound::lowTom: case Sound::midTom: case Sound::highTom:
            case Sound::bongo: case Sound::muteConga: case Sound::openConga:
                s = membrane (h);
                break;
            case Sound::rim:
            {
                const auto env = h.e[0].next();
                s = 0.45f * sine (h.ph[0], 455.0 * h.T, h.rate) * env + 0.6f * sine (h.ph[1], 1667.0 * h.T, h.rate) * env * env
                  + 0.7f * h.f[0].highPass (noise (h.n1)) * h.e[1].next();
                break;
            }
            case Sound::crash: case Sound::splash: case Sound::ride:
                cymbal (h, l, r, 0.0f);
                return;
            case Sound::china:
                cymbal (h, l, r, 2.2f);
                return;
            case Sound::rideBell:
            {
                static constexpr double ratios[] { 1.0, 2.32, 3.1, 4.41 };
                static constexpr float amps[] { 1.0f, 0.6f, 0.4f, 0.25f };

                for (size_t k = 0; k < 4; ++k)
                    s += amps[k] * sine (h.ph[k], h.v[0] * ratios[k], h.rate) * h.e[k].next();

                float metal = 0.0f;

                for (size_t i = 0; i < 6; ++i)
                    metal += square (h.ph[6 + i], h.hz[i], h.rate);

                s = 0.45f * s + 0.5f * h.f[0].highPass (metal / 6.0f) * h.e[4].next();
                break;
            }
            case Sound::cowbell:
            {
                const auto tone = 0.5f * (square (h.ph[0], 540.0 * h.T, h.rate) + square (h.ph[1], 800.0 * h.T, h.rate));
                s = 1.5f * h.f[0].bandPass (tone) * (0.65f * h.e[0].next() + 0.35f * h.e[1].next());
                break;
            }
            case Sound::shaker:
                s = 1.1f * h.f[0].highPass (noise (h.n1)) * h.e[0].next();
                break;
            case Sound::tambourine:
            {
                float metal = 0.0f;

                for (size_t i = 0; i < 6; ++i)
                    metal += square (h.ph[i], h.hz[i], h.rate);

                float shakes = 0.0f;

                for (auto at : { 0.0, 0.007, 0.015 })
                    if (const auto dt = t - at; dt >= 0.0)
                        shakes += (float) (juce::jmin (1.0, dt / 0.0005) * std::exp (-dt / 0.006));

                const auto tail = (float) juce::jmin (1.0, t / 0.002) * h.e[0].next();
                s = 1.6f * h.f[0].highPass (0.55f * metal / 6.0f + 0.45f * noise (h.n1)) * (0.6f * shakes + 0.7f * tail);
                break;
            }
            case Sound::vibraslap:
            {
                const auto beat = 0.5 + 0.5 * std::sin (twoPi * 28.0 * t);
                s = 1.6f * h.f[0].bandPass (noise (h.n1)) * (float) (beat * beat) * h.e[0].next();
                break;
            }
            case Sound::timbale:
                s = 0.6f * sine (h.ph[0], h.v[0], h.rate) * h.e[0].next() + 0.3f * sine (h.ph[1], h.v[0] * 2.7, h.rate) * h.e[1].next()
                  + 0.4f * h.f[0].highPass (noise (h.n1)) * h.e[2].next();
                break;
            case Sound::agogo:
                s = 0.6f * sine (h.ph[0], h.v[0], h.rate) * h.e[0].next() + 0.3f * sine (h.ph[1], h.v[0] * 2.65, h.rate) * h.e[1].next();
                break;
            case Sound::whistle:
            {
                const auto blow = h.v[0];
                const auto env = t < 0.01 ? t / 0.01 : (t < blow ? 1.0 : std::exp (-(t - blow) / 0.02));
                s = 0.5f * sine (h.ph[0], 2600.0 * h.T * (1.0 + 0.02 * std::sin (twoPi * 18.0 * t)), h.rate) * (float) env;
                break;
            }
            case Sound::guiro:
            {
                const auto teeth = std::fmod (t * 45.0 * h.T, 1.0);
                const auto edge = juce::jmin (1.0, teeth / 0.03);   // each tooth rises over a moment, not at once
                const auto end = juce::jmin (1.0, (h.v[0] - t) / 0.02);   // the scrape lifts off over its last 20 ms
                const auto env = t < h.v[0] ? edge * (1.0 - teeth) * (1.0 - teeth) * end * end : 0.0;
                s = 1.6f * h.f[0].bandPass (noise (h.n1)) * (float) env;
                break;
            }
            case Sound::claves:
                s = 0.75f * sine (h.ph[0], 2500.0 * h.T, h.rate) * h.e[0].next() + 0.2f * sine (h.ph[1], 2500.0 * 2.57 * h.T, h.rate) * h.e[1].next();
                break;
            case Sound::woodBlock:
                s = 0.75f * sine (h.ph[0], h.v[0], h.rate) * h.e[0].next() + 0.3f * h.f[0].bandPass (noise (h.n1)) * h.e[1].next();
                break;
            case Sound::cuica:
            {
                const auto open = h.note == 79;
                const auto hz = (open ? 300.0 + 420.0 * (1.0 - std::exp (-t / 0.06)) : 620.0 - 140.0 * (1.0 - std::exp (-t / 0.03))) * h.T;
                const auto env = t < 0.008 ? t / 0.008 : std::exp (-(t - 0.008) / h.v[0]);
                s = 0.7f * sine (h.ph[0], hz, h.rate) * (float) env;
                break;
            }
            case Sound::triangle:
                s = 0.45f * sine (h.ph[0], 4200.0 * h.T, h.rate) * h.e[0].next() + 0.2f * sine (h.ph[1], 4200.0 * 2.76 * h.T, h.rate) * h.e[1].next();
                break;
            case Sound::count:
                break;
        }

        l = r = s;
    }
}

//==============================================================================
Engine::Engine()
    : hits ((size_t) numNotes), fading ((size_t) (2 * numNotes))
{
    settings = defaultKit();
}

Engine::~Engine() = default;

void Engine::prepare (double sampleRate)
{
    rate = sampleRate;
    fadeLength = juce::jmax (1, juce::roundToInt (0.003 * rate));
    glueAttack = (float) std::exp (-1.0 / (0.010 * rate));
    glueRelease = (float) std::exp (-1.0 / (0.150 * rate));

    for (auto* s : { &driveAmount, &glueAmount, &outputGain, &feedbackAmount })
        s->reset (rate, 0.02);

    reverb.setSampleRate (rate);
    reverbSizeSet = -1.0f;
    delayL.assign ((size_t) (3.1 * rate) + 4, 0.0f);
    delayR.assign (delayL.size(), 0.0f);
    delayGlide = (float) (1.0 - std::exp (-1.0 / (0.05 * rate)));
    delayToneK = (float) (1.0 - std::exp (-twoPi * 3500.0 / rate));
    reset();
}

void Engine::reset()
{
    for (auto& h : hits)
        h = {};

    for (auto& h : fading)
        h = {};

    driveAmount.setCurrentAndTargetValue (settings.drive * 0.01f);
    glueAmount.setCurrentAndTargetValue (settings.glue * 0.01f);
    outputGain.setCurrentAndTargetValue (juce::Decibels::decibelsToGain (settings.level, -100.0f));
    glueEnvelope = 0.0f;
    reductionDb = 0.0f;
    clearEffects();
    delaySamples = delayTarget();
    feedbackAmount.setCurrentAndTargetValue (settings.feedback * 0.009f);
}

void Engine::clearEffects()
{
    setReverbSize();
    reverb.reset();
    std::fill (delayL.begin(), delayL.end(), 0.0f);
    std::fill (delayR.begin(), delayR.end(), 0.0f);
    delayWrite = 0;
    delayToneL = delayToneR = 0.0f;
    effectsFade = 0;
}

void Engine::setReverbSize()
{
    if (juce::exactlyEqual (reverbSizeSet, settings.reverbSize))
        return;

    reverbSizeSet = settings.reverbSize;
    juce::Reverb::Parameters p;
    p.roomSize = 0.35f + 0.6f * juce::jlimit (0.0f, 1.0f, settings.reverbSize * 0.01f);
    p.damping = 0.45f;
    p.wetLevel = 0.5f;
    p.dryLevel = 0.0f;
    p.width = 1.0f;
    p.freezeMode = 0.0f;
    reverb.setParameters (p);
}

float Engine::delayTarget() const
{
    if (delayL.size() < 8)
        return 1.0f;

    const auto samples = delayBeats (settings.delayTime) * 60.0 / juce::jlimit (20.0, 999.0, tempo) * rate;
    return (float) juce::jlimit (1.0, (double) delayL.size() - 3.0, samples);
}

void Engine::setTempo (double bpm)
{
    if (bpm > 0.0)
        tempo = bpm;
}

void Engine::setSettings (const KitSettings& s)
{
    settings = s;
    driveAmount.setTargetValue (s.drive * 0.01f);
    glueAmount.setTargetValue (s.glue * 0.01f);
    outputGain.setTargetValue (juce::Decibels::decibelsToGain (s.level, -100.0f));
    feedbackAmount.setTargetValue (s.feedback * 0.009f);
    setReverbSize();
}

int Engine::getNumSounding() const
{
    return (int) std::count_if (hits.begin(), hits.end(), [] (const Hit& h) { return h.active; })
         + (int) std::count_if (fading.begin(), fading.end(), [] (const Hit& h) { return h.active; });
}

void Engine::cut (int note)
{
    const auto i = (size_t) (note - firstNote);
    auto& h = hits[i];

    if (! h.active)
        return;

    // Two fading slots a note, for hits struck again within 3 ms of each other; a third takes the
    // slot whose fade is the nearer done.
    auto& a = fading[2 * i];
    auto& b = fading[2 * i + 1];
    auto& f = ! a.active ? a : ! b.active ? b : (a.fade <= b.fade ? a : b);
    f = h;
    f.fade = f.fadeLength = fadeLength;
    h.active = false;
}

void Engine::noteOn (int note, float velocity)
{
    const auto sound = soundFor (note);

    if (! sound.has_value() || velocity <= 0.0f)
        return;

    if (const auto group = chokeGroup (note); group != 0)
    {
        for (int other = firstNote; other <= lastNote; ++other)
            if (chokeGroup (other) == group)
                cut (other);
    }
    else
    {
        cut (note);
    }

    start (hits[(size_t) (note - firstNote)], note, velocity);
    hitCount[(size_t) *sound].fetch_add (1, std::memory_order_relaxed);
    lastVelocity[(size_t) *sound].store (velocity, std::memory_order_relaxed);
}

void Engine::allSoundOff()
{
    for (int note = firstNote; note <= lastNote; ++note)
        cut (note);

    effectsFade = fadeLength;
}

void Engine::start (Hit& h, int note, float velocity)
{
    const auto sound = *soundFor (note);
    const auto& st = settings.sounds[(size_t) sound];

    h = {};
    h.active = true;
    h.sound = sound;
    h.note = note;
    h.rate = rate;
    h.kick = settings.kick;
    h.T = semis (st.tune);
    h.S = st.decay / infos[(size_t) sound].decayMs;
    h.u = juce::jlimit (-1.0, 1.0, st.tone * 0.01);
    h.n1 = 0x9e3779b9u ^ ((juce::uint32) note * 2654435761u);
    h.n2 = h.n1 * 747796405u + 2891336453u;
    h.n3 = h.n2 * 747796405u + 2891336453u;

    // A Modern sound is duller the softer it is struck, by up to a third of the tone's range.
    if (st.type == 1 && hasTypes (sound))
    {
        h.modern = true;
        h.vel = juce::jlimit (0.0, 1.0, (double) velocity);
        h.u = juce::jlimit (-1.0, 1.0, h.u - 0.45 * (1.0 - h.vel));
    }

    const auto gain = juce::Decibels::decibelsToGain (st.level + infos[(size_t) sound].trimDb + (h.modern ? modernTrimDb (sound) : 0.0f), -100.0f) * velocity;
    const auto angle = (juce::jlimit (-100.0f, 100.0f, st.pan) * 0.01f + 1.0f) * juce::MathConstants<float>::pi * 0.25f;
    h.gainL = gain * std::sqrt (2.0f) * std::cos (angle);
    h.gainR = gain * std::sqrt (2.0f) * std::sin (angle);

    if (st.drive > 0.0f)
    {
        h.drive = 1.0f + 0.09f * st.drive;
        h.driveNorm = 1.0f / std::tanh (h.drive);
    }

    if (st.filter > 0)
    {
        // Up to a Q of about 12; the band-pass held to unity at its peak, the others let ring.
        const auto q = 0.6 + juce::jlimit (0.0f, 100.0f, st.resonance) * 0.11;
        h.filter = juce::jlimit (1, 3, st.filter);
        h.filterL.set (juce::jlimit (20.0f, 20000.0f, st.cutoff), q, rate);
        h.filterR = h.filterL;
        h.filterGain = h.filter == 2 ? h.filterL.k : 1.0f;
    }

    if (st.crush > 0.0f)
    {
        // From 16 bits down to 4, and from every sample to one held for 20.
        const auto c = juce::jlimit (0.0f, 1.0f, st.crush * 0.01f);
        h.crushStep = std::exp2 (1.0f - (16.0f - 12.0f * c));
        h.holdInc = 1.0f / (1.0f + 19.0f * c * c);
    }

    h.sendReverb = juce::jlimit (0.0f, 100.0f, st.reverb) * 0.01f;
    h.sendDelay = juce::jlimit (0.0f, 100.0f, st.delay) * 0.01f;

    if (tilts (sound))
    {
        h.tilt = (float) std::exp2 (1.5 * h.u);   // the top end above 2 kHz, up to 9 dB either way
        h.tiltK = (float) (1.0 - std::exp (-twoPi * 2000.0 / rate));
    }

    const auto D = st.decay * 0.001;   // seconds to fall 60 dB
    const auto u = h.u, T = h.T, S = h.S;
    auto riseSeconds = 0.00025;

    if (h.modern)
        riseSeconds = startModern (h, note, D);
    else switch (sound)
    {
        case Sound::kick:
        {
            const auto character = (int) settings.kick;
            const auto low = note == 35;
            const auto decay = D * (low ? 1.15 : 1.0);
            static constexpr double bases[] { 47.0, 52.0, 55.0 }, holds[] { 0.02, 0.012, 0.008 }, shapes[] { 1.1, 1.8, 4.0 }, clicks[] { 0.25, 0.6, 0.9 };
            static constexpr double drop1[] { 1.4, 3.2, 6.0 }, time1[] { 0.006, 0.004, 0.0025 }, drop2[] { 0.8, 1.1, 1.8 }, time2[] { 0.05, 0.028, 0.02 };
            const auto hold = holds[character] * juce::jmin (1.0, S);
            h.v[0] = bases[character] * T * (low ? semis (-2.0) : 1.0);
            h.v[1] = drop1[character];
            h.v[2] = drop2[character];
            h.v[3] = shapes[character];
            h.v[4] = clicks[character] * (0.5 + 0.5 * u);
            h.v[5] = hold * rate;
            h.e[0].set (juce::jmax (0.005, decay - hold) / 6.9, rate);
            h.e[1].set (time1[character], rate);
            h.e[2].set (time2[character], rate);
            h.e[3].set (0.0015, rate);
            h.f[0].set (700.0 * std::exp2 ((u + 1.0) * 2.5), 0.707, rate);
            h.f[1].set (3200.0 * std::sqrt (T), 0.7, rate);
            h.length = (juce::int64) ((hold + decay) * rate);
            riseSeconds = 0.0002;
            break;
        }
        case Sound::snare:
        {
            const auto electric = note == 40;
            const auto t = T * (electric ? semis (3.0) : 1.0);
            const auto tone = juce::jlimit (-1.0, 1.0, u + (electric ? 0.3 : 0.0));
            h.v[0] = 185.0 * t;
            h.v[1] = 330.0 * t;
            h.v[2] = (electric ? 0.55 : 0.75) - 0.3 * tone;
            h.v[3] = 1.4 * (1.0 + 0.4 * tone);
            h.e[0].set (0.05 * S, rate);
            h.e[1].set (0.035 * S, rate);
            h.e[2].set (0.01, rate);
            h.e[3].set (D / 6.9, rate);
            h.e[4].set (0.003, rate);
            h.e[5].set (0.006, rate);
            h.f[0].set (500.0, 0.7, rate);
            h.f[1].set (4200.0 * std::exp2 (1.2 * tone), 0.6, rate);
            h.f[2].set (2500.0, 0.7, rate);
            h.length = (juce::int64) (juce::jmax (D, 0.35 * S) * rate);
            riseSeconds = 0.0003;
            break;
        }
        case Sound::clap:
        {
            const double offsets[] { 0.0, 0.0085, 0.0175, 0.028 };

            for (size_t k = 0; k < 4; ++k)
                h.v[k] = offsets[k];

            h.v[4] = D / 6.9;
            h.v[5] = 0.16 * S;
            const auto hz = 1150.0 * T * std::exp2 (0.6 * u);
            h.f[0].set (hz, 1.8, rate);
            h.f[1].set (400.0, 0.7, rate);
            h.f[2].set (hz * 0.7, 0.8, rate);
            h.f[3].set (hz * 0.75, 0.8, rate);
            h.length = (juce::int64) ((0.028 + juce::jmax (D, 0.16 * S * 6.9)) * rate);
            riseSeconds = 0.0004;
            break;
        }
        case Sound::closedHat: case Sound::pedalHat: case Sound::openHat:
        {
            for (size_t i = 0; i < 6; ++i)
            {
                h.hz[i] = metalHz[i] * T;
                h.ph[i] = (double) ((h.n3 = h.n3 * 1664525u + 1013904223u) >> 8) / (double) (1u << 24);
            }

            const auto bright = std::exp2 (0.5 * u);
            h.f[0].set (7100.0 * bright, 1.2, rate);
            h.f[1].set (6000.0 * bright, 0.7, rate);
            h.f[2].set (7000.0 * bright, 0.7, rate);
            h.v[1] = 0.3 + 0.15 * u;
            h.e[0].set (D / 6.9, rate);
            h.e[1].set (0.01, rate);
            h.length = (juce::int64) (D * rate);
            riseSeconds = sound == Sound::pedalHat ? 0.0015 : 0.0003;
            break;
        }
        case Sound::lowTom: case Sound::midTom: case Sound::highTom:
        {
            const auto base = (note == 41 ? 80.0 : note == 43 ? 98.0 : note == 45 ? 117.0 : note == 47 ? 139.0 : note == 48 ? 165.0 : 196.0) * T;
            startMembrane (h, base, 0.35, 0.05, 2500.0 * std::exp2 (u), 0.004, 0.25 + 0.2 * u, 0.3, D);
            break;
        }
        case Sound::bongo:
            startMembrane (h, (note == 60 ? 420.0 : 310.0) * T, 0.2, 0.015, 2500.0 * std::exp2 (u), 0.006, 0.25 + 0.15 * u, 0.15, D * (note == 61 ? 1.2 : 1.0));
            break;
        case Sound::muteConga:
            startMembrane (h, 340.0 * T, 0.15, 0.01, 1800.0 * std::exp2 (u), 0.008, 0.35 + 0.2 * u, 0.1, D);
            break;
        case Sound::openConga:
            startMembrane (h, (note == 63 ? 330.0 : 225.0) * T, 0.12, 0.02, 1500.0 * std::exp2 (u), 0.006, 0.25 + 0.15 * u, 0.12, D * (note == 64 ? 1.15 : 1.0));
            break;
        case Sound::rim:
            h.e[0].set (D / 6.9, rate);
            h.e[1].set (0.0015, rate);
            h.f[0].set (2000.0 * std::exp2 (u), 0.7, rate);
            h.length = (juce::int64) (D * rate);
            break;
        case Sound::crash:
            startCymbal (h, note == 57 ? 1.35 * semis (2.0) : 1.35, 5000.0, 0.6, 3500.0, 6000.0, 0.6, 1.2, D);
            riseSeconds = 0.001;
            break;
        case Sound::splash:
            startCymbal (h, 1.7, 6500.0, 0.6, 4500.0, 7000.0, 0.5, 1.1, D);
            riseSeconds = 0.0008;
            break;
        case Sound::china:
            startCymbal (h, 1.05, 3000.0, 0.8, 1800.0, 4000.0, 0.45, 1.1, D);
            riseSeconds = 0.0008;
            break;
        case Sound::ride:
            startCymbal (h, note == 59 ? 1.55 * semis (-1.5) : 1.55, 4500.0, 1.2, 3000.0, 6000.0, 0.15, 1.0, D);
            riseSeconds = 0.0005;
            break;
        case Sound::rideBell:
        {
            h.v[0] = 850.0 * T;

            for (size_t k = 0; k < 4; ++k)
                h.e[k].set (D / 6.9 * std::array<double, 4> { 1.0, 0.7, 0.5, 0.35 }[k], rate);

            for (size_t i = 0; i < 6; ++i)
                h.hz[i] = metalHz[i] * 2.1 * T;

            h.e[4].set (0.6 * D / 6.9, rate);
            h.f[0].set (2200.0, 0.7, rate);
            h.length = (juce::int64) (D * rate);
            break;
        }
        case Sound::cowbell:
            h.e[0].set (0.012, rate);
            h.e[1].set (D / 6.9, rate);
            h.f[0].set (1000.0 * std::exp2 (0.5 * u), 0.8, rate);
            h.length = (juce::int64) (D * rate);
            riseSeconds = 0.0003;
            break;
        case Sound::shaker:
        {
            const auto cabasa = note == 69;
            const auto decay = D * (cabasa ? 1.5 : 1.0);
            h.e[0].set (decay / 6.9, rate);
            h.f[0].set ((cabasa ? 6500.0 : 5000.0) * T * std::exp2 (0.6 * u), 0.7, rate);
            h.length = (juce::int64) (decay * rate);
            riseSeconds = 0.005;
            break;
        }
        case Sound::tambourine:
            for (size_t i = 0; i < 6; ++i)
                h.hz[i] = metalHz[i] * 3.1 * T;

            h.e[0].set (D / 6.9, rate);
            h.f[0].set (6500.0 * std::exp2 (0.5 * u), 0.7, rate);
            h.length = (juce::int64) (D * rate);
            riseSeconds = 0.0005;
            break;
        case Sound::vibraslap:
            h.e[0].set (D / 6.9, rate);
            h.f[0].set (2400.0 * T, 1.5, rate);
            h.length = (juce::int64) (D * rate);
            riseSeconds = 0.001;
            break;
        case Sound::timbale:
            h.v[0] = (note == 65 ? 520.0 : 390.0) * T;
            h.e[0].set (D / 6.9, rate);
            h.e[1].set (0.5 * D / 6.9, rate);
            h.e[2].set (0.02, rate);
            h.f[0].set (3000.0, 0.7, rate);
            h.length = (juce::int64) (D * rate);
            break;
        case Sound::agogo:
            h.v[0] = (note == 67 ? 950.0 : 700.0) * T;
            h.e[0].set (D / 6.9, rate);
            h.e[1].set (0.45 * D / 6.9, rate);
            h.length = (juce::int64) (D * rate);
            riseSeconds = 0.0003;
            break;
        case Sound::whistle:
            h.v[0] = (note == 71 ? 0.09 : 0.45) * S;
            h.length = (juce::int64) ((h.v[0] + 0.15) * rate);
            riseSeconds = 0.0;   // its blow rises over 10 ms of its own
            break;
        case Sound::guiro:
            h.v[0] = (note == 73 ? 0.11 : 0.38) * S;
            h.f[0].set (3200.0, 1.5, rate);
            h.length = (juce::int64) ((h.v[0] + 0.01) * rate);
            break;
        case Sound::claves:
            h.e[0].set (D / 6.9, rate);
            h.e[1].set (0.4 * D / 6.9, rate);
            h.length = (juce::int64) (D * rate);
            break;
        case Sound::woodBlock:
            h.v[0] = (note == 76 ? 1850.0 : 1300.0) * T;
            h.e[0].set (D / 6.9, rate);
            h.e[1].set (0.004, rate);
            h.f[0].set (h.v[0], 1.0, rate);
            h.length = (juce::int64) (D * rate);
            break;
        case Sound::cuica:
        {
            const auto open = note == 79;
            const auto decay = D * (open ? 1.0 : 0.4);
            h.v[0] = decay / 6.9;
            h.length = (juce::int64) ((decay + 0.008) * rate);
            riseSeconds = 0.0;   // it rises over 8 ms of its own
            break;
        }
        case Sound::triangle:
        {
            const auto decay = D * (note == 80 ? 0.025 : 1.0);
            h.e[0].set (decay / 6.9, rate);
            h.e[1].set (0.6 * decay / 6.9, rate);
            h.length = (juce::int64) (decay * rate);
            break;
        }
        case Sound::count:
            break;
    }

    h.rise = juce::jmax (1, juce::roundToInt (riseSeconds * rate));
    h.length = juce::jmax ((juce::int64) h.rise + 1, h.length);
    h.tail = (int) juce::jmin ((juce::int64) juce::roundToInt (0.005 * rate), h.length / 2);
}

void Engine::renderHits (float* left, float* right, int count, float* toReverbL, float* toReverbR, float* toDelayL, float* toDelayR)
{
    const auto sends = toReverbL != nullptr && toReverbR != nullptr && toDelayL != nullptr && toDelayR != nullptr;

    const auto play = [=] (Hit& h, bool fadingOut)
    {
        for (int n = 0; n < count && h.active; ++n)
        {
            float l, r;
            voice (h, l, r);

            if (h.drive > 0.0f)
            {
                l = std::tanh (h.drive * l) * h.driveNorm;
                r = std::tanh (h.drive * r) * h.driveNorm;
            }

            if (h.tilt > 0.0f)
            {
                h.tiltL += h.tiltK * (l - h.tiltL);
                h.tiltR += h.tiltK * (r - h.tiltR);
                l = h.tiltL + h.tilt * (l - h.tiltL);
                r = h.tiltR + h.tilt * (r - h.tiltR);
            }

            if (h.filter > 0)
            {
                h.filterL.run (l);
                h.filterR.run (r);
                l = h.filter == 1 ? h.filterL.lp : h.filter == 2 ? h.filterL.bp * h.filterGain : h.filterL.hp;
                r = h.filter == 1 ? h.filterR.lp : h.filter == 2 ? h.filterR.bp * h.filterGain : h.filterR.hp;
            }

            if (h.crushStep > 0.0f)
            {
                if ((h.holdPhase += h.holdInc) >= 1.0f)
                {
                    h.holdPhase -= 1.0f;
                    h.heldL = std::round (l / h.crushStep) * h.crushStep;
                    h.heldR = std::round (r / h.crushStep) * h.crushStep;
                }

                l = h.heldL;
                r = h.heldR;
            }

            // It rises from silence, and falls to nothing over its last 5 ms.
            auto g = 1.0f;

            if (h.age < h.rise)
                g = (float) h.age / (float) h.rise;

            if (const auto remaining = h.length - h.age; remaining < h.tail)
                g *= (float) juce::jmax ((juce::int64) 0, remaining) / (float) h.tail;

            if (fadingOut)
                g *= (float) h.fade / (float) h.fadeLength;

            l *= g * h.gainL;
            r *= g * h.gainR;
            left[n] += l;
            right[n] += r;

            if (sends)
            {
                toReverbL[n] += l * h.sendReverb;
                toReverbR[n] += r * h.sendReverb;
                toDelayL[n] += l * h.sendDelay;
                toDelayR[n] += r * h.sendDelay;
            }

            if (++h.age >= h.length || (fadingOut && --h.fade <= 0))
                h.active = false;
        }
    };

    for (size_t i = 0; i < hits.size(); ++i)
    {
        play (hits[i], false);
        play (fading[2 * i], true);
        play (fading[2 * i + 1], true);
    }
}

void Engine::render (float* left, float* right, int count)
{
    // Mono: the right side into a scratch block a piece at a time.
    if (right == nullptr)
    {
        std::array<float, 256> scratch;

        for (int done = 0; done < count;)
        {
            const auto n = juce::jmin ((int) scratch.size(), count - done);
            render (left + done, scratch.data(), n);

            for (int i = 0; i < n; ++i)
                left[done + i] = 0.5f * (left[done + i] + scratch[(size_t) i]);

            done += n;
        }

        return;
    }

    for (int done = 0; done < count; done += chunk)
        renderBlock (left + done, right + done, juce::jmin (chunk, count - done));
}

void Engine::renderEffects (float* left, float* right, int count)
{
    if (delayL.empty())
        return;

    const auto size = (int) delayL.size();
    const auto target = delayTarget();

    for (int n = 0; n < count; ++n)
    {
        // Ping-pong: what is sent goes into the left line; the left's echo feeds the right, the
        // right's comes back round to the left, darker each time.
        delaySamples += delayGlide * (target - delaySamples);
        auto at = (float) delayWrite - delaySamples;

        if (at < 0.0f)
            at += (float) size;

        // A position a hair under 0 plus the size rounds up to the size itself: that is the line's start.
        if (at >= (float) size)
            at -= (float) size;

        const auto i0 = (int) at;
        const auto i1 = i0 + 1 < size ? i0 + 1 : 0;
        const auto frac = at - (float) i0;
        const auto outL = delayL[(size_t) i0] + frac * (delayL[(size_t) i1] - delayL[(size_t) i0]);
        const auto outR = delayR[(size_t) i0] + frac * (delayR[(size_t) i1] - delayR[(size_t) i0]);
        delayToneL += delayToneK * (outL - delayToneL);
        delayToneR += delayToneK * (outR - delayToneR);

        const auto fb = feedbackAmount.getNextValue();
        delayL[(size_t) delayWrite] = 0.5f * (sendDelayL[(size_t) n] + sendDelayR[(size_t) n]) + fb * delayToneR;
        delayR[(size_t) delayWrite] = fb * delayToneL;

        if (++delayWrite >= size)
            delayWrite = 0;

        // A sample at a time, so the reverb clears at the same sample whatever the block size.
        auto wetL = sendReverbL[(size_t) n], wetR = sendReverbR[(size_t) n];
        reverb.processStereo (&wetL, &wetR, 1);
        auto l = wetL + outL;
        auto r = wetR + outR;

        // After All Sound Off the effects fade with the hits, then start again empty.
        if (effectsFade > 0)
        {
            const auto g = (float) --effectsFade / (float) fadeLength;
            l *= g;
            r *= g;

            if (effectsFade == 0)
                clearEffects();
        }

        left[n] += l;
        right[n] += r;
    }
}

void Engine::renderBlock (float* left, float* right, int count)
{
    std::fill (left, left + count, 0.0f);
    std::fill (right, right + count, 0.0f);

    for (auto* send : { &sendReverbL, &sendReverbR, &sendDelayL, &sendDelayR })
        std::fill (send->begin(), send->begin() + count, 0.0f);

    renderHits (left, right, count, sendReverbL.data(), sendReverbR.data(), sendDelayL.data(), sendDelayR.data());
    renderEffects (left, right, count);

    auto mostReduction = 0.0f;

    for (int n = 0; n < count; ++n)
    {
        auto l = left[n], r = right[n];

        // Drive: a soft clip, small signals lifted by up to 9 dB, peaks held down.
        if (const auto d = driveAmount.getNextValue(); d > 0.0f)
        {
            const auto g = 1.0f + 7.0f * d;
            const auto makeUp = std::sqrt (g) / g;
            l = std::tanh (g * l) * makeUp;
            r = std::tanh (g * r) * makeUp;
        }

        // Glue: a linked bus compressor, threshold and ratio moving together, a little make-up.
        const auto peak = juce::jmax (std::abs (l), std::abs (r));
        glueEnvelope = peak > glueEnvelope ? peak + glueAttack * (glueEnvelope - peak) : peak + glueRelease * (glueEnvelope - peak);

        if (const auto amount = glueAmount.getNextValue(); amount > 0.0f)
        {
            const auto threshold = -4.0f - 20.0f * amount, ratio = 1.5f + 2.5f * amount, knee = 6.0f;
            const auto levelDb = juce::Decibels::gainToDecibels (glueEnvelope, -120.0f);
            const auto over = levelDb - threshold;
            auto reduction = 0.0f;

            if (over > knee * 0.5f)
                reduction = over * (1.0f / ratio - 1.0f);
            else if (over > -knee * 0.5f)
                reduction = (1.0f / ratio - 1.0f) * (over + knee * 0.5f) * (over + knee * 0.5f) / (2.0f * knee);

            mostReduction = juce::jmin (mostReduction, reduction);
            const auto makeUp = -threshold * (1.0f - 1.0f / ratio) * 0.35f;
            const auto gain = juce::Decibels::decibelsToGain (reduction + makeUp);
            l *= gain;
            r *= gain;
        }

        const auto out = outputGain.getNextValue();
        left[n] = l * out;
        right[n] = r * out;
    }

    reductionDb.store (mostReduction, std::memory_order_relaxed);
}

juce::AudioBuffer<float> Engine::renderOne (const KitSettings& kit, Sound sound, double sampleRate, double seconds, int note)
{
    Engine engine;
    engine.setSettings (kit);
    engine.prepare (sampleRate);
    engine.noteOn (note < 0 ? padNote (sound) : note, 1.0f);
    juce::AudioBuffer<float> out (2, juce::jmax (1, (int) (seconds * sampleRate)));
    out.clear();
    engine.renderHits (out.getWritePointer (0), out.getWritePointer (1), out.getNumSamples());
    return out;
}
}

//==============================================================================
namespace BuiltInEffects
{
using namespace DrumMachine;

juce::PluginDescription basicDrumsDescription()
{
    juce::PluginDescription d;
    d.name = d.descriptiveName = "Basic Drums";
    d.pluginFormatName = formatName;
    d.category = "Instrument";
    d.manufacturerName = "Torqwave";
    d.version = "1";
    d.fileOrIdentifier = basicDrumsId;
    d.uniqueId = d.deprecatedUid = 0x53424452;   // "SBDR"
    d.isInstrument = true;
    d.numInputChannels = 0;
    d.numOutputChannels = 2;
    return d;
}

BasicDrumsProcessor::BasicDrumsProcessor()
    : BuiltInEffect (basicDrumsDescription(), "TORQWAVE_BASIC_DRUMS", false, true)
{
    for (int i = 0; i < numSounds; ++i)
    {
        const auto sound = (Sound) i;
        const auto id = idOf (sound), name = nameOf (sound);
        const auto d = defaultsFor (sound);
        auto& p = sounds[(size_t) i];
        p.tune = addFloat (id + "_tune", name + " Tune", { -24.0f, 24.0f }, d.tune, "st");
        p.decay = addFloat (id + "_decay", name + " Decay", { 5.0f, 5000.0f, 0.0f, 0.3f }, d.decay, "ms");
        p.tone = addFloat (id + "_tone", name + " Tone", { -100.0f, 100.0f }, d.tone, "%");
        p.level = addFloat (id + "_level", name + " Level", { -48.0f, 6.0f }, d.level, "dB");
        p.pan = addFloat (id + "_pan", name + " Pan", { -100.0f, 100.0f }, d.pan, "%");
        p.drive = addFloat (id + "_drive", name + " Drive", { 0.0f, 100.0f }, d.drive, "%");
        p.filter = addChoice (id + "_filter", name + " Filter", filterNames(), d.filter);
        p.cutoff = addFloat (id + "_cutoff", name + " Cutoff", { 20.0f, 20000.0f, 0.0f, 0.25f }, d.cutoff, "Hz");
        p.resonance = addFloat (id + "_resonance", name + " Resonance", { 0.0f, 100.0f }, d.resonance, "%");
        p.crush = addFloat (id + "_crush", name + " Crush", { 0.0f, 100.0f }, d.crush, "%");
        p.reverb = addFloat (id + "_reverb", name + " Reverb", { 0.0f, 100.0f }, d.reverb, "%");
        p.delay = addFloat (id + "_delay", name + " Delay", { 0.0f, 100.0f }, d.delay, "%");
    }

    // After every sound's own twelve, so those keep their places: the type of each sound that has two.
    for (int i = 0; i < numSounds; ++i)
        if (const auto sound = (Sound) i; hasTypes (sound))
            sounds[(size_t) i].type = addChoice (idOf (sound) + "_type", nameOf (sound) + " Type", typeNames(), 0);

    const auto start = defaultKit();
    kickCharacter = addChoice ("kick_character", "Kick Character", kickCharacterNames(), (int) start.kick);
    kitDrive = addFloat ("drive", "Kit Drive", { 0.0f, 100.0f }, start.drive, "%");
    glue = addFloat ("glue", "Glue", { 0.0f, 100.0f }, start.glue, "%");
    level = addFloat ("level", "Level", { -48.0f, 6.0f }, start.level, "dB");
    reverbSize = addFloat ("reverb_size", "Reverb Size", { 0.0f, 100.0f }, start.reverbSize, "%");
    delayTime = addChoice ("delay_time", "Delay Time", delayTimeNames(), start.delayTime);
    feedback = addFloat ("delay_feedback", "Delay Feedback", { 0.0f, 100.0f }, start.feedback, "%");
    auditions.reserve (64);
}

KitSettings BasicDrumsProcessor::currentSettings() const
{
    KitSettings k;

    for (size_t i = 0; i < sounds.size(); ++i)
    {
        const auto& p = sounds[i];
        k.sounds[i] = { p.tune->get(), p.decay->get(), p.tone->get(), p.level->get(), p.pan->get(), p.drive->get(),
                        p.filter->getIndex(), p.cutoff->get(), p.resonance->get(), p.crush->get(), p.reverb->get(), p.delay->get(),
                        p.type != nullptr ? p.type->getIndex() : 0 };
    }

    k.kick = (KickCharacter) kickCharacter->getIndex();
    k.drive = kitDrive->get();
    k.glue = glue->get();
    k.level = level->get();
    k.reverbSize = reverbSize->get();
    k.delayTime = delayTime->getIndex();
    k.feedback = feedback->get();
    return k;
}

void BasicDrumsProcessor::applySettings (const KitSettings& k)
{
    for (size_t i = 0; i < sounds.size(); ++i)
    {
        const auto& p = sounds[i];
        const auto& s = k.sounds[i];
        setExactly (*p.tune, s.tune);
        setExactly (*p.decay, s.decay);
        setExactly (*p.tone, s.tone);
        setExactly (*p.level, s.level);
        setExactly (*p.pan, s.pan);
        setExactly (*p.drive, s.drive);
        p.filter->setValueNotifyingHost (p.filter->convertTo0to1 ((float) s.filter));
        setExactly (*p.cutoff, s.cutoff);
        setExactly (*p.resonance, s.resonance);
        setExactly (*p.crush, s.crush);
        setExactly (*p.reverb, s.reverb);
        setExactly (*p.delay, s.delay);

        if (p.type != nullptr)
            p.type->setValueNotifyingHost (p.type->convertTo0to1 ((float) juce::jlimit (0, 1, s.type)));
    }

    kickCharacter->setValueNotifyingHost (kickCharacter->convertTo0to1 ((float) (int) k.kick));
    setExactly (*kitDrive, k.drive);
    setExactly (*glue, k.glue);
    setExactly (*level, k.level);
    setExactly (*reverbSize, k.reverbSize);
    delayTime->setValueNotifyingHost (delayTime->convertTo0to1 ((float) k.delayTime));
    setExactly (*feedback, k.feedback);
}

void BasicDrumsProcessor::applyKit (const Kit& kit)
{
    applySettings (kit.settings);
    kitName = kit.name;
}

bool BasicDrumsProcessor::kitModified() const
{
    const auto index = kitIndex (kitName);

    if (index < 0)
        return true;

    const auto near = [] (float a, float b) { return std::abs (a - b) <= 1.0e-3f * juce::jmax (1.0f, std::abs (b)); };
    const auto now = currentSettings();
    const auto& kit = kits()[(size_t) index].settings;

    for (size_t i = 0; i < now.sounds.size(); ++i)
    {
        const auto &a = now.sounds[i], &b = kit.sounds[i];

        if (! (near (a.tune, b.tune) && near (a.decay, b.decay) && near (a.tone, b.tone) && near (a.level, b.level) && near (a.pan, b.pan) && near (a.drive, b.drive)
               && a.filter == b.filter && near (a.cutoff, b.cutoff) && near (a.resonance, b.resonance) && near (a.crush, b.crush)
               && near (a.reverb, b.reverb) && near (a.delay, b.delay) && a.type == b.type))
            return true;
    }

    return now.kick != kit.kick || ! near (now.drive, kit.drive) || ! near (now.glue, kit.glue) || ! near (now.level, kit.level)
        || ! near (now.reverbSize, kit.reverbSize) || now.delayTime != kit.delayTime || ! near (now.feedback, kit.feedback);
}

juce::AudioParameterFloat* BasicDrumsProcessor::parameterOf (Sound sound, const juce::String& what) const
{
    const auto& p = sounds[(size_t) sound];
    return what == "tune" ? p.tune : what == "decay" ? p.decay : what == "tone" ? p.tone
         : what == "level" ? p.level : what == "pan" ? p.pan : what == "drive" ? p.drive
         : what == "cutoff" ? p.cutoff : what == "resonance" ? p.resonance : what == "crush" ? p.crush
         : what == "reverb" ? p.reverb : what == "delay" ? p.delay : nullptr;
}

void BasicDrumsProcessor::addToState (juce::XmlElement& xml) const
{
    xml.setAttribute ("kit", kitName);
}

void BasicDrumsProcessor::readFromState (const juce::XmlElement& xml)
{
    // A state that only names a factory kit (the factory sequence presets' voices): the kit as it is now.
    if (xml.hasAttribute ("factoryKit"))
    {
        const auto index = DrumMachine::kitIndex (xml.getStringAttribute ("factoryKit"));
        applyKit (DrumMachine::kits()[(size_t) juce::jmax (0, index)]);
        return;
    }

    kitName = xml.getStringAttribute ("kit", "Standard");

    // Before step 10 there were a kick, snare, hat and tom tune (semitones, as now) and decay (in
    // percent of the sound's own); they go to the sounds they meant, the decays scaled from the new
    // sounds' own lengths.
    if (xml.hasAttribute ("kick_tune"))
        return;

    const auto legacy = [this, &xml] (const char* tuneId, const char* decayId, std::initializer_list<Sound> to)
    {
        for (auto s : to)
        {
            auto& p = sounds[(size_t) s];

            if (xml.hasAttribute (tuneId))
                setExactly (*p.tune, (float) xml.getDoubleAttribute (tuneId));

            if (xml.hasAttribute (decayId))
                setExactly (*p.decay, defaultsFor (s).decay * (float) xml.getDoubleAttribute (decayId) * 0.01f);
        }
    };

    legacy ("kickTune", "kickDecay", { Sound::kick });
    legacy ("snareTune", "snareDecay", { Sound::snare });
    legacy ("hatTune", "hatDecay", { Sound::closedHat, Sound::pedalHat, Sound::openHat });
    legacy ("tomTune", "tomDecay", { Sound::lowTom, Sound::midTom, Sound::highTom });
}

void BasicDrumsProcessor::prepareToPlay (double sampleRate, int)
{
    engine.setSettings (currentSettings());
    engine.prepare (sampleRate);
    volume = 1.0f;
}

void BasicDrumsProcessor::reset()
{
    engine.setSettings (currentSettings());
    engine.reset();
    volume = 1.0f;
}

void BasicDrumsProcessor::audition (int note, float velocity)
{
    const juce::SpinLock::ScopedLockType lock (auditionLock);

    if (auditions.size() < auditions.capacity())
        auditions.emplace_back (note, velocity);
}

void BasicDrumsProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    const auto n = buffer.getNumSamples();
    const auto channels = buffer.getNumChannels();
    buffer.clear();

    if (channels == 0)
    {
        midi.clear();
        return;
    }

    engine.setSettings (currentSettings());

    if (auto* head = getPlayHead())
        if (const auto position = head->getPosition())
            if (const auto bpm = position->getBpm())
                engine.setTempo (*bpm);

    if (const juce::SpinLock::ScopedTryLockType lock (auditionLock); lock.isLocked())
    {
        for (auto [note, velocity] : auditions)
            engine.noteOn (note, velocity);

        auditions.clear();
    }

    auto* left = buffer.getWritePointer (0);
    auto* right = channels > 1 ? buffer.getWritePointer (1) : nullptr;
    int done = 0;

    for (const auto meta : midi)
    {
        const auto at = juce::jlimit (done, n, meta.samplePosition);
        engine.render (left + done, right != nullptr ? right + done : nullptr, at - done);
        done = at;

        const auto m = meta.getMessage();

        if (m.isNoteOn())
            engine.noteOn (m.getNoteNumber(), m.getFloatVelocity());
        else if (m.isAllSoundOff())
            engine.allSoundOff();   // a 3 ms fade, not a cut, which would click
        else if (m.isController() && m.getControllerNumber() == 7)
            volume = (float) m.getControllerValue() / 127.0f;
        else if (m.isController() && m.getControllerNumber() == 121)
            volume = 1.0f;
    }

    engine.render (left + done, right != nullptr ? right + done : nullptr, n - done);

    if (! juce::exactlyEqual (volume, 1.0f))
        buffer.applyGain (volume * volume);

    for (int ch = 2; ch < channels; ++ch)
        buffer.clear (ch, 0, n);

    midi.clear();
}

juce::AudioProcessorEditor* BasicDrumsProcessor::createEditor()
{
    return new DrumMachineEditor (*this);
}
}
