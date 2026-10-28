cmake_minimum_required(VERSION 3.25)

project(VocalTuneBtw VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

option(PITCHPILOT_BUILD_PLUGIN "Build the JUCE plugin (VST3, CLAP, Standalone)" ON)
option(PITCHPILOT_BUILD_CLAP "Also build the CLAP format (VST3 does not need it)" ON)
option(PITCHPILOT_BUILD_TESTS "Build the DSP unit tests" ON)

include(FetchContent)

add_subdirectory(src/dsp)

if(PITCHPILOT_BUILD_PLUGIN)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 9.0.4
        GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(JUCE)
    if(PITCHPILOT_BUILD_CLAP)
        FetchContent_Declare(clap_juce_extensions
            GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
            # main after 0.26.0; the 0.26.0 tag predates JUCE 9 support (PR #178)
            GIT_TAG 7adee3a1bd4684d4caa5601100e364abccff4b4d
            GIT_SUBMODULES_RECURSE TRUE)
        FetchContent_MakeAvailable(clap_juce_extensions)
    endif()
    add_subdirectory(src/plugin)
endif()

if(PITCHPILOT_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
