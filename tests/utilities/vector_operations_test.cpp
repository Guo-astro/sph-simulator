// Use 1D for tests by default
static constexpr int Dim = 1;

#include "../bdd_helpers.hpp"
#include "defines.hpp"
#include "core/utilities/vector.hpp"
#include <cmath>
#include <limits>

using sph::inner_product;
using sph::abs;

FEATURE(VectorOperations) {

SCENARIO(InnerProduct, BasicCalculation) {
    GIVEN("Two vectors in " + std::to_string(Dim) + "D space") {
        real v1[Dim];
        real v2[Dim];
        
        WHEN("Vectors are parallel and pointing same direction") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = 1.0;
                v2[i] = 1.0;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Inner product should equal sum of squared components") {
                EXPECT_DOUBLE_EQ(result, static_cast<double>(Dim));
            }
        }
        
        WHEN("Vectors are orthogonal") {
#if Dim >= 2
            v1[0] = 1.0;
            v1[1] = 0.0;
            v2[0] = 0.0;
            v2[1] = 1.0;
#if Dim == 3
            v1[2] = 0.0;
            v2[2] = 0.0;
#endif
            
            real result = inner_product(v1, v2);
            
            THEN("Inner product should be zero") {
                EXPECT_DOUBLE_EQ(result, 0.0);
            }
#endif
        }
        
        WHEN("Vectors are antiparallel") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = 1.0;
                v2[i] = -1.0;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Inner product should be negative") {
                EXPECT_DOUBLE_EQ(result, -static_cast<double>(Dim));
            }
        }
    }
}

SCENARIO(InnerProduct, EdgeCases) {
    GIVEN("Vectors with edge case values") {
        real v1[Dim];
        real v2[Dim];
        
        WHEN("Both vectors are zero") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = 0.0;
                v2[i] = 0.0;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Inner product should be exactly zero") {
                EXPECT_DOUBLE_EQ(result, 0.0);
            }
        }
        
        WHEN("One vector is zero") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = 1.0;
                v2[i] = 0.0;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Inner product should be zero") {
                EXPECT_DOUBLE_EQ(result, 0.0);
            }
        }
        
        WHEN("Vectors have very small values") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = 1e-10;
                v2[i] = 1e-10;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Result should be very small but finite") {
                EXPECT_TRUE(std::isfinite(result));
                EXPECT_GT(result, 0.0);
                EXPECT_LT(result, 1e-15);
            }
        }
        
        WHEN("Vectors have very large values") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = 1e10;
                v2[i] = 1e10;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Result should be very large but finite") {
                EXPECT_TRUE(std::isfinite(result));
                EXPECT_GT(result, 1e19);
            }
        }
        
        WHEN("Vectors have mixed large and small values") {
            v1[0] = 1e10;
            v2[0] = 1e-10;
#if Dim >= 2
            v1[1] = 1e-10;
            v2[1] = 1e10;
#endif
#if Dim == 3
            v1[2] = 1.0;
            v2[2] = 1.0;
#endif
            
            real result = inner_product(v1, v2);
            
            THEN("Result should be dominated by product of similar magnitudes") {
                EXPECT_TRUE(std::isfinite(result));
                EXPECT_GT(result, 0.0);
            }
        }
    }
}

SCENARIO(InnerProduct, NumericalStability) {
    GIVEN("Vectors that could cause numerical issues") {
        real v1[Dim];
        real v2[Dim];
        
        WHEN("Computing with values near machine epsilon") {
            double eps = std::numeric_limits<double>::epsilon();
            for (int i = 0; i < Dim; ++i) {
                v1[i] = eps;
                v2[i] = eps;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Result should be finite") {
                EXPECT_TRUE(std::isfinite(result));
            }
        }
        
        WHEN("Computing with alternating signs") {
            for (int i = 0; i < Dim; ++i) {
                v1[i] = (i % 2 == 0) ? 1.0 : -1.0;
                v2[i] = (i % 2 == 0) ? 1.0 : -1.0;
            }
            
            real result = inner_product(v1, v2);
            
            THEN("Result should equal dimension") {
                EXPECT_DOUBLE_EQ(result, static_cast<double>(Dim));
            }
        }
    }
}

} // FEATURE(VectorOperations)

