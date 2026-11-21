#pragma once

#include <JuceHeader.h>

/** The curve a fade follows from silence to full level. */
enum class FadeShape
{
    sCurve,        // raised cosine: gentle at both ends
    linear,
    equalPower,    // a quarter sine: two of these crossfading keep uncorrelated sound's power steady
    slow,          // squared: starts slowly
    fast           // rises quickly, then levels off
};

namespace FadeShapes
{
    constexpr std::array<FadeShape, 5> all { FadeShape::sCurve, FadeShape::linear, FadeShape::equalPower, FadeShape::slow, FadeShape::fast };

    /** The level `frame` frames into a fade `length` frames long (1 from its end on). */
    float level (FadeShape, juce::int64 frame, juce::int64 length);

    /** A clip's level at `frame` of its `length` frames from its fade in and fade out. The fade
        in's first frame is already one step up from silence and the fade out's last frame its
        last step down, so where a fade out and a fade in of the same length play over the same
        frames, a matched pair sums to exactly 1: linear with linear, S-curve with S-curve, Slow
        Start in with Fast Start out, and Equal Power with Equal Power in power (squares). */
    float clipLevel (FadeShape inShape, juce::int64 fadeIn, FadeShape outShape, juce::int64 fadeOut,
                     juce::int64 frame, juce::int64 length);

    /** "S-Curve", "Linear", ... as menus show them, and the names session files use. */
    juce::String displayName (FadeShape);
    juce::String id (FadeShape);
    FadeShape fromId (const juce::String&);   // S-curve for one it doesn't know
}
