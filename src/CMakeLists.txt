
project(openlmi-networking C)

cmake_minimum_required(VERSION 2.6)

set(OPENLMI_VERSION_MAJOR 0)
set(OPENLMI_VERSION_MINOR 2)
set(OPENLMI_VERSION_REVISION 2)
set(OPENLMI_VERSION "${OPENLMI_VERSION_MAJOR}.${OPENLMI_VERSION_MINOR}.${OPENLMI_VERSION_REVISION}")

# Create tarball from latest git (including uncommitted changes)
add_custom_target(dist
    ${CMAKE_SOURCE_DIR}/make-release.sh
    COMMENT "Creating release tarball")

# Create rpm package from latest git (including uncommitted changes)
add_custom_target(rpm
    ${CMAKE_SOURCE_DIR}/make-rpm.sh
    COMMENT "Creating rpm package")

set(DOCDIR "share/doc/openlmi-networking" CACHE STRING "Directory where should be the documentation installed")

option(DOC_ONLY "Just generate documentation, don't build the provider" OFF)
if (DOC_ONLY)
    add_subdirectory(doc)
    return()
endif (DOC_ONLY)

# Set flags and definitions
add_definitions(-D_XOPEN_SOURCE=500 -D_GNU_SOURCE)
set(CMAKE_C_FLAGS "-std=c99 -Wall -pedantic -g -Wextra -Wno-unused-parameter -Wformat -Wparentheses -Wl,--no-undefined ${CMAKE_C_FLAGS}")

option(DEBUG "Enable debugging mode" OFF)
if (DEBUG)
    add_definitions("-DDEBUG_ENABLED")
endif (DEBUG)

# Set LIB_SUFFIX to 64 on 64bit architectures
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX "")
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    SET(LIB_SUFFIX 64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

# Find OpenLMIMacros when installed in other prefix than /usr (e.g. /usr/local)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_INSTALL_PREFIX})
include(OpenLMIMacros RESULT_VARIABLE LMIMACROS)

if (${LMIMACROS} STREQUAL "NOTFOUND")
    message(FATAL_ERROR "OpenLMIMacros.cmake not found, check if openlmi-providers(-devel) is installed")
endif (${LMIMACROS} STREQUAL "NOTFOUND")

find_package(PkgConfig)

# Find required packages
find_package(CMPI REQUIRED)
find_package(KonkretCMPI REQUIRED)
find_package(OpenLMI REQUIRED)

pkg_check_modules(NM REQUIRED NetworkManager)
pkg_check_modules(DBUSGLIB REQUIRED dbus-glib-1)
pkg_check_modules(GLIB REQUIRED glib-2.0)
pkg_check_modules(UUID REQUIRED uuid)
pkg_check_modules(CHECK REQUIRED check)

add_subdirectory(doc)
add_subdirectory(src)
add_subdirectory(mof)
add_subdirectory(test)

add_custom_target(test
    ${CMAKE_BINARY_DIR}/src/unittest/unittest
    COMMAND python ${CMAKE_SOURCE_DIR}/test/run_tests.py -f -l ${CMAKE_BINARY_DIR}/src/
    DEPENDS ${LIBRARY_NAME}
)
