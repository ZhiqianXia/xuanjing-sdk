add_library(xuanjing-shader STATIC
  src/shader_api_stub.cpp
)

target_include_directories(xuanjing-shader
  PUBLIC
    include
)
