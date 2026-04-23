add_library(xuanjing-genframe STATIC
  src/genframe_api_stub.cpp
)

target_include_directories(xuanjing-genframe
  PUBLIC
    include
)

target_link_libraries(xuanjing-genframe
  PUBLIC
    xuanjing-runtime
)
