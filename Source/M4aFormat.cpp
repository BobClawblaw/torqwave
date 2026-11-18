#include "M4aFormat.h"

#if JUCE_WINDOWS

#include "PacketReader.h"

namespace
{
    const char* const m4aFormatName = "M4A file";
}

M4aFormat::M4aFormat() : AudioFormat (m4aFormatName, ".m4a") {}

juce::AudioFormatReader* M4aFormat::createReaderFor (juce::InputStream* in, bool deleteStreamIfOpeningFails)
{
    std::unique_ptr<juce::InputStream> owned (in);
    auto track = in != nullptr ? Mp4File::readAudioTrack (*in) : std::optional<Mp4File::AudioTrack>();
    juce::String error;
    auto decoder = track.has_value() ? MediaFoundation::Decoder::create (track->codec, track->config, track->sampleRate,
                                                                         track->channels, track->bitsPerSample, error)
                                     : nullptr;

    if (decoder == nullptr || decoder->getChannels() <= 0 || decoder->getSampleRate() <= 0.0)
    {
        if (! deleteStreamIfOpeningFails)
            owned.release();

        return nullptr;
    }

    // AAC's packets overlap: one decoded on its own isn't right until the one before it has been.
    PacketReader::Stream stream { std::move (track->packets), track->skip, track->length, track->codec == "aac" ? 2 : 0 };
    return new PacketReader (owned.release(), m4aFormatName, std::move (stream), track->sampleRate, std::move (decoder));
}

#endif

#if JUCE_LINUX

#include "Ffmpeg.h"
#include "Mp4File.h"

namespace
{
    /** Reads the WAV ffmpeg decoded to, which it keeps until it goes. */
    class DecodedReader final : public juce::AudioFormatReader
    {
    public:
        /** length: the frames the file's edit list says are heard (negative: all ffmpeg gave). */
        DecodedReader (std::unique_ptr<juce::TemporaryFile> t, std::unique_ptr<juce::AudioFormatReader> r, juce::int64 length)
            : AudioFormatReader (nullptr, "M4A file"), temp (std::move (t)), wav (std::move (r))
        {
            sampleRate = wav->sampleRate;
            bitsPerSample = 32;
            usesFloatingPointData = true;
            lengthInSamples = length >= 0 ? juce::jmin (length, wav->lengthInSamples) : wav->lengthInSamples;
            numChannels = wav->numChannels;
            metadataValues = wav->metadataValues;
        }

        bool readSamples (int* const* destChannels, int numDestChannels, int startOffsetInDestBuffer,
                          juce::int64 startSampleInFile, int numSamples) override
        {
            return wav->readSamples (destChannels, numDestChannels, startOffsetInDestBuffer, startSampleInFile, numSamples);
        }

    private:
        std::unique_ptr<juce::TemporaryFile> temp;   // declared first: outlives the reader of it
        std::unique_ptr<juce::AudioFormatReader> wav;
    };
}

M4aFormat::M4aFormat() : AudioFormat ("M4A file", ".m4a") {}

juce::AudioFormatReader* M4aFormat::createReaderFor (juce::InputStream* in, bool deleteStreamIfOpeningFails)
{
    std::unique_ptr<juce::InputStream> owned (in);
    const auto fail = [&]() -> juce::AudioFormatReader*
    {
        if (! deleteStreamIfOpeningFails)
            owned.release();

        return nullptr;
    };

    const auto ffmpeg = Ffmpeg::find();

    if (in == nullptr || ffmpeg == juce::File())
        return fail();

    // The file itself when the stream is one; otherwise the stream's bytes, copied to a file for ffmpeg.
    // Only an MPEG-4 file is tried: its second box is "ftyp".
    const auto start = in->getPosition();
    char header[8] = {};

    if (in->read (header, 8) != 8 || std::memcmp (header + 4, "ftyp", 4) != 0)
        return fail();

    in->setPosition (start);
    juce::File source;
    std::unique_ptr<juce::TemporaryFile> copy;

    if (auto* fileStream = dynamic_cast<juce::FileInputStream*> (in))
    {
        source = fileStream->getFile();
    }
    else
    {
        copy = std::make_unique<juce::TemporaryFile> (".m4a");
        juce::FileOutputStream out (copy->getFile());

        if (out.failedToOpen() || out.writeFromInputStream (*in, -1) <= 0 || ! out.getStatus().wasOk())
            return fail();

        out.flush();
        source = copy->getFile();
        in->setPosition (start);
    }

    auto wav = std::make_unique<juce::TemporaryFile> (".wav");

    if (Ffmpeg::decode (ffmpeg, source, wav->getFile()).isNotEmpty())
        return fail();

    std::unique_ptr<juce::AudioFormatReader> reader (juce::WavAudioFormat().createReaderFor (new juce::FileInputStream (wav->getFile()), true));

    if (reader == nullptr)
        return fail();

    // ffmpeg leaves out the priming the edit list skips, but not the padding after the end it gives:
    // the length is the edit list's, as the Mac's and Windows' readers take it.
    juce::int64 length = -1;

    if (juce::FileInputStream track (source); track.openedOk())
        if (const auto t = Mp4File::readAudioTrack (track); t.has_value() && t->length > 0
                                                            && juce::exactlyEqual (t->sampleRate, reader->sampleRate))
            length = t->length;

    return new DecodedReader (std::move (wav), std::move (reader), length);   // `in` is done with, and goes
}

#endif
