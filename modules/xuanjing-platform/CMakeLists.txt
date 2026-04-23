add_library(xuanjing-platform STATIC
  src/platform_api_stub.cpp
)

target_include_directories(xuanjing-platform
  PUBLIC
    include
)
