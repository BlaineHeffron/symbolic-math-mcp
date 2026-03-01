#include <gtest/gtest.h>
#include "engine/parser.h"
#include "engine/output.h"
#include <string>

TEST(Parser, BasicExpression) {
    auto expr = engine::parse_expr("x**2 + 1");
    EXPECT_NE(engine::to_string(expr).find("x"), std::string::npos);
}

TEST(Parser, LaTeXFrac) {
    auto expr = engine::parse_expr(R"(\frac{x**2}{2})");
    std::string s = engine::to_string(expr);
    // Should be x**2/2 or (1/2)*x**2
    EXPECT_NE(s.find("x"), std::string::npos);
    EXPECT_NE(s.find("2"), std::string::npos);
}

TEST(Parser, LaTeXSqrt) {
    auto expr = engine::parse_expr(R"(\sqrt{x})");
    std::string s = engine::to_string(expr);
    EXPECT_NE(s.find("x"), std::string::npos);
}

TEST(Parser, LaTeXTrig) {
    auto expr = engine::parse_expr(R"(\sin(x))");
    std::string s = engine::to_string(expr);
    EXPECT_NE(s.find("sin"), std::string::npos);
}

TEST(Parser, LaTeXPi) {
    auto expr = engine::parse_expr(R"(\pi)");
    std::string s = engine::to_string(expr);
    EXPECT_NE(s.find("pi"), std::string::npos);
}

TEST(Parser, InvalidExpression) {
    EXPECT_THROW(engine::parse_expr("<<<invalid>>>"), std::runtime_error);
}

TEST(Parser, EmptyExpression) {
    EXPECT_THROW(engine::parse_expr("   "), std::runtime_error);
}

TEST(Parser, Superscript) {
    auto expr = engine::parse_expr(R"(x^{3})");
    std::string s = engine::to_string(expr);
    EXPECT_NE(s.find("x"), std::string::npos);
    EXPECT_NE(s.find("3"), std::string::npos);
}
