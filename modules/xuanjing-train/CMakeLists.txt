add_library(xuanjing-train STATIC
  src/train_api_stub.cpp
)

target_include_directories(xuanjing-train
  PUBLIC
    include
)
