cmake_minimum_required(VERSION 3.16)
project(media-console VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt6 components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick Qml Test Network Sql Concurrent NetworkAuth)
find_package(Qt6 QUIET COMPONENTS HttpServer)
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
    src/platform/QuadratureDecoder.h
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
    src/receiver/VolumeGestureController.h
    src/receiver/VolumeGestureController.cpp
    src/state/CommandSource.h
    src/audio/AudioStream.h
    src/audio/AudioBufferStats.h
    src/audio/AudioRingBuffer.h
    src/audio/AudioRingBuffer.cpp
    src/audio/LocalPlaybackController.h
    src/audio/LocalPlaybackController.cpp
    src/cd/CdAudioStream.h
    src/cd/CdAudioStream.cpp
    src/cd/CdMetadataCache.h
    src/cd/CdMetadataCache.cpp
    src/cd/CdMetadataProvider.h
    src/cd/CdMetadataProvider.cpp
    src/cd/CdAlbumArtProvider.h
    src/cd/CdAlbumArtProvider.cpp
    src/cd/CdController.h
    src/cd/CdController.cpp
    src/library/LibraryDatabase.h
    src/library/LibraryDatabase.cpp
    # Spotify integration
    src/spotify/SpotifyAuth.h
    src/spotify/SpotifyAuth.cpp
    src/spotify/SpotifyClient.h
    src/spotify/SpotifyClient.cpp
    src/spotify/SpotifyController.h
    src/spotify/SpotifyController.cpp
    # Display control
    src/display/LinuxDisplayControl.h
    src/display/LinuxDisplayControl.cpp
    src/display/ScreenTimeoutController.h
    src/display/ScreenTimeoutController.cpp
    # HTTP API server
    src/api/HttpApiServer.h
    src/api/HttpApiServer.cpp
    # Orchestration
    src/orchestration/PlaybackRouter.h
    src/orchestration/PlaybackRouter.cpp
    src/orchestration/AlbumArtResolver.h
    src/orchestration/AlbumArtResolver.cpp
)

# ALSA audio output (Linux only)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
    list(APPEND LIB_SOURCES
        src/audio/AlsaAudioOutput.h
        src/audio/AlsaAudioOutput.cpp
    )
endif()

# CD subsystem (Linux only -- libcdio + libdiscid)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT TARGET PkgConfig::ALSA)
        find_package(PkgConfig REQUIRED)
    endif()
    pkg_check_modules(CDIO IMPORTED_TARGET libcdio)
    pkg_check_modules(CDIO_PARANOIA IMPORTED_TARGET libcdio_paranoia)
    pkg_check_modules(CDIO_CDDA IMPORTED_TARGET libcdio_cdda)
    pkg_check_modules(DISCID IMPORTED_TARGET libdiscid)
    if(CDIO_FOUND AND CDIO_PARANOIA_FOUND AND CDIO_CDDA_FOUND AND DISCID_FOUND)
        list(APPEND LIB_SOURCES
            src/cd/LibcdioCdDrive.h
            src/cd/LibcdioCdDrive.cpp
        )
        message(STATUS "CD subsystem: enabled (libcdio + libdiscid found)")
    else()
        message(STATUS "CD subsystem: disabled (libcdio/libdiscid not found)")
    endif()
endif()

