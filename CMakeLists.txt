cmake_minimum_required(VERSION 3.22)

project(Torqwave VERSION 0.20.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "Minimum macOS version")

# On Windows the C and C++ runtimes are linked in, so the program runs without the Visual C++
# Redistributable installed.
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# Use a local JUCE checkout with -DTORQWAVE_JUCE_DIR=/path/to/JUCE, otherwise fetch the pinned release.
set(TORQWAVE_JUCE_DIR "" CACHE PATH "Local JUCE checkout (optional)")

if(TORQWAVE_JUCE_DIR)
    add_subdirectory("${TORQWAVE_JUCE_DIR}" JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.4
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
endif()

# LAME's MP3 encoder, built from its pinned source release. Use a local copy of that release with
# -DTORQWAVE_LAME_DIR=/path/to/lame-4.0.
set(TORQWAVE_LAME_DIR "" CACHE PATH "Local LAME 4.0 source (optional)")

if(TORQWAVE_LAME_DIR)
    set(torqwave_lame_src "${TORQWAVE_LAME_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(lame
        URL      https://downloads.sourceforge.net/project/lame/lame/4.0/lame-4.0.tar.gz
        URL_HASH SHA256=3df5124d5ad3a98312ffd7ba6a9b36230e4f8a3e66d3ce0f425e336c32d216eb
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
    FetchContent_MakeAvailable(lame)
    set(torqwave_lame_src "${lame_SOURCE_DIR}")
endif()

set(torqwave_lame_files VbrTag bitstream encoder fft gain_analysis id3tag lame newmdct presets psymodel
    quantize quantize_pvt reservoir set_get tables takehiro util vbrquantize version)
list(TRANSFORM torqwave_lame_files PREPEND "${torqwave_lame_src}/libmp3lame/")
list(TRANSFORM torqwave_lame_files APPEND ".c")
add_library(TorqwaveLame STATIC ${torqwave_lame_files})
target_include_directories(TorqwaveLame PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/lame" "${torqwave_lame_src}/libmp3lame")
target_include_directories(TorqwaveLame SYSTEM PUBLIC "${torqwave_lame_src}/include")
target_compile_definitions(TorqwaveLame PRIVATE HAVE_CONFIG_H=1)
# LAME's own code, compiled as it is: its warnings are not ours to fix.
if(MSVC)
    target_compile_options(TorqwaveLame PRIVATE /w)
    target_compile_definitions(TorqwaveLame PRIVATE _CRT_SECURE_NO_WARNINGS)
else()
    target_compile_options(TorqwaveLame PRIVATE -w -Wno-implicit-function-declaration)
endif()
set_target_properties(TorqwaveLame PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON)

# Signalsmith Stretch (time stretching and pitch shifting) and the Signalsmith Linear FFTs it uses,
# both MIT-licensed headers, each fetched at its pinned release commit (Stretch 1.4.0, Linear 0.6.4).
# Use local copies of those releases with -DTORQWAVE_STRETCH_DIR and -DTORQWAVE_LINEAR_DIR.
set(TORQWAVE_STRETCH_DIR "" CACHE PATH "Local Signalsmith Stretch 1.4.0 (optional)")
set(TORQWAVE_LINEAR_DIR "" CACHE PATH "Local Signalsmith Linear 0.6.4 (optional)")

if(TORQWAVE_STRETCH_DIR)
    set(torqwave_stretch_src "${TORQWAVE_STRETCH_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(signalsmith_stretch
        GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
        GIT_TAG        a670068d9aeb64913331d5cc29337b19a457a7df
        SOURCE_SUBDIR  not-built)
    FetchContent_MakeAvailable(signalsmith_stretch)
    set(torqwave_stretch_src "${signalsmith_stretch_SOURCE_DIR}")
endif()

if(TORQWAVE_LINEAR_DIR)
    set(torqwave_linear_src "${TORQWAVE_LINEAR_DIR}")
else()
    include(FetchContent)
    FetchContent_Declare(signalsmith_linear
        GIT_REPOSITORY https://github.com/Signalsmith-Audio/linear.git
        GIT_TAG        de55e6a50ffcf6f8f43f649692d94691c7025151
        SOURCE_SUBDIR  not-built)
    FetchContent_MakeAvailable(signalsmith_linear)
    set(torqwave_linear_src "${signalsmith_linear_SOURCE_DIR}")
endif()

# The headers include "signalsmith-linear/stft.h": a folder of that name pointing at Linear.
file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/signalsmith-include")
# Windows only makes symbolic links with Developer Mode or as administrator, so it gets a copy.
if(WIN32)
    file(COPY "${torqwave_linear_src}/" DESTINATION "${CMAKE_BINARY_DIR}/signalsmith-include/signalsmith-linear")
else()
    file(CREATE_LINK "${torqwave_linear_src}" "${CMAKE_BINARY_DIR}/signalsmith-include/signalsmith-linear" SYMBOLIC)
endif()
add_library(TorqwaveStretch INTERFACE)
target_include_directories(TorqwaveStretch SYSTEM INTERFACE "${torqwave_stretch_src}" "${CMAKE_BINARY_DIR}/signalsmith-include")

# SQLite for the Sound Library: macOS and Linux have it as a system library; on Windows the
# amalgamation of a pinned release is built in. Use a local copy of it with -DTORQWAVE_SQLITE_DIR.
add_library(TorqwaveSqlite INTERFACE)

if(WIN32)
    set(TORQWAVE_SQLITE_DIR "" CACHE PATH "Local SQLite 3.50.4 amalgamation (optional)")

    if(TORQWAVE_SQLITE_DIR)
        set(torqwave_sqlite_src "${TORQWAVE_SQLITE_DIR}")
    else()
        include(FetchContent)
        FetchContent_Declare(sqlite
            URL      https://sqlite.org/2025/sqlite-amalgamation-3500400.zip
            URL_HASH SHA256=1d3049dd0f830a025a53105fc79fd2ab9431aea99e137809d064d8ee8356b032
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_MakeAvailable(sqlite)
        set(torqwave_sqlite_src "${sqlite_SOURCE_DIR}")
    endif()

    add_library(TorqwaveSqliteLib STATIC "${torqwave_sqlite_src}/sqlite3.c")
    target_include_directories(TorqwaveSqliteLib SYSTEM PUBLIC "${torqwave_sqlite_src}")
    # FTS5 for the Sound Library's word search, as the system SQLite on macOS has it.
    target_compile_definitions(TorqwaveSqliteLib PRIVATE SQLITE_THREADSAFE=1 SQLITE_ENABLE_FTS5 SQLITE_OMIT_LOAD_EXTENSION)
    if(MSVC)
        target_compile_options(TorqwaveSqliteLib PRIVATE /w)
    endif()
    target_link_libraries(TorqwaveSqlite INTERFACE TorqwaveSqliteLib)
else()
    target_link_libraries(TorqwaveSqlite INTERFACE sqlite3)
endif()

juce_add_gui_app(Torqwave
    PRODUCT_NAME                 "Torqwave"
    COMPANY_NAME                 "Torqwave"
    COMPANY_COPYRIGHT            "Copyright (C) 2026 BobClawblaw. Licensed under the GNU AGPLv3."
    BUNDLE_ID                    org.torqwave.editor
    ICON_BIG                     "${CMAKE_CURRENT_SOURCE_DIR}/Art/torqwave-icon.png"
    ICON_SMALL                   "${CMAKE_CURRENT_SOURCE_DIR}/Art/torqwave-icon-small.png"
    PLUGINHOST_AU                TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT   "Torqwave records from your audio interface."
    HARDENED_RUNTIME_ENABLED     TRUE
    # Third-party plug-ins are signed by other teams, so library validation must be off to load them.
    HARDENED_RUNTIME_OPTIONS     com.apple.security.cs.disable-library-validation
                                 com.apple.security.device.audio-input
    # Finder's Open With and double-clicks: the audio types AudioFileIO::openedFromFinder names,
    # sessions, sequences and kits.
    DOCUMENT_EXTENSIONS          wav aif aiff aifc flac ogg mp3 m4a caf tqsession tqproject sbsession sbproject tqseq tqkit
    NEEDS_WEB_BROWSER            FALSE
    # The only requests the app makes are to the AI server named in Settings > AI, most often a
    # model server on the user's own network over plain HTTP, which App Transport Security
    # would otherwise refuse unless it is a .local name.
    PLIST_TO_MERGE               "<plist><dict><key>NSAppTransportSecurity</key><dict><key>NSAllowsArbitraryLoads</key><true/></dict><key>NSLocalNetworkUsageDescription</key><string>Ask Torq connects to the language model server you name in Settings, which may be on your local network.</string></dict></plist>"
    # Linux: HTTP and HTTPS to an AI server through libcurl (JUCE_LOAD_CURL_SYMBOLS_LAZILY below).
    NEEDS_CURL                   TRUE)

juce_generate_juce_header(Torqwave)

target_sources(Torqwave PRIVATE
    Source/Main.cpp
    Source/AudioDocument.cpp
    Source/Markers.cpp
    Source/Metronome.cpp
    Source/AudioEdits.cpp
    Source/Batch.cpp
    Source/Sequences.cpp
    Source/EffectPreview.cpp
    Source/EffectDialog.cpp
    Source/Surround.cpp
    Source/Video.cpp
    Source/FramePlayer.cpp
    Source/Project.cpp
    Source/PictureMap.cpp
    Source/AudioFileIO.cpp
    Source/GaplessMp3.cpp
    Source/OfflineRender.cpp
    Source/AudioEngine.cpp
    Source/SampleRateConverter.cpp
    Source/TimeStretch.cpp
    Source/ExtremeStretch.cpp
    Source/PhaseVocoder.cpp
    Source/TrackTemplate.cpp
    Source/SequenceFile.cpp
    Source/SequenceKit.cpp
    Source/SequenceMidi.cpp
    Source/SequenceLibrary.cpp
    Source/SequenceLibraryView.cpp
    Source/SequencePropertiesDialog.cpp
    Source/SequencePreview.cpp
    Source/IdeaBoard.cpp
    Source/IdeaBoardView.cpp
    Source/NoteTools.cpp
    Source/Sequencer.cpp
    Source/SequencerView.cpp
    Source/UndoHistory.cpp
    Source/Spectrogram.cpp
    Source/SpectralEdit.cpp
    Source/Loudness.cpp
    Source/Delivery.cpp
    Source/SoundLibrary.cpp
    Source/SoundLibraryView.cpp
    Source/SoundLibraryPreview.cpp
    Source/SoundCategories.cpp
    Source/SoundMetadata.cpp
    Source/Equalizer.cpp
    Source/GraphicEq.cpp
    Source/Dynamics.cpp
    Source/Multiband.cpp
    Source/Modulation.cpp
    Source/Reverb.cpp
    Source/Convolver.cpp
    Source/BuiltInEffects.cpp
    Source/SwarmGuitar.cpp
    Source/SwarmGuitarPresets.cpp
    Source/DrumMaps.cpp
    Source/SwarmGuitarEditor.cpp
    Source/DrumMachine.cpp
    Source/DrumMachineEditor.cpp
    Source/EffectPresets.cpp
    Source/FactoryPresets.cpp
    Source/ParametricEq.cpp
    Source/EqDrawing.cpp
    Source/DynamicEq.cpp
    Source/Oversampling.cpp
    Source/Exciter.cpp
    Source/Maximizer.cpp
    Source/TransientShaper.cpp
    Source/MatchEq.cpp
    Source/TonalBalance.cpp
    Source/Statistics.cpp
    Source/TimeFormat.cpp
    Source/Dither.cpp
    Source/DialogueIsolation.cpp
    Source/AdaptiveDenoise.cpp
    Source/Plosive.cpp
    Source/Breath.cpp
    Source/Spill.cpp
    Source/WowFlutter.cpp
    Source/SpectralRecovery.cpp
    Source/Rebalance.cpp
    Source/Ambience.cpp
    Source/DialogueContour.cpp
    Source/Azimuth.cpp
    Source/CodecPreview.cpp
    Source/NoiseReduction.cpp
    Source/Dereverb.cpp
    Source/CentreChannel.cpp
    Source/PhaseCorrection.cpp
    Source/PitchCorrection.cpp
    Source/Leveller.cpp
    Source/Scrubber.cpp
    Source/ClickRepair.cpp
    Source/Session.cpp
    Source/SessionMixer.cpp
    Source/TrackEffects.cpp
    Source/RackCopy.cpp
    Source/Recorder.cpp
    Source/SessionCollect.cpp
    Source/SessionNotes.cpp
    Source/LoadReadout.cpp
    Source/SessionRecovery.cpp
    Source/KeyShortcuts.cpp
    Source/AuditionPreview.cpp
    Source/WindowPlaces.cpp
    Source/Dock.cpp
    Source/EditorBar.cpp
    Source/TrackTools.cpp
    Source/Navigation.cpp
    Source/Restoration.cpp
    Source/Generator.cpp
    Source/Instrument.cpp
    Source/LossyFiles.cpp
    Source/AacWriter.cpp
    Source/Mp4File.cpp
    Source/MediaFoundation.cpp
    Source/M4aFormat.cpp
    Source/PacketReader.cpp
    Source/Ffmpeg.cpp
    Source/FileInfo.cpp
    Source/OpenFiles.cpp
    Source/SettingsPanel.cpp
    Source/LevelsPanel.cpp
    Source/Analysis.cpp
    Source/Theme.cpp
    Source/SoundControls.cpp
    Source/WaveTables.cpp
    Source/WaveSynth.cpp
    Source/WaveSynthPatches.cpp
    Source/WaveSynthPatchBank.cpp
    Source/WaveSynthEditor.cpp
    Source/FileFades.cpp
    Source/FileChecks.cpp
    Source/LoadingScreen.cpp
    Source/TransportBar.cpp
    Source/SessionOverview.cpp
    Source/SessionRecording.cpp
    Source/Bounce.cpp
    Source/ClipList.cpp
    Source/MarkerTable.cpp
    Source/ClipProperties.cpp
    Source/TrackInspector.cpp
    Source/PluginScanner.cpp
    Source/PluginCatalogue.cpp
    Source/MainComponent.cpp
    Source/PitchEditor.cpp
    Source/NoteEditor.cpp
    Source/MidiInput.cpp
    Source/MidiOut.cpp
    Source/ControlSurface.cpp
    Source/TempoFollow.cpp
    Source/ClipEffects.cpp
    Source/ClipEffectsPanel.cpp
    Source/CrossfadePanel.cpp
    Source/SoundDesign.cpp
    Source/Picture.cpp
    Source/FitTempoPanel.cpp
    Source/MixScenes.cpp
    Source/HardwareInsert.cpp
    Source/WaveformView.cpp
    Source/MixerView.cpp
    Source/SurroundPad.cpp
    Source/MultitrackView.cpp
    Source/RackView.cpp
    Source/Guide.cpp
    Source/AiClient.cpp
    Source/AiKeychain.cpp
    Source/AiWorkspace.cpp
    Source/AiSession.cpp
    Source/AiLibrary.cpp
    Source/AiSequence.cpp
    Source/InstrumentPresets.cpp
    Source/AiCatalogue.cpp
    Source/AiAssistant.cpp
    Source/GuideView.cpp
    Source/Home.cpp)

target_compile_definitions(Torqwave PRIVATE
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_VST=0
    JUCE_PLUGINHOST_LV2=$<IF:$<PLATFORM_ID:Windows>,0,1>
    JUCE_PLUGINHOST_ARA=0
    # ASIO devices on Windows, from the ASIO SDK headers JUCE bundles. Steinberg licenses the SDK
    # under GPLv3 as one of its two licences (since SDK 2.3.4), which combines with the AGPLv3.
    JUCE_ASIO=$<IF:$<PLATFORM_ID:Windows>,1,0>
    JUCE_USE_FLAC=1
    JUCE_USE_OGGVORBIS=1
    JUCE_USE_MP3AUDIOFORMAT=1
    JUCE_WEB_BROWSER=0
    # Linux: libcurl (NEEDS_CURL) is loaded when an AI server is first asked, so the app still
    # starts on a system without it. Elsewhere the system's own networking is used.
    JUCE_LOAD_CURL_SYMBOLS_LAZILY=1
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Torqwave,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Torqwave,JUCE_VERSION>")

# What this build is (a release, or a test build with its time and commit), written at every
# build into TorqwaveBuildStamp.h for Source/BuildStamp.h.
set(TORQWAVE_STAMP_DIR "${CMAKE_BINARY_DIR}/stamp")
add_custom_target(TorqwaveBuildStamp ALL
    COMMAND ${CMAKE_COMMAND} -DOUT=${TORQWAVE_STAMP_DIR}/TorqwaveBuildStamp.h -DSRC=${CMAKE_SOURCE_DIR}
            -DVERSION=${PROJECT_VERSION} -P ${CMAKE_SOURCE_DIR}/Scripts/build-stamp.cmake
    BYPRODUCTS ${TORQWAVE_STAMP_DIR}/TorqwaveBuildStamp.h
    COMMENT "Stamping the build")
add_dependencies(Torqwave TorqwaveBuildStamp)
target_include_directories(Torqwave PRIVATE ${TORQWAVE_STAMP_DIR})

# The logo on the loading screen, the UCS category list, and the plug-in descriptions Torqwave
# ships for Ask Torq (PluginCatalogue).
juce_add_binary_data(TorqwaveArt SOURCES Art/torqwave-banner.png ThirdParty/UCS/ucs_v8_2_1.tsv Data/plugin-knowledge.json)

# The user guide, read inside the app (Help > User Guide): its pages, the shortcut list and the
# screenshots they show. Source/Guide.cpp finds each by its file name.
file(GLOB torqwave_guide_pages CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/docs/guide/*.md)
file(GLOB torqwave_guide_images CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/docs/screenshots/*.png)
juce_add_binary_data(TorqwaveGuide NAMESPACE GuideData HEADER_NAME GuideData.h
    SOURCES ${torqwave_guide_pages} docs/SHORTCUTS.md ${torqwave_guide_images})

# The factory presets (Data/Presets, written by Scripts/make_presets.py), zipped into the app and
# unpacked on first use (SequenceLibrary::unpackFactoryPresets). The zip is made from a sorted
# list with fixed times, so the same presets make the same zip.
file(GLOB_RECURSE torqwave_presets CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/Data/Presets/*.tqseq ${CMAKE_SOURCE_DIR}/Data/Presets/*.tqkit)
list(SORT torqwave_presets)
set(torqwave_preset_names)
foreach(preset IN LISTS torqwave_presets)
    file(RELATIVE_PATH name ${CMAKE_SOURCE_DIR}/Data/Presets ${preset})
    list(APPEND torqwave_preset_names ${name})
endforeach()
set(torqwave_presets_zip ${CMAKE_BINARY_DIR}/factory-presets.zip)
add_custom_command(OUTPUT ${torqwave_presets_zip}
    COMMAND ${CMAKE_COMMAND} -E rm -f ${torqwave_presets_zip}
    COMMAND ${CMAKE_COMMAND} -E tar cf ${torqwave_presets_zip} --format=zip --mtime=@1767225600 -- ${torqwave_preset_names}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/Data/Presets
    DEPENDS ${torqwave_presets}
    COMMENT "Zipping the factory presets"
    VERBATIM)
juce_add_binary_data(TorqwavePresets NAMESPACE FactoryPresetData HEADER_NAME FactoryPresetData.h SOURCES ${torqwave_presets_zip})

target_link_libraries(Torqwave PRIVATE
    TorqwaveArt
    TorqwavePresets
    TorqwaveGuide
    TorqwaveLame
    TorqwaveStretch
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_video
    TorqwaveSqlite
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Windows' own AAC encoder and AAC and Apple Lossless decoders (Media Foundation), for .m4a files.
if(WIN32)
    target_link_libraries(Torqwave PRIVATE mfplat mfuuid wmcodecdspuuid delayimp)
    target_link_options(Torqwave PRIVATE /DELAYLOAD:mfplat.dll)   # see MediaFoundation.cpp, present()
endif()

# Integration checks: plug-in scanning, device routing and rack restore on this machine.
juce_add_console_app(TorqwaveTests PRODUCT_NAME "TorqwaveTests" PLUGINHOST_AU TRUE NEEDS_CURL TRUE)
juce_generate_juce_header(TorqwaveTests)
target_sources(TorqwaveTests PRIVATE
    Tests/EngineTests.cpp
    Tests/DocumentTests.cpp
    Tests/RecorderTests.cpp
    Tests/MarkerTests.cpp
    Tests/ConverterTests.cpp
    Tests/SpectralTests.cpp
    Tests/CommandAuditTests.cpp
    Tests/PlayheadTests.cpp
    Tests/LoudnessTests.cpp
    Tests/SoundLibraryTests.cpp
    Tests/EqTests.cpp
    Tests/GraphicEqTests.cpp
    Tests/DynamicsTests.cpp
    Tests/MultibandTests.cpp
    Tests/DynamicEqTests.cpp
    Tests/ExciterTests.cpp
    Tests/MaximizerTests.cpp
    Tests/TransientShaperTests.cpp
    Tests/MatchEqTests.cpp
    Tests/TonalBalanceTests.cpp
    Tests/StatisticsTests.cpp
    Tests/ExtremeStretchTests.cpp
    Tests/TimeFormatTests.cpp
    Tests/DitherTests.cpp
    Tests/DialogueIsolationTests.cpp
    Tests/AdaptiveDenoiseTests.cpp
    Tests/PlosiveTests.cpp
    Tests/BreathTests.cpp
    Tests/SpillTests.cpp
    Tests/WowFlutterTests.cpp
    Tests/SpectralRecoveryTests.cpp
    Tests/RebalanceTests.cpp
    Tests/AmbienceTests.cpp
    Tests/DialogueContourTests.cpp
    Tests/AzimuthTests.cpp
    Tests/CodecPreviewTests.cpp
    Tests/ModulationTests.cpp
    Tests/ReverbTests.cpp
    Tests/ConvolverTests.cpp
    Tests/EffectPresetsTests.cpp
    Tests/BuiltInEffectsTests.cpp
    Tests/AuditionTests.cpp
    Tests/NoiseTests.cpp
    Tests/DereverbTests.cpp
    Tests/CentreChannelTests.cpp
    Tests/PhaseCorrectionTests.cpp
    Tests/PitchCorrectionTests.cpp
    Tests/LevellerTests.cpp
    Tests/ScrubberTests.cpp
    Tests/ClickTests.cpp
    Tests/SessionTests.cpp
    Tests/TrackEffectsTests.cpp
    Tests/BusTests.cpp
    Tests/SidechainTests.cpp
    Tests/BatchTests.cpp
    Tests/SequencesTests.cpp
    Tests/EffectPreviewTests.cpp
    Tests/SurroundTests.cpp
    Tests/SurroundSessionTests.cpp
    Tests/SurroundEffectsTests.cpp
    Tests/SessionScrubTests.cpp
    Tests/VideoTests.cpp
    Tests/ProjectTests.cpp
    Tests/AutomationTests.cpp
    Tests/MultitrackViewTests.cpp
    Tests/MixDownTests.cpp
    Tests/EffectAutomationTests.cpp
    Tests/TrackRecordingTests.cpp
    Tests/CompTests.cpp
    Tests/ArrangerTests.cpp
    Tests/AutomationModeTests.cpp
    Tests/DrumGridTests.cpp
    Tests/MidiOutTests.cpp
    Tests/ControlSurfaceTests.cpp
    Tests/TempoFollowTests.cpp
    Tests/StressTests.cpp
    Tests/SequenceTests.cpp
    Tests/PresetTests.cpp
    Tests/LibraryFacetTests.cpp
    Tests/PreviewOnTests.cpp
    Tests/CrossfadeTests.cpp
    Tests/SoundDesignTests.cpp
    Tests/PictureTests.cpp
    Tests/SmallPiecesTests.cpp
    Tests/MetronomeTests.cpp
    Tests/MixerTests.cpp
    Tests/BounceTests.cpp
    Tests/PluginTests.cpp
    Tests/DockTests.cpp
    Tests/OpenFilesTests.cpp
    Tests/SettingsPanelTests.cpp
    Tests/PanelTests.cpp
    Tests/ThemeTests.cpp
    Tests/EditorBarTests.cpp
    Tests/NavigationTests.cpp
    Tests/RestorationTests.cpp
    Tests/GeneratorTests.cpp
    Tests/InstrumentTests.cpp
    Tests/SwarmGuitarTests.cpp
    Tests/SoundDemos.cpp
    Tests/DrumMachineTests.cpp
    Tests/SoundControlsTests.cpp
    Tests/WaveSynthTests.cpp
    Tests/SynthClickTests.cpp
    Tests/InstrumentTrackTests.cpp
    Tests/NoteEditorTests.cpp
    Tests/MidiInputTests.cpp
    Tests/LossyFilesTests.cpp
    Tests/FfmpegTests.cpp
    Tests/FileInfoTests.cpp
    Tests/SpectralShapeTests.cpp
    Tests/DuckingTests.cpp
    Tests/FadeHandleTests.cpp
    Tests/GuideTests.cpp
    Tests/HomeTests.cpp
    Tests/AiTests.cpp
    Tests/AiEval.cpp
    Source/AiClient.cpp
    Source/AiKeychain.cpp
    Source/AiWorkspace.cpp
    Source/AiSession.cpp
    Source/AiLibrary.cpp
    Source/AiSequence.cpp
    Source/InstrumentPresets.cpp
    Source/AiCatalogue.cpp
    Source/AiAssistant.cpp
    Source/AudioDocument.cpp
    Source/Markers.cpp
    Source/Metronome.cpp
    Source/AudioEdits.cpp
    Source/Batch.cpp
    Source/Sequences.cpp
    Source/EffectPreview.cpp
    Source/EffectDialog.cpp
    Source/Surround.cpp
    Source/Video.cpp
    Source/FramePlayer.cpp
    Source/Project.cpp
    Source/PictureMap.cpp
    Source/AudioFileIO.cpp
    Source/GaplessMp3.cpp
    Source/OfflineRender.cpp
    Source/AudioEngine.cpp
    Source/SampleRateConverter.cpp
    Source/TimeStretch.cpp
    Source/ExtremeStretch.cpp
    Source/PhaseVocoder.cpp
    Source/TrackTemplate.cpp
    Source/SequenceFile.cpp
    Source/SequenceKit.cpp
    Source/SequenceMidi.cpp
    Source/SequenceLibrary.cpp
    Source/SequenceLibraryView.cpp
    Source/SequencePropertiesDialog.cpp
    Source/SequencePreview.cpp
    Source/IdeaBoard.cpp
    Source/IdeaBoardView.cpp
    Source/NoteTools.cpp
    Source/Sequencer.cpp
    Source/SequencerView.cpp
    Source/UndoHistory.cpp
    Source/Spectrogram.cpp
    Source/SpectralEdit.cpp
    Source/Loudness.cpp
    Source/Delivery.cpp
    Source/SoundLibrary.cpp
    Source/SoundLibraryView.cpp
    Source/SoundLibraryPreview.cpp
    Source/SoundCategories.cpp
    Source/SoundMetadata.cpp
    Source/Equalizer.cpp
    Source/GraphicEq.cpp
    Source/Dynamics.cpp
    Source/Multiband.cpp
    Source/Modulation.cpp
    Source/Reverb.cpp
    Source/Convolver.cpp
    Source/BuiltInEffects.cpp
    Source/SwarmGuitar.cpp
    Source/SwarmGuitarPresets.cpp
    Source/DrumMaps.cpp
    Source/SwarmGuitarEditor.cpp
    Source/DrumMachine.cpp
    Source/DrumMachineEditor.cpp
    Source/EffectPresets.cpp
    Source/FactoryPresets.cpp
    Source/ParametricEq.cpp
    Source/EqDrawing.cpp
    Source/DynamicEq.cpp
    Source/Oversampling.cpp
    Source/Exciter.cpp
    Source/Maximizer.cpp
    Source/TransientShaper.cpp
    Source/MatchEq.cpp
    Source/TonalBalance.cpp
    Source/Statistics.cpp
    Source/TimeFormat.cpp
    Source/Dither.cpp
    Source/DialogueIsolation.cpp
    Source/AdaptiveDenoise.cpp
    Source/Plosive.cpp
    Source/Breath.cpp
    Source/Spill.cpp
    Source/WowFlutter.cpp
    Source/SpectralRecovery.cpp
    Source/Rebalance.cpp
    Source/Ambience.cpp
    Source/DialogueContour.cpp
    Source/Azimuth.cpp
    Source/CodecPreview.cpp
    Source/NoiseReduction.cpp
    Source/Dereverb.cpp
    Source/CentreChannel.cpp
    Source/PhaseCorrection.cpp
    Source/PitchCorrection.cpp
    Source/PitchEditor.cpp
    Source/NoteEditor.cpp
    Source/MidiInput.cpp
    Source/MidiOut.cpp
    Source/ControlSurface.cpp
    Source/TempoFollow.cpp
    Source/ClipEffects.cpp
    Source/ClipEffectsPanel.cpp
    Source/CrossfadePanel.cpp
    Source/SoundDesign.cpp
    Source/Picture.cpp
    Source/FitTempoPanel.cpp
    Source/MixScenes.cpp
    Source/HardwareInsert.cpp
    Source/Leveller.cpp
    Source/Scrubber.cpp
    Source/ClickRepair.cpp
    Source/Session.cpp
    Source/SessionMixer.cpp
    Source/TrackEffects.cpp
    Source/RackCopy.cpp
    Source/Recorder.cpp
    Source/SessionCollect.cpp
    Source/SessionNotes.cpp
    Source/LoadReadout.cpp
    Source/SessionRecovery.cpp
    Source/KeyShortcuts.cpp
    Source/AuditionPreview.cpp
    Source/WindowPlaces.cpp
    Source/Dock.cpp
    Source/EditorBar.cpp
    Source/TrackTools.cpp
    Source/Navigation.cpp
    Source/Restoration.cpp
    Source/Generator.cpp
    Source/Instrument.cpp
    Source/LossyFiles.cpp
    Source/AacWriter.cpp
    Source/Mp4File.cpp
    Source/MediaFoundation.cpp
    Source/M4aFormat.cpp
    Source/PacketReader.cpp
    Source/Ffmpeg.cpp
    Source/FileInfo.cpp
    Source/OpenFiles.cpp
    Source/SettingsPanel.cpp
    Source/LevelsPanel.cpp
    Source/Analysis.cpp
    Source/Theme.cpp
    Source/SoundControls.cpp
    Source/WaveTables.cpp
    Source/WaveSynth.cpp
    Source/WaveSynthPatches.cpp
    Source/WaveSynthPatchBank.cpp
    Source/WaveSynthEditor.cpp
    Source/FileFades.cpp
    Source/FileChecks.cpp
    Source/LoadingScreen.cpp
    Source/TransportBar.cpp
    Source/WaveformView.cpp
    Source/SessionOverview.cpp
    Source/SessionRecording.cpp
    Source/Bounce.cpp
    Source/ClipList.cpp
    Source/MarkerTable.cpp
    Source/ClipProperties.cpp
    Source/TrackInspector.cpp
    Source/PluginScanner.cpp
    Source/PluginCatalogue.cpp
    Source/MixerView.cpp
    Source/SurroundPad.cpp
    Source/RackView.cpp
    Source/MultitrackView.cpp
    Source/Guide.cpp
    Source/GuideView.cpp
    Source/Home.cpp)
# Plug-in bundles that misbehave on purpose, for the scan tests (see Tests/TestBundle.cpp).
# On Windows each is a .vst3 DLL; the generator expression keeps multi-config builds from adding a
# configuration folder. IntelOnlyBundle there is a bundle folder holding only 32-bit (x86-win) code.
# On the Mac, ArmOnlyBundle is the Intel half's counterpart of IntelOnlyBundle.
set(torqwave_test_bundles IntelOnlyBundle HangingBundle CrashingBundle)
if(APPLE)
    list(APPEND torqwave_test_bundles ArmOnlyBundle)
endif()
foreach(bundle ${torqwave_test_bundles})
    add_library(${bundle} MODULE Tests/TestBundle.cpp)

    if(APPLE)
        set_target_properties(${bundle} PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION vst3
                              LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TestBundles")
    elseif(UNIX)
        # On Linux a VST3 is a folder with the module at Contents/<architecture>-linux/<name>.so.
        set_target_properties(${bundle} PROPERTIES PREFIX "" SUFFIX ".so"
                              LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/TestBundles/${bundle}.vst3/Contents/${CMAKE_SYSTEM_PROCESSOR}-linux")
    else()
        set_target_properties(${bundle} PROPERTIES PREFIX "" SUFFIX ".vst3"
                              LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/TestBundles>")
    endif()

    add_dependencies(TorqwaveTests ${bundle})
endforeach()

if(APPLE)
    set_target_properties(IntelOnlyBundle PROPERTIES OSX_ARCHITECTURES x86_64)
    set_target_properties(ArmOnlyBundle PROPERTIES OSX_ARCHITECTURES arm64)
elseif(WIN32)
    set_target_properties(IntelOnlyBundle PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "$<1:${CMAKE_BINARY_DIR}/TestBundles/IntelOnlyBundle.vst3/Contents/x86-win>")
endif()

# An LV2 effect for the LV2 hosting tests (LV2 is hosted on the Mac and Linux): a bundle folder holding its Turtle files and module.
if(NOT WIN32)
    if(TORQWAVE_JUCE_DIR)
        set(torqwave_lv2_sdk "${TORQWAVE_JUCE_DIR}/modules/juce_audio_processors_headless/format_types/LV2_SDK/lv2")
    else()
        set(torqwave_lv2_sdk "${juce_SOURCE_DIR}/modules/juce_audio_processors_headless/format_types/LV2_SDK/lv2")
    endif()

    set(test_lv2_bundle "${CMAKE_BINARY_DIR}/TestBundles/TorqwaveTestGain.lv2")
    add_library(TorqwaveTestGain MODULE Tests/TestLv2Gain/TestLv2Gain.c)
    target_include_directories(TorqwaveTestGain PRIVATE "${torqwave_lv2_sdk}")
    set_target_properties(TorqwaveTestGain PROPERTIES PREFIX "" C_VISIBILITY_PRESET hidden
                          LIBRARY_OUTPUT_DIRECTORY "$<1:${test_lv2_bundle}>")

    # A Mac module is a .dylib, as LV2 bundles there have it; on Linux a .so.
    if(APPLE)
        set(test_gain_suffix ".dylib")
    else()
        set(test_gain_suffix "${CMAKE_SHARED_MODULE_SUFFIX}")
    endif()

    set_target_properties(TorqwaveTestGain PROPERTIES SUFFIX "${test_gain_suffix}")
    set(TEST_GAIN_BINARY "TorqwaveTestGain${test_gain_suffix}")

    configure_file(Tests/TestLv2Gain/manifest.ttl.in "${test_lv2_bundle}/manifest.ttl" @ONLY)
    configure_file(Tests/TestLv2Gain/gain.ttl "${test_lv2_bundle}/gain.ttl" COPYONLY)
    add_dependencies(TorqwaveTests TorqwaveTestGain)
endif()

target_compile_definitions(HangingBundle PRIVATE TEST_BUNDLE_HANGS=1)
target_compile_definitions(CrashingBundle PRIVATE TEST_BUNDLE_CRASHES=1)
target_compile_definitions(TorqwaveTests PRIVATE TORQWAVE_TEST_BUNDLES="${CMAKE_BINARY_DIR}/TestBundles")

get_target_property(torqwave_defs Torqwave COMPILE_DEFINITIONS)
target_compile_definitions(TorqwaveTests PRIVATE ${torqwave_defs} JUCE_MODAL_LOOPS_PERMITTED=1)
add_dependencies(TorqwaveTests Torqwave)
target_compile_definitions(TorqwaveTests PRIVATE TORQWAVE_APP_EXECUTABLE="$<TARGET_FILE:Torqwave>")
target_compile_definitions(TorqwaveTests PRIVATE TORQWAVE_SOURCE_DIR="${CMAKE_SOURCE_DIR}")
target_include_directories(TorqwaveTests PRIVATE ${TORQWAVE_STAMP_DIR})

if(WIN32)
    target_link_libraries(TorqwaveTests PRIVATE mfplat mfuuid wmcodecdspuuid delayimp)
    target_link_options(TorqwaveTests PRIVATE /DELAYLOAD:mfplat.dll)
endif()
target_link_libraries(TorqwaveTests PRIVATE
    TorqwaveArt
    TorqwavePresets
    TorqwaveGuide
    TorqwaveLame
    TorqwaveStretch
    juce::juce_audio_utils
    juce::juce_audio_processors
    juce::juce_dsp
    juce::juce_video
    TorqwaveSqlite
    juce::juce_recommended_config_flags
    juce::juce_recommended_warning_flags)

# JUCE writes each target's Windows version resource (its .rc, from the version) once and never again:
# its rule doesn't depend on the version, so a build folder made at an older version kept stamping
# that version into the program's file properties. Removed at each configure, it is made again.
if(WIN32)
    foreach(rc_target IN ITEMS Torqwave TorqwaveTests)
        get_target_property(rc_dir ${rc_target} JUCE_GENERATED_SOURCES_DIRECTORY)
        file(REMOVE "${rc_dir}/${rc_target}_resources.rc")
    endforeach()
endif()
