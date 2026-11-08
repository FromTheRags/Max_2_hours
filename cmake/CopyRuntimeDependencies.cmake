cmake_policy(SET CMP0207 NEW)

if(NOT DEFINED EXECUTABLE OR NOT DEFINED DESTINATION)
    message(FATAL_ERROR "EXECUTABLE and DESTINATION must be set.")
endif()

if(DEFINED MINGW_BIN_DIR)
    set(DIRS "${MINGW_BIN_DIR}")
else()
    set(DIRS "")
endif()

file(GET_RUNTIME_DEPENDENCIES
    EXECUTABLES "${EXECUTABLE}"
    DIRECTORIES ${DIRS}
    RESOLVED_DEPENDENCIES_VAR RUNTIME_DEPENDENCIES
    UNRESOLVED_DEPENDENCIES_VAR UNRESOLVED_DEPENDENCIES
    PRE_EXCLUDE_REGEXES
        "^api-ms-.*"
        "^ext-ms-.*"
        "^ieshims\\.dll"
        "^emclient\\.dll"
        "^devobj\\.dll"
    POST_EXCLUDE_REGEXES
        ".*[/\\\\][Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\].*"
)

foreach(DEPENDENCY IN LISTS RUNTIME_DEPENDENCIES)
    if(EXISTS "${DEPENDENCY}")
        message(STATUS "Copying dependency: ${DEPENDENCY}")
        file(COPY "${DEPENDENCY}" DESTINATION "${DESTINATION}")
    endif()
endforeach()
