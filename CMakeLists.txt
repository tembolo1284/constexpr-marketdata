cmake_minimum_required(VERSION 3.25)
project(constexpr-marketdata LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(ReflectionSupport)
include(FetchContent)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

ctmd_require_reflection()          # hard-errors on a compiler without -freflection

FetchContent_Declare(simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG        master             # pin to a release tag once one carries reflection
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(simdjson)

set(CTMD_SNAPSHOT "${CMAKE_CURRENT_SOURCE_DIR}/data/market_snapshot.json"
    CACHE FILEPATH "JSON document baked into the binary")

# ---- the reusable piece: header-only, carries the compile-time cost ----------
add_library(ctmd INTERFACE)
target_include_directories(ctmd INTERFACE
  "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(ctmd INTERFACE
  SIMDJSON_STATIC_REFLECTION=1
  CTMD_SNAPSHOT_PATH="${CTMD_SNAPSHOT}")
target_compile_options(ctmd INTERFACE
  ${CTMD_REFLECTION_FLAGS}          # -freflection (+ --embed-dir= if supported)
  -fconstexpr-depth=2048
  -fconstexpr-ops-limit=1000000000)
target_link_libraries(ctmd INTERFACE simdjson::simdjson)
target_compile_features(ctmd INTERFACE cxx_std_26)

# ---- the demo binary --------------------------------------------------------
add_executable(snapshot src/main.cpp)
target_link_libraries(snapshot PRIVATE ctmd)

# #embed is a preprocessor read, so make the JSON a real build dependency.
set_property(SOURCE src/main.cpp APPEND PROPERTY OBJECT_DEPENDS "${CTMD_SNAPSHOT}")

add_custom_target(check-fold
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_fold.sh" "$<TARGET_FILE:snapshot>"
  DEPENDS snapshot
  COMMENT "Disassembling to confirm the JSON folded away"
  VERBATIM)

include(CTest)
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
