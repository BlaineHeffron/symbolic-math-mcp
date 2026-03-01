#include <gtest/gtest.h>
#include "engine/engine.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

TEST(Engine, Expand) {
    auto result = engine::expand("(x+1)**3");
    std::string r = result["result"].get<std::string>();
    EXPECT_NE(r.find("x**3"), std::string::npos);
    EXPECT_NE(r.find("3*x**2"), std::string::npos);
}

TEST(Engine, Factor) {
    auto result = engine::factor("x**2 - 1");
    std::string r = result["result"].get<std::string>();
    // SymEngine may output "-1 + x" or "x - 1" depending on ordering
    EXPECT_NE(r.find("-1 + x"), std::string::npos);
    EXPECT_NE(r.find("1 + x"), std::string::npos);
}

TEST(Engine, Differentiate) {
    auto result = engine::differentiate("x**3 + 2*x", "x");
    EXPECT_EQ(result["result"].get<std::string>(), "2 + 3*x**2");
}

TEST(Engine, DifferentiateHigherOrder) {
    auto result = engine::differentiate("x**4", "x", 2);
    EXPECT_EQ(result["result"].get<std::string>(), "12*x**2");
}

TEST(Engine, Integrate) {
    auto result = engine::integrate("x**2", "x");
    std::string r = result["result"].get<std::string>();
    // Should contain x**3/3 or (1/3)*x**3
    EXPECT_TRUE(r.find("x**3") != std::string::npos);
    EXPECT_TRUE(r.find("3") != std::string::npos);
}

TEST(Engine, Series) {
    auto result = engine::series("sin(x)", "x", "0", 6);
    std::string r = result["result"].get<std::string>();
    EXPECT_NE(r.find("x"), std::string::npos);
}

TEST(Engine, Solve) {
    auto result = engine::solve("x**2 - 4", "x");
    std::string r = result["result"].get<std::string>();
    EXPECT_NE(r.find("2"), std::string::npos);
    EXPECT_NE(r.find("-2"), std::string::npos);
}

TEST(Engine, Substitute) {
    auto result = engine::substitute("x**2 + y", {{"x", "3"}, {"y", "1"}});
    EXPECT_EQ(result["result"].get<std::string>(), "10");
}

TEST(Engine, Simplify) {
    auto result = engine::simplify("x**2 + 2*x + 1");
    EXPECT_TRUE(result.contains("result"));
    EXPECT_TRUE(result.contains("latex"));
}

TEST(Engine, Limit) {
    auto result = engine::limit("sin(x)/x", "x", "0");
    EXPECT_EQ(result["result"].get<std::string>(), "1");
}

TEST(Engine, LatexRender) {
    auto result = engine::latex_render("x**2 + 1");
    std::string l = result["latex"].get<std::string>();
    EXPECT_NE(l.find("x"), std::string::npos);
}

TEST(Engine, LatexOutput) {
    auto result = engine::expand("(x+1)**2");
    EXPECT_TRUE(result.contains("latex"));
    EXPECT_GT(result["latex"].get<std::string>().size(), 0u);
}
