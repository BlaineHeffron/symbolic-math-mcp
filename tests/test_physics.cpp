#include <gtest/gtest.h>
#include "physics/physics.h"
#include <nlohmann/json.hpp>
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
