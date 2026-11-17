cmake_minimum_required(VERSION 3.22)
project(HBHalo VERSION 1.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

add_subdirectory(JUCE)

# iPad (Logic Pro for iPad, AUM, GarageBand): AUv3 inside a standalone app
# macOS (Logic Pro): AU + VST3 + Standalone      Windows (FL Studio): VST3 + Standalone
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(HALO_FORMATS AUv3 Standalone)
elseif(APPLE)
    set(HALO_FORMATS AU VST3 Standalone)
else()
    set(HALO_FORMATS VST3 Standalone)
endif()

juce_add_plugin(HBHalo
    COMPANY_NAME "HALO Studios"
    COMPANY_WEBSITE "https://hamkobarezz.com"
    BUNDLE_ID com.halostudios.halo
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Hbst
    PLUGIN_CODE Halo
    FORMATS ${HALO_FORMATS}
    PRODUCT_NAME "HALO"
    DESCRIPTION "Vocal, mix and mastering chain"
    VST3_CATEGORIES Fx Dynamics EQ Reverb
    AU_MAIN_TYPE kAudioUnitType_Effect
    AU_SANDBOX_SAFE TRUE
    HARDENED_RUNTIME_ENABLED TRUE
    MICROPHONE_PERMISSION_ENABLED TRUE
    MICROPHONE_PERMISSION_TEXT "HALO processes your microphone input in standalone mode."
    TARGETED_DEVICE_FAMILY 2
    REQUIRES_FULL_SCREEN FALSE
    IPAD_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight
    IPHONE_SCREEN_ORIENTATIONS UIInterfaceOrientationLandscapeLeft UIInterfaceOrientationLandscapeRight)

set(HALO_SOURCES
    Source/PluginProcessor.cpp
    Source/Presets.cpp
    Source/PluginEditor.cpp)

target_sources(HBHalo PRIVATE ${HALO_SOURCES})

# Embedded Montserrat (SIL Open Font License) so the UI looks the same on every machine
juce_add_binary_data(HaloAssets SOURCES
    Resources/fonts/Montserrat-Regular.ttf
    Resources/fonts/Montserrat-Medium.ttf
    Resources/fonts/Montserrat-SemiBold.ttf
    Resources/fonts/Montserrat-Bold.ttf
    Resources/logo/halo_studios_logo.svg)
set_target_properties(HaloAssets PROPERTIES POSITION_INDEPENDENT_CODE TRUE)

# Silent self-test build: feeds an internal test signal so the UI can be checked without audio I/O
option(HALO_DEMO "Internal demo signal, output muted (for UI checks only)" OFF)
if(HALO_DEMO)
    target_compile_definitions(HBHalo PUBLIC HALO_DEMO=1)
endif()
target_include_directories(HBHalo PRIVATE Source)

target_compile_definitions(HBHalo PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(HBHalo
    PRIVATE
        HaloAssets
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
# ---- Offline DSP test (renders test signals through every module, checks for NaN / runaway) ----
juce_add_console_app(HaloTest PRODUCT_NAME "HaloTest")
target_sources(HaloTest PRIVATE Tests/HaloTest.cpp Source/PluginProcessor.cpp Source/Presets.cpp)
target_include_directories(HaloTest PRIVATE Source)
target_compile_definitions(HaloTest PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 HALO_TEST_BUILD=1)
target_link_libraries(HaloTest PRIVATE juce::juce_audio_utils juce::juce_dsp
    PUBLIC juce::juce_recommended_config_flags)
endif()


# ---- Windows installer (HALO-Setup.exe). Payload is staged by packaging/windows/build_installer.ps1 ----
option(HALO_BUILD_INSTALLER "Build the branded Windows installer" OFF)
if(WIN32 AND HALO_BUILD_INSTALLER)
    juce_add_binary_data(HaloSetupPayload HEADER_NAME SetupData.h NAMESPACE SetupData SOURCES
        packaging/windows/payload/HALO.vst3
        packaging/windows/payload/moduleinfo.json
        packaging/windows/payload/HALO.exe
        Resources/logo/halo_studios_logo.svg
        Resources/fonts/Montserrat-Medium.ttf
        Resources/fonts/Montserrat-SemiBold.ttf)
    juce_add_gui_app(HALOSetup PRODUCT_NAME "HALO Setup" COMPANY_NAME "HALO Studios" VERSION 1.1.0)
    target_sources(HALOSetup PRIVATE packaging/windows/Installer.cpp)
    target_compile_definitions(HALOSetup PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0)
    target_link_libraries(HALOSetup PRIVATE HaloSetupPayload juce::juce_gui_extra PUBLIC juce::juce_recommended_config_flags)
    option(HALO_INSTALLER_PREVIEW "UI preview only: no admin manifest" OFF)
    if(NOT HALO_INSTALLER_PREVIEW)
        target_link_options(HALOSetup PRIVATE "/MANIFESTUAC:level='requireAdministrator' uiAccess='false'")
    endif()
endif()
