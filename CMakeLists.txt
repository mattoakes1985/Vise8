cmake_minimum_required(VERSION 3.22)
project(VISE8 VERSION 1.0.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE location: -DJUCE_DIR=/path/to/JUCE  (default: ./JUCE as a git submodule or checkout)
if(NOT DEFINED JUCE_DIR)
    set(JUCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/JUCE)
endif()
add_subdirectory(${JUCE_DIR} JUCE)

set(VISE_SOURCES
    Source/ViseEngine.cpp Source/ViseEngine.h
    Source/PluginProcessor.cpp Source/PluginProcessor.h
    Source/PluginEditor.cpp Source/PluginEditor.h)

# VST3 everywhere; Audio Unit on macOS only
set(VISE_FORMATS VST3)
if(APPLE)
    list(APPEND VISE_FORMATS AU)
endif()

juce_add_plugin(VISE8
    COMPANY_NAME "Anvil Dynamics"
    PLUGIN_MANUFACTURER_CODE Anvl
    PLUGIN_CODE Vs08
    FORMATS ${VISE_FORMATS}
    AU_MAIN_TYPE kAudioUnitType_Effect
    COMPONENT_MANUFACTURER_CODE Anvl
    COMPONENT_SUBTYPE_CODE Vs08
    COMPONENT_NAME "Anvil Dynamics: VISE-8"
    COMPONENT_DESCRIPTION "VISE-8 knee compressor"
    PRODUCT_NAME "VISE-8"
    VST3_CATEGORIES Fx Dynamics
    IS_SYNTH FALSE NEEDS_MIDI_INPUT FALSE NEEDS_MIDI_OUTPUT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(VISE8 PRIVATE ${VISE_SOURCES})
target_compile_definitions(VISE8 PUBLIC
    JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_JACK=0)
target_link_libraries(VISE8 PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_lto_flags)

# Offline verification harness (not shipped)
juce_add_console_app(VISE8Test PRODUCT_NAME "vise8test")
target_sources(VISE8Test PRIVATE ${VISE_SOURCES} Tests/VerifyMain.cpp)
target_include_directories(VISE8Test PRIVATE Source)
target_compile_definitions(VISE8Test PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_JACK=0 JUCE_USE_MP3AUDIOFORMAT=0)
target_link_libraries(VISE8Test PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
