cmake_minimum_required(VERSION 3.14)
project(subcollider VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Options
option(SUBCOLLIDER_BUILD_TESTS "Build unit tests" ON)
option(SUBCOLLIDER_BUILD_EXAMPLES "Build example applications" ON)
option(SUBCOLLIDER_BUILD_JACK_EXAMPLE "Build JACK audio example (requires JACK)" OFF)
option(SUBCOLLIDER_BUILD_JACK_PLAYBACK_EXAMPLE "Build JACK playback example (requires JACK and libsndfile)" OFF)
option(SUBCOLLIDER_BUILD_MOOG_EXAMPLE "Build Moog filter example (requires JACK and X11)" OFF)
option(SUBCOLLIDER_BUILD_SUPERSAW_EXAMPLE "Build SuperSaw example (requires JACK and X11)" OFF)

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
        tests/test_lftri.cpp
        tests/test_envelopear.cpp
        tests/test_envelopeadsr.cpp
        tests/test_lfnoise2.cpp
        tests/test_pan2.cpp
        tests/test_balance2.cpp
        tests/test_audioloop.cpp
        tests/test_examplevoice.cpp
        tests/test_moogladders.cpp
        tests/test_xline.cpp
        tests/test_phasor.cpp
        tests/test_supersaw.cpp
        tests/test_downsampler.cpp
        tests/test_buffer.cpp
        tests/test_bufferallocator.cpp
        tests/test_bufrd.cpp
        tests/test_bufrd_phasor.cpp
        tests/test_playback_oversampling.cpp
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

# JACK playback example (optional)
if(SUBCOLLIDER_BUILD_JACK_PLAYBACK_EXAMPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JACK REQUIRED jack)
    pkg_check_modules(SNDFILE REQUIRED sndfile)

    add_executable(example_jack_playback examples/jack_playback_example.cpp)
    target_link_libraries(example_jack_playback PRIVATE subcollider ${JACK_LIBRARIES} ${SNDFILE_LIBRARIES})
    target_include_directories(example_jack_playback PRIVATE ${JACK_INCLUDE_DIRS} ${SNDFILE_INCLUDE_DIRS})
    target_compile_options(example_jack_playback PRIVATE ${JACK_CFLAGS_OTHER} ${SNDFILE_CFLAGS_OTHER})
endif()

# Moog example (optional, requires JACK and X11)
if(SUBCOLLIDER_BUILD_MOOG_EXAMPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JACK REQUIRED jack)
    find_package(X11 REQUIRED)

    add_executable(example_moog examples/moog_example.cpp)
    target_link_libraries(example_moog PRIVATE subcollider ${JACK_LIBRARIES} ${X11_LIBRARIES})
    target_include_directories(example_moog PRIVATE ${JACK_INCLUDE_DIRS} ${X11_INCLUDE_DIR})
    target_compile_options(example_moog PRIVATE ${JACK_CFLAGS_OTHER})
endif()

# SuperSaw example (optional, requires JACK and X11)
if(SUBCOLLIDER_BUILD_SUPERSAW_EXAMPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(JACK REQUIRED jack)
    find_package(X11 REQUIRED)

    add_executable(example_supersaw examples/supersaw_example.cpp)
    target_link_libraries(example_supersaw PRIVATE subcollider ${JACK_LIBRARIES} ${X11_LIBRARIES})
    target_include_directories(example_supersaw PRIVATE ${JACK_INCLUDE_DIRS} ${X11_INCLUDE_DIR})
    target_compile_options(example_supersaw PRIVATE ${JACK_CFLAGS_OTHER})
endif()

# Installation
install(DIRECTORY include/ DESTINATION include)
install(TARGETS subcollider EXPORT subcolliderTargets)
install(EXPORT subcolliderTargets
    FILE subcolliderTargets.cmake
    NAMESPACE subcollider::
    DESTINATION lib/cmake/subcollider
)
