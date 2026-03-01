#include "engine/parser.h"
#include <regex>
#include <stdexcept>
#include <symengine/parser.h>

namespace engine {

std::string latex_to_symengine(const std::string& latex_str) {
    std::string s = latex_str;

    // Remove display math delimiters
    s = std::regex_replace(s, std::regex(R"(^\$\$?|\$\$?$)"), "");
    s = std::regex_replace(s, std::regex(R"(^\\[(\[]|\\[)\]]$)"), "");

    // \frac{a}{b} -> (a)/(b) — iteratively handle nesting
    std::regex frac_re(R"(\\frac\{([^{}]*)\}\{([^{}]*)\})");
    while (std::regex_search(s, frac_re)) {
        s = std::regex_replace(s, frac_re, "($1)/($2)");
    }

    // \sqrt[n]{x} -> (x)**(1/(n))
    s = std::regex_replace(s, std::regex(R"(\\sqrt\[([^\]]*)\]\{([^{}]*)\})"), "($2)**(1/($1))");
    // \sqrt{x} -> sqrt(x)
    s = std::regex_replace(s, std::regex(R"(\\sqrt\{([^{}]*)\})"), "sqrt($1)");

    // Common functions
    const char* fns[] = {
        "sin", "cos", "tan", "log", "ln", "exp",
        "arcsin", "arccos", "arctan",
        "sinh", "cosh", "tanh", "sec", "csc", "cot",
    };
    for (const auto& fn : fns) {
        std::string from = std::string("\\") + fn;
        size_t pos;
        while ((pos = s.find(from)) != std::string::npos) {
            s.replace(pos, from.size(), fn);
        }
    }
    // \ln -> log
    {
        size_t pos;
        while ((pos = s.find("\\ln")) != std::string::npos) {
            s.replace(pos, 3, "log");
        }
    }

    // Constants
    {
        size_t pos;
        while ((pos = s.find("\\pi")) != std::string::npos)
            s.replace(pos, 3, "pi");
        while ((pos = s.find("\\infty")) != std::string::npos)
            s.replace(pos, 6, "oo");
        while ((pos = s.find("\\inf")) != std::string::npos)
            s.replace(pos, 4, "oo");
    }

    // Superscripts: x^{2} -> x**(2), x^n -> x**n
    s = std::regex_replace(s, std::regex(R"(\^\{([^{}]*)\})"), "**($1)");
    s = std::regex_replace(s, std::regex(R"(\^(\w))"), "**$1");

    // Subscripts: remove braces for simple cases
    s = std::regex_replace(s, std::regex(R"(_\{([^{}]*)\})"), "_$1");

    // \left, \right, \cdot, \times
    {
        size_t pos;
        while ((pos = s.find("\\left")) != std::string::npos)
            s.erase(pos, 5);
        while ((pos = s.find("\\right")) != std::string::npos)
            s.erase(pos, 6);
        while ((pos = s.find("\\cdot")) != std::string::npos)
            s.replace(pos, 5, "*");
        while ((pos = s.find("\\times")) != std::string::npos)
            s.replace(pos, 6, "*");
    }

    // Replace { } with ( )
    for (auto& c : s) {
        if (c == '{') c = '(';
        else if (c == '}') c = ')';
    }

    // Whitespace cleanup
    s = std::regex_replace(s, std::regex(R"(\s+)"), " ");
    // Trim
    size_t start = s.find_first_not_of(' ');
    size_t end = s.find_last_not_of(' ');
    if (start == std::string::npos) return "";
    return s.substr(start, end - start + 1);
}

SymEngine::Expression parse_expr(const std::string& input) {
    std::string s = input;
    // Trim whitespace
    size_t start = s.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        throw std::runtime_error("Empty expression");
    }
    s = s.substr(start);
    size_t end = s.find_last_not_of(" \t\n\r");
    s = s.substr(0, end + 1);

    // Try direct SymEngine parse first
    try {
        auto basic = SymEngine::parse(s);
        return SymEngine::Expression(basic);
    } catch (...) {}

    // Try as LaTeX
    std::string converted = latex_to_symengine(s);
    try {
        auto basic = SymEngine::parse(converted);
        return SymEngine::Expression(basic);
    } catch (...) {}

    throw std::runtime_error("Could not parse expression: " + input);
}

} // namespace engine
