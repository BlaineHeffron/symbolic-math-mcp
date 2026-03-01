#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace tensors {

using json = nlohmann::json;

// 4 tensor tools

json tensor_contract(const std::string& expr_str,
                     const std::vector<std::vector<int>>& index_pairs);

json tensor_symmetrize(const std::string& expr_str,
                       const std::vector<std::string>& indices,
                       bool antisym = false);

json gamma_algebra(const std::string& expr_str, int dimension = 4);

json fierz_identity(const std::string& expr_str);

} // namespace tensors
