#pragma once

#include <JuceHeader.h>
#include <vector>

/** Track-level modulators (Bitwig / Ableton style): tempo-synced or free LFOs and audio envelope
    followers attached to a session track, able to modulate any automatable parameter of any plug-in
    (built-in, VST3, AU, LV2, or CLAP) in that track's chain. Evaluated on the audio thread in
    TrackEffects at every automation step, deterministically from session time. */
struct TrackModulator
{
    enum class Kind { lfo, envelopeFollower };
    enum class Shape { sine, triangle, sawUp, sawDown, square, sampleAndHold };

    juce::uint32 id = 0;
    Kind kind = Kind::lfo;
    bool enabled = true;
    juce::String name;

    // Target parameter in the track's chain
    juce::uint32 targetSlotId = 0;
    juce::String targetParameterId;
    float amount = 0.25f;              // -1.0 to +1.0 normalised parameter offset
    bool bipolar = true;               // LFO: true = [-1, 1], false = [0, 1]

    // LFO settings
    Shape shape = Shape::sine;
    bool sync = true;
    double rateHz = 1.0;               // 0.05 to 20 Hz when sync == false
    double divisionBeats = 1.0;        // beats per cycle when sync == true (1.0 = 1/4)
    float phaseOffset = 0.0f;          // 0..1

    // Envelope follower settings
    float attackMs = 10.0f;            // 1..500 ms
    float releaseMs = 120.0f;          // 10..2000 ms
    float sensitivity = 2.0f;          // 0.2..10.0 gain into follower

    bool operator== (const TrackModulator& o) const noexcept
    {
        return id == o.id && kind == o.kind && enabled == o.enabled && name == o.name
            && targetSlotId == o.targetSlotId && targetParameterId == o.targetParameterId
            && juce::exactlyEqual (amount, o.amount) && bipolar == o.bipolar
            && shape == o.shape && sync == o.sync
            && juce::exactlyEqual (rateHz, o.rateHz) && juce::exactlyEqual (divisionBeats, o.divisionBeats)
            && juce::exactlyEqual (phaseOffset, o.phaseOffset)
            && juce::exactlyEqual (attackMs, o.attackMs) && juce::exactlyEqual (releaseMs, o.releaseMs)
            && juce::exactlyEqual (sensitivity, o.sensitivity);
    }

    std::unique_ptr<juce::XmlElement> toXml() const;
    static std::optional<TrackModulator> fromXml (const juce::XmlElement&);
};

namespace TrackModulators
{
    /** Evaluates an LFO modulator at a given cycle phase (0..1+) and returns [-1, 1] (or [0, 1] if unipolar). */
    float evaluateLfo (const TrackModulator&, double cyclePhase) noexcept;

    /** Runtime audio-thread state for one track's modulators (allocates nothing in audio processing). */
    struct Runtime
    {
        static constexpr int maxModulators = 16;
        std::array<float, maxModulators> envState {};

        void reset() noexcept { envState.fill (0.0f); }

        /** Advances envelope follower `index` over `numSamples` with peak magnitude `blockPeak` and returns 0..1. */
        float advanceFollower (int index, const TrackModulator&, float blockPeak, double sampleRate, int numSamples) noexcept;
    };
}
