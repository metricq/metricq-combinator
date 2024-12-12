macro(add_test_case test_name)
  add_executable(${test_name} ${test_name}.cpp)
  add_test(${test_name} ${test_name})
  target_link_libraries(${test_name} PRIVATE metricq-combinator-lib)
endmacro()

add_test_case(test_single_derivation)
add_test_case(test_display_expression)
