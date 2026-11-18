#pragma once

#include <JuceHeader.h>

#if JUCE_WINDOWS || JUCE_LINUX

/** .m4a files read through Windows' own AAC and Apple Lossless decoders, as Core Audio reads them on
    the Mac: from the first frame of the audio to its last, the encoder's delay and padding left out
    as the file's edit list (or iTunSMPB) says, at any position to the frame. Reading only; AAC is
    written by AacWriter.

    On Linux, which has no decoders of its own for them, the file is decoded whole by the user's ffmpeg
    (which also leaves out the priming and padding by the edit list) into a temporary WAV, read from there. */
class M4aFormat final : public juce::AudioFormat
{
public:
    M4aFormat();

    juce::Array<int> getPossibleSampleRates() override    { return {}; }
    juce::Array<int> getPossibleBitDepths() override      { return {}; }
    bool canDoStereo() override                           { return true; }
    bool canDoMono() override                             { return true; }
    bool isCompressed() override                          { return true; }

    juce::AudioFormatReader* createReaderFor (juce::InputStream*, bool deleteStreamIfOpeningFails) override;

    std::unique_ptr<juce::AudioFormatWriter> createWriterFor (std::unique_ptr<juce::OutputStream>&,
                                                              const juce::AudioFormatWriterOptions&) override
    {
        return {};
    }
};

#endif
