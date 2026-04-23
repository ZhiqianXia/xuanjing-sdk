add_library(xuanjing-eval STATIC
  src/eval_api_stub.cpp
  src/eval_api_impl.cpp
)


target_include_directories(xuanjing-eval
  PUBLIC
    include
)

target_link_libraries(xuanjing-eval
  PUBLIC
    xuanjing-runtime
)
