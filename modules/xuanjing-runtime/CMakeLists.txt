add_library(xuanjing-runtime STATIC
  src/runtime_api_stub.cpp
)


target_include_directories(xuanjing-runtime
  PUBLIC
    include
)
