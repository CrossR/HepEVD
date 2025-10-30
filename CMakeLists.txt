# CMake configuration for HepEVD.
#
# This isn't necessary to use HepEVD, if you just want to use the library,
# follow the instructions in the README.md file / HepEVD wiki.
#
# This file does however, mean you can integrate HepEVD into your own CMake
# project and pull HepEVD down via fetch_content.
#
# See the Project Integration section of the README.md for more information.

cmake_minimum_required(VERSION 3.14) # Required for FetchContent

project(HepEVD
  VERSION 0.4.0
  LANGUAGES CXX
  DESCRIPTION "HepEVD - A C++ & Web-based Event Display for HEP Events"
)

# This is a header only library, so we don't need to build anything
add_library(HepEVD INTERFACE)

# However, we do need to make sure the required dependencies are available.
# This is achieved using the get_extern_deps.sh script.
message(STATUS "Fetching external dependencies for HepEVD")
set(SETUP_SCRIPT ${CMAKE_CURRENT_SOURCE_DIR}/get_extern_deps.sh)
execute_process(
    COMMAND ${SETUP_SCRIPT}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    OUTPUT_QUIET
    ERROR_QUIET
)

# So just include the headers
target_include_directories(HepEVD INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

# Set a C++17 requirement
target_compile_features(HepEVD INTERFACE cxx_std_17)
