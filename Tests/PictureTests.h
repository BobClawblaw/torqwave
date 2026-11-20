#pragma once

class AudioEngine;

/** Working to picture: timecode at every rate and from a start, saved with the session; cuts
    found in a generated video; tempos and tempo maps fitted to hits, in code and in the Fit Tempo
    panel; and sounds spotted on markers by their sync points, in code and dropped in the view. */
void runPictureTests (AudioEngine&);
