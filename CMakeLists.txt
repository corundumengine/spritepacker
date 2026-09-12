cmake_minimum_required(VERSION 3.28)
project(
  Spritepacker
  VERSION 0.4.0
  LANGUAGES CXX
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ccache (auto-detect; transparent to the build)
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
  set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}" CACHE FILEPATH "ccache launcher for C++")
endif()

option(SPRITEPACKER_BUILD_TESTS "Build unit tests" ON)

# Developer targets (format_code). Included before Dependencies.cmake so a
# missing clang-format is reported before the dependency fetches.
include(cmake/Format.cmake)
include(cmake/Dependencies.cmake)
include(cmake/CompilerWarnings.cmake)

include(GNUInstallDirs)
find_package(Threads REQUIRED)

configure_file(src/version.hpp.in src/version.hpp)

add_library(lodepng_lib STATIC ${lodepng_SOURCE_DIR}/lodepng.cpp)
target_include_directories(lodepng_lib PUBLIC ${lodepng_SOURCE_DIR})

add_library(spritepacker_lib STATIC
    src/options.cpp
    src/sprites.cpp
    src/utils.cpp
    src/rect_packer.cpp
    src/sprite_packer.cpp)
target_include_directories(spritepacker_lib PUBLIC src ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(spritepacker_lib PUBLIC nlohmann_json::nlohmann_json lodepng_lib Threads::Threads)
spritepacker_use_cxx23(spritepacker_lib)
spritepacker_enable_warnings(spritepacker_lib)

add_executable(spritepacker src/main.cpp)
target_link_libraries(spritepacker PRIVATE spritepacker_lib)
spritepacker_use_cxx23(spritepacker)
spritepacker_enable_warnings(spritepacker)

install(TARGETS spritepacker RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

include(CTest)

if(SPRITEPACKER_BUILD_TESTS)
  add_executable(spritepacker_tests
        tests/test_main.cpp
        tests/test_utils.cpp
        tests/test_sprites.cpp
        tests/test_rect_packer.cpp
        tests/test_sprite_packer.cpp
        tests/test_options.cpp)
  target_include_directories(spritepacker_tests PRIVATE src)
  target_link_libraries(spritepacker_tests PRIVATE spritepacker_lib doctest::doctest)
  spritepacker_use_cxx23(spritepacker_tests)
  spritepacker_enable_warnings(spritepacker_tests)
  target_compile_definitions(spritepacker_tests PRIVATE
        TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")
  add_test(NAME spritepacker_tests COMMAND spritepacker_tests)
endif()
