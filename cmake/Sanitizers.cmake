# cmake/Sanitizers.cmake — AddressSanitizer / UBSan / ThreadSanitizer helpers.

option(XUANJING_ENABLE_ASAN   "Enable AddressSanitizer"           OFF)
option(XUANJING_ENABLE_UBSAN  "Enable UndefinedBehaviorSanitizer" OFF)
option(XUANJING_ENABLE_TSAN   "Enable ThreadSanitizer"            OFF)

function(xuanjing_enable_sanitizers target)
  if(XUANJING_ENABLE_TSAN AND (XUANJING_ENABLE_ASAN OR XUANJING_ENABLE_UBSAN))
    message(FATAL_ERROR "TSan is incompatible with ASan/UBSan.")
  endif()

  if(XUANJING_ENABLE_ASAN)
    target_compile_options(${target} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
    target_link_options(${target}    PRIVATE -fsanitize=address)
  endif()

  if(XUANJING_ENABLE_UBSAN)
    target_compile_options(${target} PRIVATE -fsanitize=undefined)
    target_link_options(${target}    PRIVATE -fsanitize=undefined)
  endif()

  if(XUANJING_ENABLE_TSAN)
    target_compile_options(${target} PRIVATE -fsanitize=thread)
    target_link_options(${target}    PRIVATE -fsanitize=thread)
  endif()
endfunction()
