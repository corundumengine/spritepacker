# Third-party dependencies.
#
# Everything is fetched at configure time via FetchContent (no find_package).
# FETCHCONTENT_QUIET is forced to FALSE so dependency configure logs stay
# visible.

include_guard(GLOBAL)

set(FETCHCONTENT_QUIET FALSE)
include(FetchContent)

FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.12.0
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
set(JSON_Install OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json)

FetchContent_Declare(lodepng
    GIT_REPOSITORY https://github.com/lvandeve/lodepng.git
    GIT_TAG        ed6fe5825c6a4fbb7f58ab35a4231c7543cd452a
    GIT_SHALLOW    TRUE
    GIT_PROGRESS   TRUE)
FetchContent_MakeAvailable(lodepng)

if(SPRITEPACKER_BUILD_TESTS)
  set(DOCTEST_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(doctest
      GIT_REPOSITORY https://github.com/doctest/doctest.git
      GIT_TAG        v2.5.3
      GIT_SHALLOW    TRUE
      GIT_PROGRESS   TRUE
      EXCLUDE_FROM_ALL)
  FetchContent_MakeAvailable(doctest)
endif()
