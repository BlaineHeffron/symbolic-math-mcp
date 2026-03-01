#pragma once

#include <string>
#include <symengine/expression.h>

namespace engine {

// Parse a string expression (standard math notation or LaTeX) into SymEngine
SymEngine::Expression parse_expr(const std::string& input);

// Convert LaTeX notation to SymEngine-parseable string
std::string latex_to_symengine(const std::string& latex_str);

} // namespace engine
