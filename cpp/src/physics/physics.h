#pragma once

#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace physics {

using json = nlohmann::json;

// 7 GR tools

json christoffel(const std::vector<std::vector<std::string>>& metric,
                 const std::vector<std::string>& coords);

json riemann_tensor(const std::vector<std::vector<std::string>>& metric,
                    const std::vector<std::string>& coords);

json ricci_tensor(const std::vector<std::vector<std::string>>& metric,
                  const std::vector<std::string>& coords);

json ricci_scalar(const std::vector<std::vector<std::string>>& metric,
                  const std::vector<std::string>& coords);

json einstein_tensor(const std::vector<std::vector<std::string>>& metric,
                     const std::vector<std::string>& coords);

json covariant_derivative(const std::string& expr_str,
                          const std::vector<std::vector<std::string>>& metric,
                          const std::vector<std::string>& coords,
                          const std::string& direction);

json evaluate_component(const std::string& expr_str,
                        const std::map<std::string, std::string>& substitutions);

} // namespace physics
