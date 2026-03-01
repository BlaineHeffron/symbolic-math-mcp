#pragma once

#include <iostream>
#include <map>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "mcp/tool.h"

namespace mcp {

using json = nlohmann::json;

class Server {
public:
    explicit Server(const std::string& name, const std::string& version = "0.1.0");

    // Register a tool with its handler
    void register_tool(ToolDefinition def, ToolHandler handler);

    // Run the stdio event loop (blocks until EOF)
    void run(std::istream& in = std::cin, std::ostream& out = std::cout);

    // Process a single JSON-RPC request and return the response
    json handle_request(const json& request);

    // Accessors
    const std::vector<ToolDefinition>& tools() const { return tools_; }
    const std::string& name() const { return name_; }

private:
    json handle_initialize(const json& id, const json& params);
    json handle_initialized(const json& id);
    json handle_tools_list(const json& id, const json& params);
    json handle_tools_call(const json& id, const json& params);

    std::string name_;
    std::string version_;
    std::vector<ToolDefinition> tools_;
    std::map<std::string, ToolHandler> handlers_;
    bool initialized_ = false;
};

// Register all 21 symbolic math tools on the server
void register_all_tools(Server& server);

} // namespace mcp
