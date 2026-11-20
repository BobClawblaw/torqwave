#pragma once

class AudioEngine;

/** Sound design in the session: a clip edited in place (a new take, undoable, other clips on
    its file untouched; a following clip's original), clip effects (played as rendered, the same
    at any block size and in a mixdown; the background renderer; the panel; the session file),
    Combine to New Sound, Add to Library, the editor's link to its clip and the double-click. */
void runSoundDesignTests (AudioEngine&);
