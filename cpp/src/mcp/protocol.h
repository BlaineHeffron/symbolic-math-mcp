#pragma once

#include <optional>
#include <string>
#include <nlohmann/json.hpp>

namespace mcp {

using json = nlohmann::json;

// Parsed JSON-RPC 2.0 request
struct JsonRpcRequest {
    std::string jsonrpc;
    json id;                    // string, int, or null
    std::string method;
    json params;                // may be null/missing
};

// Parse a JSON-RPC 2.0 request from a JSON object.
// Throws std::runtime_error on invalid format.
JsonRpcRequest parse_request(const json& j);

// Build a JSON-RPC 2.0 success response
json make_response(const json& id, const json& result);

// Build a JSON-RPC 2.0 error response
json make_error(const json& id, int code, const std::string& message,
                const json& data = nullptr);

// Standard JSON-RPC error codes
constexpr int PARSE_ERROR      = -32700;
constexpr int INVALID_REQUEST  = -32600;
constexpr int METHOD_NOT_FOUND = -32601;
constexpr int INVALID_PARAMS   = -32602;
constexpr int INTERNAL_ERROR   = -32603;

} // namespace mcp
