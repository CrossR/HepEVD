cmake_minimum_required(VERSION 3.15...3.27)
project(HepEVD)

if (CMAKE_VERSION VERSION_LESS 3.18)
    set (DEV_MODULE Development)
else()
    set (DEV_MODULE Development.Module)
endif()

# 3.8 is the minimum version for python, not the required version.
find_package(Python 3.8 REQUIRED COMPONENTS Interpreter ${DEV_MODULE})

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Find nanobind
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/nanobind)

# Add HepEVD to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Finally, build the extension
nanobind_add_module(hep_evd_two bindings/basics.cpp)