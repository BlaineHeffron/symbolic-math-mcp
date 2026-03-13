#include <gtest/gtest.h>
#include "mcp/server.h"
#include "mcp/protocol.h"
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>
#include <sstream>

using json = nlohmann::json;

class McpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        mcp::register_all_tools(server);
    }

    mcp::Server server{"symbolic-math", "0.1.0"};
};

TEST_F(McpServerTest, ToolCount) {
    // Server should expose 21 tools
    EXPECT_EQ(server.tools().size(), 21u);
}

TEST_F(McpServerTest, AllToolsHaveSchemas) {
    for (const auto& tool : server.tools()) {
        EXPECT_FALSE(tool.name.empty());
        EXPECT_FALSE(tool.description.empty());
        EXPECT_TRUE(tool.input_schema.contains("type"));
        EXPECT_EQ(tool.input_schema["type"], "object");
        EXPECT_TRUE(tool.input_schema.contains("properties"));
    }
}

TEST_F(McpServerTest, Initialize) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params", {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", json::object()},
            {"clientInfo", {{"name", "test"}, {"version", "1.0"}}},
        }},
    };
    auto response = server.handle_request(request);
    EXPECT_TRUE(response.contains("result"));
    EXPECT_EQ(response["result"]["serverInfo"]["name"], "symbolic-math");
}

TEST_F(McpServerTest, ToolsList) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"method", "tools/list"},
    };
    auto response = server.handle_request(request);
    EXPECT_TRUE(response.contains("result"));
    auto tools = response["result"]["tools"];
    EXPECT_EQ(tools.size(), 21u);
}

TEST_F(McpServerTest, FramedRunInitializeAndToolsList) {
    auto frame = [](const json& message) {
        std::string body = message.dump();
        return "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n" + body;
    };

    std::stringstream in;
    std::stringstream out;
    in << frame({
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "initialize"},
        {"params", {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", json::object()},
            {"clientInfo", {{"name", "test"}, {"version", "1.0"}}},
        }},
    });
    in << frame({
        {"jsonrpc", "2.0"},
        {"method", "notifications/initialized"},
        {"params", json::object()},
    });
    in << frame({
        {"jsonrpc", "2.0"},
        {"id", 2},
        {"method", "tools/list"},
        {"params", json::object()},
    });

    server.run(in, out);

    std::string output = out.str();
    ASSERT_FALSE(output.empty());
    EXPECT_EQ(output.find("\"id\":null"), std::string::npos);
    EXPECT_NE(output.find("\"id\":1"), std::string::npos);
    EXPECT_NE(output.find("\"id\":2"), std::string::npos);
    EXPECT_NE(output.find("\"tools\""), std::string::npos);
}

TEST_F(McpServerTest, ToolsCallSimplify) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 3},
        {"method", "tools/call"},
        {"params", {
            {"name", "simplify"},
            {"arguments", {{"expression", "x**2 + 2*x + 1"}}},
        }},
    };
    auto response = server.handle_request(request);
    EXPECT_TRUE(response.contains("result"));
    EXPECT_TRUE(response["result"].contains("content"));
    // Parse the text content to check for result fields
    auto text = response["result"]["content"][0]["text"].get<std::string>();
    auto parsed = json::parse(text);
    EXPECT_TRUE(parsed.contains("result"));
    EXPECT_TRUE(parsed.contains("latex"));
}

TEST_F(McpServerTest, ToolsCallExpand) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 4},
        {"method", "tools/call"},
        {"params", {
            {"name", "expand"},
            {"arguments", {{"expression", "(x+1)**2"}}},
        }},
    };
    auto response = server.handle_request(request);
    auto text = response["result"]["content"][0]["text"].get<std::string>();
    auto parsed = json::parse(text);
    EXPECT_NE(parsed["result"].get<std::string>().find("x**2"), std::string::npos);
}

TEST_F(McpServerTest, ToolsCallChristoffelFlat) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 5},
        {"method", "tools/call"},
        {"params", {
            {"name", "christoffel"},
            {"arguments", {
                {"metric", json::array({json::array({"1", "0"}), json::array({"0", "1"})})},
                {"coordinates", json::array({"x", "y"})},
            }},
        }},
    };
    auto response = server.handle_request(request);
    auto text = response["result"]["content"][0]["text"].get<std::string>();
    auto parsed = json::parse(text);
    std::string r = parsed["result"].is_string() ? parsed["result"].get<std::string>() : parsed["result"].dump();
    std::string lower_r = r;
    std::transform(lower_r.begin(), lower_r.end(), lower_r.begin(), ::tolower);
    EXPECT_NE(lower_r.find("zero"), std::string::npos);
}

TEST_F(McpServerTest, UnknownTool) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 6},
        {"method", "tools/call"},
        {"params", {
            {"name", "nonexistent_tool"},
            {"arguments", json::object()},
        }},
    };
    auto response = server.handle_request(request);
    EXPECT_TRUE(response.contains("error"));
}

TEST_F(McpServerTest, UnknownMethod) {
    json request = {
        {"jsonrpc", "2.0"},
        {"id", 7},
        {"method", "unknown/method"},
    };
    auto response = server.handle_request(request);
    EXPECT_TRUE(response.contains("error"));
}

TEST_F(McpServerTest, InvalidJsonRpc) {
    json request = {
        {"id", 8},
        {"method", "tools/list"},
    };
    auto response = server.handle_request(request);
    EXPECT_TRUE(response.contains("error"));
}

TEST(McpProtocol, ParseRequest) {
    json j = {
        {"jsonrpc", "2.0"},
        {"id", 1},
        {"method", "tools/list"},
        {"params", json::object()},
    };
    auto req = mcp::parse_request(j);
    EXPECT_EQ(req.method, "tools/list");
    EXPECT_EQ(req.id, 1);
}

TEST(McpProtocol, MakeResponse) {
    auto resp = mcp::make_response(1, {{"key", "value"}});
    EXPECT_EQ(resp["jsonrpc"], "2.0");
    EXPECT_EQ(resp["id"], 1);
    EXPECT_EQ(resp["result"]["key"], "value");
}

TEST(McpProtocol, MakeError) {
    auto resp = mcp::make_error(1, -32601, "Method not found");
    EXPECT_EQ(resp["error"]["code"], -32601);
    EXPECT_EQ(resp["error"]["message"], "Method not found");
}
