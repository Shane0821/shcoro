# Define the library
add_executable(generator-demo)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} DEMO_SRC)
target_sources(generator-demo PRIVATE ${DEMO_SRC})

set_target_properties(generator-demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(generator-demo PRIVATE shcoro)