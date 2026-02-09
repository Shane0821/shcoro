# Define the library
add_executable(async-demo-8)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} DEMO_SRC)
target_sources(async-demo-8 PRIVATE ${DEMO_SRC})

set_target_properties(async-demo-8 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(async-demo-8 PRIVATE shcoro)