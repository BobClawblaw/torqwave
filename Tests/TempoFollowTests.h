#pragma once

/** Audio that follows the tempo: clicks landing on the session's beats through tempo changes and
    tempo edits, warp markers, the session file, tempo from names and from the audio, slicing at
    transients, and the renderer that stretches clips in the background. */
void runTempoFollowTests();
