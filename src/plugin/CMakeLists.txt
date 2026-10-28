juce_add_plugin(VocalTuneBtw
    COMPANY_NAME "VocalTuneBtw"
    PRODUCT_NAME "VocalTuneBtw"
    BUNDLE_ID "dev.vocaltunebtw.vocaltunebtw"
    # Keep both codes: the host identifies the plugin (and saved projects) by them.
    PLUGIN_MANUFACTURER_CODE Ptpl
    PLUGIN_CODE Ppc1
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Pitch Shift
    COPY_PLUGIN_AFTER_BUILD FALSE)

if(PITCHPILOT_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET VocalTuneBtw
        CLAP_ID "dev.vocaltunebtw.vocaltunebtw"
        CLAP_FEATURES audio-effect pitch-correction mono stereo)
endif()

target_sources(VocalTuneBtw PRIVATE
    Parameters.cpp
    PluginProcessor.cpp
    PluginEditor.cpp
    ../ui/LookAndFeel.cpp
    ../ui/Knob.cpp
    ../ui/TuningMeter.cpp
    ../ui/NoteKeyboard.cpp
    ../ui/MainPanel.cpp)

target_compile_definitions(VocalTuneBtw PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(VocalTuneBtw
    PRIVATE
        pitchpilot_dsp
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
