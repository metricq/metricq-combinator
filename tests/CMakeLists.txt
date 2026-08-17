add_executable(metricq-combinator.test_single_derivation test_single_derivation.cpp)
add_test(metricq-combinator.test_single_derivation metricq-combinator.test_single_derivation)

target_link_libraries(
    metricq-combinator.test_single_derivation
    PRIVATE
        metricq-combinator-lib
)

add_executable(metricq-combinator.test_metadata_defaults test_metadata_defaults.cpp)
add_test(metricq-combinator.test_metadata_defaults metricq-combinator.test_metadata_defaults)

target_link_libraries(
    metricq-combinator.test_metadata_defaults
    PRIVATE
        metricq-combinator-lib
)

add_executable(metricq-combinator.test_metadata_resolution test_metadata_resolution.cpp)
add_test(metricq-combinator.test_metadata_resolution metricq-combinator.test_metadata_resolution)

target_link_libraries(
    metricq-combinator.test_metadata_resolution
    PRIVATE
        metricq-combinator-lib
)
