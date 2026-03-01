#pragma once

#include <string>
#include <symengine/expression.h>
#include <symengine/symbol.h>

namespace engine {

// Indefinite integration (custom, since SymEngine lacks native integrate)
// Supports: polynomials, standard functions (sin, cos, exp, log), 1/x, basic compositions
SymEngine::Expression integrate_expr(const SymEngine::Expression& expr,
                                     const SymEngine::RCP<const SymEngine::Symbol>& var);

// Definite integration: evaluate antiderivative at bounds
SymEngine::Expression integrate_definite(const SymEngine::Expression& expr,
                                         const SymEngine::RCP<const SymEngine::Symbol>& var,
                                         const SymEngine::Expression& lower,
                                         const SymEngine::Expression& upper);

} // namespace engine
