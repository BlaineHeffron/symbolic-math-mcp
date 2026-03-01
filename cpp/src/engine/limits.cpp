#include "engine/limits.h"
#include <symengine/subs.h>
#include <symengine/constants.h>
#include <symengine/functions.h>
#include <symengine/series.h>
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/integer.h>
#include <symengine/infinity.h>
#include <stdexcept>

using namespace SymEngine;

namespace engine {

// Check if a SymEngine expression is finite (not inf, -inf, nan, zoo)
static bool is_finite_value(const RCP<const Basic>& expr) {
    if (is_a<Infty>(*expr)) {
        return false;
    }
    if (expr->__str__() == "nan" || expr->__str__() == "zoo") {
        return false;
    }
    return true;
}

Expression compute_limit(const Expression& expr,
                         const RCP<const Symbol>& var,
                         const Expression& point) {
    auto basic = expr.get_basic();
    auto pt = point.get_basic();

    // Strategy 1: Direct substitution
    try {
        auto result = basic->subs({{var, pt}});
        if (is_finite_value(result) && result->__str__() != "nan" && result->__str__() != "zoo") {
            return Expression(result);
        }
    } catch (...) {}

    // Strategy 2: Series expansion around the point, extract constant term
    try {
        // Substitute var -> (var + point) so series around 0 = series around point
        auto shifted = basic->subs({{var, add(var, pt)}});
        auto series_result = SymEngine::series(shifted, var, 6);
        // The constant term = value at var=0 in the shifted series
        auto val = series_result->as_basic()->subs({{var, zero}});
        if (is_finite_value(val)) {
            return Expression(val);
        }
    } catch (...) {}

    // Strategy 3: L'Hopital's rule for f(x)/g(x) form
    if (is_a<Mul>(*basic)) {
        auto& m = down_cast<const Mul&>(*basic);
        RCP<const Basic> numerator = one;
        RCP<const Basic> denominator = one;

        for (const auto& p : m.get_dict()) {
            auto exp_val = p.second;
            if (is_a<Integer>(*exp_val) &&
                down_cast<const Integer&>(*exp_val).is_negative()) {
                denominator = mul(denominator, pow(p.first, neg(exp_val)));
            } else {
                numerator = mul(numerator, pow(p.first, exp_val));
            }
        }
        numerator = mul(m.get_coef(), numerator);

        if (!eq(*denominator, *one)) {
            // Apply L'Hopital: lim f/g = lim f'/g'
            for (int i = 0; i < 5; ++i) {
                auto f_prime = numerator->diff(var);
                auto g_prime = denominator->diff(var);

                if (eq(*g_prime, *zero)) break;

                auto ratio = div(f_prime, g_prime);
                try {
                    auto result = ratio->subs({{var, pt}});
                    if (is_finite_value(result) && result->__str__() != "nan") {
                        return Expression(result);
                    }
                } catch (...) {}

                numerator = f_prime;
                denominator = g_prime;
            }
        }
    }

    throw std::runtime_error("Could not compute limit of: " + basic->__str__());
}

} // namespace engine
