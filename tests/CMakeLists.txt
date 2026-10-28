FetchContent_Declare(Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.7.1
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(Catch2)

add_executable(pitchpilot_tests
    test_scale_mapping.cpp
    test_pitch_detection.cpp
    test_correction.cpp
    test_pitch_shifter.cpp
    test_realtime.cpp)

target_link_libraries(pitchpilot_tests PRIVATE pitchpilot_dsp Catch2::Catch2WithMain)

include(${catch2_SOURCE_DIR}/extras/Catch.cmake)
catch_discover_tests(pitchpilot_tests)
