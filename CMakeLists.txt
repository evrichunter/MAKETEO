cmake_minimum_required(VERSION 3.22)
project(Maketeo VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(MK_LTO "Link-time optimisation (CI default ON; switch OFF for fast local iteration)" ON)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.8
    GIT_SHALLOW ON)
FetchContent_MakeAvailable(JUCE)

juce_add_plugin(Maketeo
    COMPANY_NAME "Evric Hunter"
    PLUGIN_MANUFACTURER_CODE EvHu
    PLUGIN_CODE MkTo
    FORMATS VST3 Standalone
    PRODUCT_NAME "MAKETEO By @EVRICHUNTER"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    VST3_CATEGORIES Fx
    COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(Maketeo PRIVATE
    Source/PluginProcessor.cpp
    Source/PluginEditor.cpp)

target_include_directories(Maketeo PRIVATE Source)

target_compile_definitions(Maketeo PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(Maketeo
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags)
if(MK_LTO)
    target_link_libraries(Maketeo PUBLIC juce::juce_recommended_lto_flags)
endif()

# Headless DSP validation (no JUCE): cmake --build build --target mk_dsp_test && ./build/mk_dsp_test
add_executable(mk_dsp_test Tests/dsp_test.cpp)
target_include_directories(mk_dsp_test PRIVATE Source)
