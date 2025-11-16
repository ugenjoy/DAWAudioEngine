cmake_minimum_required(VERSION 3.22)
project(DAWAudioEngine VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Build options
option(BUILD_TESTS "Build unit tests" OFF)
# TODO: [LOW] Add more build options:
# option(ENABLE_SIMD "Enable SIMD optimizations" ON)
# option(BUILD_BENCHMARKS "Build performance benchmarks" OFF)
# option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# JUCE
add_subdirectory(JUCE)

# Crow (WebSocket server) + ASIO dependency
include(FetchContent)

# Fetch ASIO (required by Crow)
FetchContent_Declare(
    asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG asio-1-30-2
)
FetchContent_MakeAvailable(asio)
set(ASIO_INCLUDE_DIR ${asio_SOURCE_DIR}/asio/include)

# Fetch Crow
FetchContent_Declare(
    crow
    GIT_REPOSITORY https://github.com/CrowCpp/Crow.git
    GIT_TAG v1.2.0
)
FetchContent_MakeAvailable(crow)

# Fetch nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Console application
juce_add_console_app(DAWAudioEngine 
    PRODUCT_NAME "DAW Audio Engine")

# Source files (explicit listing is better than GLOB)
target_sources(DAWAudioEngine PRIVATE
    src/main.cpp
    src/audio-engine-core.cpp
    src/audio-track.cpp
    src/beat-track.cpp
    src/song.cpp
    src/tracks-manager.cpp
)

target_include_directories(DAWAudioEngine PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include)

# JUCE configuration
target_compile_definitions(DAWAudioEngine PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_ALSA=1
    JUCE_JACK=1
    JUCE_APPLICATION_NAME_STRING="DAW Audio Engine"
    JUCE_APPLICATION_VERSION_STRING="1.0.0")

# Required JUCE modules
target_link_libraries(DAWAudioEngine PRIVATE
    juce::juce_audio_basics
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_utils
    juce::juce_core
    juce::juce_events
    Crow::Crow
    nlohmann_json::nlohmann_json)

# Unit tests
if(BUILD_TESTS)
    enable_testing()
    
    # Create test runner main file
    file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/test_main.cpp
        "#include <juce_core/juce_core.h>\n"
        "\n"
        "int main(int argc, char* argv[]) {\n"
        "    juce::UnitTestRunner runner;\n"
        "    runner.setAssertOnFailure(false);\n"
        "    runner.runAllTests();\n"
        "    \n"
        "    int numFailures = 0;\n"
        "    for (int i = 0; i < runner.getNumResults(); ++i) {\n"
        "        auto* result = runner.getResult(i);\n"
        "        for (const auto& msg : result->messages) {\n"
        "            juce::Logger::writeToLog(msg);\n"
        "        }\n"
        "        numFailures += result->failures;\n"
        "    }\n"
        "    \n"
        "    return numFailures > 0 ? 1 : 0;\n"
        "}"
    )
    
    # Test executable
    juce_add_console_app(DAWAudioEngine_Tests
        PRODUCT_NAME "DAW Audio Engine Tests")
    
    target_sources(DAWAudioEngine_Tests PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}/test_main.cpp
        tests/test.wavetable.cpp
        tests/test.beattrack.cpp
        src/audio-track.cpp
        src/beat-track.cpp
    )
    
    target_include_directories(DAWAudioEngine_Tests PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include)
    
    target_compile_definitions(DAWAudioEngine_Tests PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_ALSA=1
        JUCE_JACK=1
        JUCE_APPLICATION_NAME_STRING="DAW Audio Engine Tests"
        JUCE_APPLICATION_VERSION_STRING="1.0.0")
    
    target_link_libraries(DAWAudioEngine_Tests PRIVATE
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_events)
    
    # Register tests with CTest
    add_test(NAME WaveTableTests 
             COMMAND DAWAudioEngine_Tests)
    add_test(NAME BeatTrackTests 
             COMMAND DAWAudioEngine_Tests)
    
    message(STATUS "Unit tests enabled")
endif()