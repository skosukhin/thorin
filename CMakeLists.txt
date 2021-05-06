cmake_minimum_required(VERSION 3.13.4 FATAL_ERROR)

project(Thorin)

set(PACKAGE_VERSION "0.3.9")
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "limited config" FORCE)
set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS 1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#
# Options and option-like cache variables:
#
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(THORIN_PROFILE "profile complexity in thorin::HashTable - only works in Debug build" ON)

# Check for colored terminal:
set(COLOR_TTY_AVAILABLE TRUE)
if(WIN32)
    # Windows console does not support ANSI escape codes:
    set(COLOR_TTY_AVAILABLE FALSE)
endif()
set(COLORIZE_OUTPUT ${COLOR_TTY_AVAILABLE} CACHE BOOL "Set to true to enable colored error messages")

set(CMAKE_POSITION_INDEPENDENT_CODE FALSE CACHE BOOL "Set to true to enable PIC in static libraries")

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Debug or Release" FORCE)
elseif(NOT CMAKE_BUILD_TYPE STREQUAL "Debug" AND
       NOT CMAKE_BUILD_TYPE STREQUAL "Release")
    message(FATAL_ERROR "CMAKE_BUILD_TYPE is not recognized: ${CMAKE_BUILD_TYPE}")
endif()

#
# Dependencies:
#
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules")

# Check for Half library:
find_package(Half REQUIRED)

# Check for possible LLVM extension:
find_package(LLVM CONFIG)
if(LLVM_FOUND)
    message(STATUS "Found LLVM: ${LLVM_DIR} (found version \"${LLVM_PACKAGE_VERSION}\")")
    if(LLVM_LINK_LLVM_DYLIB)
        set(AnyDSL_LLVM_LINK_SHARED "USE_SHARED")
    endif()

    # Check for RV:
    find_package(RV)
    if(NOT RV_FOUND)
        message(STATUS "Building without RV. Install RV as part of LLVM.")
    endif()
else()
    message(STATUS "Building without LLVM and RV. Specify LLVM_DIR to compile with LLVM.")
endif()

#
# Build type and common flags:
#
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Using Debug flags: ${CMAKE_CXX_FLAGS_DEBUG}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Using Release flags: ${CMAKE_CXX_FLAGS_RELEASE}")
endif()

#
# Build and installation directory layout:
#
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(GNUInstallDirs)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON)
set(CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR})
add_subdirectory(src)

# Export target files
#   (enables find_package(Thorin) before the package is installed):
export(EXPORT thorin-targets
       NAMESPACE "AnyDSL::"
       FILE ${CMAKE_BINARY_DIR}/share/anydsl/cmake/thorin-targets.cmake
)

# Generate thorin-config.cmake:
include(CMakePackageConfigHelpers)
configure_package_config_file(
    cmake/thorin-config.cmake.in
    ${CMAKE_BINARY_DIR}/share/anydsl/cmake/thorin-config.cmake
    INSTALL_DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thorin/cmake
    NO_SET_AND_CHECK_MACRO
    NO_CHECK_REQUIRED_COMPONENTS_MACRO
)

# Generate thorin-config-version.cmake
#   (enables find_package(Thorin X.Y.Z)
write_basic_package_version_file(
    ${CMAKE_BINARY_DIR}/share/anydsl/cmake/thorin-config-version.cmake
    VERSION ${PACKAGE_VERSION}
    COMPATIBILITY AnyNewerVersion
)

# Install the config files
#   (the destination enables find_package(Thorin) with
#    CMAKE_PREFIX_PATH=/prefix/to/thorin):
install(FILES
        ${CMAKE_BINARY_DIR}/share/anydsl/cmake/thorin-config.cmake
        ${CMAKE_BINARY_DIR}/share/anydsl/cmake/thorin-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thorin/cmake
)

# Install the exported target files
#   (enables find_package(Thorin) after the package is installed):
install(EXPORT thorin-targets
        NAMESPACE "AnyDSL::"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/thorin/cmake
)
