#include "mcp/server.h"
#include "mcp/protocol.h"
#include "engine/engine.h"
#include "engine/parser.h"
#include "engine/integrate.h"
#include "engine/limits.h"
#include "engine/output.h"
#include "physics/physics.h"
#include "tensors/tensors.h"
#include <sstream>
#include <stdexcept>

namespace mcp {

Server::Server(const std::string& name, const std::string& version)
    : name_(name), version_(version) {}

void Server::register_tool(ToolDefinition def, ToolHandler handler) {
    handlers_[def.name] = std::move(handler);
    tools_.push_back(std::move(def));
}

void Server::run(std::istream& in, std::ostream& out) {
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;

        json response;
        try {
            json request = json::parse(line);
            response = handle_request(request);
        } catch (json::parse_error& e) {
            response = make_error(nullptr, PARSE_ERROR, "Parse error");
        } catch (std::exception& e) {
            response = make_error(nullptr, INTERNAL_ERROR, e.what());
        }

        out << response.dump() << "\n";
        out.flush();
    }
}

json Server::handle_request(const json& request) {
    try {
        auto req = parse_request(request);

        if (req.method == "initialize") {
            return handle_initialize(req.id, req.params);
        } else if (req.method == "notifications/initialized") {
            return handle_initialized(req.id);
        } else if (req.method == "tools/list") {
            return handle_tools_list(req.id, req.params);
        } else if (req.method == "tools/call") {
            return handle_tools_call(req.id, req.params);
        } else {
            return make_error(req.id, METHOD_NOT_FOUND,
                              "Method not found: " + req.method);
        }
    } catch (std::runtime_error& e) {
        return make_error(nullptr, INVALID_REQUEST, e.what());
    }
}

json Server::handle_initialize(const json& id, const json& params) {
    initialized_ = true;
    json capabilities = {
        {"tools", json::object()},
    };
    json result = {
        {"protocolVersion", "2024-11-05"},
        {"capabilities", capabilities},
        {"serverInfo", {
            {"name", name_},
            {"version", version_},
        }},
    };
    return make_response(id, result);
}

json Server::handle_initialized(const json& id) {
    // Notification, no response needed per MCP spec, but we respond for compatibility
    return make_response(id, json::object());
}

json Server::handle_tools_list(const json& id, const json& params) {
    json tool_list = json::array();
    for (const auto& tool : tools_) {
        tool_list.push_back({
            {"name", tool.name},
            {"description", tool.description},
            {"inputSchema", tool.input_schema},
        });
    }
    return make_response(id, {{"tools", tool_list}});
}

json Server::handle_tools_call(const json& id, const json& params) {
    if (!params.contains("name") || !params["name"].is_string()) {
        return make_error(id, INVALID_PARAMS, "Missing 'name' parameter");
    }
    std::string tool_name = params["name"].get<std::string>();
    json arguments = params.value("arguments", json::object());

    auto it = handlers_.find(tool_name);
    if (it == handlers_.end()) {
        return make_error(id, METHOD_NOT_FOUND,
                          "Unknown tool: " + tool_name);
    }

    try {
        json result = it->second(arguments);
        json content = json::array();
        content.push_back({
            {"type", "text"},
            {"text", result.dump(2)},
        });
        return make_response(id, {{"content", content}});
    } catch (std::exception& e) {
        json error_result = {
            {"error", e.what()},
            {"type", "error"},
        };
        json content = json::array();
        content.push_back({
            {"type", "text"},
            {"text", error_result.dump(2)},
        });
        return make_response(id, {{"content", content}, {"isError", true}});
    }
}

// --- Tool Registration ---