# FLAC library subsystem (Linux only -- libsndfile + libsamplerate + taglib)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT TARGET PkgConfig::ALSA)
        find_package(PkgConfig REQUIRED)
    endif()
    pkg_check_modules(SNDFILE IMPORTED_TARGET sndfile)
    pkg_check_modules(SAMPLERATE IMPORTED_TARGET samplerate)
    pkg_check_modules(TAGLIB IMPORTED_TARGET taglib)
    if(SNDFILE_FOUND AND SAMPLERATE_FOUND AND TAGLIB_FOUND)
        list(APPEND LIB_SOURCES
            src/library/FlacAudioStream.h
            src/library/FlacAudioStream.cpp
            src/library/LibraryScanner.h
            src/library/LibraryScanner.cpp
            src/library/LibraryAlbumArtProvider.h
            src/library/LibraryAlbumArtProvider.cpp
            src/library/LibraryArtistModel.h
            src/library/LibraryArtistModel.cpp
            src/library/LibraryAlbumModel.h
            src/library/LibraryAlbumModel.cpp
            src/library/LibraryTrackModel.h
            src/library/LibraryTrackModel.cpp
            src/library/FlacLibraryController.h
            src/library/FlacLibraryController.cpp
        )
        message(STATUS "FLAC library: enabled (libsndfile + libsamplerate + taglib found)")
    else()
        message(STATUS "FLAC library: disabled (libsndfile/libsamplerate/taglib not found)")
    endif()
endif()

# DDC/CI display control (Linux only -- ddcutil)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(DDCUTIL_EXE NAMES ddcutil)
    if(DDCUTIL_EXE)
        message(STATUS "DDC/CI display: enabled (ddcutil found at ${DDCUTIL_EXE})")
    else()
        message(STATUS "DDC/CI display: disabled (ddcutil not found)")
    endif()
endif()

# GPIO subsystem (Linux only -- libgpiod v2)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    if(NOT TARGET PkgConfig::ALSA)
        find_package(PkgConfig REQUIRED)
    endif()
    pkg_check_modules(GPIOD IMPORTED_TARGET libgpiodcxx)
    if(GPIOD_FOUND)
        list(APPEND LIB_SOURCES
            src/platform/LinuxGpioMonitor.h
            src/platform/LinuxGpioMonitor.cpp
        )
        message(STATUS "GPIO subsystem: enabled (libgpiodcxx found)")
    else()
        message(STATUS "GPIO subsystem: disabled (libgpiodcxx not found)")
    endif()
endif()

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
    Qt6::Sql
    Qt6::Concurrent
    Qt6::NetworkAuth
)

# Link ALSA on Linux
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_link_libraries(media-console-lib PUBLIC PkgConfig::ALSA)
    target_compile_definitions(media-console-lib PUBLIC HAS_ALSA=1)
endif()

# Link CD subsystem libraries on Linux
if(CDIO_FOUND AND CDIO_PARANOIA_FOUND AND CDIO_CDDA_FOUND AND DISCID_FOUND)
    target_link_libraries(media-console-lib PUBLIC
        PkgConfig::CDIO
        PkgConfig::CDIO_PARANOIA
        PkgConfig::CDIO_CDDA
        PkgConfig::DISCID
    )
    target_compile_definitions(media-console-lib PUBLIC HAS_CDIO=1)
endif()

# Qt6 HttpServer (optional -- available on Linux/Pi, not always on macOS dev)
if(Qt6HttpServer_FOUND)
    target_link_libraries(media-console-lib PUBLIC Qt6::HttpServer)
    target_compile_definitions(media-console-lib PUBLIC HAS_QT_HTTPSERVER=1)
    message(STATUS "HTTP API server: enabled (Qt6::HttpServer found)")
else()
    message(STATUS "HTTP API server: disabled (Qt6::HttpServer not found)")
endif()

# DDC/CI display define on Linux
if(DDCUTIL_EXE)
    target_compile_definitions(media-console-lib PUBLIC HAS_DDCUTIL=1)
endif()

# Link GPIO subsystem on Linux
if(GPIOD_FOUND)
    target_link_libraries(media-console-lib PUBLIC PkgConfig::GPIOD)
    target_compile_definitions(media-console-lib PUBLIC HAS_GPIOD=1)
endif()

# Link FLAC library libraries on Linux
if(SNDFILE_FOUND AND SAMPLERATE_FOUND AND TAGLIB_FOUND)
    target_link_libraries(media-console-lib PUBLIC
        PkgConfig::SNDFILE
        PkgConfig::SAMPLERATE
        PkgConfig::TAGLIB
    )
    target_compile_definitions(media-console-lib PUBLIC HAS_SNDFILE=1)
endif()

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
