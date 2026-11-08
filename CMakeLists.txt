cmake_minimum_required(VERSION 3.20)
project(max_2_hours LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Architecture detection (x64 vs x86) for Windows prebuilt libraries
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

# Check bundled cmake/sdl3 vendor directories first on Windows
set(SDL3_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl3")
if(EXISTS "${SDL3_VENDOR_DIR}/SDL3")
    list(APPEND CMAKE_PREFIX_PATH "${SDL3_VENDOR_DIR}/SDL3" "${SDL3_VENDOR_DIR}/SDL3_ttf")
endif()

# Try finding system / vendor SDL3 and SDL3_ttf
find_package(SDL3 CONFIG QUIET)
find_package(SDL3_ttf CONFIG QUIET)

# If not found via CONFIG, try find_package without CONFIG (e.g. FindPkgConfig)
if(NOT SDL3_FOUND OR NOT SDL3_ttf_FOUND)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        if(NOT SDL3_FOUND)
            pkg_check_modules(PKG_SDL3 QUIET sdl3)
            if(PKG_SDL3_FOUND)
                find_package(SDL3 QUIET)
            endif()
        endif()
        if(NOT SDL3_ttf_FOUND)
            pkg_check_modules(PKG_SDL3_TTF QUIET sdl3-ttf)
            if(PKG_SDL3_TTF_FOUND)
                find_package(SDL3_ttf QUIET)
            endif()
        endif()
    endif()
endif()

# Fallback on Windows: download official release VC zip via FetchContent if still not found
if(WIN32 AND (NOT SDL3_FOUND OR NOT SDL3_ttf_FOUND))
    include(FetchContent)
    if(NOT SDL3_FOUND)
        FetchContent_Declare(
            SDL3_prebuilt
            URL "https://github.com/libsdl-org/SDL/releases/download/release-3.2.0/SDL3-devel-3.2.0-VC.zip"
        )
        FetchContent_MakeAvailable(SDL3_prebuilt)
        list(APPEND CMAKE_PREFIX_PATH "${sdl3_prebuilt_SOURCE_DIR}")
        find_package(SDL3 CONFIG REQUIRED)
    endif()
    if(NOT SDL3_ttf_FOUND)
        FetchContent_Declare(
            SDL3_ttf_prebuilt
            URL "https://github.com/libsdl-org/SDL_ttf/releases/download/release-3.2.2/SDL3_ttf-devel-3.2.2-VC.zip"
        )
        FetchContent_MakeAvailable(SDL3_ttf_prebuilt)
        list(APPEND CMAKE_PREFIX_PATH "${sdl3_ttf_prebuilt_SOURCE_DIR}")
        find_package(SDL3_ttf CONFIG REQUIRED)
    endif()
endif()

# Fallback on Linux if still not found: FetchContent build from source
if(NOT SDL3_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.2.0
    )
    FetchContent_MakeAvailable(SDL3)
endif()

if(NOT SDL3_ttf_FOUND)
    include(FetchContent)
    FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-3.2.2
    )
    FetchContent_MakeAvailable(SDL3_ttf)
endif()

# Source files
set(SOURCES
    src/main.cpp
    src/common.cpp
    src/timer.cpp
    src/ui.cpp
    src/systray.cpp
)

# Windows icon resource
if(WIN32)
    if(MINGW OR MSVC)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/icone.rc")
            list(APPEND SOURCES icone.rc)
        endif()
    endif()
endif()

# Executable target
add_executable(max_2_hours ${SOURCES})

target_include_directories(max_2_hours PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(max_2_hours PRIVATE
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
)

if(WIN32 AND MINGW)
    target_link_options(max_2_hours PRIVATE -mwindows)
endif()

# Copy assets to output directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET max_2_hours POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:max_2_hours>/assets"
        COMMENT "Copying assets directory to output"
    )
endif()

# On Windows, copy runtime DLLs next to the executable
if(WIN32)
    if(EXISTS "${SDL3_VENDOR_DIR}/SDL3/lib/${ARCH_DIR}")
        file(GLOB BUNDLED_DLLS CONFIGURE_DEPENDS
            "${SDL3_VENDOR_DIR}/SDL3/lib/${ARCH_DIR}/*.dll"
            "${SDL3_VENDOR_DIR}/SDL3_ttf/lib/${ARCH_DIR}/*.dll"
        )
        foreach(DLL_FILE ${BUNDLED_DLLS})
            add_custom_command(
                TARGET max_2_hours POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    "${DLL_FILE}"
                    "$<TARGET_FILE_DIR:max_2_hours>/"
                COMMENT "Copying runtime DLL ${DLL_FILE}"
            )
        endforeach()
    endif()
endif()

# Install rules
install(TARGETS max_2_hours DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)
