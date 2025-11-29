cmake_minimum_required(VERSION 3.14)
project(subcollider VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SUBCOLLIDER_BUILD_TESTS "Build unit tests" ON)
option(SUBCOLLIDER_BUILD_EXAMPLES "Build example applications" ON)
option(SUBCOLLIDER_BUILD_JACK_EXAMPLE "Build JACK audio example (requires JACK)" OFF)

# Header-only library
add_library(subcollider INTERFACE)
target_include_directories(subcollider INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Compiler warnings
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(subcollider INTERFACE
        $<BUILD_INTERFACE:-Wall -Wextra -Wpedantic>
    )
elseif(MSVC)
    target_compile_options(subcollider INTERFACE
        $<BUILD_INTERFACE:/W4>
    )
endif()

# Unit tests
if(SUBCOLLIDER_BUILD_TESTS)
    enable_testing()
    
    add_executable(subcollider_tests
        tests/test_main.cpp
        tests/test_types.cpp
        tests/test_sinosc.cpp
        tests/test_sawdpw.cpp
        tests/test_envelopear.cpp
        tests/test_lfnoise2.cpp
        tests/test_pan2.cpp
        tests/test_audioloop.cpp
        tests/test_examplevoice.cpp
        tests/test_moogladders.cpp
    )
    target_link_libraries(subcollider_tests PRIVATE subcollider)
    
    add_test(NAME subcollider_tests COMMAND subcollider_tests)

    # Benchmark executable
    add_executable(subcollider_benchmark tests/benchmark.cpp)
    target_link_libraries(subcollider_benchmark PRIVATE subcollider)
endif()

# Example applications
if(SUBCOLLIDER_BUILD_EXAMPLES)
    add_executable(example_basic examples/basic_example.cpp)
    target_link_libraries(example_basic PRIVATE subcollider)
endif()

# JACK example (optional)
if(SUBCOLLIDER_BUILD_JACK_EXAMPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JACK REQUIRED jack)
    
    add_executable(example_jack examples/jack_example.cpp)
    target_link_libraries(example_jack PRIVATE subcollider ${JACK_LIBRARIES})
    target_include_directories(example_jack PRIVATE ${JACK_INCLUDE_DIRS})
    target_compile_options(example_jack PRIVATE ${JACK_CFLAGS_OTHER})
endif()

# Installation
install(DIRECTORY include/ DESTINATION include)
install(TARGETS subcollider EXPORT subcolliderTargets)
install(EXPORT subcolliderTargets
    FILE subcolliderTargets.cmake
    NAMESPACE subcollider::
    DESTINATION lib/cmake/subcollider
)
