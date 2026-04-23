# cmake/Coverage.cmake — Code coverage via gcov/lcov.

option(XUANJING_ENABLE_COVERAGE "Enable code coverage instrumentation" OFF)

function(xuanjing_enable_coverage target)
  if(NOT XUANJING_ENABLE_COVERAGE)
    return()
  endif()
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    message(WARNING "Coverage only supported with GCC or Clang.")
    return()
  endif()
  target_compile_options(${target} PRIVATE --coverage -O0 -g)
  target_link_options(${target}    PRIVATE --coverage)
endfunction()
