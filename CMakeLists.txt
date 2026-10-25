cmake_minimum_required(VERSION 3.20)
project(magicians_saga_research VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
# Preserve the existing build/desktop/*.exe and build/viewer/*.exe interfaces.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
include(cmake/ProjectTargets.cmake)
include(CTest)

add_subdirectory(src/assets)
add_subdirectory(src/core)
add_subdirectory(src/platform/desktop)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
