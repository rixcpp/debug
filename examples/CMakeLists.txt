add_executable(rix_debug_basic
  basic.cpp
)

target_link_libraries(rix_debug_basic
  PRIVATE
    rix::debug
)
