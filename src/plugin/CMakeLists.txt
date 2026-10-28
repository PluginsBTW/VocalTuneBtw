juce_add_plugin(PitchPilot
    COMPANY_NAME "PitchPilot"
    PRODUCT_NAME "PitchPilot"
    BUNDLE_ID "dev.pitchpilot.pitchpilot"
    PLUGIN_MANUFACTURER_CODE Ptpl
    PLUGIN_CODE Ppc1
    FORMATS VST3 Standalone
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx Pitch Shift
    COPY_PLUGIN_AFTER_BUILD FALSE)

if(PITCHPILOT_BUILD_CLAP)
    clap_juce_extensions_plugin(TARGET PitchPilot
        CLAP_ID "dev.pitchpilot.pitchpilot"
        CLAP_FEATURES audio-effect pitch-correction mono stereo)
endif()

target_sources(PitchPilot PRIVATE
    Parameters.cpp
    PluginProcessor.cpp
    PluginEditor.cpp)

target_compile_definitions(PitchPilot PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(PitchPilot
    PRIVATE
        pitchpilot_dsp
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)
