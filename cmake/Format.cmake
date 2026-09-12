# Developer convenience target: format_code.
#
# clang-format is resolved by find_program with HINTS so the target works
# whether or not Homebrew LLVM is on PATH (same search order as
# scripts/run_tidy.sh, and platform-aware like cmake/llvm-clang.cmake).
# clang-tidy itself lives in scripts/run_tidy.sh — it takes an explicit file
# list, so the per-file diagnostics you get from clangd in the editor stay the
# primary flow.

include_guard(GLOBAL)

# Resolve LLVM_PREFIX: explicit cache variable → $LLVM_PREFIX env → Homebrew
# prefixes → Windows installer prefix.
if(NOT DEFINED LLVM_PREFIX)
  if(DEFINED ENV{LLVM_PREFIX})
    set(LLVM_PREFIX "$ENV{LLVM_PREFIX}")
  elseif(EXISTS "/opt/homebrew/opt/llvm/bin/clang-format")
    set(LLVM_PREFIX "/opt/homebrew/opt/llvm")
  elseif(EXISTS "/usr/local/opt/llvm/bin/clang-format")
    set(LLVM_PREFIX "/usr/local/opt/llvm")
  elseif(EXISTS "$ENV{ProgramFiles}/LLVM/bin/clang-format.exe")
    set(LLVM_PREFIX "$ENV{ProgramFiles}/LLVM")
  else()
    set(LLVM_PREFIX "")
  endif()
endif()
set(LLVM_PREFIX "${LLVM_PREFIX}" CACHE PATH "LLVM install prefix")

find_program(LLVM_CLANG_FORMAT clang-format
    HINTS "${LLVM_PREFIX}/bin"
    DOC "clang-format from LLVM")

# Reformat every first-party source in place: `cmake --build --preset format`.
# clang-format is optional so a machine without LLVM can still configure and
# build; the target only works once clang-format is installed.
if(LLVM_CLANG_FORMAT)
  file(GLOB_RECURSE FORMAT_SOURCES
        "src/*.cpp"
        "src/*.hpp"
        "tests/*.cpp"
    )
  add_custom_target(format_code
        COMMAND ${LLVM_CLANG_FORMAT} -i ${FORMAT_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running ${LLVM_CLANG_FORMAT} on all source files"
        VERBATIM
    )
else()
  add_custom_target(format_code
        COMMAND ${CMAKE_COMMAND} -E echo
            "clang-format not found — install LLVM to use this target"
    )
endif()
