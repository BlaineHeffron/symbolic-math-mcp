#pragma once

#include <string>
#include <vector>
#include <symengine/expression.h>
#include <symengine/symbol.h>
#include <symengine/matrix.h>

namespace physics {

using SymEngine::Expression;
using SymEngine::DenseMatrix;
using SymEngine::Symbol;
using SymEngine::RCP;

struct MetricData {
    DenseMatrix g;                               // metric tensor
    DenseMatrix g_inv;                           // inverse metric
    std::vector<RCP<const Symbol>> coords;       // coordinate symbols
    int n;                                       // dimension
};

// Parse metric and coordinates from string arrays
MetricData parse_metric(const std::vector<std::vector<std::string>>& metric,
                        const std::vector<std::string>& coords);

} // namespace physics
