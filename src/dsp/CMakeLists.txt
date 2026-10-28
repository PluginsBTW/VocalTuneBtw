add_library(pitchpilot_dsp STATIC
    Scales.cpp
    PitchDetector.cpp
    CorrectionController.cpp
    PitchShifter.cpp
    PitchCorrector.cpp)

target_include_directories(pitchpilot_dsp PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_compile_features(pitchpilot_dsp PUBLIC cxx_std_20)
target_compile_options(pitchpilot_dsp PRIVATE -Wall -Wextra -Wpedantic -Wshadow -Wconversion)
