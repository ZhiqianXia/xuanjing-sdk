add_library(xuanjing-upscale STATIC
  src/upscale_api_stub.cpp
)

target_include_directories(xuanjing-upscale
  PUBLIC
    include
)
