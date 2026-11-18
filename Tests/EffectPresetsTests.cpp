#include <JuceHeader.h>
#include "../Source/EffectPresets.h"
#include "../Source/BuiltInEffects.h"
#include "TestUtils.h"
#include "EffectPresetsTests.h"

using TestUtils::check;

void runEffectPresetsTests()
{
    const auto folder = juce::File::createTempFile ("presets");
    folder.deleteRecursively();

    // The store: values back exactly, names sorted, replaced by name, removed; kept on disk.
    {
        EffectPresets::Store store (folder);
        const EffectPresets::Values tricky { { "a", 0.1 }, { "b", 1.0e-7 }, { "c", -48.0 }, { "d", 3.9999997615814209 }, { "e", 1.0 / 3.0 }, { "f", 0.0 } };
        const auto saved = store.save ("torqwave:compressor", "Vocal", tricky)
                        && store.save ("torqwave:compressor", "a/b: <drums> & \"bass\"", { { "a", 2.0 } })
                        && store.save ("torqwave:compressor", "Bus", { { "a", 3.0 } })
                        && store.save ("torqwave:limiter", "Vocal", { { "gain", 6.0 } })
                        && store.save ("torqwave:compressor", "Bus", { { "a", 4.0 } })   // replaces
                        && ! store.save ("torqwave:compressor", "  ", { { "a", 5.0 } });

        EffectPresets::Store again (folder);   // read from the files
        const auto names = again.getNames ("torqwave:compressor");
        const auto back = again.load ("torqwave:compressor", "Vocal");
        auto exact = back.has_value() && back->size() == tricky.size();

        for (auto& [id, v] : tricky)
            exact = exact && back->count (id) == 1 && juce::exactlyEqual (back->at (id), v);

        const auto bus = again.load ("torqwave:compressor", "Bus");
        const auto odd = again.load ("torqwave:compressor", "a/b: <drums> & \"bass\"");
        const auto removed = again.remove ("torqwave:compressor", "Bus") && ! again.remove ("torqwave:compressor", "Bus")
                          && ! again.getNames ("torqwave:compressor").contains ("Bus");
        const auto separate = again.getNames ("torqwave:limiter") == juce::StringArray { "Vocal" }
                           && juce::exactlyEqual (again.load ("torqwave:limiter", "Vocal")->at ("gain"), 6.0);

        // A damaged file reads as no presets.
        folder.getChildFile ("phaser.xml").replaceWithText ("<not xml");
        const auto damaged = again.getNames ("torqwave:phaser").isEmpty() && ! again.load ("torqwave:phaser", "x").has_value();

        check (saved && names == juce::StringArray { "a/b: <drums> & \"bass\"", "Bus", "Vocal" } && exact && bus.has_value() && juce::exactlyEqual (bus->at ("a"), 4.0)
                   && odd.has_value() && removed && separate && damaged && folder.getChildFile ("compressor.xml").existsAsFile(),
               "effect presets: values (0.1, 1e-7, a float's double, 1/3) come back exactly from the files; names sorted, any characters, replaced by name, "
               "removed; each effect its own file; a blank name refused; a damaged file reads as none");
    }

    // Presets saved by SpectraBlade (the app's name up to 0.15.10) load, and saving one writes
    // the file as Torqwave's, keeping the others.
    {
        const auto oldFolder = juce::File::createTempFile ("oldpresets");
        oldFolder.getChildFile ("limiter.xml").create();
        oldFolder.getChildFile ("limiter.xml").replaceWithText (
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n<SPECTRABLADE_PRESETS effect=\"spectrablade:limiter\">"
            "<PRESET name=\"Loud\"><VALUE id=\"gain\" value=\"6.0\"/></PRESET></SPECTRABLADE_PRESETS>");
        EffectPresets::Store store (oldFolder);
        const auto loud = store.load ("torqwave:limiter", "Loud");
        const auto read = store.getNames ("torqwave:limiter") == juce::StringArray { "Loud" } && loud.has_value() && loud->count ("gain") == 1 && juce::exactlyEqual (loud->at ("gain"), 6.0);
        const auto saved = store.save ("torqwave:limiter", "Quiet", { { "gain", -6.0 } });
        const auto file = juce::XmlDocument::parse (oldFolder.getChildFile ("limiter.xml"));
        const auto rewritten = file != nullptr && file->hasTagName ("TORQWAVE_PRESETS") && file->getStringAttribute ("effect") == "torqwave:limiter"
                            && EffectPresets::Store (oldFolder).getNames ("torqwave:limiter") == juce::StringArray { "Loud", "Quiet" };
        check (read && saved && rewritten,
               "effect presets: a SpectraBlade preset file (SPECTRABLADE_PRESETS, spectrablade:limiter) loads, and saving rewrites it as Torqwave's "
               "with its presets kept");
        oldFolder.deleteRecursively();
    }

    // A built-in effect's values: another effect set from them holds the same settings, exactly;
    // names it doesn't have are ignored and settings a preset doesn't name are left alone.
    {
        BuiltInEffects::CompressorProcessor a, b;
        a.setSettings ({ -33.5, 7.0, 4.0, 2.5, 250.0, 6.0 });
        b.setPresetValues (a.getPresetValues());
        const auto sa = a.getSettings(), sb = b.getSettings();
        const auto same = juce::exactlyEqual (sa.thresholdDb, sb.thresholdDb) && juce::exactlyEqual (sa.ratio, sb.ratio) && juce::exactlyEqual (sa.kneeDb, sb.kneeDb)
                       && juce::exactlyEqual (sa.attackMs, sb.attackMs) && juce::exactlyEqual (sa.releaseMs, sb.releaseMs) && juce::exactlyEqual (sa.makeupDb, sb.makeupDb);

        BuiltInEffects::CompressorProcessor c, untouched;
        c.setPresetValues ({ { "threshold", -12.0 }, { "lookahead", 20.0 }, { "nonsense", 1.0 } });
        const auto partial = std::abs (c.getSettings().thresholdDb + 12.0) < 1.0e-5 && juce::exactlyEqual (c.getSettings().ratio, untouched.getSettings().ratio)
                          && juce::exactlyEqual (c.getSettings().releaseMs, untouched.getSettings().releaseMs);

        // A dialog's values (its fields named as the parameters) set the live phaser, stages by index.
        BuiltInEffects::PhaserProcessor phaser;
        phaser.setPresetValues ({ { "rate", 1.25 }, { "low", 150.0 }, { "high", 5000.0 }, { "stages", 4.0 }, { "feedback", -0.25 }, { "mix", 60.0 }, { "spread", 30.0 } });
        const auto ps = phaser.getSettings();

        check (same && partial && ps.stages == 10 && std::abs (ps.rateHz - 1.25) < 1.0e-6 && std::abs (ps.feedback + 0.25) < 1.0e-6 && a.getPresetKey() == BuiltInEffects::compressorId,
               "effect presets: a built-in effect's values set another to the same settings exactly; names it lacks are ignored and settings not named "
               "kept; a phaser's stages go by index (4 is 10 stages); the key is the effect's identifier");
    }

    // The EQ: the panel's bands to values and back, and into the live EQ under its parameter IDs.
    {
        ParametricEq::Settings s;

        for (int i = 0; i < 5; ++i)
            s[(size_t) i] = { i % 2 == 0, { (Equalizer::BandType) (i % 5), 100.0 * (i + 1), -3.0 + i, 0.5 + 0.25 * i } };

        const auto values = BuiltInEffects::eqPresetValues (s);
        const auto back = BuiltInEffects::eqSettingsFrom (values, {});
        auto same = true;

        for (size_t i = 0; i < 5; ++i)
            same = same && back[i].on == s[i].on && back[i].band.type == s[i].band.type && juce::exactlyEqual (back[i].band.frequency, s[i].band.frequency)
                && juce::exactlyEqual (back[i].band.gainDb, s[i].band.gainDb) && juce::exactlyEqual (back[i].band.q, s[i].band.q);

        BuiltInEffects::ParametricEqProcessor eq;
        eq.setPresetValues (values);
        const auto bands = eq.getBands();
        auto live = true;

        for (size_t i = 0; i < 5; ++i)
            live = live && bands[i].first == s[i].on && bands[i].second.type == s[i].band.type && std::abs (bands[i].second.frequency - s[i].band.frequency) < 1.0e-3
                && std::abs (bands[i].second.gainDb - s[i].band.gainDb) < 1.0e-5 && std::abs (bands[i].second.q - s[i].band.q) < 1.0e-6;

        const auto partial = BuiltInEffects::eqSettingsFrom ({ { "band2Gain", 9.0 } }, s);

        check (same && live && juce::exactlyEqual (partial[1].band.gainDb, 9.0) && juce::exactlyEqual (partial[0].band.frequency, s[0].band.frequency),
               "effect presets: the EQ window's bands go to values and back exactly, set the live EQ (to its parameters' precision), and a preset "
               "naming one value changes only that");
    }

    folder.deleteRecursively();
}
