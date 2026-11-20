#pragma once

/** MIDI out: a track's note clips through a virtual MIDI port (the messages and their times,
    channel and offset, stopping and jumps, live input), MIDI clock and transport, MIDI from one
    track's plug-ins into another's instrument, and the settings in the session file. */
void runMidiOutTests();
