#pragma once

#include <map>
#include <string>
#include <nlohmann/json.hpp>

namespace engine {

using json = nlohmann::json;

// 10 core algebra tools — all accept string expressions, return JSON output dicts

json simplify(const std::string& expr_str);
json expand(const std::string& expr_str);
json factor(const std::string& expr_str);
json differentiate(const std::string& expr_str, const std::string& var, int order = 1);
json integrate(const std::string& expr_str, const std::string& var);
json integrate(const std::string& expr_str, const std::string& var,
               const std::string& lower, const std::string& upper);
json series(const std::string& expr_str, const std::string& var,
            const std::string& point = "0", int order = 6);
json solve(const std::string& expr_str, const std::string& var);
json substitute(const std::string& expr_str, const std::map<std::string, std::string>& subs);
json limit(const std::string& expr_str, const std::string& var, const std::string& point);
json latex_render(const std::string& expr_str);

} // namespace engine
