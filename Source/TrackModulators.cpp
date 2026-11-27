#include "TrackModulators.h"
#include <cmath>

std::unique_ptr<juce::XmlElement> TrackModulator::toXml() const
{
    auto xml = std::make_unique<juce::XmlElement> ("MODULATOR");
    xml->setAttribute ("id", (int) id);
    xml->setAttribute ("kind", kind == Kind::lfo ? "lfo" : "env");
    xml->setAttribute ("enabled", enabled);
    if (name.isNotEmpty()) xml->setAttribute ("name", name);
    xml->setAttribute ("targetSlot", (int) targetSlotId);
    xml->setAttribute ("targetParam", targetParameterId);
    xml->setAttribute ("amount", (double) amount);
    xml->setAttribute ("bipolar", bipolar);
    xml->setAttribute ("shape", (int) shape);
    xml->setAttribute ("sync", sync);
    xml->setAttribute ("rateHz", rateHz);
    xml->setAttribute ("divisionBeats", divisionBeats);
    xml->setAttribute ("phaseOffset", (double) phaseOffset);
    xml->setAttribute ("attackMs", (double) attackMs);
    xml->setAttribute ("releaseMs", (double) releaseMs);
    xml->setAttribute ("sensitivity", (double) sensitivity);
    return xml;
}

std::optional<TrackModulator> TrackModulator::fromXml (const juce::XmlElement& xml)
{
    if (! xml.hasTagName ("MODULATOR"))
        return std::nullopt;

    TrackModulator m;
    m.id = (juce::uint32) xml.getIntAttribute ("id", 0);
    m.kind = xml.getStringAttribute ("kind", "lfo") == "env" ? Kind::envelopeFollower : Kind::lfo;
    m.enabled = xml.getBoolAttribute ("enabled", true);
    m.name = xml.getStringAttribute ("name");
    m.targetSlotId = (juce::uint32) xml.getIntAttribute ("targetSlot", 0);
    m.targetParameterId = xml.getStringAttribute ("targetParam");
    m.amount = juce::jlimit (-1.0f, 1.0f, (float) xml.getDoubleAttribute ("amount", 0.25));
    m.bipolar = xml.getBoolAttribute ("bipolar", true);
    m.shape = (Shape) juce::jlimit (0, (int) Shape::sampleAndHold, xml.getIntAttribute ("shape", 0));
    m.sync = xml.getBoolAttribute ("sync", true);
    m.rateHz = juce::jlimit (0.05, 20.0, xml.getDoubleAttribute ("rateHz", 1.0));
    m.divisionBeats = juce::jlimit (0.0625, 32.0, xml.getDoubleAttribute ("divisionBeats", 1.0));
    m.phaseOffset = juce::jlimit (0.0f, 1.0f, (float) xml.getDoubleAttribute ("phaseOffset", 0.0));
    m.attackMs = juce::jlimit (1.0f, 500.0f, (float) xml.getDoubleAttribute ("attackMs", 10.0));
    m.releaseMs = juce::jlimit (10.0f, 2000.0f, (float) xml.getDoubleAttribute ("releaseMs", 120.0));
    m.sensitivity = juce::jlimit (0.2f, 10.0f, (float) xml.getDoubleAttribute ("sensitivity", 2.0));
    return m;
}

namespace TrackModulators
{
float evaluateLfo (const TrackModulator& m, double cyclePhase) noexcept
{
    const auto pTotal = cyclePhase + (double) m.phaseOffset;
    const auto p = pTotal - std::floor (pTotal);
    float v = 0.0f;

    switch (m.shape)
    {
        case TrackModulator::Shape::sine:
            v = (float) std::sin (juce::MathConstants<double>::twoPi * p);
            break;
        case TrackModulator::Shape::triangle:
            v = (float) (1.0 - 4.0 * std::abs (p - 0.5));
            break;
        case TrackModulator::Shape::sawUp:
            v = (float) (2.0 * p - 1.0);
            break;
        case TrackModulator::Shape::sawDown:
            v = (float) (1.0 - 2.0 * p);
            break;
        case TrackModulator::Shape::square:
            v = p < 0.5 ? 1.0f : -1.0f;
            break;
        case TrackModulator::Shape::sampleAndHold:
        {
            auto h = (juce::uint32) ((juce::int64) std::floor (pTotal) * 2654435761u + m.id * 7919u);
            h ^= h >> 16; h *= 0x7feb352du; h ^= h >> 15; h *= 0x846ca68bu; h ^= h >> 16;
            v = (float) (h >> 8) * (2.0f / 16777216.0f) - 1.0f;
            break;
        }
    }

    return m.bipolar ? v : (0.5f * v + 0.5f);
}

float Runtime::advanceFollower (int index, const TrackModulator& m, float blockPeak, double sampleRate, int numSamples) noexcept
{
    if (index < 0 || index >= maxModulators || sampleRate <= 0.0 || numSamples <= 0)
        return 0.0f;

    const auto target = juce::jlimit (0.0f, 1.0f, blockPeak * m.sensitivity);
    const auto dtSec = (double) numSamples / sampleRate;
    const auto tauSec = (double) (target > envState[(size_t) index] ? m.attackMs : m.releaseMs) * 0.001;
    const auto coeff = (float) (1.0 - std::exp (-dtSec / juce::jmax (1.0e-4, tauSec)));
    envState[(size_t) index] += coeff * (target - envState[(size_t) index]);
    return envState[(size_t) index];
}
}
