# cmake/CompilerWarnings.cmake — Enable strict warning flags.

option(XUANJING_ENABLE_WARNINGS "Enable strict compiler warnings" OFF)

function(xuanjing_enable_warnings target)
  if(NOT XUANJING_ENABLE_WARNINGS)
    return()
  endif()
  target_compile_options(${target} PRIVATE
    -Wall -Wextra -Wpedantic
    -Wshadow -Wnon-virtual-dtor -Wcast-align
    -Woverloaded-virtual -Wconversion -Wsign-conversion
    -Wnull-dereference -Wdouble-promotion -Wformat=2
    -Werror
  )
endfunction()
