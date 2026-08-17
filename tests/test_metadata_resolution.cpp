#include <iostream>
#include <stdexcept>

#include <metricq/json.hpp>
#include <metricq/metadata.hpp>

#include "../src/metadata_resolution.hpp"

static void check(bool passed, const char* what)
{
    if (!passed)
    {
        std::cerr << "!!! CHECK FAILED: " << what << " !!!\n";
        std::exit(1);
    }
}

#define CHECK(cond) check((cond), #cond)

static metricq::Metadata make_metadata(const metricq::json& j)
{
    metricq::Metadata m;
    m.json(j);
    return m;
}

// End-to-end (module-level, no live MetricQ connection needed) exercise of
// Combinator::on_transformer_ready()'s whole algorithm: a single combined metric with two
// external inputs that agree on "unit" and have different rates. Verifies both the generic
// default merge and the rate-max aggregation happen together correctly.
static void test_single_combined_metric_with_external_inputs()
{
    std::cerr << "test_single_combined_metric_with_external_inputs\n";

    std::unordered_map<std::string, std::vector<std::string>> combined_metric_inputs = {
        { "sum", { "foo", "bar" } }
    };
    std::unordered_map<std::string, metricq::Metadata> explicit_metadata = {
        { "sum", metricq::Metadata{} }
    };
    std::unordered_map<std::string, metricq::Metadata> external_metadata = {
        { "foo", make_metadata({ { "unit", "W" }, { "rate", 10.0 } }) },
        { "bar", make_metadata({ { "unit", "W" }, { "rate", 5.0 } }) },
    };

    auto resolution = combinator::resolve_combined_metadata(combined_metric_inputs,
                                                            explicit_metadata, external_metadata);

    CHECK(!resolution.missing_inputs);
    const auto& sum_metadata = resolution.metadata.at("sum");
    CHECK(sum_metadata.json().at("unit") == "W");
    CHECK(sum_metadata.rate() == 10.0);
}

// Indirect combination: "downstream" depends on "upstream" (itself a combined metric) and on
// an external input. Whichever of the two gets popped off the resolver queue first is an
// implementation detail of std::unordered_map's iteration order, not something this test
// controls -- but the algorithm must produce the same correct result either way: if
// "downstream" is attempted before "upstream" is resolved, it must be deferred and retried,
// not fail or silently skip "upstream"'s contribution.
static void test_indirect_combination_is_resolved_regardless_of_processing_order()
{
    std::cerr << "test_indirect_combination_is_resolved_regardless_of_processing_order\n";

    // Insertion order chosen so that, with this toolchain's std::unordered_map iteration
    // behavior for a small map, "downstream" is popped off the resolver queue before
    // "upstream" -- i.e. before its dependency is resolved -- which is what actually
    // exercises the deferral path below (verified by inspecting the "deferring resolving of
    // indirectly combined metric" log line while writing this test). Iteration order isn't
    // part of any documented contract, so this is inherently toolchain-specific; the
    // assertions below hold regardless of which one is processed first, which is the actual
    // guarantee resolve_combined_metadata() has to provide.
    std::unordered_map<std::string, std::vector<std::string>> combined_metric_inputs = {
        { "upstream", { "foo", "bar" } },
        { "downstream", { "upstream", "baz" } },
    };
    std::unordered_map<std::string, metricq::Metadata> explicit_metadata = {
        { "upstream", metricq::Metadata{} },
        { "downstream", metricq::Metadata{} },
    };
    std::unordered_map<std::string, metricq::Metadata> external_metadata = {
        { "foo", make_metadata({ { "unit", "W" }, { "rate", 10.0 } }) },
        { "bar", make_metadata({ { "unit", "W" }, { "rate", 5.0 } }) },
        { "baz", make_metadata({ { "unit", "W" }, { "rate", 20.0 } }) },
    };

    auto resolution = combinator::resolve_combined_metadata(combined_metric_inputs,
                                                            explicit_metadata, external_metadata);

    CHECK(!resolution.missing_inputs);

    const auto& upstream_metadata = resolution.metadata.at("upstream");
    CHECK(upstream_metadata.json().at("unit") == "W");
    CHECK(upstream_metadata.rate() == 10.0);

    const auto& downstream_metadata = resolution.metadata.at("downstream");
    // Inherited from upstream's already-resolved metadata, agreeing with "baz".
    CHECK(downstream_metadata.json().at("unit") == "W");
    // max(upstream.rate() == 10.0, baz.rate() == 20.0)
    CHECK(downstream_metadata.rate() == 20.0);
}

