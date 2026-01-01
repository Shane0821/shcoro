aux_source_directory(
  ${CMAKE_CURRENT_LIST_DIR} SRC
)

add_executable(
  stackless_test
  ${SRC}
)

target_link_libraries(
  stackless_test
  GTest::gtest_main
  shcoro
)

gtest_discover_tests(stackless_test)