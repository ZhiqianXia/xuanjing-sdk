add_library(xuanjing-temporal STATIC
  src/temporal_api_stub.cpp
)

target_include_directories(xuanjing-temporal
  PUBLIC
    include
)
