#include "../src/combinator.hpp"
#include <iostream>
#include <metricq/json.hpp>

struct TestDisplayExpression
{
    nlohmann::json input;
    std::string expected;
};

int main()
{
    try
    {
        std::vector<TestDisplayExpression> testCases;

        testCases.emplace_back(TestDisplayExpression{
            { { "expression",
                { { "operation", "*" },
                  { "left", 5 },
                  { "right", { { "operation", "-" }, { "left", 45 }, { "right", 3 } } } } } },
            "(5 * (45 - 3))" });

        testCases.emplace_back(TestDisplayExpression{
            { { "expression",
                { { "operation", "*" },
                  { "left", { { "operation", "+" }, { "left", 1 }, { "right", 2 } } },
                  { "right",
                    { { "operation", "-" }, { "left", 10 }, { "right", "dummy.source" } } } } } },
            "((1 + 2) * (10 - dummy.source))" });

        testCases.emplace_back(TestDisplayExpression{
            { { "expression",
                { { "operation", "-" },
                  { "left",
                    { { "operation", "+" },
                      { "left", 15.3 },
                      { "right", { { "operation", "min" }, { "inputs", { 42, 24, 8, 12 } } } } } },
                  { "right",
                    { { "operation", "throttle" },
                      { "cooldown_period", "42" },
                      { "input", 8 } } } } } },
            "((15.300000 + min[42, 24, 8, 12]) - 8)" });

        // Test the cases
        for (const auto& testCase : testCases)
        {

            auto expression = testCase.input["expression"];
            std::string result = Combinator::displayExpression(expression);

            // Compare with expected result
            if (result != testCase.expected) // comparing with the expected output
            {
                std::cerr << "Test case " << testCase.input << " failed:\n";
                std::cerr << "Expected: " << testCase.expected << "\n";
                std::cerr << "Got: " << result << "\n";
                return 1;
            }
        }

        std::cout << "All test cases passed successfully!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
