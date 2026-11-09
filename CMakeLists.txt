cmake_minimum_required(VERSION 3.22)

project(Replicant VERSION 1.0.2 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

# ---------------------------------------------------------------------------
# JUCE: use a local copy if one is given (-DJUCE_DIR=...), otherwise download.
# ---------------------------------------------------------------------------
set(JUCE_DIR "" CACHE PATH "Path to a local JUCE checkout (optional)")
if(JUCE_DIR)
    add_subdirectory(${JUCE_DIR} JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        8.0.9
        GIT_SHALLOW    ON)
    FetchContent_MakeAvailable(JUCE)
endif()

option(REP_BUILD_TESTS "Build the offline render test and UI snapshot tools" OFF)

juce_add_plugin(Replicant
    COMPANY_NAME            "APT Industrial"
    PLUGIN_MANUFACTURER_CODE Aptl
    PLUGIN_CODE             Rp80
    FORMATS                 VST3 Standalone
    PRODUCT_NAME            "Replicant"
    IS_SYNTH                TRUE
    NEEDS_MIDI_INPUT        TRUE
    NEEDS_MIDI_OUTPUT       FALSE
    IS_MIDI_EFFECT          FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    VST3_CATEGORIES         Instrument Synth
    DESCRIPTION             "Eight-voice dual-layer polysynth - a tribute to the Yamaha CS-80")

juce_add_binary_data(ReplicantData SOURCES
    Resources/fonts/Michroma-Regular.ttf
    Resources/fonts/ShareTechMono-Regular.ttf
    Resources/fonts/SortsMillGoudy-Italic.ttf)

set(REP_CORE_SOURCES
    Source/PluginProcessor.cpp
    Source/Params.cpp
    Source/Presets.cpp
    Source/dsp/Effects.cpp)

set(REP_UI_SOURCES
    Source/PluginEditor.cpp
    Source/ui/NeonLookAndFeel.cpp
    Source/ui/Backdrop.cpp
    Source/ui/Widgets.cpp
    Source/ui/Displays.cpp)

target_sources(Replicant PRIVATE ${REP_CORE_SOURCES} ${REP_UI_SOURCES})
target_include_directories(Replicant PRIVATE Source)

target_compile_definitions(Replicant PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Replicant
    PRIVATE
        ReplicantData
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# ---------------------------------------------------------------------------
# Development tools (not shipped): offline renders + stability checks, and a
# tool that renders the editor to PNG.
# ---------------------------------------------------------------------------
if(REP_BUILD_TESTS)
    juce_add_console_app(RepRenderTest PRODUCT_NAME "RepRenderTest")
    target_sources(RepRenderTest PRIVATE tools/RenderTest.cpp ${REP_CORE_SOURCES})
    target_include_directories(RepRenderTest PRIVATE Source)
    target_compile_definitions(RepRenderTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 REP_HEADLESS=1)
    target_link_libraries(RepRenderTest PRIVATE
        juce::juce_audio_utils juce::juce_dsp juce::juce_audio_formats
        juce::juce_recommended_config_flags)

    juce_add_gui_app(RepUiSnapshot PRODUCT_NAME "RepUiSnapshot")
    target_sources(RepUiSnapshot PRIVATE tools/UiSnapshot.cpp ${REP_CORE_SOURCES} ${REP_UI_SOURCES})
    target_include_directories(RepUiSnapshot PRIVATE Source)
    target_compile_definitions(RepUiSnapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(RepUiSnapshot PRIVATE ReplicantData
        juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
