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

# Manual generator for the checked-in Go differential-test fixtures. It is not
# a CTest test: normal Go tests consume the generated JSON and do not require a
# C++ build.
add_executable(
    metricq-combinator.generate_go_fixtures
    ../../metricq-gombinator/combinator/testdata/generate_expected.cpp
)
target_link_libraries(
    metricq-combinator.generate_go_fixtures
    PRIVATE
        metricq-combinator-lib
)
