add_library(xuanjing-genframe STATIC
  src/genframe_api_stub.cpp
)

target_include_directories(xuanjing-genframe
  PUBLIC
    include
)
