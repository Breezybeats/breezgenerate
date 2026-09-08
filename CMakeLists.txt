cmake_minimum_required(VERSION 3.22)
project(NeuroJ VERSION 0.2.0 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(NEUROJ_CORE_ONLY "Build independent DSP tests without JUCE" OFF)
enable_testing()
add_executable(neuroj_core_tests CoreTests.cpp)
target_include_directories(neuroj_core_tests PRIVATE Source)
add_test(NAME CoreDSP COMMAND neuroj_core_tests)
if(NEUROJ_CORE_ONLY)
  return()
endif()
include(FetchContent)
FetchContent_Declare(JUCE URL https://github.com/juce-framework/JUCE/archive/refs/tags/8.0.6.tar.gz
  URL_HASH SHA256=95752f8e8b37d802b4c10182bc757de7d88f0c8899e879afa8798589be306a11)
FetchContent_MakeAvailable(JUCE)
juce_add_plugin(NeuroJ
  COMPANY_NAME "NeuroJ Audio" BUNDLE_ID "audio.neuroj.synth"
  PLUGIN_MANUFACTURER_CODE Njau PLUGIN_CODE Nj02
  PRODUCT_NAME "NEUROJ" FORMATS VST3 Standalone
  IS_SYNTH TRUE NEEDS_MIDI_INPUT TRUE NEEDS_MIDI_OUTPUT FALSE
  VST3_CATEGORIES Instrument Synth
  COPY_PLUGIN_AFTER_BUILD FALSE)
target_sources(NeuroJ PRIVATE Source/Processor.cpp Source/Editor.cpp)
target_include_directories(NeuroJ PUBLIC Source)
target_compile_definitions(NeuroJ PUBLIC JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0 JUCE_DISPLAY_SPLASH_SCREEN=0)
target_link_libraries(NeuroJ PRIVATE juce::juce_audio_utils
  PUBLIC juce::juce_recommended_config_flags)
juce_add_console_app(neuroj_state_tests PRODUCT_NAME "neuroj_state_tests")
target_sources(neuroj_state_tests PRIVATE StateTests.cpp Source/Processor.cpp Source/Editor.cpp)
target_include_directories(neuroj_state_tests PRIVATE Source)
target_compile_definitions(neuroj_state_tests PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0)
target_link_libraries(neuroj_state_tests PRIVATE juce::juce_audio_utils)
add_test(NAME PluginState COMMAND neuroj_state_tests)
juce_add_console_app(neuroj_host_smoke PRODUCT_NAME "neuroj_host_smoke")
target_sources(neuroj_host_smoke PRIVATE HostSmoke.cpp)
target_compile_definitions(neuroj_host_smoke PRIVATE JUCE_PLUGINHOST_VST3=1
  JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
target_link_libraries(neuroj_host_smoke PRIVATE juce::juce_audio_utils)
