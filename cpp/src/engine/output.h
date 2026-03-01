#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include <symengine/expression.h>
#include <symengine/basic.h>

namespace engine {

using json = nlohmann::json;

// Convert a SymEngine expression to the standard output dict
json to_output(const SymEngine::Expression& expr,
               const std::string& expr_type = "expression");

// Convert a list of expressions (e.g., solutions) to output dict
json to_output_list(const std::vector<SymEngine::Expression>& exprs,
                    const std::string& expr_type = "solutions");

// Get LaTeX representation of an expression
std::string to_latex(const SymEngine::Expression& expr);

// Get string representation of an expression
std::string to_string(const SymEngine::Expression& expr);

} // namespace engine
