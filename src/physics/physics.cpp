#include "physics/physics.h"
#include "physics/metric.h"
#include "engine/parser.h"
#include "engine/output.h"

#include <symengine/expression.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/rational.h>
#include <symengine/constants.h>
#include <symengine/subs.h>
#include <symengine/printers.h>

#include <sstream>

using namespace SymEngine;

namespace physics {

// --- Internal helpers ---

using Tensor3 = std::vector<std::vector<std::vector<RCP<const Basic>>>>;
using Tensor4 = std::vector<std::vector<std::vector<std::vector<RCP<const Basic>>>>>;

static Tensor3 compute_christoffel_array(const MetricData& m) {
    int n = m.n;
    Tensor3 gamma(n, std::vector<std::vector<RCP<const Basic>>>(
        n, std::vector<RCP<const Basic>>(n, zero)));

    for (int sigma = 0; sigma < n; ++sigma) {
        for (int mu = 0; mu < n; ++mu) {
            for (int nu = 0; nu < n; ++nu) {
                RCP<const Basic> val = zero;
                for (int rho = 0; rho < n; ++rho) {
                    auto g_inv_sr = m.g_inv.get(sigma, rho);
                    auto dg_nu_rho = m.g.get(nu, rho)->diff(m.coords[mu]);
                    auto dg_mu_rho = m.g.get(mu, rho)->diff(m.coords[nu]);
                    auto dg_mu_nu  = m.g.get(mu, nu)->diff(m.coords[rho]);

                    val = add(val, mul(
                        div(one, integer(2)),
                        mul(g_inv_sr, add(dg_nu_rho, sub(dg_mu_rho, dg_mu_nu)))
                    ));
                }
                gamma[sigma][mu][nu] = expand(val);
            }
        }
    }
    return gamma;
}

static Tensor4 compute_riemann_array(const Tensor3& gamma,
                                      const std::vector<RCP<const Symbol>>& coords,
                                      int n) {
    Tensor4 riemann(n, Tensor3(n, std::vector<std::vector<RCP<const Basic>>>(
        n, std::vector<RCP<const Basic>>(n, zero))));

    for (int rho = 0; rho < n; ++rho) {
        for (int sigma = 0; sigma < n; ++sigma) {
            for (int mu = 0; mu < n; ++mu) {
                for (int nu = 0; nu < n; ++nu) {
                    // R^rho_{sigma mu nu} = d_mu Gamma^rho_{nu sigma}
                    //                     - d_nu Gamma^rho_{mu sigma}
                    //                     + Gamma^rho_{mu lam} Gamma^lam_{nu sigma}
                    //                     - Gamma^rho_{nu lam} Gamma^lam_{mu sigma}
                    auto val = sub(
                        gamma[rho][nu][sigma]->diff(coords[mu]),
                        gamma[rho][mu][sigma]->diff(coords[nu])
                    );
                    for (int lam = 0; lam < n; ++lam) {
                        val = add(val, sub(
                            mul(gamma[rho][mu][lam], gamma[lam][nu][sigma]),
                            mul(gamma[rho][nu][lam], gamma[lam][mu][sigma])
                        ));
                    }
                    riemann[rho][sigma][mu][nu] = expand(val);
                }
            }
        }
    }
    return riemann;
}

static std::string format_tensor_latex(const json& components) {
    std::string result;
    bool first = true;
    for (auto& [key, val] : components.items()) {
        if (!first) result += " \\\\\n";
        result += key + " = " + val["latex"].get<std::string>();
        first = false;
    }
    return result;
}

// --- Public API ---

json christoffel(const std::vector<std::vector<std::string>>& metric,
                 const std::vector<std::string>& coords) {
    auto m = parse_metric(metric, coords);
    auto gamma = compute_christoffel_array(m);

    json components = json::object();
    for (int sigma = 0; sigma < m.n; ++sigma) {
        for (int mu = 0; mu < m.n; ++mu) {
            for (int nu = mu; nu < m.n; ++nu) {  // symmetric in lower indices
                auto val = gamma[sigma][mu][nu];
                if (!eq(*val, *zero)) {
                    std::string key = "Gamma^" + coords[sigma] + "_{" +
                                      coords[mu] + coords[nu] + "}";
                    components[key] = {
                        {"value", val->__str__()},
                        {"latex", latex(*val)},
                    };
                }
            }
        }
    }

    if (components.empty()) {
        return {
            {"result", "All Christoffel symbols are zero"},
            {"latex", "\\Gamma^{\\sigma}_{\\mu\\nu} = 0"},
            {"type", "christoffel_symbols"},
        };
    }

    return {
        {"result", components},
        {"latex", format_tensor_latex(components)},
        {"type", "christoffel_symbols"},
    };
}

json riemann_tensor(const std::vector<std::vector<std::string>>& metric,
                    const std::vector<std::string>& coords) {
    auto m = parse_metric(metric, coords);
    auto gamma = compute_christoffel_array(m);
    auto riemann = compute_riemann_array(gamma, m.coords, m.n);

    json components = json::object();
    for (int rho = 0; rho < m.n; ++rho) {
        for (int sigma = 0; sigma < m.n; ++sigma) {
            for (int mu = 0; mu < m.n; ++mu) {
                for (int nu = mu + 1; nu < m.n; ++nu) {  // antisymmetric
                    auto val = riemann[rho][sigma][mu][nu];
                    if (!eq(*val, *zero)) {
                        std::string key = "R^" + coords[rho] + "_{" +
                                          coords[sigma] + coords[mu] + coords[nu] + "}";
                        components[key] = {
                            {"value", val->__str__()},
                            {"latex", latex(*val)},
                        };
                    }
                }
            }
        }
    }

    if (components.empty()) {
        return {
            {"result", "Riemann tensor is identically zero (flat space)"},
            {"latex", "R^{\\rho}_{\\sigma\\mu\\nu} = 0"},
            {"type", "riemann_tensor"},
        };
    }

    return {
        {"result", components},
        {"latex", format_tensor_latex(components)},
        {"type", "riemann_tensor"},
    };
}

json ricci_tensor(const std::vector<std::vector<std::string>>& metric,
                  const std::vector<std::string>& coords) {
    auto m = parse_metric(metric, coords);
    auto gamma = compute_christoffel_array(m);
    auto riemann = compute_riemann_array(gamma, m.coords, m.n);

    json components = json::object();
    // Ricci matrix for output
    std::vector<std::vector<std::string>> ricci_matrix(m.n,
        std::vector<std::string>(m.n, "0"));

    for (int mu = 0; mu < m.n; ++mu) {
        for (int nu = mu; nu < m.n; ++nu) {  // symmetric
            RCP<const Basic> val = zero;
            for (int rho = 0; rho < m.n; ++rho) {
                val = add(val, riemann[rho][mu][rho][nu]);
            }
            val = expand(val);
            ricci_matrix[mu][nu] = val->__str__();
            ricci_matrix[nu][mu] = val->__str__();
            if (!eq(*val, *zero)) {
                std::string key = "R_{" + coords[mu] + coords[nu] + "}";
                components[key] = {
                    {"value", val->__str__()},
                    {"latex", latex(*val)},
                };
            }
        }
    }

    json result;
    if (components.empty()) {
        result = {
            {"result", "Ricci tensor is identically zero"},
            {"latex", "R_{\\mu\\nu} = 0"},
            {"type", "ricci_tensor"},
        };
    } else {
        result = {
            {"result", components},
            {"latex", format_tensor_latex(components)},
            {"type", "ricci_tensor"},
        };
    }
    result["matrix"] = ricci_matrix;
    return result;
}

json ricci_scalar(const std::vector<std::vector<std::string>>& metric,
                  const std::vector<std::string>& coords) {
    auto m = parse_metric(metric, coords);
    auto gamma = compute_christoffel_array(m);
    auto riemann = compute_riemann_array(gamma, m.coords, m.n);

    // Ricci tensor R_{mu nu}
    DenseMatrix ricci(m.n, m.n);
    for (int mu = 0; mu < m.n; ++mu) {
        for (int nu = 0; nu < m.n; ++nu) {
            RCP<const Basic> val = zero;
            for (int rho = 0; rho < m.n; ++rho) {
                val = add(val, riemann[rho][mu][rho][nu]);
            }
            ricci.set(mu, nu, val);
        }
    }

    // Contract: R = g^{mu nu} R_{mu nu}
    RCP<const Basic> scalar = zero;
    for (int mu = 0; mu < m.n; ++mu) {
        for (int nu = 0; nu < m.n; ++nu) {
            scalar = add(scalar, mul(m.g_inv.get(mu, nu), ricci.get(mu, nu)));
        }
    }
    scalar = expand(scalar);

    return {
        {"result", scalar->__str__()},
        {"latex", latex(*scalar)},
        {"type", "scalar"},
    };
}

json einstein_tensor(const std::vector<std::vector<std::string>>& metric,
                     const std::vector<std::string>& coords) {
    auto m = parse_metric(metric, coords);
    auto gamma = compute_christoffel_array(m);
    auto riemann = compute_riemann_array(gamma, m.coords, m.n);

    // Ricci tensor
    DenseMatrix ricci(m.n, m.n);
    for (int mu = 0; mu < m.n; ++mu) {
        for (int nu = 0; nu < m.n; ++nu) {
            RCP<const Basic> val = zero;
            for (int rho = 0; rho < m.n; ++rho) {
                val = add(val, riemann[rho][mu][rho][nu]);
            }
            ricci.set(mu, nu, val);
        }
    }

    // Ricci scalar
    RCP<const Basic> scalar = zero;
    for (int mu = 0; mu < m.n; ++mu) {
        for (int nu = 0; nu < m.n; ++nu) {
            scalar = add(scalar, mul(m.g_inv.get(mu, nu), ricci.get(mu, nu)));
        }
    }
    scalar = expand(scalar);

    // G_{mu nu} = R_{mu nu} - 1/2 R g_{mu nu}
    json components = json::object();
    std::vector<std::vector<std::string>> einstein_matrix(m.n,
        std::vector<std::string>(m.n, "0"));

    for (int mu = 0; mu < m.n; ++mu) {
        for (int nu = mu; nu < m.n; ++nu) {
            auto val = expand(sub(ricci.get(mu, nu),
                                   mul(div(one, integer(2)),
                                       mul(scalar, m.g.get(mu, nu)))));
            einstein_matrix[mu][nu] = val->__str__();
            einstein_matrix[nu][mu] = val->__str__();
            if (!eq(*val, *zero)) {
                std::string key = "G_{" + coords[mu] + coords[nu] + "}";
                components[key] = {
                    {"value", val->__str__()},
                    {"latex", latex(*val)},
                };
            }
        }
    }

    json result;
    if (components.empty()) {
        result = {
            {"result", "Einstein tensor is identically zero (vacuum solution)"},
            {"latex", "G_{\\mu\\nu} = 0"},
            {"type", "einstein_tensor"},
        };
    } else {
        result = {
            {"result", components},
            {"latex", format_tensor_latex(components)},
            {"type", "einstein_tensor"},
        };
    }
    result["matrix"] = einstein_matrix;
    return result;
}

json covariant_derivative(const std::string& expr_str,
                          const std::vector<std::vector<std::string>>& metric,
                          const std::vector<std::string>& coords,
                          const std::string& direction) {
    auto m = parse_metric(metric, coords);
    auto expr = engine::parse_expr(expr_str);

    // Find direction index
    int dir_idx = -1;
    for (int i = 0; i < m.n; ++i) {
        if (coords[i] == direction) {
            dir_idx = i;
            break;
        }
    }
    if (dir_idx < 0) {
        throw std::runtime_error("Direction '" + direction + "' not in coordinates");
    }

    // For a scalar, covariant derivative = partial derivative
    auto result = expr.get_basic()->diff(m.coords[dir_idx]);
    result = expand(result);

    return engine::to_output(Expression(result));
}

json evaluate_component(const std::string& expr_str,
                        const std::map<std::string, std::string>& substitutions) {
    auto expr = engine::parse_expr(expr_str);
    auto basic = expr.get_basic();

    map_basic_basic sub_map;
    for (const auto& [k, v] : substitutions) {
        sub_map[symbol(k)] = engine::parse_expr(v).get_basic();
    }

    auto result = basic->subs(sub_map);
    result = expand(result);
    return engine::to_output(Expression(result));
}

} // namespace physics
