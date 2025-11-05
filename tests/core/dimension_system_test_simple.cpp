// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file dimension_system_test_simple.cpp
 * @brief Simplified BDD tests for dimension-agnostic vector system
 */

#include "../bdd_helpers.hpp"
#include "../../include/core/vector.hpp"
#include <cmath>

using namespace sph;

// Test 1D vectors
TEST(DimensionAgnosticVector, Vector1D_Construction) {
    Vector1D v1(5.0);
    EXPECT_EQ(v1[0], 5.0);
}

TEST(DimensionAgnosticVector, Vector1D_Addition) {
    Vector1D v1(5.0);
    Vector1D v2(3.0);
    Vector1D result = v1 + v2;
    EXPECT_EQ(result[0], 8.0);
}

TEST(DimensionAgnosticVector, Vector1D_InnerProduct) {
    Vector1D v1(5.0);
    Vector1D v2(3.0);
    real dot = inner_product(v1, v2);
    EXPECT_EQ(dot, 15.0);
}

// Test 2D vectors
TEST(DimensionAgnosticVector, Vector2D_Construction) {
    Vector2D v(3.0, 4.0);
    EXPECT_EQ(v[0], 3.0);
    EXPECT_EQ(v[1], 4.0);
}

TEST(DimensionAgnosticVector, Vector2D_Addition) {
    Vector2D v1(1.0, 2.0);
    Vector2D v2(3.0, 4.0);
    Vector2D result = v1 + v2;
    EXPECT_EQ(result[0], 4.0);
    EXPECT_EQ(result[1], 6.0);
}

TEST(DimensionAgnosticVector, Vector2D_InnerProduct) {
    Vector2D v1(1.0, 2.0);
    Vector2D v2(3.0, 4.0);
    real dot = inner_product(v1, v2);
    EXPECT_EQ(dot, 11.0);  // 1*3 + 2*4 = 11
}

TEST(DimensionAgnosticVector, Vector2D_Magnitude) {
    Vector2D v(3.0, 4.0);
    real mag = abs(v);
    EXPECT_EQ(mag, 5.0);  // 3-4-5 triangle
}

TEST(DimensionAgnosticVector, Vector2D_VectorProduct) {
    Vector2D v1(1.0, 0.0);
    Vector2D v2(0.0, 1.0);
    real cross_z = vector_product(v1, v2);
    EXPECT_EQ(cross_z, 1.0);
}

// Test 3D vectors
TEST(DimensionAgnosticVector, Vector3D_Construction) {
    Vector3D v(1.0, 2.0, 3.0);
    EXPECT_EQ(v[0], 1.0);
    EXPECT_EQ(v[1], 2.0);
    EXPECT_EQ(v[2], 3.0);
}

TEST(DimensionAgnosticVector, Vector3D_Addition) {
    Vector3D v1(1.0, 0.0, 0.0);
    Vector3D v2(0.0, 1.0, 0.0);
    Vector3D result = v1 + v2;
    EXPECT_EQ(result[0], 1.0);
    EXPECT_EQ(result[1], 1.0);
    EXPECT_EQ(result[2], 0.0);
}

TEST(DimensionAgnosticVector, Vector3D_InnerProduct) {
    Vector3D v1(1.0, 2.0, 3.0);
    Vector3D v2(4.0, 5.0, 6.0);
    real dot = inner_product(v1, v2);
    EXPECT_EQ(dot, 32.0);  // 1*4 + 2*5 + 3*6 = 32
}

// Test operators
TEST(DimensionAgnosticVector, VectorOperators_Subtraction) {
    Vector2D v1(5.0, 8.0);
    Vector2D v2(3.0, 4.0);
    Vector2D result = v1 - v2;
    EXPECT_EQ(result[0], 2.0);
    EXPECT_EQ(result[1], 4.0);
}

TEST(DimensionAgnosticVector, VectorOperators_ScalarMultiplication) {
    Vector2D v(2.0, 3.0);
    Vector2D result = v * 2.0;
    EXPECT_EQ(result[0], 4.0);
    EXPECT_EQ(result[1], 6.0);
}

TEST(DimensionAgnosticVector, VectorOperators_ScalarDivision) {
    Vector2D v(10.0, 20.0);
    Vector2D result = v / 2.0;
    EXPECT_EQ(result[0], 5.0);
    EXPECT_EQ(result[1], 10.0);
}

TEST(DimensionAgnosticVector, VectorOperators_Negation) {
    Vector2D v(1.0, -2.0);
    Vector2D result = -v;
    EXPECT_EQ(result[0], -1.0);
    EXPECT_EQ(result[1], 2.0);
}

TEST(DimensionAgnosticVector, VectorOperators_CompoundAddition) {
    Vector2D v(1.0, 2.0);
    Vector2D other(3.0, 4.0);
    v += other;
    EXPECT_EQ(v[0], 4.0);
    EXPECT_EQ(v[1], 6.0);
}

TEST(DimensionAgnosticVector, VectorOperators_CompoundMultiplication) {
    Vector2D v(2.0, 3.0);
    v *= 2.0;
    EXPECT_EQ(v[0], 4.0);
    EXPECT_EQ(v[1], 6.0);
}

// Test compile-time dimension info
TEST(DimensionAgnosticVector, CompileTime_DimensionInformation) {
    EXPECT_EQ(Vector1D::dimension, 1);
    EXPECT_EQ(Vector2D::dimension, 2);
    EXPECT_EQ(Vector3D::dimension, 3);
}