void register_all_tools(Server& server) {
    using json = nlohmann::json;

    // === Core algebra (10 tools) ===

    server.register_tool(
        {"simplify", "Simplify a symbolic expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to simplify"}}},
            }},
            {"required", json::array({"expression"})},
        }},
        [](const json& args) -> json {
            return engine::simplify(args.at("expression").get<std::string>());
        }
    );

    server.register_tool(
        {"expand", "Expand products and powers in an expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to expand"}}},
            }},
            {"required", json::array({"expression"})},
        }},
        [](const json& args) -> json {
            return engine::expand(args.at("expression").get<std::string>());
        }
    );

    server.register_tool(
        {"factor", "Factor a polynomial expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to factor"}}},
            }},
            {"required", json::array({"expression"})},
        }},
        [](const json& args) -> json {
            return engine::factor(args.at("expression").get<std::string>());
        }
    );

    server.register_tool(
        {"differentiate", "Symbolic differentiation (partial derivatives)", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to differentiate"}}},
                {"variable", {{"type", "string"}, {"description", "Variable to differentiate with respect to"}}},
                {"order", {{"type", "integer"}, {"description", "Order of derivative (default: 1)"}, {"default", 1}}},
            }},
            {"required", json::array({"expression", "variable"})},
        }},
        [](const json& args) -> json {
            int order = args.value("order", 1);
            return engine::differentiate(
                args.at("expression").get<std::string>(),
                args.at("variable").get<std::string>(),
                order
            );
        }
    );

    server.register_tool(
        {"integrate", "Symbolic integration (definite or indefinite)", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to integrate"}}},
                {"variable", {{"type", "string"}, {"description", "Variable of integration"}}},
                {"lower", {{"type", "string"}, {"description", "Lower bound (for definite integral)"}}},
                {"upper", {{"type", "string"}, {"description", "Upper bound (for definite integral)"}}},
            }},
            {"required", json::array({"expression", "variable"})},
        }},
        [](const json& args) -> json {
            std::string lower, upper;
            if (args.contains("lower") && args.contains("upper")) {
                lower = args["lower"].get<std::string>();
                upper = args["upper"].get<std::string>();
                return engine::integrate(
                    args.at("expression").get<std::string>(),
                    args.at("variable").get<std::string>(),
                    lower, upper
                );
            }
            return engine::integrate(
                args.at("expression").get<std::string>(),
                args.at("variable").get<std::string>()
            );
        }
    );

    server.register_tool(
        {"series", "Taylor/Laurent series expansion", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to expand"}}},
                {"variable", {{"type", "string"}, {"description", "Variable for expansion"}}},
                {"point", {{"type", "string"}, {"description", "Point to expand around (default: 0)"}, {"default", "0"}}},
                {"order", {{"type", "integer"}, {"description", "Number of terms (default: 6)"}, {"default", 6}}},
            }},
            {"required", json::array({"expression", "variable"})},
        }},
        [](const json& args) -> json {
            std::string point = args.value("point", "0");
            int order = args.value("order", 6);
            return engine::series(
                args.at("expression").get<std::string>(),
                args.at("variable").get<std::string>(),
                point, order
            );
        }
    );

    server.register_tool(
        {"solve", "Solve an equation (expression = 0) for a variable", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to solve (set equal to 0)"}}},
                {"variable", {{"type", "string"}, {"description", "Variable to solve for"}}},
            }},
            {"required", json::array({"expression", "variable"})},
        }},
        [](const json& args) -> json {
            return engine::solve(
                args.at("expression").get<std::string>(),
                args.at("variable").get<std::string>()
            );
        }
    );

    server.register_tool(
        {"substitute", "Substitute values or expressions into an expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to substitute into"}}},
                {"substitutions", {{"type", "object"}, {"description", "Mapping of variable names to values/expressions"}}},
            }},
            {"required", json::array({"expression", "substitutions"})},
        }},
        [](const json& args) -> json {
            auto subs = args.at("substitutions").get<std::map<std::string, std::string>>();
            return engine::substitute(
                args.at("expression").get<std::string>(),
                subs
            );
        }
    );

    server.register_tool(
        {"limit", "Compute the limit of an expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to take limit of"}}},
                {"variable", {{"type", "string"}, {"description", "Variable approaching the point"}}},
                {"point", {{"type", "string"}, {"description", "Point the variable approaches"}}},
            }},
            {"required", json::array({"expression", "variable", "point"})},
        }},
        [](const json& args) -> json {
            return engine::limit(
                args.at("expression").get<std::string>(),
                args.at("variable").get<std::string>(),
                args.at("point").get<std::string>()
            );
        }
    );

    server.register_tool(
        {"latex_render", "Parse an expression and render it as LaTeX", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to render as LaTeX"}}},
            }},
            {"required", json::array({"expression"})},
        }},
        [](const json& args) -> json {
            return engine::latex_render(args.at("expression").get<std::string>());
        }
    );

    // === Tensor operations (4 tools) ===

    server.register_tool(
        {"tensor_contract", "Contract tensor indices", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Tensor expression"}}},
                {"index_pairs", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "integer"}}}}},
                    {"description", "Pairs of indices to contract, e.g. [[0,1]]"}}},
            }},
            {"required", json::array({"expression", "index_pairs"})},
        }},
        [](const json& args) -> json {
            auto pairs = args.at("index_pairs").get<std::vector<std::vector<int>>>();
            return tensors::tensor_contract(
                args.at("expression").get<std::string>(), pairs
            );
        }
    );

    server.register_tool(
        {"tensor_symmetrize", "Symmetrize or antisymmetrize a tensor expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Tensor expression"}}},
                {"indices", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Index names to symmetrize over"}}},
                {"antisymmetric", {{"type", "boolean"}, {"description", "Antisymmetrize instead (default: false)"}, {"default", false}}},
            }},
            {"required", json::array({"expression", "indices"})},
        }},
        [](const json& args) -> json {
            auto indices = args.at("indices").get<std::vector<std::string>>();
            bool antisym = args.value("antisymmetric", false);
            return tensors::tensor_symmetrize(
                args.at("expression").get<std::string>(), indices, antisym
            );
        }
    );

    server.register_tool(
        {"gamma_algebra", "Evaluate Clifford/gamma matrix algebra (traces, products)", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Gamma matrix expression, e.g. 'tr(gamma^mu gamma^nu)'"}}},
                {"dimension", {{"type", "integer"}, {"description", "Spacetime dimension (default: 4)"}, {"default", 4}}},
            }},
            {"required", json::array({"expression"})},
        }},
        [](const json& args) -> json {
            int dim = args.value("dimension", 4);
            return tensors::gamma_algebra(
                args.at("expression").get<std::string>(), dim
            );
        }
    );

    server.register_tool(
        {"fierz_identity", "Apply Fierz identities to spinor expressions (requires Cadabra2)", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Spinor expression"}}},
            }},
            {"required", json::array({"expression"})},
        }},
        [](const json& args) -> json {
            return tensors::fierz_identity(args.at("expression").get<std::string>());
        }
    );

    // === Physics operations (7 tools) ===

    server.register_tool(
        {"christoffel", "Compute Christoffel symbols from a metric tensor", {
            {"type", "object"},
            {"properties", {
                {"metric", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                    {"description", "Metric tensor as 2D array of expressions"}}},
                {"coordinates", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Coordinate names, e.g. ['t','r','theta','phi']"}}},
            }},
            {"required", json::array({"metric", "coordinates"})},
        }},
        [](const json& args) -> json {
            auto metric = args.at("metric").get<std::vector<std::vector<std::string>>>();
            auto coords = args.at("coordinates").get<std::vector<std::string>>();
            return physics::christoffel(metric, coords);
        }
    );

    server.register_tool(
        {"riemann_tensor", "Compute the Riemann curvature tensor from a metric", {
            {"type", "object"},
            {"properties", {
                {"metric", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                    {"description", "Metric tensor as 2D array of expressions"}}},
                {"coordinates", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Coordinate names"}}},
            }},
            {"required", json::array({"metric", "coordinates"})},
        }},
        [](const json& args) -> json {
            auto metric = args.at("metric").get<std::vector<std::vector<std::string>>>();
            auto coords = args.at("coordinates").get<std::vector<std::string>>();
            return physics::riemann_tensor(metric, coords);
        }
    );

    server.register_tool(
        {"ricci_tensor", "Compute the Ricci tensor from a metric", {
            {"type", "object"},
            {"properties", {
                {"metric", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                    {"description", "Metric tensor as 2D array"}}},
                {"coordinates", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Coordinate names"}}},
            }},
            {"required", json::array({"metric", "coordinates"})},
        }},
        [](const json& args) -> json {
            auto metric = args.at("metric").get<std::vector<std::vector<std::string>>>();
            auto coords = args.at("coordinates").get<std::vector<std::string>>();
            return physics::ricci_tensor(metric, coords);
        }
    );

    server.register_tool(
        {"ricci_scalar", "Compute the Ricci scalar curvature from a metric", {
            {"type", "object"},
            {"properties", {
                {"metric", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                    {"description", "Metric tensor as 2D array"}}},
                {"coordinates", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Coordinate names"}}},
            }},
            {"required", json::array({"metric", "coordinates"})},
        }},
        [](const json& args) -> json {
            auto metric = args.at("metric").get<std::vector<std::vector<std::string>>>();
            auto coords = args.at("coordinates").get<std::vector<std::string>>();
            return physics::ricci_scalar(metric, coords);
        }
    );

    server.register_tool(
        {"einstein_tensor", "Compute the Einstein tensor G_{mu nu} from a metric", {
            {"type", "object"},
            {"properties", {
                {"metric", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                    {"description", "Metric tensor as 2D array"}}},
                {"coordinates", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Coordinate names"}}},
            }},
            {"required", json::array({"metric", "coordinates"})},
        }},
        [](const json& args) -> json {
            auto metric = args.at("metric").get<std::vector<std::vector<std::string>>>();
            auto coords = args.at("coordinates").get<std::vector<std::string>>();
            return physics::einstein_tensor(metric, coords);
        }
    );

    server.register_tool(
        {"covariant_derivative", "Compute the covariant derivative of a scalar expression", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Scalar expression"}}},
                {"metric", {{"type", "array"},
                    {"items", {{"type", "array"}, {"items", {{"type", "string"}}}}},
                    {"description", "Metric tensor as 2D array"}}},
                {"coordinates", {{"type", "array"}, {"items", {{"type", "string"}}},
                    {"description", "Coordinate names"}}},
                {"direction", {{"type", "string"}, {"description", "Coordinate direction for derivative"}}},
            }},
            {"required", json::array({"expression", "metric", "coordinates", "direction"})},
        }},
        [](const json& args) -> json {
            auto metric = args.at("metric").get<std::vector<std::vector<std::string>>>();
            auto coords = args.at("coordinates").get<std::vector<std::string>>();
            return physics::covariant_derivative(
                args.at("expression").get<std::string>(),
                metric, coords,
                args.at("direction").get<std::string>()
            );
        }
    );

    server.register_tool(
        {"evaluate_component", "Evaluate a tensor component expression with numerical substitutions", {
            {"type", "object"},
            {"properties", {
                {"expression", {{"type", "string"}, {"description", "Expression to evaluate"}}},
                {"substitutions", {{"type", "object"}, {"description", "Variable -> value mapping"}}},
            }},
            {"required", json::array({"expression", "substitutions"})},
        }},
        [](const json& args) -> json {
            auto subs = args.at("substitutions").get<std::map<std::string, std::string>>();
            return physics::evaluate_component(
                args.at("expression").get<std::string>(), subs
            );
        }
    );
}

} // namespace mcp
