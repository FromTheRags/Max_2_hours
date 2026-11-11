cmake_minimum_required(VERSION 3.16)
project(max_2_hours LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Application version: read from VERSION file, can be overridden with -DAPP_VERSION=x.y.z (CI does this on tags)
if(NOT APP_VERSION)
    file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" APP_VERSION LIMIT_COUNT 1)
endif()
string(STRIP "${APP_VERSION}" APP_VERSION)
message(STATUS "Application version: ${APP_VERSION}")

# Architecture detection (x64 vs x86) for Windows prebuilt libraries
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(ARCH_DIR "x64")
else()
    set(ARCH_DIR "x86")
endif()

set(SDL12_VENDOR_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/sdl12")

# 1. Try finding system SDL 1.2
find_package(SDL QUIET)

# 2. Try finding system SDL_ttf
find_path(SDL_TTF_INCLUDE_DIR NAMES SDL_ttf.h
    PATH_SUFFIXES SDL include/SDL include
    PATHS
        ${SDL_INCLUDE_DIR}
        /usr/include/SDL
        /usr/local/include/SDL
        /opt/homebrew/include/SDL
        /opt/local/include/SDL
)

find_library(SDL_TTF_LIBRARY
    NAMES SDL_ttf SDL_ttf-static
    PATH_SUFFIXES lib
    PATHS
        /usr/lib
        /usr/local/lib
        /opt/homebrew/lib
        /opt/local/lib
)

# 3. Automatic fallback on Windows (CLion / Visual Studio / MinGW) using bundled cmake/sdl12
if(WIN32)
    if(NOT SDL_FOUND OR NOT SDL_INCLUDE_DIR OR NOT SDL_LIBRARY)
        set(SDL_INCLUDE_DIR "${SDL12_VENDOR_DIR}/include/SDL" CACHE PATH "SDL Include Dir" FORCE)
        find_library(SDL_LIBRARY_BUNDLED NAMES SDL PATHS "${SDL12_VENDOR_DIR}/lib/${ARCH_DIR}" NO_DEFAULT_PATH)
        set(SDL_LIBRARY "${SDL_LIBRARY_BUNDLED}" CACHE FILEPATH "SDL Library" FORCE)
        set(SDL_FOUND TRUE)
    endif()

    if(NOT SDL_TTF_INCLUDE_DIR)
        set(SDL_TTF_INCLUDE_DIR "${SDL12_VENDOR_DIR}/include/SDL" CACHE PATH "SDL TTF Include Dir" FORCE)
    endif()

    if(NOT SDL_TTF_LIBRARY)
        find_library(SDL_TTF_LIBRARY_BUNDLED NAMES SDL_ttf PATHS "${SDL12_VENDOR_DIR}/lib/${ARCH_DIR}" NO_DEFAULT_PATH)
        set(SDL_TTF_LIBRARY "${SDL_TTF_LIBRARY_BUNDLED}" CACHE FILEPATH "SDL TTF Library" FORCE)
    endif()
endif()

if(NOT SDL_TTF_INCLUDE_DIR OR NOT SDL_TTF_LIBRARY)
    find_package(PkgConfig QUIET)
    if(PKG_CONFIG_FOUND)
        pkg_check_modules(PKG_SDL_TTF QUIET SDL_ttf)
        if(PKG_SDL_TTF_FOUND)
            set(SDL_TTF_INCLUDE_DIR ${PKG_SDL_TTF_INCLUDE_DIRS})
            set(SDL_TTF_LIBRARY ${PKG_SDL_TTF_LIBRARIES})
        endif()
    endif()
endif()

if(NOT SDL_FOUND OR NOT SDL_LIBRARY)
    message(FATAL_ERROR "SDL 1.2 library not found. Please install libsdl1.2-dev or equivalent.")
endif()

if(NOT SDL_TTF_LIBRARY)
    message(FATAL_ERROR "SDL_ttf library not found. Please install libsdl-ttf2.0-dev or equivalent.")
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

target_compile_definitions(max_2_hours PRIVATE APP_VERSION="${APP_VERSION}")

target_include_directories(max_2_hours PRIVATE
    ${SDL_INCLUDE_DIR}
    ${SDL_TTF_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(max_2_hours PRIVATE
    ${SDL_LIBRARY}
    ${SDL_TTF_LIBRARY}
)

if(UNIX AND NOT APPLE)
    set_target_properties(max_2_hours PROPERTIES
        BUILD_WITH_INSTALL_RPATH TRUE
        INSTALL_RPATH "\$ORIGIN:\$ORIGIN/lib"
    )
endif()

if(WIN32)
    if(MSVC)
        find_library(LEGACY_STDIO_LIB legacy_stdio_definitions)
        if(LEGACY_STDIO_LIB)
            target_link_libraries(max_2_hours PRIVATE ${LEGACY_STDIO_LIB})
        endif()
        find_library(SDLMAIN_LIBRARY NAMES SDLmain PATHS "${SDL12_VENDOR_DIR}/lib/${ARCH_DIR}" ${SDL_INCLUDE_DIR}/../lib)
        if(SDLMAIN_LIBRARY)
            target_link_libraries(max_2_hours PRIVATE ${SDLMAIN_LIBRARY})
        endif()
    endif()
    if(MINGW)
        target_link_libraries(max_2_hours PRIVATE mingw32)
        target_link_options(max_2_hours PRIVATE -mwindows)
    endif()
endif()

# Copy assets to output directory
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(
        TARGET max_2_hours POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:max_2_hours>/assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/assets"
            "$<TARGET_FILE_DIR:max_2_hours>"
        COMMENT "Copying assets directory and files to output"
    )
endif()

# On Windows, also copy bundled DLLs next to the executable
if(WIN32 AND EXISTS "${SDL12_VENDOR_DIR}/lib/${ARCH_DIR}")
    file(GLOB BUNDLED_DLLS CONFIGURE_DEPENDS "${SDL12_VENDOR_DIR}/lib/${ARCH_DIR}/*.dll")
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

# Install rules
install(TARGETS max_2_hours DESTINATION bin)
install(DIRECTORY assets/ DESTINATION bin/assets)
install(DIRECTORY assets/ DESTINATION bin)
