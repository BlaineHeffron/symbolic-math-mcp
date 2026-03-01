#include <gtest/gtest.h>
#include "tensors/tensors.h"
#include <nlohmann/json.hpp>
#include <string>

using json = nlohmann::json;

TEST(Tensors, GammaTraceTwo) {
    // tr(gamma^mu gamma^nu) = 4*g^{mu nu} in 4D
    auto result = tensors::gamma_algebra("tr(gamma^mu gamma^nu)", 4);
    std::string r = result["result"].get<std::string>();
    EXPECT_NE(r.find("4"), std::string::npos);
    EXPECT_NE(r.find("g^"), std::string::npos);
}

TEST(Tensors, GammaTraceOdd) {
    // Trace of odd number of gamma matrices is zero
    auto result = tensors::gamma_algebra("tr(gamma^mu gamma^nu gamma^rho)", 4);
    EXPECT_EQ(result["result"].get<std::string>(), "0");
}

TEST(Tensors, GammaTraceFour) {
    // Trace of four gamma matrices
    auto result = tensors::gamma_algebra("tr(gamma^a gamma^b gamma^c gamma^d)", 4);
    std::string r = result["result"].get<std::string>();
    EXPECT_NE(r.find("4"), std::string::npos);
}

TEST(Tensors, GammaTraceZero) {
    // tr(1) = dimension
    auto result = tensors::gamma_algebra("tr(1)", 4);
    EXPECT_EQ(result["result"].get<std::string>(), "4");
}

TEST(Tensors, FierzRequiresCadabra) {
    // Without Cadabra2, fierz_identity should throw
#ifndef HAS_CADABRA
    EXPECT_THROW(tensors::fierz_identity("test"), std::runtime_error);
#endif
}

TEST(Tensors, TensorSymmetrizeFallback) {
    auto result = tensors::tensor_symmetrize("T_{ab}", {"a", "b"});
    EXPECT_TRUE(result.contains("result"));
#ifndef HAS_CADABRA
    EXPECT_TRUE(result.contains("note"));
#endif
}