FEATURE(MathematicalFunctions) {

SCENARIO(PowerFunctions, BasicCalculation) {
    GIVEN("A positive number") {
        double x = 2.0;
        
        WHEN("Computing squares") {
            double result = sqr(x);
            
            THEN("Result should be x*x") {
                EXPECT_DOUBLE_EQ(result, 4.0);
            }
        }
        
        WHEN("Computing cubes") {
            double result = pow3(x);
            
            THEN("Result should be x*x*x") {
                EXPECT_DOUBLE_EQ(result, 8.0);
            }
        }
        
        WHEN("Computing fourth power") {
            double result = pow4(x);
            
            THEN("Result should be x^4") {
                EXPECT_DOUBLE_EQ(result, 16.0);
            }
        }
        
        WHEN("Computing fifth power") {
            double result = pow5(x);
            
            THEN("Result should be x^5") {
                EXPECT_DOUBLE_EQ(result, 32.0);
            }
        }
        
        WHEN("Computing sixth power") {
            double result = pow6(x);
            
            THEN("Result should be x^6") {
                EXPECT_DOUBLE_EQ(result, 64.0);
            }
        }
    }
}

SCENARIO(PowerFunctions, EdgeCases) {
    GIVEN("Edge case input values") {
        WHEN("Input is zero") {
            double x = 0.0;
            
            THEN("All powers should be zero") {
                EXPECT_DOUBLE_EQ(sqr(x), 0.0);
                EXPECT_DOUBLE_EQ(pow3(x), 0.0);
                EXPECT_DOUBLE_EQ(pow4(x), 0.0);
                EXPECT_DOUBLE_EQ(pow5(x), 0.0);
                EXPECT_DOUBLE_EQ(pow6(x), 0.0);
            }
        }
        
        WHEN("Input is one") {
            double x = 1.0;
            
            THEN("All powers should be one") {
                EXPECT_DOUBLE_EQ(sqr(x), 1.0);
                EXPECT_DOUBLE_EQ(pow3(x), 1.0);
                EXPECT_DOUBLE_EQ(pow4(x), 1.0);
                EXPECT_DOUBLE_EQ(pow5(x), 1.0);
                EXPECT_DOUBLE_EQ(pow6(x), 1.0);
            }
        }
        
        WHEN("Input is negative one") {
            double x = -1.0;
            
            THEN("Even powers should be positive, odd powers negative") {
                EXPECT_DOUBLE_EQ(sqr(x), 1.0);
                EXPECT_DOUBLE_EQ(pow3(x), -1.0);
                EXPECT_DOUBLE_EQ(pow4(x), 1.0);
                EXPECT_DOUBLE_EQ(pow5(x), -1.0);
                EXPECT_DOUBLE_EQ(pow6(x), 1.0);
            }
        }
        
        WHEN("Input is very small positive") {
            double x = 1e-5;
            
            THEN("Higher powers should be even smaller") {
                EXPECT_LT(sqr(x), x);
                EXPECT_LT(pow3(x), sqr(x));
                EXPECT_LT(pow4(x), pow3(x));
                EXPECT_LT(pow5(x), pow4(x));
                EXPECT_LT(pow6(x), pow5(x));
            }
        }
        
        WHEN("Input is very large") {
            double x = 1e3;
            
            THEN("Higher powers should grow rapidly") {
                EXPECT_GT(sqr(x), x);
                EXPECT_GT(pow3(x), sqr(x));
                EXPECT_GT(pow4(x), pow3(x));
                EXPECT_TRUE(std::isfinite(pow6(x)));
            }
        }
        
        WHEN("Input is negative") {
            double x = -2.0;
            
            THEN("Signs should alternate appropriately") {
                EXPECT_GT(sqr(x), 0.0);   // +4
                EXPECT_LT(pow3(x), 0.0);  // -8
                EXPECT_GT(pow4(x), 0.0);  // +16
                EXPECT_LT(pow5(x), 0.0);  // -32
                EXPECT_GT(pow6(x), 0.0);  // +64
            }
        }
    }
}

SCENARIO(PowerFunctions, NumericalPrecision) {
    GIVEN("Values that test numerical precision") {
        WHEN("Computing with machine epsilon") {
            double eps = std::numeric_limits<double>::epsilon();
            
            THEN("Results should remain finite") {
                EXPECT_TRUE(std::isfinite(sqr(eps)));
                EXPECT_TRUE(std::isfinite(pow3(eps)));
                // Higher powers may underflow to zero, which is acceptable
            }
        }
        
        WHEN("Computing with values near overflow threshold") {
            double x = std::sqrt(std::numeric_limits<double>::max()) / 10.0;
            
            THEN("Lower powers should be finite") {
                EXPECT_TRUE(std::isfinite(sqr(x)));
                // pow3 may overflow for values near sqrt(max)/10, which is acceptable
                // The cube of ~1.34e153 is ~2.4e459, which exceeds max double (~1.8e308)
            }
        }
        
        WHEN("Comparing with std::pow") {
            double x = 3.14159;
            
            THEN("Custom power functions should match std::pow") {
                EXPECT_NEAR(sqr(x), std::pow(x, 2.0), 1e-14);
                EXPECT_NEAR(pow3(x), std::pow(x, 3.0), 1e-13);
                EXPECT_NEAR(pow4(x), std::pow(x, 4.0), 1e-12);
            }
        }
    }
}

} // FEATURE(MathematicalFunctions)

FEATURE(VectorMagnitude) {

SCENARIO(VectorNorm, Calculation) {
    GIVEN("Various vectors") {
        real v[Dim];
        
        WHEN("Vector is unit vector in x-direction") {
            v[0] = 1.0;
#if Dim >= 2
            v[1] = 0.0;
#endif
#if Dim == 3
            v[2] = 0.0;
#endif
            
            double mag_sq = inner_product(v, v);
            double mag = std::sqrt(mag_sq);
            
            THEN("Magnitude should be 1") {
                EXPECT_DOUBLE_EQ(mag, 1.0);
            }
        }
        
        WHEN("Vector has all components equal") {
            for (int i = 0; i < Dim; ++i) {
                v[i] = 1.0;
            }
            
            double mag_sq = inner_product(v, v);
            double mag = std::sqrt(mag_sq);
            
            THEN("Magnitude should be sqrt(Dim)") {
                EXPECT_NEAR(mag, std::sqrt(static_cast<double>(Dim)), 1e-14);
            }
        }
        
        WHEN("Vector is zero") {
            for (int i = 0; i < Dim; ++i) {
                v[i] = 0.0;
            }
            
            double mag_sq = inner_product(v, v);
            
            THEN("Magnitude squared should be zero") {
                EXPECT_DOUBLE_EQ(mag_sq, 0.0);
            }
        }
    }
}

} // FEATURE(VectorMagnitude)
