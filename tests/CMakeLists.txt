add_executable(rix_debug_tests
  debug_tests.cpp
)

target_link_libraries(rix_debug_tests
  PRIVATE
    rix::debug
)

add_test(
  NAME rix_debug_tests
  COMMAND rix_debug_tests
)
