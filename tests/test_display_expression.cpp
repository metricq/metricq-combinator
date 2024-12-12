#include "../src/combinator.hpp"
#include <iostream>
#include <metricq/json.hpp>

int main()
{
    try
    {
        // Input test data
        nlohmann::json data = {
            { "testCases",
              { { { "expression",
                    { { "operation", "*" },
                      { "left", 5 },
                      { "right", { { "operation", "-" }, { "left", 45 }, { "right", 3 } } } } } },
                { { "expression",
                    { { "operation", "*" },
                      { "left", { { "operation", "+" }, { "left", 1 }, { "right", 2 } } },
                      { "right",
                        { { "operation", "-" },
                          { "left", 10 },
                          { "right", "dummy.source" } } } } } },
                { { "expression",
                    { { "operation", "-" },
                      { "left",
                        { { "operation", "+" },
                          { "left", 15.3 },
                          { "right",
                            { { "operation", "min" }, { "inputs", { 42, 24, 8, 12 } } } } } },
                      { "right",
                        { { "operation", "throttle" },
                          { "cooldown_period", "42" },
                          { "input", 8 } } } } } } } }
        };

        // Expected outputs
        std::vector<std::string> expected = { "(5 * (45 - 3))", "((1 + 2) * (10 - dummy.source))",
                                              "((15.300000 + min[42, 24, 8, 12]) - 8)" };

        // Test the cases
        const auto& testCases = data["testCases"];
        for (size_t i = 0; i < testCases.size(); ++i)
        {
            const auto& testCase = testCases[i];
            const auto& expression = testCase["expression"];

            // Call the function to display the expression
            std::string result = Combinator::displayExpression(expression);

            // Compare with expected result
            if (result != expected[i])
            {
                std::cerr << "Test case " << i + 1 << " failed:\n";
                std::cerr << "Expected: " << expected[i] << "\n";
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
