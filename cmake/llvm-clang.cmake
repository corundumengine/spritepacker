# ─────────────────────────────────────────────────────────────────────────────
# Optional LLVM/Clang toolchain pin.
#
# Referenced from every CMakePresets.json entry via
#   "CMAKE_TOOLCHAIN_FILE": "${sourceDir}/cmake/llvm-clang.cmake"
#
# CMake loads this file before project() runs. If an LLVM install is found the
# C++ compiler is pinned to its clang++; otherwise nothing is set and CMake
# falls back to its normal platform detection (MSVC, GCC, or system Clang).
# Override explicitly with -DLLVM_PREFIX=<prefix> or -DCMAKE_CXX_COMPILER=<path>.
# ─────────────────────────────────────────────────────────────────────────────

if(DEFINED _SPRITEPACKER_LLVM_CLANG_CMAKE_LOADED)
  return()
endif()
set(_SPRITEPACKER_LLVM_CLANG_CMAKE_LOADED TRUE)

# Explicit request: cache variable first, then environment.
set(_spritepacker_llvm_bin "")
if(DEFINED LLVM_PREFIX AND NOT "${LLVM_PREFIX}" STREQUAL "" AND NOT DEFINED ENV{LLVM_PREFIX})
  set(_spritepacker_llvm_bin "${LLVM_PREFIX}/bin")
elseif(DEFINED ENV{LLVM_PREFIX} AND NOT "$ENV{LLVM_PREFIX}" STREQUAL "")
  set(_spritepacker_llvm_bin "$ENV{LLVM_PREFIX}/bin")
endif()

# Well-known install prefixes: Homebrew (macOS) and the LLVM installer (Windows).
if(NOT _spritepacker_llvm_bin)
  foreach(_prefix
      "/opt/homebrew/opt/llvm"
      "/usr/local/opt/llvm"
      "$ENV{ProgramFiles}/LLVM")
    if(EXISTS "${_prefix}/bin/clang++")
      set(_spritepacker_llvm_bin "${_prefix}/bin")
      break()
    endif()
  endforeach()
endif()

# Linux distros install versioned LLVM packages; ask llvm-config where it lives.
if(NOT _spritepacker_llvm_bin)
  find_program(_spritepacker_llvm_config
      NAMES llvm-config llvm-config-21 llvm-config-20 llvm-config-19 llvm-config-18)
  if(_spritepacker_llvm_config)
    execute_process(
        COMMAND "${_spritepacker_llvm_config}" --bindir
        OUTPUT_VARIABLE _spritepacker_llvm_bin
        OUTPUT_STRIP_TRAILING_WHITESPACE)
  endif()
endif()

# Last resort: an llvm clang++ already on PATH.
if(NOT _spritepacker_llvm_bin)
  find_program(_spritepacker_clangxx NAMES clang++)
  if(_spritepacker_clangxx)
    get_filename_component(_spritepacker_llvm_bin "${_spritepacker_clangxx}" DIRECTORY)
  endif()
endif()

if(_spritepacker_llvm_bin AND EXISTS "${_spritepacker_llvm_bin}/clang++")
  set(CMAKE_CXX_COMPILER "${_spritepacker_llvm_bin}/clang++" CACHE FILEPATH "C++ compiler")
  message(STATUS "LLVM toolchain: ${_spritepacker_llvm_bin}")
else()
  message(STATUS "LLVM toolchain: not found — using the default C++ compiler")
endif()
