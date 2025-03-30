cmake_minimum_required(VERSION 3.15...3.27)

# Ensure there is a working C++ compiler
project(HepEVD LANGUAGES CXX)

if (NOT SKBUILD)
    message(WARNING "\
        The 'HepEVD' extension requires the 'skbuild' package. \
        
        If you are a user trying to install this package, use the \
        command below, which will install all necessary build dependencies, \
        compile the package in an isolated environment, and then install it. \
        ==================================================================== \
        $ pip install . \
        ==================================================================== \
        If you are a developer trying to build the package, use the \
        command below, which will install all necessary build dependencies, \
        compile the package in an isolated environment, and then install it. \
        ==================================================================== \
        $ pip install nanobind scikit-build-core[pyproject] \
        $ pip install --no-build-isolation -ve . \
        ==================================================================== \
        You may optionally add -Ceditable.rebuild=true to auto-rebuild when \
        the package is imported. Otherwise, you need to rerun the above \
        after editing C++ files.")
endif()

if (CMAKE_VERSION VERSION_LESS 3.18)
    set (DEV_MODULE Development)
else()
    set (DEV_MODULE Development.Module)
endif()

# 3.8 is the minimum version for python, not the required version.
find_package(Python 3.8
    REQUIRED COMPONENTS Interpreter ${DEV_MODULE}
    OPTIONAL_COMPONENTS Development.SABIModule)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# Find nanobind
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/nanobind)

# Add HepEVD to the include path
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

# Define the path to the Web folder, so it can be found when installed.
add_compile_definitions(HEP_EVD_WEB_DIR="web")

# Finally, build the extension
nanobind_add_module(
    # Module Name
    _hepevd_impl

    # Target the stable ABI for Python 3.12+, which reduces
    # the number of binary wheels that must be built. This
    # does nothing on older Python versions
    STABLE_ABI

    # Source Code...
    src/include/array_list_utils.hpp
    src/array_list_utils.cpp

    # ... Global-state related code
    src/include/global.hpp
    src/global.cpp

    # ... Geometry related code
    src/include/geometry.hpp
    src/include/detectors.hpp
    src/geometry.cpp
    
    # ... Hit related code
    src/include/hits.hpp
    src/hits.cpp

    # ... Marker related code
    src/include/markers.hpp
    src/markers.cpp

    # Finally, the actual HepEVD Python Bindings
    src/HepEVD.cpp
)

# Install the library
install(TARGETS _hepevd_impl LIBRARY DESTINATION HepEVD)

# And install the web data
install(DIRECTORY ../web DESTINATION HepEVD)

# Add the python typing stub
nanobind_add_stub(
    _hepevd_impl_stub
    MODULE _hepevd_impl
    DEPENDS _hepevd_impl
    OUTPUT __init__.pyi
    PYTHON_PATH $<TARGET_FILE_DIR:_hepevd_impl>
    MARKER_FILE py.typed
    INCLUDE_PRIVATE true
    VERBOSE false
)

# Install the python typing stub
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/HepEVD/__init__.py" DESTINATION HepEVD)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/__init__.pyi DESTINATION HepEVD)
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/py.typed DESTINATION HepEVD)
