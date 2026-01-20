# Define the library
add_executable(async-scheduler-demo)

aux_source_directory(${CMAKE_CURRENT_LIST_DIR} DEMO_SRC)
target_sources(async-scheduler-demo PRIVATE ${DEMO_SRC})

set_target_properties(async-scheduler-demo PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED YES
)

target_link_libraries(async-scheduler-demo PRIVATE shcoro)