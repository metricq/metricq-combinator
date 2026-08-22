#include "../src/display_expression.hpp"
#include <iostream>
#include <metricq/json.hpp>

struct TestMakeString
{
    double input;
    std::string expected;
};

struct TestDisplayExpression
{
    nlohmann::json input;
    std::string expected;
};

void runMakeStringTests()
{
    std::vector<TestMakeString> stringTestCases = { { 100, "100" },
                                                    { 10.0, "10" },
                                                    { 10.3, "10.3" },
                                                    { 10.25, "10.25" },
                                                    { 0.125, "0.125" },
                                                    { 0.123456789, "0.123456789" },
                                                    { 123456789, "123456789" },
                                                    { 123456789.01234, "123456789.01234" } };

    for (const auto& testCase : stringTestCases)
    {
        std::string result = toFormattedString(testCase.input);
        if (result != testCase.expected)
        {
            std::cerr << "Test case " << testCase.input << " failed:\n";
            std::cerr << "Expected: " << testCase.expected << "\n";
            std::cerr << "Got: " << result << "\n";
            exit(1);
        }
    }
    std::cout << "All toFormattedString test cases passed successfully!" << std::endl;
}

void testExpression()
{
    std::vector<TestDisplayExpression> testCases;

    testCases.emplace_back(TestDisplayExpression{
        { { "expression",
            { { "operation", "*" },
              { "left", 5 },
              { "right", { { "operation", "-" }, { "left", 45 }, { "right", 3 } } } } } },
        "5 * (45 - 3)" });

    testCases.emplace_back(TestDisplayExpression{
        { { "expression",
            { { "operation", "*" },
              { "left", { { "operation", "+" }, { "left", 1 }, { "right", 2 } } },
              { "right",
                { { "operation", "-" }, { "left", 10 }, { "right", "dummy.source" } } } } } },
        "(1 + 2) * (10 - dummy.source)" });

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
        "(15.3 + min[42, 24, 8, 12]) - 8" });

    // Test the cases
    for (const auto& testCase : testCases)
    {

        auto expression = testCase.input["expression"];
        std::string result = displayExpression(expression);

        // Compare with expected result
        if (result != testCase.expected) // comparing with the expected output
        {
            std::cerr << "Test case " << testCase.input << " failed:\n";
            std::cerr << "Expected: " << testCase.expected << "\n";
            std::cerr << "Got: " << result << "\n";
            return;
        }
    }

    std::cout << "All displayExpression test cases passed successfully!" << std::endl;
}

int main()
{
    try
    {
        testExpression();
        runMakeStringTests();

        std::cout << "All tests passed successfully!" << std::endl;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
