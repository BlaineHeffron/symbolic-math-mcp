#include <gtest/gtest.h>
#include "physics/physics.h"
#include <nlohmann/json.hpp>
#include <set>
#include <string>

using json = nlohmann::json;

TEST(Physics, FlatMetricChristoffel) {
    // Christoffel symbols of Minkowski metric should all be zero
    std::vector<std::vector<std::string>> flat = {
        {"-1", "0", "0", "0"},
        {"0", "1", "0", "0"},
        {"0", "0", "1", "0"},
        {"0", "0", "0", "1"},
    };
    std::vector<std::string> coords = {"t", "x", "y", "z"};
    auto result = physics::christoffel(flat, coords);
    EXPECT_EQ(result["result"].get<std::string>(), "All Christoffel symbols are zero");
}

TEST(Physics, FlatRiemann) {
    // Riemann tensor of flat metric is zero
    std::vector<std::vector<std::string>> flat = {
        {"-1", "0", "0", "0"},
        {"0", "1", "0", "0"},
        {"0", "0", "1", "0"},
        {"0", "0", "0", "1"},
    };
    std::vector<std::string> coords = {"t", "x", "y", "z"};
    auto result = physics::riemann_tensor(flat, coords);
    std::string r = result["result"].is_string() ? result["result"].get<std::string>() : result["result"].dump();
    // Should contain "zero" somewhere
    std::string lower_r = r;
    std::transform(lower_r.begin(), lower_r.end(), lower_r.begin(), ::tolower);
    EXPECT_NE(lower_r.find("zero"), std::string::npos);
}

TEST(Physics, SphereRicciScalar) {
    // Ricci scalar of 2-sphere metric = 2/r^2
    std::vector<std::vector<std::string>> sphere = {
        {"r**2", "0"},
        {"0", "r**2*sin(theta)**2"},
    };
    std::vector<std::string> coords = {"theta", "phi"};
    auto result = physics::ricci_scalar(sphere, coords);
    std::string r = result["result"].get<std::string>();
    EXPECT_NE(r.find("2"), std::string::npos);
    EXPECT_NE(r.find("r"), std::string::npos);
}

TEST(Physics, Flat2DChristoffel) {
    // Flat 2D metric has zero Christoffel symbols
    std::vector<std::vector<std::string>> flat2d = {{"1", "0"}, {"0", "1"}};
    std::vector<std::string> coords = {"x", "y"};
    auto result = physics::christoffel(flat2d, coords);
    EXPECT_EQ(result["result"].get<std::string>(), "All Christoffel symbols are zero");
}

// --- Structured output tests ---

TEST(Physics, StructuredChristoffelZero) {
    // Structured output for flat metric: zero flag, empty components
    std::vector<std::vector<std::string>> flat2d = {{"1", "0"}, {"0", "1"}};
    std::vector<std::string> coords = {"x", "y"};
    auto result = physics::christoffel(flat2d, coords, "structured");
    EXPECT_EQ(result["result_format"].get<std::string>(), "structured");
    EXPECT_EQ(result["type"].get<std::string>(), "christoffel_symbols");
    EXPECT_TRUE(result["zero"].get<bool>());
    EXPECT_TRUE(result["components"].is_array());
    EXPECT_EQ(result["components"].size(), 0u);
    EXPECT_EQ(result["rank"].get<int>(), 3);
    EXPECT_EQ(result["dimensions"].get<int>(), 2);
    EXPECT_EQ(result["coordinates"], json({"x", "y"}));
}

TEST(Physics, StructuredChristoffelNonZero) {
    // 2-sphere should yield non-zero structured Christoffel symbols
    std::vector<std::vector<std::string>> sphere = {
        {"r**2", "0"},
        {"0", "r**2*sin(theta)**2"},
    };
    std::vector<std::string> coords = {"theta", "phi"};
    auto result = physics::christoffel(sphere, coords, "structured");
    EXPECT_EQ(result["result_format"].get<std::string>(), "structured");
    EXPECT_FALSE(result["zero"].get<bool>());
    EXPECT_TRUE(result["components"].is_array());
    EXPECT_GT(result["components"].size(), 0u);

    // Each component must have indices, variance, value, latex
    for (const auto& comp : result["components"]) {
        EXPECT_TRUE(comp.contains("indices"));
        EXPECT_TRUE(comp.contains("variance"));
        EXPECT_TRUE(comp.contains("value"));
        EXPECT_TRUE(comp.contains("latex"));
        EXPECT_TRUE(comp.contains("index_names"));
        EXPECT_EQ(comp["variance"].get<std::string>(), "udd");
        EXPECT_EQ(comp["indices"].size(), 3u);
    }
}

