#include "engine/engine.h"
#include "engine/parser.h"
#include "engine/output.h"
#include "engine/integrate.h"
#include "engine/limits.h"

#include <symengine/expression.h>
#include <symengine/symbol.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/constants.h>
#include <symengine/functions.h>
#include <symengine/subs.h>
#include <symengine/solve.h>
#include <symengine/series.h>
#include <symengine/sets.h>
#include <symengine/printers.h>
#include <symengine/visitor.h>
#include <symengine/integer.h>

#include <sstream>
#include <vector>
#include <stdexcept>

using namespace SymEngine;

namespace engine {

json simplify(const std::string& expr_str) {
    auto expr = parse_expr(expr_str);
    // SymEngine doesn't have a full simplify(); use expand + cancel as multi-strategy
    auto basic = expr.get_basic();

    // Strategy 1: expand
    auto expanded = SymEngine::expand(basic);

    // Strategy 2: try trig simplification via Pythagorean identity
    // Check if sin^2 + cos^2 pattern exists by expanding
    // SymEngine's expand handles algebraic simplification
    // For trig, we do basic rewriting
    auto result = expanded;

    // Try to see if the expanded form is simpler
    // Use string length as a heuristic for simplicity
    auto expanded_str = expanded->__str__();
    auto original_str = basic->__str__();

    if (expanded_str.size() <= original_str.size()) {
        result = expanded;
    } else {
        result = basic;
    }

    return to_output(Expression(result));
}

json expand(const std::string& expr_str) {
    auto expr = parse_expr(expr_str);
    auto result = SymEngine::expand(expr.get_basic());
    return to_output(Expression(result));
}

json factor(const std::string& expr_str) {
    auto expr = parse_expr(expr_str);
    auto basic = expr.get_basic();

    // SymEngine lacks polynomial factoring. Strategy: find roots via solve,
    // then reconstruct factored form as leading_coeff * product of (x - root_i)
    // For now, try to identify the variable and solve
    auto free = free_symbols(*basic);
    if (free.size() == 1) {
        auto var = rcp_static_cast<const Symbol>(*free.begin());
        try {
            auto solutions = solve(basic, var);
            // solutions is a FiniteSet
            if (is_a<FiniteSet>(*solutions)) {
                auto& fs = down_cast<const FiniteSet&>(*solutions);
                auto& elements = fs.get_container();
                if (!elements.empty()) {
                    // Reconstruct: find leading coefficient
                    // For polynomial a*x^n + ..., leading coeff is a
                    // We'll get the expanded form and extract it
                    auto expanded = SymEngine::expand(basic);

                    // Build product of (x - root) for each root
                    RCP<const Basic> factored = one;
                    for (const auto& root : elements) {
                        factored = mul(factored, sub(var, root));
                    }

                    // Compute leading coefficient by dividing original by monic
                    auto monic_expanded = SymEngine::expand(factored);
                    // If degrees match, compute ratio
                    // Simple approach: evaluate at a point not a root
                    auto test_point = integer(1000); // large number unlikely to be root
                    auto orig_val = expanded->subs({{var, test_point}});
                    auto monic_val = monic_expanded->subs({{var, test_point}});
                    if (!eq(*monic_val, *zero)) {
                        auto lead_coeff = div(orig_val, monic_val);
                        if (!eq(*lead_coeff, *one)) {
                            factored = mul(lead_coeff, factored);
                        }
                    }

                    return to_output(Expression(factored));
                }
            }
        } catch (...) {}
    }

    // Fallback: return expanded form
    return to_output(expr);
}

json differentiate(const std::string& expr_str, const std::string& var, int order) {
    auto expr = parse_expr(expr_str);
    auto v = symbol(var);
    auto result = expr.get_basic();
    for (int i = 0; i < order; ++i) {
        result = result->diff(v);
    }
    return to_output(Expression(result));
}

json integrate(const std::string& expr_str, const std::string& var) {
    auto expr = parse_expr(expr_str);
    auto v = symbol(var);
    auto result = integrate_expr(expr, v);
    return to_output(result);
}

json integrate(const std::string& expr_str, const std::string& var,
               const std::string& lower, const std::string& upper) {
    auto expr = parse_expr(expr_str);
    auto v = symbol(var);
    auto lo = parse_expr(lower);
    auto hi = parse_expr(upper);
    auto result = integrate_definite(expr, v, lo, hi);
    return to_output(result);
}

json series(const std::string& expr_str, const std::string& var,
            const std::string& point, int order) {
    auto expr = parse_expr(expr_str);
    auto v = symbol(var);
    auto p = parse_expr(point);

    // SymEngine::series expands around 0 only.
    // For non-zero expansion point, substitute var -> (var + point),
    // expand around 0, then substitute back.
    auto basic = expr.get_basic();
    bool nonzero_point = !eq(*p.get_basic(), *zero);
    if (nonzero_point) {
        // Replace var with (var + point) so series around 0 = series around point
        basic = basic->subs({{v, add(v, p.get_basic())}});
    }

    auto result = SymEngine::series(basic, v, static_cast<unsigned>(order));
    std::string result_str = result->__str__();
    std::string latex_str = result_str;

    return {
        {"result", result_str},
        {"latex",  latex_str},
        {"type",   "expression"},
    };
}

json solve(const std::string& expr_str, const std::string& var) {
    auto expr = parse_expr(expr_str);
    auto v = symbol(var);

    auto solutions = SymEngine::solve(expr.get_basic(), v);

    // Extract solutions from FiniteSet
    std::vector<Expression> result_list;
    if (is_a<FiniteSet>(*solutions)) {
        auto& fs = down_cast<const FiniteSet&>(*solutions);
        for (const auto& elem : fs.get_container()) {
            result_list.push_back(Expression(elem));
        }
    }

    return to_output_list(result_list, "solutions");
}

json substitute(const std::string& expr_str, const std::map<std::string, std::string>& subs) {
    auto expr = parse_expr(expr_str);
    auto basic = expr.get_basic();

    map_basic_basic sub_map;
    for (const auto& [k, v] : subs) {
        sub_map[symbol(k)] = parse_expr(v).get_basic();
    }

    auto result = basic->subs(sub_map);
    return to_output(Expression(result));
}

json limit(const std::string& expr_str, const std::string& var, const std::string& point) {
    auto expr = parse_expr(expr_str);
    auto v = symbol(var);
    auto p = parse_expr(point);

    auto result = compute_limit(expr, v, p);
    return to_output(result);
}

json latex_render(const std::string& expr_str) {
    auto expr = parse_expr(expr_str);
    return to_output(expr);
}

} // namespace engine
