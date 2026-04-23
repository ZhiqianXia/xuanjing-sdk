add_library(xuanjing-tensor STATIC
  src/tensor_api_stub.cpp
)

target_include_directories(xuanjing-tensor
  PUBLIC
    include
)

target_link_libraries(xuanjing-tensor
  PUBLIC
    xuanjing-model
)