TEST(Physics, StructuredRiemannZero) {
    std::vector<std::vector<std::string>> flat = {
        {"-1", "0"}, {"0", "1"},
    };
    std::vector<std::string> coords = {"t", "x"};
    auto result = physics::riemann_tensor(flat, coords, "structured");
    EXPECT_TRUE(result["zero"].get<bool>());
    EXPECT_EQ(result["rank"].get<int>(), 4);
    EXPECT_EQ(result["components"].size(), 0u);
}

TEST(Physics, StructuredRicciTensorHasMatrix) {
    std::vector<std::vector<std::string>> sphere = {
        {"r**2", "0"},
        {"0", "r**2*sin(theta)**2"},
    };
    std::vector<std::string> coords = {"theta", "phi"};
    auto result = physics::ricci_tensor(sphere, coords, "structured");
    EXPECT_EQ(result["rank"].get<int>(), 2);
    EXPECT_TRUE(result.contains("dense_matrix"));
    EXPECT_TRUE(result["dense_matrix"].is_array());
    EXPECT_EQ(result["dense_matrix"].size(), 2u);
    // Check variance
    for (const auto& comp : result["components"]) {
        EXPECT_EQ(comp["variance"].get<std::string>(), "dd");
    }
}

TEST(Physics, StructuredEinsteinTensorHasMatrix) {
    std::vector<std::vector<std::string>> flat2d = {{"1", "0"}, {"0", "1"}};
    std::vector<std::string> coords = {"x", "y"};
    auto result = physics::einstein_tensor(flat2d, coords, "structured");
    EXPECT_TRUE(result["zero"].get<bool>());
    EXPECT_TRUE(result.contains("dense_matrix"));
    EXPECT_EQ(result["dense_matrix"].size(), 2u);
}

TEST(Physics, StructuredComponentsSelfSufficient) {
    // Verify structured output emits full (non-reduced) component sets
    // so adapters don't need symmetry reconstruction logic
    std::vector<std::vector<std::string>> sphere = {
        {"r**2", "0"},
        {"0", "r**2*sin(theta)**2"},
    };
    std::vector<std::string> coords = {"theta", "phi"};

    // Christoffel: lower-index symmetric pairs should both appear
    auto christoffel_r = physics::christoffel(sphere, coords, "structured");
    // Gamma^theta_{phi phi} is non-zero; check for pairs like (sigma,mu,nu) and (sigma,nu,mu)
    std::set<std::string> christoffel_keys;
    for (const auto& comp : christoffel_r["components"]) {
        auto idx = comp["indices"];
        christoffel_keys.insert(
            std::to_string(idx[0].get<int>()) + "," +
            std::to_string(idx[1].get<int>()) + "," +
            std::to_string(idx[2].get<int>()));
    }
    // If (sigma, mu, nu) with mu!=nu exists, (sigma, nu, mu) must also exist
    for (const auto& comp : christoffel_r["components"]) {
        auto idx = comp["indices"];
        int s = idx[0].get<int>(), mu = idx[1].get<int>(), nu = idx[2].get<int>();
        if (mu != nu) {
            std::string mirror = std::to_string(s) + "," +
                                 std::to_string(nu) + "," + std::to_string(mu);
            EXPECT_TRUE(christoffel_keys.count(mirror) > 0)
                << "Missing symmetric partner for Christoffel [" << s << "," << mu << "," << nu << "]";
        }
    }

    // Ricci: symmetric pairs should both appear
    auto ricci_r = physics::ricci_tensor(sphere, coords, "structured");
    std::set<std::string> ricci_keys;
    for (const auto& comp : ricci_r["components"]) {
        auto idx = comp["indices"];
        ricci_keys.insert(
            std::to_string(idx[0].get<int>()) + "," +
            std::to_string(idx[1].get<int>()));
    }
    for (const auto& comp : ricci_r["components"]) {
        auto idx = comp["indices"];
        int mu = idx[0].get<int>(), nu = idx[1].get<int>();
        if (mu != nu) {
            std::string mirror = std::to_string(nu) + "," + std::to_string(mu);
            EXPECT_TRUE(ricci_keys.count(mirror) > 0)
                << "Missing symmetric partner for Ricci [" << mu << "," << nu << "]";
        }
    }
}

