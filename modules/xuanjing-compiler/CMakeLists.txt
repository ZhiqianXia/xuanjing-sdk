add_library(xuanjing-compiler STATIC
  src/compiler_api_stub.cpp
)

target_include_directories(xuanjing-compiler
  PUBLIC
    include
)

target_link_libraries(xuanjing-compiler
  PRIVATE
    xuanjing-platform
)