// A combined metric referencing an input metric that is neither an external input nor
// another combined metric must be reported via missing_inputs, not silently ignored or
// deferred forever.
static void test_missing_external_input_is_reported()
{
    std::cerr << "test_missing_external_input_is_reported\n";

    std::unordered_map<std::string, std::vector<std::string>> combined_metric_inputs = {
        { "sum", { "foo", "does_not_exist" } }
    };
    std::unordered_map<std::string, metricq::Metadata> explicit_metadata = {
        { "sum", metricq::Metadata{} }
    };
    std::unordered_map<std::string, metricq::Metadata> external_metadata = {
        { "foo", make_metadata({ { "unit", "W" } }) },
    };

    auto resolution = combinator::resolve_combined_metadata(combined_metric_inputs,
                                                            explicit_metadata, external_metadata);

    CHECK(resolution.missing_inputs);
}

// Two combined metrics depending on each other can never be resolved and must raise, rather
// than deferring forever -- this is the "circular dependency in your combinator config"
// case.
static void test_circular_dependency_throws()
{
    std::cerr << "test_circular_dependency_throws\n";

    std::unordered_map<std::string, std::vector<std::string>> combined_metric_inputs = {
        { "combined_a", { "combined_b" } },
        { "combined_b", { "combined_a" } },
    };
    std::unordered_map<std::string, metricq::Metadata> explicit_metadata = {
        { "combined_a", metricq::Metadata{} },
        { "combined_b", metricq::Metadata{} },
    };
    std::unordered_map<std::string, metricq::Metadata> external_metadata;

    bool threw = false;
    try
    {
        combinator::resolve_combined_metadata(combined_metric_inputs, explicit_metadata,
                                              external_metadata);
    }
    catch (const std::runtime_error&)
    {
        threw = true;
    }

    CHECK(threw);
}

// Explicit config metadata (rate and otherwise) must survive the full resolution pipeline
// unchanged, even though inputs would suggest different values.
static void test_explicit_metadata_survives_resolution()
{
    std::cerr << "test_explicit_metadata_survives_resolution\n";

    std::unordered_map<std::string, std::vector<std::string>> combined_metric_inputs = {
        { "sum", { "foo", "bar" } }
    };
    std::unordered_map<std::string, metricq::Metadata> explicit_metadata = {
        { "sum", make_metadata({ { "unit", "V" }, { "rate", 42.0 } }) }
    };
    std::unordered_map<std::string, metricq::Metadata> external_metadata = {
        { "foo", make_metadata({ { "unit", "W" }, { "rate", 10.0 } }) },
        { "bar", make_metadata({ { "unit", "W" }, { "rate", 5.0 } }) },
    };

    auto resolution = combinator::resolve_combined_metadata(combined_metric_inputs,
                                                            explicit_metadata, external_metadata);

    const auto& sum_metadata = resolution.metadata.at("sum");
    CHECK(sum_metadata.json().at("unit") == "V");
    CHECK(sum_metadata.rate() == 42.0);
}

// Reserved keys ("rate" and friends, see combinator::reserved_metadata_keys) must not leak
// through the full resolution pipeline either, not just through apply_metadata_defaults()
// called in isolation.
static void test_reserved_keys_do_not_leak_through_resolution()
{
    std::cerr << "test_reserved_keys_do_not_leak_through_resolution\n";

    std::unordered_map<std::string, std::vector<std::string>> combined_metric_inputs = {
        { "sum", { "foo", "bar" } }
    };
    std::unordered_map<std::string, metricq::Metadata> explicit_metadata = {
        { "sum", metricq::Metadata{} }
    };
    std::unordered_map<std::string, metricq::Metadata> external_metadata = {
        { "foo", make_metadata({ { "source", "source-a" }, { "historic", true } }) },
        { "bar", make_metadata({ { "source", "source-a" }, { "historic", true } }) },
    };

    auto resolution = combinator::resolve_combined_metadata(combined_metric_inputs,
                                                            explicit_metadata, external_metadata);

    const auto& sum_metadata = resolution.metadata.at("sum");
    CHECK(sum_metadata.json().count("source") == 0);
    CHECK(sum_metadata.json().count("historic") == 0);
}

int main()
{
    test_single_combined_metric_with_external_inputs();
    test_indirect_combination_is_resolved_regardless_of_processing_order();
    test_missing_external_input_is_reported();
    test_circular_dependency_throws();
    test_explicit_metadata_survives_resolution();
    test_reserved_keys_do_not_leak_through_resolution();

    std::cerr << "All metadata resolution tests passed.\n";
    return 0;
}
