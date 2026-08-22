#include <cmath>
#include <iomanip>
#include <metricq/json.hpp>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <vector>

inline std::string toFormattedString(double value)
{
    // Special case for integers
    if (std::round(value) == value)
    {
        return std::to_string(static_cast<long long>(value));
    }

    std::ostringstream oss;
    oss << std::defaultfloat << std::setprecision(15);
    oss << value;
    std::string str = oss.str();

    if (str.find('e') != std::string::npos)
    {
        oss.str("");
        oss << std::fixed << std::setprecision(15);
        oss << value;
        str = oss.str();
    }

    size_t decimal_pos = str.find('.');
    if (decimal_pos != std::string::npos)
    {
        // Remove trailing zeros
        str.erase(str.find_last_not_of('0') + 1, std::string::npos);

        if (str.back() == '.')
        {
            str.pop_back();
        }

        // Handle floating point representation issues
        if (str.length() > decimal_pos + 15)
        {
            str = str.substr(0, decimal_pos + 15);
            // Remove trailing zeros again
            str.erase(str.find_last_not_of('0') + 1, std::string::npos);
            if (str.back() == '.')
            {
                str.pop_back();
            }
        }
    }

    return str;
}

inline std::string handleBasicExpression(const nlohmann::json& expression)
{
    if (expression.is_number())
    {
        const auto value = expression.get<double>();

        return toFormattedString(value);
    }

    if (expression.is_string())
    {
        return expression.get<std::string>();
    }

    throw std::invalid_argument("Expression is not a basic type (number or string)!");
}

inline std::string handleOperatorExpression(const std::string& operation,
                                            const std::string& leftStr, const std::string& rightStr)
{
    if (operation.size() > 1)
    {
        throw std::invalid_argument("Invalid operator length!");
    }

    switch (operation[0])
    {
    case '+':
    case '-':
    case '*':
    case '/':
        return "(" + leftStr + " " + operation + " " + rightStr + ")";
    default:
        throw std::invalid_argument("Invalid operator: " + operation);
    }
}

inline std::string handleCombinationExpression(const std::string& operation,
                                               const std::vector<std::string>& inputs)
{
    static const std::unordered_set<std::string> validAggregates = { "sum", "min", "max" };

    if (validAggregates.find(operation) == validAggregates.end())
    {
        throw std::invalid_argument("Invalid aggregate operation: " + operation);
    }

    if (inputs.empty())
    {
        throw std::invalid_argument("Aggregate operation missing inputs!");
    }

    auto input = std::accumulate(std::next(inputs.begin()), inputs.end(), inputs[0],
                                 [](std::string a, const std::string& b) { return a + ", " + b; });

    return operation + "[" + input + "]";
}

inline std::string buildExpression(const nlohmann::json& expression)
{
    if (expression.is_number() || expression.is_string())
    {
        return handleBasicExpression(expression);
    }

    if (!expression.is_object() || !expression.contains("operation"))
    {
        throw std::invalid_argument("Unknown expression format!");
    }

    std::string operation = expression.value("operation", "");

    if (operation == "throttle")
    {
        if (!expression.contains("input"))
        {
            throw std::invalid_argument("Throttle does not contain a input");
        }
        return handleBasicExpression(expression["input"]);
    }

    if (expression.contains("left") && expression.contains("right"))
    {
        std::string leftStr = buildExpression(expression["left"]);
        std::string rightStr = buildExpression(expression["right"]);
        return handleOperatorExpression(operation, leftStr, rightStr);
    }

    if (expression.contains("inputs"))
    {
        if (!expression["inputs"].is_array())
        {
            throw std::invalid_argument("Inputs must be an array!");
        }

        std::vector<std::string> inputStrings;
        for (const auto& input : expression["inputs"])
        {
            inputStrings.push_back(buildExpression(input));
        }
        return handleCombinationExpression(operation, inputStrings);
    }

    throw std::invalid_argument("Unsupported operation type: " + operation);
}

inline std::string displayExpression(const nlohmann::json& expression)
{
    std::string result = buildExpression(expression);
    if (!result.empty() && result.front() == '(' && result.back() == ')')
    {
        result = result.substr(1, result.size() - 2);
    }

    return result;
}
