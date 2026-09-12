# Per-target compiler settings for first-party targets: language standard and
# warning policy.
#
# -Werror is mandatory for first-party C++: with the compiler pinned to LLVM
# Clang (see cmake/llvm-clang.cmake), warnings are treated as errors in CI and
# local builds alike. These flags are applied per-target via
# `spritepacker_enable_warnings(<target>)` so third-party dependencies and
# generated code are unaffected.
#
# The C++ standard is set the same way: `spritepacker_use_cxx23(<target>)`
# marks the feature PUBLIC so it travels with the libraries, instead of relying
# on global CMAKE_CXX_STANDARD.

include_guard(GLOBAL)

function(spritepacker_enable_warnings target)
  if(NOT MSVC)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic -Werror)
  endif()
endfunction()

function(spritepacker_use_cxx23 target)
  target_compile_features(${target} PUBLIC cxx_std_23)
endfunction()
