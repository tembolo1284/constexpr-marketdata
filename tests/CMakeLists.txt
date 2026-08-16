add_executable(test_depth test_depth.cpp)
target_link_libraries(test_depth PRIVATE ctmd)
set_property(SOURCE test_depth.cpp APPEND PROPERTY OBJECT_DEPENDS "${CTMD_SNAPSHOT}")
add_test(NAME depth COMMAND test_depth)

# Negative tests: these must FAIL to compile. Built on demand, never by `all`.
function(ctmd_add_negative_test name source json)
  add_executable(${name} EXCLUDE_FROM_ALL "${source}")
  target_link_libraries(${name} PRIVATE ctmd)
  target_compile_definitions(${name} PRIVATE
    CTMD_SNAPSHOT_PATH="${CMAKE_SOURCE_DIR}/data/invalid/${json}")
  add_test(NAME ${name}
    COMMAND "${CMAKE_COMMAND}" --build "${CMAKE_BINARY_DIR}" --target ${name})
  set_tests_properties(${name} PROPERTIES WILL_FAIL TRUE)
endfunction()

ctmd_add_negative_test(neg_malformed     negative/malformed.cpp     malformed.json)
ctmd_add_negative_test(neg_wrong_schema  negative/wrong_schema.cpp  wrong_schema.json)
