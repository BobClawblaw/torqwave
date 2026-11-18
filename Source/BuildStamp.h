#pragma once

#include <JuceHeader.h>
#include "TorqwaveBuildStamp.h"   // written at every build by Scripts/build-stamp.cmake

/** What this build is, for the loading screen and the About box. */
namespace BuildStamp
{
    /** The version as shown: the plain version for a release build (made at its tag, or by the
        release workflows); for any other, "<version> (test build <time>, <commit>)" so one test
        build can be told from another. */
    inline juce::String describe (const juce::String& version)
    {
        if (TORQWAVE_BUILD_RELEASE != 0 || version.isEmpty())
            return version;

        const juce::String commit (TORQWAVE_BUILD_COMMIT);
        return version + " (test build " + juce::String (TORQWAVE_BUILD_TIME) + (commit.isNotEmpty() ? ", " + commit : juce::String()) + ")";
    }
}
