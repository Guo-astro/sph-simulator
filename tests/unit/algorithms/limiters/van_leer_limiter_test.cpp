// Use 1D for tests by default
static constexpr int Dim = 1;

/**
 * @file van_leer_limiter_test.cpp
 * @brief BDD tests for Van Leer slope limiter
 * 
 * The Van Leer limiter is a Total Variation Diminishing (TVD) slope limiter
 * used in MUSCL reconstruction to prevent spurious oscillations near discontinuities.
 * 
 * Key properties tested:
 * - TVD property: limiter preserves monotonicity
 * - Symmetry: φ(r) = φ(1/r) / r
 * - Extrema preservation: returns 0 when gradients have opposite signs
 * - Second-order accuracy in smooth regions
 * 
 * Reference: van Leer, B. (1979). "Towards the ultimate conservative difference scheme"
 */

#include <gtest/gtest.h>
#include <cmath>
#include "../../../helpers/bdd_helpers.hpp"
#include "algorithms/limiters/van_leer_limiter.hpp"
#include "utilities/constants.hpp"

using namespace sph::algorithms::limiters;
using namespace sph::utilities::constants;

FEATURE(VanLeerLimiter) {
    
    SCENARIO(VanLeerLimiter, BasicFunctionalityAndProperties) {
        GIVEN("a Van Leer limiter instance") {
            VanLeerLimiter limiter;
            
            WHEN("the limiter name is queried") {
                THEN("it returns the correct name") {
                    EXPECT_EQ(limiter.name(), "VanLeer");
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, TVDPropertyMonotonicityPreservation) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("both gradients are positive and similar magnitude") {
                const real upstream_gradient = 2.0;
                const real local_gradient = 2.5;
                
                THEN("the limiter returns a positive value preserving monotonicity") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_GT(phi, ZERO);
                    EXPECT_LE(phi, std::max(upstream_gradient, local_gradient));
                }
            }
            
            WHEN("both gradients are negative and similar magnitude") {
                const real upstream_gradient = -2.0;
                const real local_gradient = -2.5;
                
                THEN("the limiter returns a positive value") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_GT(phi, ZERO);
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, ExtremaDetectionAndPreservation) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("gradients have opposite signs (local extremum)") {
                const real upstream_gradient = 2.0;
                const real local_gradient = -1.5;
                
                THEN("the limiter returns zero to prevent new extrema") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_EQ(phi, ZERO);
                }
            }
            
            WHEN("positive upstream, negative local") {
                const real upstream_gradient = 5.0;
                const real local_gradient = -3.0;
                
                THEN("the limiter returns zero") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_EQ(phi, ZERO);
                }
            }
            
            WHEN("negative upstream, positive local") {
                const real upstream_gradient = -5.0;
                const real local_gradient = 3.0;
                
                THEN("the limiter returns zero") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_EQ(phi, ZERO);
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, SymmetryProperty) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("computing limiter for ratio r and its reciprocal 1/r") {
                const real dq1 = 3.0;
                const real dq2 = 6.0;  // r = dq1/dq2 = 0.5
                
                const real phi_r = limiter.limit(dq1, dq2);
                const real phi_inv_r = limiter.limit(dq2, dq1);
                
                THEN("the symmetry property φ(r) = φ(1/r) / r holds") {
                    const real r = dq1 / dq2;
                    EXPECT_NEAR(phi_r, phi_inv_r / (ONE / r), FLOAT_TOLERANCE);
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, SmoothRegionBehavior) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("gradients are equal (smooth region, r = 1)") {
                const real upstream_gradient = 4.0;
                const real local_gradient = 4.0;
                
                THEN("the limiter returns the gradient value for second-order accuracy") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_NEAR(phi, upstream_gradient, FLOAT_TOLERANCE);
                }
            }
            
            WHEN("gradients are nearly equal") {
                const real upstream_gradient = 3.999;
                const real local_gradient = 4.001;
                
                THEN("the limiter returns approximately the gradient value") {
                    const real phi = limiter.limit(upstream_gradient, local_gradient);
                    EXPECT_NEAR(phi, upstream_gradient, 1e-2);
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, VanLeerFormula) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("specific gradient values are provided") {
                const real dq1 = 2.0;
                const real dq2 = 8.0;
                
                THEN("the Van Leer formula is correctly applied") {
                    const real phi = limiter.limit(dq1, dq2);
                    const real expected = TWO * dq1 * dq2 / (dq1 + dq2);
                    EXPECT_NEAR(phi, expected, FLOAT_TOLERANCE);
                }
            }
            
            WHEN("gradient values are (3.0, 6.0)") {
                const real dq1 = 3.0;
                const real dq2 = 6.0;
                
                THEN("the result is 2*3*6/(3+6) = 4.0") {
                    const real phi = limiter.limit(dq1, dq2);
                    EXPECT_NEAR(phi, 4.0, FLOAT_TOLERANCE);
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, EdgeCasesAndNumericalStability) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("one gradient is zero") {
                const real dq1 = ZERO;
                const real dq2 = 5.0;
                
                THEN("the limiter returns zero") {
                    const real phi = limiter.limit(dq1, dq2);
                    EXPECT_EQ(phi, ZERO);
                }
            }
            
            WHEN("both gradients are zero") {
                const real dq1 = ZERO;
                const real dq2 = ZERO;
                
                THEN("the limiter returns zero") {
                    const real phi = limiter.limit(dq1, dq2);
                    EXPECT_EQ(phi, ZERO);
                }
            }
            
            WHEN("gradients are very small but same sign") {
                const real dq1 = 1e-12;
                const real dq2 = 1e-12;
                
                THEN("the limiter handles it correctly") {
                    const real phi = limiter.limit(dq1, dq2);
                    EXPECT_GT(phi, ZERO);
                    EXPECT_NEAR(phi, 1e-12, 1e-13);
                }
            }
            
            WHEN("gradients have very large magnitude") {
                const real dq1 = 1e10;
                const real dq2 = 2e10;
                
                THEN("the limiter remains stable") {
                    const real phi = limiter.limit(dq1, dq2);
                    const real expected = TWO * dq1 * dq2 / (dq1 + dq2);
                    EXPECT_NEAR(phi / expected, ONE, FLOAT_TOLERANCE);
                }
            }
        }
    }
    
    SCENARIO(VanLeerLimiter, ComparisonWithGradientRatios) {
        GIVEN("a Van Leer limiter") {
            VanLeerLimiter limiter;
            
            WHEN("upstream gradient is much larger (steep gradient)") {
                const real dq1 = 10.0;
                const real dq2 = 1.0;
                
                THEN("the limiter reduces the slope to avoid oscillations") {
                    const real phi = limiter.limit(dq1, dq2);
                    EXPECT_LT(phi, dq1);
                    EXPECT_GT(phi, dq2);
                }
            }
            
            WHEN("local gradient is much larger") {
                const real dq1 = 1.0;
                const real dq2 = 10.0;
                
                THEN("the limiter reduces the slope appropriately") {
                    const real phi = limiter.limit(dq1, dq2);
                    EXPECT_GT(phi, dq1);
                    EXPECT_LT(phi, dq2);
                }
            }
        }
    }
}
