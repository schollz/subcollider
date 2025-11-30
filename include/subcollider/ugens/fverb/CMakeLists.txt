cmake_minimum_required(VERSION 3.14)
set(CMAKE_CXX_STANDARD 17)

# FVerb library - provides the FVerb UGen implementation
add_library(fverb STATIC FVerb.cpp)

# Include paths for subcollider types
target_include_directories(fverb PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../../..>  # Root include directory
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>  # fverb directory for FVerbDSP.h
    $<INSTALL_INTERFACE:include>
)

target_compile_options(fverb PRIVATE -O3)