TEST(Physics, HumanFormatUnchanged) {
    // Default (human) format should still return the old shape
    std::vector<std::vector<std::string>> flat2d = {{"1", "0"}, {"0", "1"}};
    std::vector<std::string> coords = {"x", "y"};
    auto result = physics::christoffel(flat2d, coords);
    EXPECT_TRUE(result["result"].is_string());
    EXPECT_EQ(result["type"].get<std::string>(), "christoffel_symbols");
    // Should NOT have structured fields
    EXPECT_FALSE(result.contains("rank"));
    EXPECT_FALSE(result.contains("zero"));
    EXPECT_FALSE(result.contains("components"));
}

TEST(Physics, EvaluateComponentComplexSubstitutions) {
    auto result = physics::evaluate_component(
        "x + y",
        {{"x", "3+2*I"}, {"y", "1-I"}}
    );

    EXPECT_EQ(result["result"].get<std::string>(), "4 + I");
    EXPECT_TRUE(result["latex"].get<std::string>().find("4") != std::string::npos);
    EXPECT_TRUE(result["latex"].get<std::string>().find("j") != std::string::npos);
}

TEST(Physics, StructuredChristoffelComplexMetricPreservesImaginaryUnit) {
    std::vector<std::vector<std::string>> metric = {
        {"1 + I*x", "0"},
        {"0", "1"},
    };
    std::vector<std::string> coords = {"x", "y"};

    auto result = physics::christoffel(metric, coords, "structured");

    EXPECT_FALSE(result["zero"].get<bool>());
    ASSERT_EQ(result["components"].size(), 1u);
    const auto& component = result["components"][0];
    EXPECT_EQ(component["indices"], json({0, 0, 0}));
    EXPECT_EQ(component["index_names"], json({"x", "x", "x"}));
    EXPECT_EQ(component["variance"].get<std::string>(), "udd");
    EXPECT_TRUE(component["value"].get<std::string>().find("I") != std::string::npos);
    EXPECT_TRUE(component["latex"].get<std::string>().find("j") != std::string::npos);
}

// --- Additional complex-valued regression tests (handoff items 1-2) ---

TEST(Physics, EvaluateComponentRealBaseline) {
    // Baseline: real substitution should yield real result
    auto result = physics::evaluate_component("x**2 + 1", {{"x", "3"}});
    EXPECT_EQ(result["result"].get<std::string>(), "10");
}

TEST(Physics, EvaluateComponentPureImaginary) {
    // x = I => x**2 = -1
    auto result = physics::evaluate_component("x**2", {{"x", "I"}});
    EXPECT_EQ(result["result"].get<std::string>(), "-1");
}

TEST(Physics, EvaluateComponentComplexSquare) {
    // (3+2i)^2 + 1 = 9+12i-4+1 = 6+12i
    auto result = physics::evaluate_component("x**2 + 1", {{"x", "3+2*I"}});
    std::string val = result["result"].get<std::string>();
    EXPECT_NE(val.find("I"), std::string::npos) << "Expected complex result, got: " << val;
    EXPECT_NE(val.find("6"), std::string::npos);
    EXPECT_NE(val.find("12"), std::string::npos);
}

TEST(Physics, EvaluateComponentSchwarzschildRealHorizon) {
    // g_tt = -(1 - 2M/r) at M=1, r=2 => -(1-1) = 0
    auto result = physics::evaluate_component("-(1 - 2*M/r)", {{"M", "1"}, {"r", "2"}});
    EXPECT_EQ(result["result"].get<std::string>(), "0");
}

TEST(Physics, EvaluateComponentSchwarzschildComplexR) {
    // g_tt at M=1, r=1+I: -(1 - 2/(1+I))
    // 2/(1+I) = 2(1-I)/2 = 1-I, so -(1-(1-I)) = -(I) = -I
    auto result = physics::evaluate_component("-(1 - 2*M/r)", {{"M", "1"}, {"r", "1+I"}});
    std::string val = result["result"].get<std::string>();
    EXPECT_NE(val.find("I"), std::string::npos) << "Expected complex result, got: " << val;
}
