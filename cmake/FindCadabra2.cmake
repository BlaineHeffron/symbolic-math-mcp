# FindCadabra2.cmake
# Finds the Cadabra2 C++ library
#
# Sets:
#   Cadabra2_FOUND / CADABRA2_FOUND
#   CADABRA2_INCLUDE_DIRS
#   CADABRA2_LIBRARIES

find_path(CADABRA2_INCLUDE_DIR
    NAMES cadabra2/Kernel.hh
    PATHS
        /usr/include
        /usr/local/include
        $ENV{CADABRA2_DIR}/include
)

find_library(CADABRA2_LIBRARY
    NAMES cadabra2 cadabra2_cpp
    PATHS
        /usr/lib
        /usr/lib64
        /usr/local/lib
        /usr/local/lib64
        $ENV{CADABRA2_DIR}/lib
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Cadabra2
    REQUIRED_VARS CADABRA2_LIBRARY CADABRA2_INCLUDE_DIR
)

if(Cadabra2_FOUND)
    set(CADABRA2_FOUND TRUE)
    set(CADABRA2_INCLUDE_DIRS ${CADABRA2_INCLUDE_DIR})
    set(CADABRA2_LIBRARIES ${CADABRA2_LIBRARY})
endif()

mark_as_advanced(CADABRA2_INCLUDE_DIR CADABRA2_LIBRARY)
