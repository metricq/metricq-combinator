#include <iostream>
#include <unordered_set>

#include <metricq/json.hpp>
#include <metricq/metadata.hpp>

#include "../src/metadata_defaults.hpp"

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

// Several inputs agreeing on a metadata field, all of them setting it: the field is taken
// over as a default.
static void test_agreeing_inputs_become_defaults()
{
    std::cerr << "test_agreeing_inputs_become_defaults\n";

    metricq::Metadata target;
    auto a = make_metadata({ { "unit", "W" }, { "quantity", "power" } });
    auto b = make_metadata({ { "unit", "W" }, { "quantity", "power" } });

    combinator::apply_metadata_defaults(target, { &a, &b }, {});

    CHECK(target.json().at("unit") == "W");
    CHECK(target.json().at("quantity") == "power");
}

// A field that only *some* inputs set (even if those that do agree) must not be defaulted:
// "no input disagrees" is not the same as "every input agrees". Absence (e.g. a
// ConstantInput without any metadata) is not agreement either.
static void test_field_missing_from_some_inputs_is_not_defaulted()
{
    std::cerr << "test_field_missing_from_some_inputs_is_not_defaulted\n";

    metricq::Metadata target;
    auto a = make_metadata({ { "unit", "W" }, { "quantity", "power" } });
    auto b = make_metadata({ { "unit", "W" } }); // no "quantity"

    combinator::apply_metadata_defaults(target, { &a, &b }, {});

    CHECK(target.json().at("unit") == "W");
    CHECK(target.json().count("quantity") == 0);
}

// Inputs disagreeing on a metadata field: the field must not be guessed, so it stays unset.
static void test_conflicting_inputs_are_dropped()
{
    std::cerr << "test_conflicting_inputs_are_dropped\n";

    metricq::Metadata target;
    auto a = make_metadata({ { "unit", "W" } });
    auto b = make_metadata({ { "unit", "kW" } });

    combinator::apply_metadata_defaults(target, { &a, &b }, {});

    CHECK(target.json().count("unit") == 0);
}

// A field already present on the target (e.g. set explicitly via combined_config["metadata"])
// must never be overwritten by a default, even if all inputs agree on a different value.
static void test_explicit_config_wins_over_defaults()
{
    std::cerr << "test_explicit_config_wins_over_defaults\n";

    auto target = make_metadata({ { "unit", "V" } });
    auto a = make_metadata({ { "unit", "W" } });
    auto b = make_metadata({ { "unit", "W" } });

    combinator::apply_metadata_defaults(target, { &a, &b }, {});

    CHECK(target.json().at("unit") == "V");
}

// An input that is itself another combined metric is, by the time its metadata is looked up
// here, indistinguishable from a "direct" external input metric: its metadata already mixes
// its own explicit config with fields that were themselves defaulted from *its* inputs.
static void test_indirect_input_metadata_is_used_as_default()
{
    std::cerr << "test_indirect_input_metadata_is_used_as_default\n";

    metricq::Metadata target;
    auto indirectly_combined_input =
        make_metadata({ { "unit", "W" }, { "description", "derived power metric" } });

    combinator::apply_metadata_defaults(target, { &indirectly_combined_input }, {});

    CHECK(target.json().at("unit") == "W");
    CHECK(target.json().at("description") == "derived power metric");
}

// Locks down the exact content of the shared reserved-keys constant that both
// Combinator::on_transformer_ready() and the tests below use as `excluded_keys`. Without
// this, a test passing its own ad-hoc literal set (e.g. {"source"}) would keep passing even
// if "source" were accidentally dropped from the production call site in combinator.cpp,
// since the two lists would have silently diverged. Every test below therefore uses
// combinator::reserved_metadata_keys itself, and this test is what actually pins down what
// that constant has to contain.
static void test_reserved_metadata_keys_matches_expected_set()
{
    std::cerr << "test_reserved_metadata_keys_matches_expected_set\n";

    const std::unordered_set<std::string> expected = { "rate", "chunkSize", "source",
                                                       "date", "historic",  "interval" };

    CHECK(combinator::reserved_metadata_keys == expected);
}

// "rate" and "chunkSize" have their own dedicated handling in the combinator (rate: max over
// inputs; chunkSize: derived from combined_config["chunk_size"]) and must never be inherited
// via the generic default mechanism, even if every input agrees on their value.
static void test_excluded_keys_are_never_defaulted()
{
    std::cerr << "test_excluded_keys_are_never_defaulted\n";

    metricq::Metadata target;
    auto a = make_metadata({ { "rate", 10.0 }, { "chunkSize", 1000 }, { "unit", "W" } });
    auto b = make_metadata({ { "rate", 10.0 }, { "chunkSize", 1000 }, { "unit", "W" } });

    combinator::apply_metadata_defaults(target, { &a, &b }, combinator::reserved_metadata_keys);

    CHECK(target.json().count("rate") == 0);
    CHECK(target.json().count("chunkSize") == 0);
    CHECK(target.json().at("unit") == "W");
}

