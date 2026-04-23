cmake_minimum_required(VERSION 3.20)
project(xuanjing_sdk VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Sanitizers)
include(Coverage)
include(CompilerWarnings)

option(XUANJING_BUILD_SAMPLES "Build sample applications" OFF)
option(XUANJING_BUILD_TESTS "Build tests" OFF)

add_subdirectory(modules/xuanjing-runtime)
add_subdirectory(modules/xuanjing-upscale)
add_subdirectory(modules/xuanjing-genframe)
add_subdirectory(modules/xuanjing-temporal)
add_subdirectory(modules/xuanjing-tensor)
add_subdirectory(modules/xuanjing-shader)
add_subdirectory(modules/xuanjing-eval)
add_subdirectory(modules/xuanjing-train)
add_subdirectory(modules/xuanjing-platform)

if(XUANJING_BUILD_SAMPLES)
  add_subdirectory(samples)
endif()

if(XUANJING_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
