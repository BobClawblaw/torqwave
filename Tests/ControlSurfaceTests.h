#pragma once

class AudioEngine;

/** Control surfaces: the Mackie Control protocol's messages, a surface driven by a simulated
    device over virtual MIDI ports (faders, touch, buttons, banks, V-pots, the transport, what it
    shows and motorised faders following automation, automation written from it), and MIDI learn
    (learning, each kind of binding, which CCs it takes, the session file, removing a track). */
void runControlSurfaceTests (AudioEngine&);
