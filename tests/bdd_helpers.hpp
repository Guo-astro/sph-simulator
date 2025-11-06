#pragma once

#include <gtest/gtest.h>
#include <string>
#include <functional>
#include <chrono>
#include <iostream>

/**
 * @file bdd_helpers.hpp
 * @brief BDD (Behavior-Driven Development) style testing macros for Google Test
 * 
 * Usage:
 *   FEATURE("Vector Operations") {
 *     SCENARIO("Adding two vectors") {
 *       GIVEN("Two 3D vectors") {
 *         vec_t v1 = {1.0, 2.0, 3.0};
 *         vec_t v2 = {4.0, 5.0, 6.0};
 *         
 *         WHEN("We add them together") {
 *           vec_t result = v1 + v2;
 *           
 *           THEN("The result should be correct") {
 *             EXPECT_DOUBLE_EQ(result[0], 5.0);
 *             EXPECT_DOUBLE_EQ(result[1], 7.0);
 *             EXPECT_DOUBLE_EQ(result[2], 9.0);
 *           }
 *         }
 *       }
 *     }
 *   }
 */

// BDD-style macros that map to Google Test
#define FEATURE(description) namespace
#define SCENARIO(suite_name, test_name) TEST(suite_name, test_name)
#define GIVEN(description) 
#define WHEN(description)
#define THEN(description)
#define AND(description)
#define AND_THEN(description)
#define BUT(description)

// Test fixture with BDD naming
#define SCENARIO_WITH_FIXTURE(fixture_class, scenario_name) \
    TEST_F(fixture_class, scenario_name)

// Parameterized test with BDD naming
#define SCENARIO_WITH_PARAMS(test_suite, scenario_name) \
    TEST_P(test_suite, scenario_name)

// Custom assertions with better error messages
#define EXPECT_NEAR_PERCENT(val1, val2, percent) \
    EXPECT_NEAR(val1, val2, std::abs(val1) * percent / 100.0) \
    << "Values differ by more than " << percent << "%"

#define EXPECT_VECTOR_NEAR(vec1, vec2, tolerance) \
    do { \
        for (int i = 0; i < DIM; ++i) { \
            EXPECT_NEAR(vec1[i], vec2[i], tolerance) \
                << "Vector component " << i << " differs"; \
        } \
    } while(0)

// Edge case testing helpers
namespace test_helpers {
    
    // Common edge case values
    constexpr double EPSILON = 1e-15;
    constexpr double TINY = 1e-10;
    constexpr double VERY_LARGE = 1e10;
    constexpr double ZERO = 0.0;
    constexpr double NEGATIVE_TINY = -1e-10;
    
    // Helper to test boundary conditions
    template<typename Func>
    void test_edge_cases(Func test_func) {
        test_func(ZERO);
        test_func(EPSILON);
        test_func(TINY);
        test_func(HUGE);
        test_func(-TINY);
        test_func(-HUGE);
    }
    
    // Helper to test array bounds
    template<typename T>
    bool is_in_bounds(const T& value, const T& min, const T& max) {
        return value >= min && value <= max;
    }
    
    // Helper to check for NaN or Inf
    inline bool is_finite(double value) {
        return std::isfinite(value);
    }
    
} // namespace test_helpers

// Performance testing helpers
#define BENCHMARK_START() auto start_time = std::chrono::high_resolution_clock::now()

#define BENCHMARK_END(operation_name) \
    do { \
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - start_time); \
        std::cout << "[BENCHMARK] " << operation_name << " took " \
                  << duration.count() << " μs" << std::endl; \
    } while(0)
