#pragma once

#include <symengine/expression.h>
#include <symengine/symbol.h>

namespace engine {

// Compute limit of expr as var -> point
// Strategy: direct substitution -> series expansion -> L'Hopital
SymEngine::Expression compute_limit(const SymEngine::Expression& expr,
                                    const SymEngine::RCP<const SymEngine::Symbol>& var,
                                    const SymEngine::Expression& point);

} // namespace engine
