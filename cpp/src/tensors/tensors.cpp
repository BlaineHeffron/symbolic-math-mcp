#include "tensors/tensors.h"
#include "tensors/cadabra_bridge.h"
#include "engine/parser.h"
#include "engine/output.h"

#include <regex>
#include <stdexcept>
#include <sstream>
#include <vector>
#include <string>

namespace tensors {

// --- Gamma trace evaluation (regex-based fallback) ---

static json gamma_trace(const std::string& expr_str, int dimension) {
    // Extract content inside tr(...) or Tr(...)
    std::regex trace_re(R"([Tt]r\((.+)\))");
    std::smatch match;
    if (!std::regex_search(expr_str, match, trace_re)) {
        return {
            {"result", expr_str},
            {"latex", expr_str},
            {"type", "expression"},
        };
    }

    std::string inner = match[1].str();

    // Count gamma matrices: gamma^mu, gamma^nu, etc.
    std::regex gamma_re(R"(gamma\^?(\w+))", std::regex::icase);
    std::vector<std::string> gammas;
    auto begin = std::sregex_iterator(inner.begin(), inner.end(), gamma_re);
    auto end = std::sregex_iterator();
    for (auto it = begin; it != end; ++it) {
        gammas.push_back((*it)[1].str());
    }

    int n_gammas = static_cast<int>(gammas.size());

    if (n_gammas == 0) {
        // tr(1) = dimension
        std::string result = std::to_string(dimension);
        return {
            {"result", result},
            {"latex", result},
            {"type", "expression"},
        };
    }

    if (n_gammas % 2 == 1) {
        // Trace of odd number of gammas is zero
        return {
            {"result", "0"},
            {"latex", "0"},
            {"type", "expression"},
        };
    }

    if (n_gammas == 2) {
        // tr(gamma^mu gamma^nu) = d * g^{mu nu}
        const auto& mu = gammas[0];
        const auto& nu = gammas[1];
        std::string result = std::to_string(dimension) + "*g^{" + mu + nu + "}";
        std::string latex_result = std::to_string(dimension) + " \\, g^{" + mu + nu + "}";
        return {
            {"result", result},
            {"latex", latex_result},
            {"type", "expression"},
        };
    }

    if (n_gammas == 4) {
        // tr(g^a g^b g^c g^d) = d*(g^{ab}g^{cd} - g^{ac}g^{bd} + g^{ad}g^{bc})
        const auto& a = gammas[0];
        const auto& b = gammas[1];
        const auto& c = gammas[2];
        const auto& d = gammas[3];
        std::string dim_str = std::to_string(dimension);

        std::string result = dim_str + "*(g^{" + a + b + "}*g^{" + c + d + "}"
            + " - g^{" + a + c + "}*g^{" + b + d + "}"
            + " + g^{" + a + d + "}*g^{" + b + c + "})";

        std::string latex_result = dim_str + "\\left(g^{" + a + b + "} g^{" + c + d + "}"
            + " - g^{" + a + c + "} g^{" + b + d + "}"
            + " + g^{" + a + d + "} g^{" + b + c + "}\\right)";

        return {
            {"result", result},
            {"latex", latex_result},
            {"type", "expression"},
        };
    }

    return {
        {"result", "Trace of " + std::to_string(n_gammas) + " gamma matrices"},
        {"latex", expr_str},
        {"type", "expression"},
        {"note", "Higher-order traces require Cadabra2 for full evaluation"},
    };
}

// --- Public API ---

json tensor_contract(const std::string& expr_str,
                     const std::vector<std::vector<int>>& index_pairs) {
#ifdef HAS_CADABRA
    try {
        return cadabra_contract(expr_str, index_pairs);
    } catch (...) {}
#endif

    // Fallback: parse as symbolic expression and simplify
    try {
        auto expr = engine::parse_expr(expr_str);
        return engine::to_output(expr);
    } catch (...) {
        return {
            {"result", expr_str},
            {"latex", expr_str},
            {"type", "expression"},
        };
    }
}

json tensor_symmetrize(const std::string& expr_str,
                       const std::vector<std::string>& indices,
                       bool antisym) {
#ifdef HAS_CADABRA
    try {
        return cadabra_symmetrize(expr_str, indices, antisym);
    } catch (...) {}
#endif

    return {
        {"result", expr_str},
        {"latex", expr_str},
        {"type", "expression"},
        {"note", "Full tensor symmetrization requires Cadabra2"},
    };
}

json gamma_algebra(const std::string& expr_str, int dimension) {
    std::string trimmed = expr_str;
    // Trim whitespace
    size_t start = trimmed.find_first_not_of(" \t\n\r");
    if (start != std::string::npos) trimmed = trimmed.substr(start);
    size_t end = trimmed.find_last_not_of(" \t\n\r");
    if (end != std::string::npos) trimmed = trimmed.substr(0, end + 1);

    // Check for trace patterns
    if (trimmed.find("tr") != std::string::npos || trimmed.find("Tr") != std::string::npos) {
        return gamma_trace(trimmed, dimension);
    }

    // For general gamma algebra, try Cadabra2
#ifdef HAS_CADABRA
    try {
        return cadabra_gamma(trimmed, dimension);
    } catch (...) {}
#endif

    return {
        {"result", expr_str},
        {"latex", expr_str},
        {"type", "expression"},
        {"note", "Complex gamma algebra requires Cadabra2"},
    };
}

json fierz_identity(const std::string& expr_str) {
#ifdef HAS_CADABRA
    return cadabra_fierz(expr_str);
#else
    throw std::runtime_error(
        "Cadabra2 required for Fierz identities. "
        "Install Cadabra2 and rebuild with -DCMAKE_PREFIX_PATH=<cadabra2_path>"
    );
#endif
}

} // namespace tensors
