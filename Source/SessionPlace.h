#pragma once

#include <JuceHeader.h>

/** Where the multitrack view was in a session: the cursor (the playhead while stopped), the time
    range, the zoom and scroll, and the selected track, put back when the session is shown again.
    A zero framesPerPixel means none was kept (the session is fitted). */
struct SessionPlace
{
    juce::int64 cursor = 0;
    juce::Range<juce::int64> selection;
    double viewStart = 0.0, framesPerPixel = 0.0;
    int scrollY = 0;                    // the lanes' scroll, in pixels from the top
    int selectedTrack = -1;             // its place among the session's tracks (ids change when a session is read), or -1
};
