#pragma once

class AudioEngine;

/** The factory presets: unpacked from the app read-only, shown under Presets in the library,
    rated beside their files, copied (never changed) when kept or edited, and the rules every
    preset keeps (the grid, the drum map, mono bass, the key, lengths, descriptions, styles). */
void runPresetTests (AudioEngine&);
