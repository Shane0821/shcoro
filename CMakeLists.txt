cmake_minimum_required(VERSION 3.2)
PROJECT(shcoro VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options(-fcoroutines)
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    add_compile_options(/await)
endif()

# Options to build demo and test
option(SHCORO_BUILD_DEMO "Build the demo" OFF)
option(SHCORO_BUILD_TEST "Build the test" OFF)

# Option to enable logging (default: ON for Debug, OFF for Release)
option(SHCORO_ENABLE_LOG "Enable debug log" OFF)

# ============================
# Directories
# ============================

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(SRC_DIR "${ROOT_DIR}/src")
set(INCLUDE_DIR "${ROOT_DIR}/include")
set(DEMO_DIR "${ROOT_DIR}/demo")
set(TEST_DIR "${ROOT_DIR}/test")
set(THIRD_PARTY_DIR "${ROOT_DIR}/3rd")
set(CONFIG_DIR "${ROOT_DIR}/config")
set(CMAKE_DIR "${ROOT_DIR}/cmake")

# ============================
# Libraries
# ============================

include(FetchContent)


# ============================
# Pakcages
# ============================


# ============================
# Files
# ============================


# ============================
# Build
# ============================

# Create different configurations
if(SHCORO_ENABLE_LOG)
    add_compile_definitions(SHCORO_ENABLE_LOG)
    message(STATUS "Logging enabled")
else()
    message(STATUS "Logging disabled")
endif()

# add_subdirectory(libs)
add_subdirectory(src)

# Conditionally build demo
if (SHCORO_BUILD_DEMO)
    add_subdirectory(demo)
endif()

# Conditionally build test
if (SHCORO_BUILD_TEST)
    add_subdirectory(test)
endif()

# ============================
# Export
# ============================

add_library(shcoro::shcoro ALIAS shcoro)

set(targets_export_name shcoro-targets)

include(GNUInstallDirs)

# Install library and headers
install(TARGETS shcoro
    EXPORT ${targets_export_name}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

# Export targets (to build tree for development) and install tree for consumers
export(EXPORT ${targets_export_name}
    NAMESPACE shcoro::
    FILE "${CMAKE_CURRENT_BINARY_DIR}/${targets_export_name}.cmake"
)

# Generate Config and Version files
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/shcoro-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_DIR}/shcoro-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/shcoro-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shcoro
)

# Install exported targets and config files
install(EXPORT ${targets_export_name}
    NAMESPACE shcoro::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shcoro
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/shcoro-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/shcoro-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/shcoro
)