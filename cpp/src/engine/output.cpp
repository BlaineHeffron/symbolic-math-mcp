#include "engine/output.h"
#include <symengine/printers.h>
#include <sstream>

namespace engine {

std::string to_latex(const SymEngine::Expression& expr) {
    return SymEngine::latex(*expr.get_basic());
}

std::string to_string(const SymEngine::Expression& expr) {
    std::ostringstream oss;
    oss << expr;
    return oss.str();
}

json to_output(const SymEngine::Expression& expr, const std::string& expr_type) {
    return {
        {"result", to_string(expr)},
        {"latex",  to_latex(expr)},
        {"type",   expr_type},
    };
}

json to_output_list(const std::vector<SymEngine::Expression>& exprs,
                    const std::string& expr_type) {
    std::string result_str = "[";
    std::string latex_str;
    for (size_t i = 0; i < exprs.size(); ++i) {
        if (i > 0) {
            result_str += ", ";
            latex_str += ", ";
        }
        result_str += to_string(exprs[i]);
        latex_str += to_latex(exprs[i]);
    }
    result_str += "]";

    return {
        {"result", result_str},
        {"latex",  latex_str},
        {"type",   expr_type},
    };
}

} // namespace engine
