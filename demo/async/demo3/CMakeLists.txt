# Define the library
add_executable(async-demo-3)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} DEMO_SRC)
target_sources(async-demo-3 PRIVATE ${DEMO_SRC})

set_target_properties(async-demo-3 PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(async-demo-3 PRIVATE shcoro)