// "source" describes the producing MetricQ source. For a combined metric that's always this
// combinator itself, never whatever produced its input(s) -- regardless of whether there's
// only a single input, several inputs that happen to share the same source, or an indirect
// input (itself a combined metric) whose resolved metadata would otherwise pass its own
// "source" along.
static void test_source_is_never_defaulted()
{
    std::cerr << "test_source_is_never_defaulted\n";

    // Single input.
    {
        metricq::Metadata target;
        auto a = make_metadata({ { "source", "some-source" }, { "unit", "W" } });
        combinator::apply_metadata_defaults(target, { &a }, combinator::reserved_metadata_keys);
        CHECK(target.json().count("source") == 0);
        CHECK(target.json().at("unit") == "W");
    }

    // Multiple inputs sharing the same source -- "agreement" must not matter here.
    {
        metricq::Metadata target;
        auto a = make_metadata({ { "source", "shared-source" } });
        auto b = make_metadata({ { "source", "shared-source" } });
        combinator::apply_metadata_defaults(target, { &a, &b }, combinator::reserved_metadata_keys);
        CHECK(target.json().count("source") == 0);
    }

    // Indirect input: itself a combined metric whose already-resolved metadata carries a
    // "source" that must not leak through to this metric.
    {
        metricq::Metadata target;
        auto indirectly_combined_input = make_metadata({ { "source", "other-combinator" } });
        combinator::apply_metadata_defaults(target, { &indirectly_combined_input },
                                            combinator::reserved_metadata_keys);
        CHECK(target.json().count("source") == 0);
    }
}

// "date" (last metadata update, belongs to this new metric, not its inputs), "historic" (set
// by the manager for this specific metric), and "interval" (reciprocal of "rate", which the
// combinator computes separately) are all technical/reserved per the MetricQ metadata docs
// and must never be inherited from inputs, even when every input agrees on their value.
static void test_date_historic_and_interval_are_never_defaulted()
{
    std::cerr << "test_date_historic_and_interval_are_never_defaulted\n";

    metricq::Metadata target;
    auto a = make_metadata({ { "date", "2026-08-17T00:00:00Z" },
                             { "historic", true },
                             { "interval", "PT1S" },
                             { "unit", "W" } });
    auto b = make_metadata({ { "date", "2026-08-17T00:00:00Z" },
                             { "historic", true },
                             { "interval", "PT1S" },
                             { "unit", "W" } });

    combinator::apply_metadata_defaults(target, { &a, &b }, combinator::reserved_metadata_keys);

    CHECK(target.json().count("date") == 0);
    CHECK(target.json().count("historic") == 0);
    CHECK(target.json().count("interval") == 0);
    CHECK(target.json().at("unit") == "W");
}

// Keys starting with "_" (e.g. "_id") are reserved for the MetricQ metadata backend. They
// must be excluded unconditionally, without having to be listed in excluded_keys, and even
// when every input agrees on the value.
static void test_underscore_prefixed_keys_are_never_defaulted()
{
    std::cerr << "test_underscore_prefixed_keys_are_never_defaulted\n";

    metricq::Metadata target;
    auto a = make_metadata({ { "_id", "input-a-doc-id" }, { "unit", "W" } });
    auto b = make_metadata({ { "_id", "input-a-doc-id" }, { "unit", "W" } });

    // Note: excluded_keys is empty on purpose -- "_id" must be excluded regardless.
    combinator::apply_metadata_defaults(target, { &a, &b }, {});

    CHECK(target.json().count("_id") == 0);
    CHECK(target.json().at("unit") == "W");
}

// Regression test for a bug where a combined metric's output Metric persists across
// reconfigurations: without resetting its metadata before re-deriving defaults, a value
// defaulted from a previous configuration's inputs would survive and be mistaken for an
// explicitly configured value, blocking the new default from ever being applied. This
// mirrors the metric.metadata.json(metricq::json::object()) reset that
// Combinator::on_transformer_config() performs before every re-derivation.
static void test_reconfiguration_does_not_leak_stale_defaults()
{
    std::cerr << "test_reconfiguration_does_not_leak_stale_defaults\n";

    metricq::Metadata metric_metadata;
    auto gen1_input = make_metadata({ { "unit", "W" } });
    combinator::apply_metadata_defaults(metric_metadata, { &gen1_input }, {});
    CHECK(metric_metadata.json().at("unit") == "W");

    // Reconfiguration: reset before re-deriving, as Combinator::on_transformer_config() does.
    metric_metadata.json(metricq::json::object());

    auto gen2_input = make_metadata({ { "unit", "A" } });
    combinator::apply_metadata_defaults(metric_metadata, { &gen2_input }, {});

    CHECK(metric_metadata.json().at("unit") == "A");
}

int main()
{
    test_agreeing_inputs_become_defaults();
    test_field_missing_from_some_inputs_is_not_defaulted();
    test_conflicting_inputs_are_dropped();
    test_explicit_config_wins_over_defaults();
    test_indirect_input_metadata_is_used_as_default();
    test_reserved_metadata_keys_matches_expected_set();
    test_excluded_keys_are_never_defaulted();
    test_source_is_never_defaulted();
    test_date_historic_and_interval_are_never_defaulted();
    test_underscore_prefixed_keys_are_never_defaulted();
    test_reconfiguration_does_not_leak_stale_defaults();

    std::cerr << "All metadata default tests passed.\n";
    return 0;
}
