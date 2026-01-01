# Define the library
add_library(shcoro)

target_include_directories(shcoro
    PUBLIC
    $<BUILD_INTERFACE:${INCLUDE_DIR}>
    $<INSTALL_INTERFACE:include>
)

set_target_properties(shcoro PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(shcoro pthread)

add_subdirectory(stackless)