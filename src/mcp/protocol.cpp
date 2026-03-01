#include "mcp/protocol.h"
#include <stdexcept>

namespace mcp {

JsonRpcRequest parse_request(const json& j) {
    if (!j.is_object()) {
        throw std::runtime_error("Request must be a JSON object");
    }

    JsonRpcRequest req;
    req.jsonrpc = j.value("jsonrpc", "");
    if (req.jsonrpc != "2.0") {
        throw std::runtime_error("jsonrpc must be \"2.0\"");
    }

    if (j.contains("id")) {
        req.id = j["id"];
    } else {
        req.id = nullptr;
    }

    if (!j.contains("method") || !j["method"].is_string()) {
        throw std::runtime_error("method must be a string");
    }
    req.method = j["method"].get<std::string>();

    if (j.contains("params")) {
        req.params = j["params"];
    } else {
        req.params = json::object();
    }

    return req;
}

json make_response(const json& id, const json& result) {
    return {
        {"jsonrpc", "2.0"},
        {"id",      id},
        {"result",  result},
    };
}

json make_error(const json& id, int code, const std::string& message,
                const json& data) {
    json err = {
        {"code",    code},
        {"message", message},
    };
    if (!data.is_null()) {
        err["data"] = data;
    }
    return {
        {"jsonrpc", "2.0"},
        {"id",      id},
        {"error",   err},
    };
}

} // namespace mcp
