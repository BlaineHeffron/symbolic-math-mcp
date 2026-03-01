#include "tensors/cadabra_bridge.h"

#ifdef HAS_CADABRA
#include <cadabra2/Kernel.hh>
#include <cadabra2/Ex.hh>
#include <cadabra2/algorithms/eliminate_metric.hh>
#include <cadabra2/algorithms/rename_dummies.hh>
#include <cadabra2/algorithms/canonicalise.hh>
#include <cadabra2/algorithms/fierz.hh>
#include <cadabra2/algorithms/join_gamma.hh>
#include <cadabra2/algorithms/sym.hh>
#include <cadabra2/algorithms/asym.hh>

namespace tensors {

json cadabra_contract(const std::string& expr_str, const std::vector<std::vector<int>>& index_pairs) {
    cadabra::Kernel kernel;
    auto ex = std::make_shared<cadabra::Ex>(expr_str);
    cadabra::eliminate_metric elim(kernel, *ex);
    elim.apply();
    cadabra::rename_dummies ren(kernel, *ex);
    ren.apply();
    std::string result = ex->str();
    return {
        {"result", result},
        {"latex", result},
        {"type", "expression"},
    };
}

json cadabra_symmetrize(const std::string& expr_str, const std::vector<std::string>& indices, bool antisym) {
    cadabra::Kernel kernel;
    auto ex = std::make_shared<cadabra::Ex>(expr_str);
    if (antisym) {
        cadabra::asym as(kernel, *ex);
        as.apply();
    } else {
        cadabra::sym sy(kernel, *ex);
        sy.apply();
    }
    std::string result = ex->str();
    return {
        {"result", result},
        {"latex", result},
        {"type", "expression"},
    };
}

json cadabra_canonicalize(const std::string& expr_str) {
    cadabra::Kernel kernel;
    auto ex = std::make_shared<cadabra::Ex>(expr_str);
    cadabra::canonicalise canon(kernel, *ex);
    canon.apply();
    std::string result = ex->str();
    return {
        {"result", result},
        {"latex", result},
        {"type", "expression"},
    };
}

json cadabra_fierz(const std::string& expr_str) {
    cadabra::Kernel kernel;
    auto ex = std::make_shared<cadabra::Ex>(expr_str);
    cadabra::fierz fi(kernel, *ex);
    fi.apply();
    std::string result = ex->str();
    return {
        {"result", result},
        {"latex", result},
        {"type", "expression"},
    };
}

json cadabra_gamma(const std::string& expr_str, int dimension) {
    cadabra::Kernel kernel;
    // Declare gamma as GammaMatrix
    auto gamma_decl = std::make_shared<cadabra::Ex>(R"(\Gamma^{#}::GammaMatrix(metric=\delta).)");
    auto ex = std::make_shared<cadabra::Ex>(expr_str);
    cadabra::join_gamma jg(kernel, *ex);
    jg.apply();
    std::string result = ex->str();
    return {
        {"result", result},
        {"latex", result},
        {"type", "expression"},
    };
}

} // namespace tensors
#endif // HAS_CADABRA
