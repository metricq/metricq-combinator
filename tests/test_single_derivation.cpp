#include <cassert>
#include <iostream>

#include <metricq/json.hpp>

#include "../src/combined_metric.hpp"

static std::ostream& operator<<(std::ostream& os, metricq::TimeValue tv)
{
    return os << "TimeValue { time: " << tv.time.time_since_epoch().count()
              << ", value: " << tv.value << " }";
}

static void check(bool passed)
{
    if (!passed)
    {
        std::cerr << "!!! CHECK FAILED !!!\n";
        std::exit(1);
    }
}

template <std::size_t N>
static void check_and_fill_input(NodeInput::MetricInputsByName& input_map, const char* name,
                                 const metricq::TimeValue (&tvs)[N])
{
    auto inputs_it = input_map.find(name);
    std::cerr << "Checking for input " << name << "...\n";
    check(inputs_it != input_map.end());

    std::vector<MetricInput*>& inputs_for_name = inputs_it->second;
    std::cerr << "Checking that input occurs only once...\n";
    check(inputs_for_name.size() == 1);

    std::cerr << "Filling in values...\n";
    for (auto tv : tvs)
    {
        std::cerr << "`-- " << tv << '\n';
        inputs_for_name[0]->put(tv);
    }
}

constexpr metricq::TimePoint t(metricq::TimePoint::duration::rep r)
{
    return metricq::TimePoint(metricq::TimePoint::duration(r));
}

//    time -0----1----2----3----4----5-
//
//     foo _.(---1)(------------3)___._
//
//     bar _.(--------1)(--2)___.____._
//
// foo+bar _.(---2)(--4)(--5)___.____._

constexpr metricq::TimeValue FOO_INPUT[] = { { t(1), 1.0 }, { t(4), 3.0 } };
constexpr metricq::TimeValue BAR_INPUT[] = { { t(2), 1.0 }, { t(3), 2.0 } };
constexpr metricq::TimeValue EXPECTED_OUTPUT[] = { { t(1), 2.0 }, { t(2), 4.0 }, { t(3), 5.0 } };

int main()
{
    metricq::json config = {
        { "operation", "+" },
        { "left", "foo" },
        { "right", "bar" },
    };

    CombinedMetric combined(config);

    NodeInput::MetricInputsByName inputs = combined.collect_metric_inputs();

    check_and_fill_input(inputs, "foo", FOO_INPUT);
    check_and_fill_input(inputs, "bar", BAR_INPUT);

    std::cerr << "Updating combined metrics...\n";
    combined.update();

    auto& input = combined.input();
    std::cerr << "Checking that output has correct length...\n";
    check(input.queue_length() == 3);

    std::cerr << "Checking that output values are correct...\n";
    for (auto tv :
         std::initializer_list<metricq::TimeValue>{ { t(1), 2.0 }, { t(2), 4.0 }, { t(3), 5.0 } })
    {
        metricq::TimeValue out = input.peek();
        std::cerr << "`-- Checking: " << tv << " == " << out << '\n';
        check(tv.time == out.time && tv.value == out.value);
        input.discard();
    }

    return 0;
}
