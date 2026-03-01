#include "physics/metric.h"
#include "engine/parser.h"
#include <symengine/matrix.h>
#include <stdexcept>

namespace physics {

MetricData parse_metric(const std::vector<std::vector<std::string>>& metric,
                        const std::vector<std::string>& coords) {
    int n = static_cast<int>(coords.size());
    if (static_cast<int>(metric.size()) != n) {
        throw std::runtime_error("Metric dimensions must match coordinate count");
    }

    MetricData data;
    data.n = n;

    // Parse coordinate symbols
    for (const auto& c : coords) {
        data.coords.push_back(SymEngine::symbol(c));
    }

    // Parse metric components
    data.g = DenseMatrix(n, n);
    for (int i = 0; i < n; ++i) {
        if (static_cast<int>(metric[i].size()) != n) {
            throw std::runtime_error("Metric row " + std::to_string(i) + " has wrong size");
        }
        for (int j = 0; j < n; ++j) {
            data.g.set(i, j, engine::parse_expr(metric[i][j]).get_basic());
        }
    }

    // Compute inverse
    data.g_inv = DenseMatrix(n, n);
    data.g.inv(data.g_inv);

    return data;
}

} // namespace physics
