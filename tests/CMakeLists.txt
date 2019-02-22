add_executable(metricq-combinator.test_single_derivation test_single_derivation.cpp)
add_test(metricq-combinator.test_single_derivation metricq-combinator.test_single_derivation)

target_link_libraries(
    metricq-combinator.test_single_derivation
    PRIVATE
        metricq-combinator-lib
)
