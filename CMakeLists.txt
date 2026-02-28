cmake_minimum_required(VERSION 3.16)
project(media-console VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Test Network)
qt_standard_project_setup(REQUIRES 6.8)

# clang-tidy integration (runs during build, does not block commits)
find_program(CLANG_TIDY_EXE NAMES clang-tidy)
if(CLANG_TIDY_EXE)
    set(CMAKE_CXX_CLANG_TIDY "${CLANG_TIDY_EXE}")
    message(STATUS "clang-tidy found: ${CLANG_TIDY_EXE}")
else()
    message(STATUS "clang-tidy not found, static analysis disabled")
endif()

# Get version from git tags + commit count
# Uses git describe to get: v1.0.0-5-ga1b2c3d
# where: v1.0.0 = last tag, 5 = commits since tag, a1b2c3d = current commit
execute_process(
    COMMAND git describe --tags --long --match "v*.*.*" --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_DESCRIBE
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)

# Parse git describe output: v1.0.0-5-ga1b2c3d
if(GIT_DESCRIBE MATCHES "^v([0-9]+)\\.([0-9]+)\\.[0-9]+-([0-9]+)-g([0-9a-f]+)$")
    set(VERSION_MAJOR ${CMAKE_MATCH_1})
    set(VERSION_MINOR ${CMAKE_MATCH_2})
    set(VERSION_PATCH ${CMAKE_MATCH_3})
    set(GIT_COMMIT_HASH ${CMAKE_MATCH_4})
    set(VERSION_STRING "v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${GIT_COMMIT_HASH}")
    message(STATUS "Version from git tag: ${VERSION_STRING}")
else()
    # No version tags found - fallback to total commit count
    execute_process(
        COMMAND git rev-list --count HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE COMMIT_COUNT
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE GIT_COMMIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    if(COMMIT_COUNT)
        set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
        set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
        set(VERSION_PATCH ${COMMIT_COUNT})
        set(GIT_COMMIT_HASH ${GIT_COMMIT_HASH})
        set(VERSION_STRING "v${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}-${GIT_COMMIT_HASH}")
    else()
        set(VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
        set(VERSION_MINOR ${PROJECT_VERSION_MINOR})
        set(VERSION_PATCH ${PROJECT_VERSION_PATCH})
        set(GIT_COMMIT_HASH "unknown")
        set(VERSION_STRING "v${PROJECT_VERSION}-${GIT_COMMIT_HASH}")
    endif()

    message(STATUS "Version from commits: ${VERSION_STRING}")
endif()

# Source files for the core library (everything except main.cpp)
set(LIB_SOURCES
    src/utils/Logging.h
    src/utils/Logging.cpp
    src/platform/IAudioOutput.h
    src/platform/ICdDrive.h
    src/platform/IGpioMonitor.h
    src/platform/IDisplayControl.h
    src/platform/PlatformFactory.h
    src/platform/PlatformFactory.cpp
    src/platform/stubs/StubAudioOutput.h
    src/platform/stubs/StubAudioOutput.cpp
    src/platform/stubs/StubCdDrive.h
    src/platform/stubs/StubCdDrive.cpp
    src/platform/stubs/StubGpioMonitor.h
    src/platform/stubs/StubGpioMonitor.cpp
    src/platform/stubs/StubDisplayControl.h
    src/platform/stubs/StubDisplayControl.cpp
    src/app/AppConfig.h
    src/app/AppConfig.cpp
    src/app/AppContext.h
    src/app/AppBuilder.h
    src/app/AppBuilder.cpp
    src/state/MediaSource.h
    src/state/MediaSource.cpp
    src/state/StreamingService.h
    src/state/PlaybackMode.h
    src/state/ActiveView.h
    src/state/ReceiverState.h
    src/state/ReceiverState.cpp
    src/state/PlaybackState.h
    src/state/PlaybackState.cpp
    src/state/UIState.h
    src/state/UIState.cpp
    src/receiver/EiscpMessage.h
    src/receiver/EiscpMessage.cpp
    src/receiver/EiscpConnection.h
    src/receiver/EiscpConnection.cpp
    src/receiver/ReceiverController.h
    src/receiver/ReceiverController.cpp
)

# Create static library for testing without main()
add_library(media-console-lib STATIC ${LIB_SOURCES})

target_include_directories(media-console-lib PUBLIC
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(media-console-lib PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::Qml
    Qt6::Network
)

# Pass version info to C++ code
target_compile_definitions(media-console-lib PUBLIC
    VERSION_STRING="${VERSION_STRING}"
    VERSION_MAJOR=${VERSION_MAJOR}
    VERSION_MINOR=${VERSION_MINOR}
    VERSION_PATCH=${VERSION_PATCH}
    GIT_COMMIT="${GIT_COMMIT_HASH}"
)

# Main executable
qt_add_executable(media-console
    src/main.cpp
)

target_link_libraries(media-console PRIVATE
    media-console-lib
)

# QML module
qt_add_qml_module(media-console
    URI MediaConsole
    VERSION 1.0
    QML_FILES
        src/qml/main.qml
)

# Testing
option(BUILD_TESTS "Build unit tests" ON)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# Install target
install(TARGETS media-console
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
