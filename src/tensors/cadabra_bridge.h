#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace tensors {

using json = nlohmann::json;

// Cadabra2 bridge functions — only available when HAS_CADABRA is defined

#ifdef HAS_CADABRA
json cadabra_contract(const std::string& expr_str, const std::vector<std::vector<int>>& index_pairs);
json cadabra_symmetrize(const std::string& expr_str, const std::vector<std::string>& indices, bool antisym);
json cadabra_canonicalize(const std::string& expr_str);
json cadabra_fierz(const std::string& expr_str);
json cadabra_gamma(const std::string& expr_str, int dimension);
#endif

} // namespace tensors
