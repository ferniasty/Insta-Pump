cmake_minimum_required(VERSION 3.22)
project(InstantPump VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "")

include(FetchContent)
# Pass -DFETCHCONTENT_SOURCE_DIR_JUCE=/path/to/JUCE to use a local checkout
FetchContent_Declare(JUCE GIT_REPOSITORY https://github.com/juce-framework/JUCE.git GIT_TAG 8.0.4 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(KD_FORMATS VST3 Standalone)
if(APPLE)
    list(APPEND KD_FORMATS AU)
endif()

juce_add_plugin(InstantPump
    COMPANY_NAME "Rave Academy"
    PRODUCT_NAME "Instant Pump"
    BUNDLE_ID "com.raveacademy.instantpump"
    PLUGIN_MANUFACTURER_CODE Rave
    PLUGIN_CODE Ipmp
    FORMATS ${KD_FORMATS}
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Dynamics
    AU_MAIN_TYPE kAudioUnitType_Effect
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(InstantPump PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_compile_definitions(InstantPump PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(InstantPump
    PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags juce::juce_recommended_warning_flags)

# ---- tests + screenshot tool (not shipped)
option(KD_BUILD_TOOLS "Build test and snapshot tools" OFF)
if(KD_BUILD_TOOLS)
    add_executable(kd_envelope_test Tests/EnvelopeTest.cpp)
    target_include_directories(kd_envelope_test PRIVATE Source)

    juce_add_console_app(kd_snapshot PRODUCT_NAME "kd_snapshot")
    target_sources(kd_snapshot PRIVATE Tests/Snapshot.cpp Source/PluginProcessor.cpp Source/PluginEditor.cpp)
    target_compile_definitions(kd_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JucePlugin_Name="Instant Pump"
        JUCE_STANDALONE_APPLICATION=1 JUCE_MODAL_LOOPS_PERMITTED=1)
    target_link_libraries(kd_snapshot PRIVATE juce::juce_audio_utils juce::juce_dsp juce::juce_recommended_config_flags)
endif()
