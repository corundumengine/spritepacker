cmake_minimum_required(VERSION 3.28)
project(
  Spritepacker
  VERSION 0.1.0
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(lodepng
    GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
    GIT_TAG        ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
FetchContent_MakeAvailable(lodepng)

FetchContent_Declare(doctest
    GIT_REPOSITORY https://github.com/doctest/doctest.git
    GIT_TAG        v2.4.11
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
FetchContent_MakeAvailable(doctest)

configure_file(src/version.hpp.in src/version.hpp)

set(WARN_FLAGS -Wall -Wextra -Wpedantic)

add_library(lodepng_lib STATIC ${lodepng_SOURCE_DIR}/lodepng.cpp)
target_include_directories(lodepng_lib PUBLIC ${lodepng_SOURCE_DIR})

add_library(spritepacker_lib STATIC
    src/options.cpp
    src/sprites.cpp
    src/utils.cpp
    src/sprite_packer.cpp)
target_include_directories(spritepacker_lib PUBLIC src ${CMAKE_CURRENT_BINARY_DIR}/src)
target_link_libraries(spritepacker_lib PUBLIC nlohmann_json::nlohmann_json lodepng_lib)
target_compile_options(spritepacker_lib PRIVATE ${WARN_FLAGS})

add_executable(spritepacker src/main.cpp)
target_link_libraries(spritepacker PRIVATE spritepacker_lib)
target_compile_options(spritepacker PRIVATE ${WARN_FLAGS})

include(CTest)
enable_testing()

if(BUILD_TESTING)
  add_executable(spritepacker_tests
        tests/test_main.cpp
        tests/test_utils.cpp
        tests/test_sprites.cpp
        tests/test_sprite_packer.cpp
        tests/test_options.cpp)
  target_include_directories(spritepacker_tests PRIVATE src)
  target_link_libraries(spritepacker_tests PRIVATE spritepacker_lib doctest::doctest)
  target_compile_options(spritepacker_tests PRIVATE ${WARN_FLAGS})
  target_compile_definitions(spritepacker_tests PRIVATE
        TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures")
  add_test(NAME spritepacker_tests COMMAND spritepacker_tests)
endif()
