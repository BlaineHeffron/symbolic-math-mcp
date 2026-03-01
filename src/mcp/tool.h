#pragma once

#include <functional>
#include <string>
#include <nlohmann/json.hpp>

namespace mcp {

using json = nlohmann::json;

struct ToolDefinition {
    std::string name;
    std::string description;
    json input_schema;  // JSON Schema for the tool's arguments
};

// Tool handler: takes arguments JSON, returns result JSON
using ToolHandler = std::function<json(const json& args)>;

} // namespace mcp
