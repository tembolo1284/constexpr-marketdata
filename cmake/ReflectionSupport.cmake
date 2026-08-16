include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

function(ctmd_require_reflection)
  if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
     OR CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16)
    message(FATAL_ERROR
      "constexpr-marketdata needs GCC 16+ for P2996 static reflection. "
      "Found ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}.")
  endif()

  set(CMAKE_REQUIRED_FLAGS "-std=c++26 -freflection")
  check_cxx_compiler_flag("-freflection" CTMD_HAS_FREFLECTION)
  if(NOT CTMD_HAS_FREFLECTION)
    message(FATAL_ERROR "Compiler rejected -freflection.")
  endif()

  check_cxx_source_compiles("
    constexpr const char d[] = {
    #embed __FILE__
      , 0 };
    static_assert(sizeof(d) > 1);
    int main() {}
  " CTMD_HAS_EMBED)
  if(NOT CTMD_HAS_EMBED)
    message(FATAL_ERROR "Compiler does not support #embed.")
  endif()

  set(flags "-freflection")
  check_cxx_compiler_flag("--embed-dir=." CTMD_HAS_EMBED_DIR)
  if(CTMD_HAS_EMBED_DIR)
    list(APPEND flags "--embed-dir=${CMAKE_SOURCE_DIR}/data")
  endif()
  set(CTMD_REFLECTION_FLAGS "${flags}" PARENT_SCOPE)
endfunction()
