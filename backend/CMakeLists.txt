cmake_minimum_required(VERSION 3.22)
project(DAWAudioEngine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE
add_subdirectory(JUCE)

# Notre application console
juce_add_console_app(DAWAudioEngine 
    PRODUCT_NAME "DAW Audio Engine")

# Source
file(GLOB_RECURSE SOURCES "src/*.cpp")
target_sources(DAWAudioEngine PRIVATE ${SOURCES})

target_include_directories(DAWAudioEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Configuration JUCE
target_compile_definitions(DAWAudioEngine PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_ALSA=1
    JUCE_JACK=1
    JUCE_APPLICATION_NAME_STRING="DAW Audio Engine"
    JUCE_APPLICATION_VERSION_STRING="1.0.0")

# Modules JUCE nécessaires
target_link_libraries(DAWAudioEngine PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events)