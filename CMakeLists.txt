cmake_minimum_required(VERSION 3.15)
project(symbolic-math-mcp VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# --- Dependencies ---
include(FetchContent)

# nlohmann/json (header-only)
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(json)

# SymEngine — try system install first, fall back to FetchContent
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")
find_package(SymEngine QUIET)
if(NOT SYMENGINE_FOUND)
    message(STATUS "SymEngine not found on system — building from source via FetchContent")
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_BENCHMARKS OFF CACHE BOOL "" FORCE)
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(WITH_SYMENGINE_RCP ON CACHE BOOL "" FORCE)
    # Use GMP if available, otherwise fall back to Boost multiprecision
    find_path(GMP_HEADER_DIR gmp.h)
    if(GMP_HEADER_DIR)
        set(INTEGER_CLASS gmp CACHE STRING "" FORCE)
        set(WITH_BOOST OFF CACHE BOOL "" FORCE)
    else()
        set(INTEGER_CLASS boostmp CACHE STRING "" FORCE)
    endif()
    FetchContent_Declare(
        symengine
        GIT_REPOSITORY https://github.com/symengine/symengine.git
        GIT_TAG        v0.13.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(symengine)
    set(SYMENGINE_INCLUDE_DIRS
        ${symengine_SOURCE_DIR}
        ${symengine_BINARY_DIR}
    )
    set(SYMENGINE_LIBRARIES symengine)
endif()

# Cadabra2 (optional)
find_package(Cadabra2 QUIET)
if(Cadabra2_FOUND)
    add_compile_definitions(HAS_CADABRA)
    message(STATUS "Cadabra2 found — tensor algebra enabled")
else()
    message(STATUS "Cadabra2 not found — tensor algebra will use fallbacks")
endif()

# --- Main executable ---
add_executable(symbolic-math-mcp
    src/main.cpp
    src/mcp/protocol.cpp
    src/mcp/server.cpp
    src/engine/engine.cpp
    src/engine/parser.cpp
    src/engine/integrate.cpp
    src/engine/limits.cpp
    src/engine/output.cpp
    src/physics/physics.cpp
    src/physics/metric.cpp
    src/tensors/tensors.cpp
    src/tensors/cadabra_bridge.cpp
)

target_include_directories(symbolic-math-mcp PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SYMENGINE_INCLUDE_DIRS}
)

target_link_libraries(symbolic-math-mcp PRIVATE
    nlohmann_json::nlohmann_json
    ${SYMENGINE_LIBRARIES}
)

if(Cadabra2_FOUND)
    target_include_directories(symbolic-math-mcp PRIVATE ${CADABRA2_INCLUDE_DIRS})
    target_link_libraries(symbolic-math-mcp PRIVATE ${CADABRA2_LIBRARIES})
endif()

# --- Tests ---
option(SYMBOLIC_MATH_BUILD_TESTS "Build tests" ON)
if(SYMBOLIC_MATH_BUILD_TESTS)
    enable_testing()
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.14.0
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(googletest)

    # Shared test library (all source except main.cpp)
    add_library(symbolic-math-lib STATIC
        src/mcp/protocol.cpp
        src/mcp/server.cpp
        src/engine/engine.cpp
        src/engine/parser.cpp
        src/engine/integrate.cpp
        src/engine/limits.cpp
        src/engine/output.cpp
        src/physics/physics.cpp
        src/physics/metric.cpp
        src/tensors/tensors.cpp
        src/tensors/cadabra_bridge.cpp
    )
    target_include_directories(symbolic-math-lib PUBLIC
        ${CMAKE_SOURCE_DIR}/src
        ${SYMENGINE_INCLUDE_DIRS}
    )
    target_link_libraries(symbolic-math-lib PUBLIC
        nlohmann_json::nlohmann_json
        ${SYMENGINE_LIBRARIES}
    )
    if(Cadabra2_FOUND)
        target_include_directories(symbolic-math-lib PUBLIC ${CADABRA2_INCLUDE_DIRS})
        target_link_libraries(symbolic-math-lib PUBLIC ${CADABRA2_LIBRARIES})
    endif()

    # Test executables
    set(TEST_SOURCES
        tests/test_engine.cpp
        tests/test_parser.cpp
        tests/test_tensors.cpp
        tests/test_physics.cpp
        tests/test_mcp_protocol.cpp
    )

    foreach(test_src ${TEST_SOURCES})
        get_filename_component(test_name ${test_src} NAME_WE)
        add_executable(${test_name} ${test_src})
        target_link_libraries(${test_name} PRIVATE
            symbolic-math-lib
            GTest::gtest_main
        )
        add_test(NAME ${test_name} COMMAND ${test_name})
    endforeach()
endif()
