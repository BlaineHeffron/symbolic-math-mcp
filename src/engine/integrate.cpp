#include "engine/integrate.h"
#include <symengine/add.h>
#include <symengine/mul.h>
#include <symengine/pow.h>
#include <symengine/constants.h>
#include <symengine/functions.h>
#include <symengine/subs.h>
#include <symengine/rational.h>
#include <symengine/integer.h>
#include <symengine/visitor.h>
#include <stdexcept>

using namespace SymEngine;

namespace engine {

// Check if expression is free of variable
static bool is_free_of(const RCP<const Basic>& expr, const RCP<const Symbol>& var) {
    auto fs = free_symbols(*expr);
    return fs.find(var) == fs.end();
}

// Integrate a single term (monomial, basic function)
static RCP<const Basic> integrate_term(const RCP<const Basic>& term,
                                        const RCP<const Symbol>& var) {
    // Case 1: constant (no var) -> c * x
    if (is_free_of(term, var)) {
        return mul(term, var);
    }

    // Case 2: just x -> x^2/2
    if (eq(*term, *var)) {
        return div(pow(var, integer(2)), integer(2));
    }

    // Case 3: x^n (power of var with constant exponent)
    if (is_a<Pow>(*term)) {
        auto base = down_cast<const Pow&>(*term).get_base();
        auto exp = down_cast<const Pow&>(*term).get_exp();
        if (eq(*base, *var) && is_free_of(exp, var)) {
            // x^n -> x^(n+1)/(n+1), provided n != -1
            auto new_exp = add(exp, one);
            // Check if n+1 == 0 (i.e., n == -1 => 1/x case)
            if (eq(*new_exp, *zero)) {
                return log(abs(var));
            }
            return div(pow(var, new_exp), new_exp);
        }
    }

    // Case 4: c * f(x) -> c * integrate(f(x))
    if (is_a<Mul>(*term)) {
        auto& mul_term = down_cast<const Mul&>(*term);
        RCP<const Basic> coeff = mul_term.get_coef();
        // Collect constant and variable parts
        RCP<const Basic> var_part = one;
        RCP<const Basic> const_part = coeff;
        for (const auto& p : mul_term.get_dict()) {
            auto factor = pow(p.first, p.second);
            if (is_free_of(factor, var)) {
                const_part = mul(const_part, factor);
            } else {
                var_part = mul(var_part, factor);
            }
        }
        if (!eq(*const_part, *one) && !eq(*var_part, *one)) {
            auto integrated = integrate_term(var_part, var);
            return mul(const_part, integrated);
        }
        // If entire thing is variable-dependent, try var_part
        if (eq(*const_part, *one) || eq(*const_part, *coeff)) {
            // Reconstruct the full expression as var_part
            auto full_var = div(term, const_part);
            // Try x^n pattern on the var part
            if (is_a<Pow>(*full_var)) {
                auto base = down_cast<const Pow&>(*full_var).get_base();
                auto exp = down_cast<const Pow&>(*full_var).get_exp();
                if (eq(*base, *var) && is_free_of(exp, var)) {
                    auto new_exp = add(exp, one);
                    if (eq(*new_exp, *zero)) {
                        return mul(const_part, log(abs(var)));
                    }
                    return mul(const_part, div(pow(var, new_exp), new_exp));
                }
            }
            // Try just var
            if (eq(*full_var, *var)) {
                return mul(const_part, div(pow(var, integer(2)), integer(2)));
            }
        }
    }

    // Case 5: Standard functions of x
    if (is_a<Sin>(*term)) {
        auto arg = down_cast<const Sin&>(*term).get_arg();
        if (eq(*arg, *var)) {
            return mul(minus_one, cos(var));
        }
    }
    if (is_a<Cos>(*term)) {
        auto arg = down_cast<const Cos&>(*term).get_arg();
        if (eq(*arg, *var)) {
            return sin(var);
        }
    }
    // Case 6: exp(x) stored as E^x (Pow with base E)
    if (is_a<Pow>(*term)) {
        auto base = down_cast<const Pow&>(*term).get_base();
        auto exp_val = down_cast<const Pow&>(*term).get_exp();
        if (eq(*base, *E) && eq(*exp_val, *var)) {
            return term; // integral of e^x is e^x
        }
        // e^(a*x) -> e^(a*x)/a
        if (eq(*base, *E) && is_a<Mul>(*exp_val)) {
            auto& m = down_cast<const Mul&>(*exp_val);
            RCP<const Basic> const_coeff = one;
            bool has_var = false;
            bool only_linear = true;
            for (const auto& p : m.get_dict()) {
                auto f = pow(p.first, p.second);
                if (eq(*f, *var)) {
                    has_var = true;
                } else if (is_free_of(f, var)) {
                    const_coeff = mul(const_coeff, f);
                } else {
                    only_linear = false;
                }
            }
            const_coeff = mul(const_coeff, m.get_coef());
            if (has_var && only_linear && !eq(*const_coeff, *zero)) {
                return div(term, const_coeff);
            }
        }
    }

    throw std::runtime_error("Integration not supported for: " + term->__str__());
}

Expression integrate_expr(const Expression& expr,
                          const RCP<const Symbol>& var) {
    auto basic = expr.get_basic();

    // If it's a sum, integrate term by term
    if (is_a<Add>(*basic)) {
        auto& add_expr = down_cast<const Add&>(*basic);
        RCP<const Basic> result = zero;
        // Handle the numeric coefficient
        if (!eq(*add_expr.get_coef(), *zero)) {
            result = integrate_term(add_expr.get_coef(), var);
        }
        for (const auto& p : add_expr.get_dict()) {
            auto term = mul(p.second, p.first);
            result = add(result, integrate_term(term, var));
        }
        return Expression(result);
    }

    // Single term
    return Expression(integrate_term(basic, var));
}

Expression integrate_definite(const Expression& expr,
                              const RCP<const Symbol>& var,
                              const Expression& lower,
                              const Expression& upper) {
    auto antideriv = integrate_expr(expr, var);
    auto upper_val = antideriv.get_basic()->subs({{var, upper.get_basic()}});
    auto lower_val = antideriv.get_basic()->subs({{var, lower.get_basic()}});
    return Expression(sub(upper_val, lower_val));
}

} // namespace engine
