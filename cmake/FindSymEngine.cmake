# FindSymEngine.cmake
# Finds the SymEngine library
#
# Sets:
#   SYMENGINE_FOUND
#   SYMENGINE_INCLUDE_DIRS
#   SYMENGINE_LIBRARIES
#   SYMENGINE_VERSION

# Try CMake config first (preferred)
find_package(SymEngine CONFIG QUIET)
if(SymEngine_FOUND OR SYMENGINE_FOUND)
    # SymEngine's own CMake config was found
    if(NOT SYMENGINE_INCLUDE_DIRS)
        set(SYMENGINE_INCLUDE_DIRS ${SYMENGINE_INCLUDE_DIR})
    endif()
    if(NOT SYMENGINE_LIBRARIES)
        set(SYMENGINE_LIBRARIES symengine)
    endif()
    set(SYMENGINE_FOUND TRUE)
    return()
endif()

# Manual search
find_path(SYMENGINE_INCLUDE_DIR
    NAMES symengine/expression.h
    PATHS
        /usr/include
        /usr/local/include
        $ENV{SYMENGINE_DIR}/include
        $ENV{HOME}/.local/include
)

find_library(SYMENGINE_LIBRARY
    NAMES symengine
    PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        $ENV{SYMENGINE_DIR}/lib
        $ENV{HOME}/.local/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SymEngine
    REQUIRED_VARS SYMENGINE_LIBRARY SYMENGINE_INCLUDE_DIR
)

if(SYMENGINE_FOUND)
    set(SYMENGINE_INCLUDE_DIRS ${SYMENGINE_INCLUDE_DIR})
    set(SYMENGINE_LIBRARIES ${SYMENGINE_LIBRARY})
endif()

mark_as_advanced(SYMENGINE_INCLUDE_DIR SYMENGINE_LIBRARY)
