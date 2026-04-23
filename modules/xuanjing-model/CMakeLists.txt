add_library(xuanjing-model STATIC
  src/model_api_stub.cpp
)

target_include_directories(xuanjing-model
  PUBLIC
    include
)
