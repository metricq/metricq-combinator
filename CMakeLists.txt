cmake_minimum_required(VERSION 3.8)

project(metricq-combinator VERSION 0.1)

include(cmake/DefaultBuildType.cmake)
include(cmake/GitSubmoduleUpdate.cmake)
git_submodule_update()

add_subdirectory(lib/metricq)

set(SRCS
    src/input_node.cpp
    src/unary_node.cpp
    src/throttle_node.cpp
    src/binary_node.cpp
    src/variadic_node.cpp
    src/combined_metric.cpp
    src/combinator.cpp
)

add_library(metricq-combinator-lib STATIC ${SRCS})
target_link_libraries(metricq-combinator-lib
    PUBLIC
        metricq::transformer
        metricq::logger-nitro
        fmt::fmt
        Nitro::options
)

target_compile_features(metricq-combinator-lib PUBLIC cxx_std_17)
target_compile_options(metricq-combinator-lib PUBLIC -Wall -Wextra -pedantic)

add_executable(metricq-combinator src/main.cpp)
target_link_libraries(metricq-combinator
    PUBLIC
        metricq-combinator-lib
)

install(TARGETS metricq-combinator RUNTIME DESTINATION bin)

include(CTest)
add_subdirectory(tests)
