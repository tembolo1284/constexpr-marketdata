cmake_minimum_required(VERSION 3.25)

# --- pick a reflection-capable compiler ------------------------------------
# Must happen before project(). Honours an explicit -DCMAKE_CXX_COMPILER or $CXX.
if(NOT DEFINED CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
  foreach(_cand g++-18 g++-17 g++-16 g++)
    find_program(_ctmd_gxx "${_cand}"
                 HINTS "$ENV{HOME}/opt/gcc-16/bin" "$ENV{HOME}/opt/gcc/bin")
    if(_ctmd_gxx)
      execute_process(COMMAND "${_ctmd_gxx}" -dumpfullversion -dumpversion
                      OUTPUT_VARIABLE _ctmd_ver ERROR_QUIET
                      OUTPUT_STRIP_TRAILING_WHITESPACE)
      if(_ctmd_ver VERSION_GREATER_EQUAL 16)
        set(CMAKE_CXX_COMPILER "${_ctmd_gxx}" CACHE FILEPATH "C++ compiler")
        string(REPLACE "g++" "gcc" _ctmd_cc "${_ctmd_gxx}")
        if(EXISTS "${_ctmd_cc}")
          set(CMAKE_C_COMPILER "${_ctmd_cc}" CACHE FILEPATH "C compiler")
        endif()
        message(STATUS "Selected GCC ${_ctmd_ver}: ${_ctmd_gxx}")
        unset(_ctmd_gxx CACHE)
        break()
      endif()
    endif()
    unset(_ctmd_gxx CACHE)
  endforeach()
endif()

project(constexpr-marketdata LANGUAGES C CXX)

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

ctmd_require_reflection()

# simdjson probes for reflection at configure time using the GLOBAL flags,
# so these must be set before FetchContent brings it in. This is why the
# INTERFACE target alone was not enough.
string(JOIN " " _ctmd_flags ${CTMD_REFLECTION_FLAGS})
string(APPEND CMAKE_CXX_FLAGS " -std=c++26 ${_ctmd_flags}")

FetchContent_Declare(simdjson
  GIT_REPOSITORY https://github.com/simdjson/simdjson.git
  GIT_TAG        master
  GIT_SHALLOW    TRUE)
FetchContent_MakeAvailable(simdjson)

set(CTMD_SNAPSHOT "${CMAKE_CURRENT_SOURCE_DIR}/data/market_snapshot.json"
    CACHE FILEPATH "JSON document baked into the binary")
if(NOT EXISTS "${CTMD_SNAPSHOT}")
  message(FATAL_ERROR "Snapshot JSON not found: ${CTMD_SNAPSHOT}")
endif()

add_library(ctmd INTERFACE)
target_include_directories(ctmd INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/include")
target_compile_definitions(ctmd INTERFACE
  SIMDJSON_STATIC_REFLECTION=1
  CTMD_SNAPSHOT_PATH="${CTMD_SNAPSHOT}")
target_compile_options(ctmd INTERFACE
  ${CTMD_REFLECTION_FLAGS}
  -fconstexpr-depth=2048
  -fconstexpr-ops-limit=1000000000)
target_link_libraries(ctmd INTERFACE simdjson::simdjson)
target_compile_features(ctmd INTERFACE cxx_std_26)

add_executable(snapshot src/main.cpp)
target_link_libraries(snapshot PRIVATE ctmd)
set_property(SOURCE src/main.cpp APPEND PROPERTY OBJECT_DEPENDS "${CTMD_SNAPSHOT}")

add_custom_target(check-fold
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_fold.sh" "$<TARGET_FILE:snapshot>"
  DEPENDS snapshot
  COMMENT "Disassembling to confirm the JSON folded away"
  VERBATIM)

include(CTest)
if(BUILD_TESTING AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_depth.cpp")
  add_subdirectory(tests)
endif()
