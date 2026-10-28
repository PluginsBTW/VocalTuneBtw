add_executable(vtb-render vtb-render.cpp)
target_link_libraries(vtb-render PRIVATE pitchpilot_eval)
target_compile_options(vtb-render PRIVATE -Wall -Wextra -Wpedantic -Wshadow)
