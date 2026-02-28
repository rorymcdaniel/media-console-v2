include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

# Prevent GoogleTest from overriding compiler/linker options on Windows
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(googletest)

# Test sources
set(TEST_SOURCES
    test_PlatformFactory.cpp
    test_StubAudioOutput.cpp
    test_StubCdDrive.cpp
    test_StubGpioMonitor.cpp
    test_StubDisplayControl.cpp
    test_AppConfig.cpp
    test_AppBuilder.cpp
    test_MediaSource.cpp
    test_ReceiverState.cpp
    test_PlaybackState.cpp
    test_UIState.cpp
    test_EiscpMessage.cpp
    test_ReceiverController.cpp
    test_VolumeGestureController.cpp
    test_AudioStream.cpp
    test_AudioRingBuffer.cpp
    test_LocalPlaybackController.cpp
)

add_executable(media-console-tests ${TEST_SOURCES})

target_link_libraries(media-console-tests PRIVATE
    GTest::gtest_main
    Qt6::Core
    Qt6::Test
    media-console-lib
)

target_include_directories(media-console-tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

include(GoogleTest)
gtest_discover_tests(media-console-tests)
