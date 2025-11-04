#include "../../bdd_helpers.hpp"
#include "algorithms/riemann/hll_solver.hpp"
#include "utilities/constants.hpp"
#include <cmath>

using namespace sph::algorithms::riemann;
using namespace sph::utilities::constants;

FEATURE(HLLRiemannSolver) {

SCENARIO(HLLSolver, BasicFunctionality) {
    GIVEN("An HLL solver instance") {
        HLLSolver solver;
        
        THEN("Solver should have correct name") {
            EXPECT_EQ(solver.get_name(), "HLL");
        }
    }
}

SCENARIO(HLLSolver, SodShockTube) {
    GIVEN("Sod shock tube initial conditions") {
        // Classic Riemann problem: high pressure left, low pressure right
        // This is a standard test case from Toro (2009)
        RiemannState left_state;
        left_state.density = ONE;
        left_state.pressure = ONE;
        left_state.velocity = ZERO;
        left_state.sound_speed = std::sqrt(GAMMA_MONOATOMIC * left_state.pressure / left_state.density);
        
        RiemannState right_state;
        right_state.density = 0.125;
        right_state.pressure = 0.1;
        right_state.velocity = ZERO;
        right_state.sound_speed = std::sqrt(GAMMA_MONOATOMIC * right_state.pressure / right_state.density);
        
        WHEN("States are validated") {
            THEN("Both states should be valid") {
                EXPECT_TRUE(left_state.is_valid());
                EXPECT_TRUE(right_state.is_valid());
            }
        }
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Interface pressure should be between left and right") {
                EXPECT_GT(solution.pressure, right_state.pressure);
                EXPECT_LT(solution.pressure, left_state.pressure);
            }
            
            AND("Interface velocity should be positive (rightward flow)") {
                EXPECT_GT(solution.velocity, ZERO);
            }
            
            AND("Solution should be physically valid") {
                EXPECT_TRUE(solution.is_valid());
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_GT(solution.pressure, ZERO);
            }
        }
    }
}

SCENARIO(HLLSolver, VacuumFormation) {
    GIVEN("Two states moving away from each other (vacuum formation)") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        
        // Left state moving leftward
        RiemannState left_state;
        left_state.density = ONE;
        left_state.pressure = ONE;
        left_state.velocity = -10.0;  // Large leftward velocity
        left_state.sound_speed = std::sqrt(GAMMA * left_state.pressure / left_state.density);
        
        // Right state moving rightward
        RiemannState right_state;
        right_state.density = ONE;
        right_state.pressure = ONE;
        right_state.velocity = 10.0;  // Large rightward velocity
        right_state.sound_speed = std::sqrt(GAMMA * right_state.pressure / right_state.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Interface pressure should be very low (near vacuum)") {
                // Pressure should drop significantly when states move apart
                EXPECT_LT(solution.pressure, 0.1 * left_state.pressure);
            }
            
            AND("Interface velocity should be near zero") {
                // Velocity should be near zero in the vacuum region
                EXPECT_NEAR(solution.velocity, ZERO, 1.0);
            }
            
            AND("Solution should remain finite (no NaN or Inf)") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_GT(solution.pressure, ZERO);  // Should remain positive
            }
        }
    }
}

SCENARIO(HLLSolver, StrongShock) {
    GIVEN("Strong shock with extreme pressure ratio") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        constexpr real PRESSURE_RATIO = 1e6;  // Very strong shock
        
        // High pressure, high density left state
        RiemannState left_state;
        left_state.density = 10.0;
        left_state.pressure = PRESSURE_RATIO;
        left_state.velocity = ZERO;
        left_state.sound_speed = std::sqrt(GAMMA * left_state.pressure / left_state.density);
        
        // Low pressure, low density right state
        RiemannState right_state;
        right_state.density = ONE;
        right_state.pressure = ONE;
        right_state.velocity = ZERO;
        right_state.sound_speed = std::sqrt(GAMMA * right_state.pressure / right_state.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Interface pressure should be much higher than right state") {
                EXPECT_GT(solution.pressure, 100.0 * right_state.pressure);
            }
            
            AND("Interface velocity should be large and positive") {
                // Strong shock drives flow rightward
                EXPECT_GT(solution.velocity, 10.0);
            }
            
            AND("Solution should not overflow") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_LT(solution.pressure, 1e20);  // No overflow
            }
        }
    }
}

SCENARIO(HLLSolver, ContactDiscontinuity) {
    GIVEN("Contact discontinuity (pressure equilibrium, density jump)") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        constexpr real PRESSURE_EQUILIBRIUM = ONE;
        
        // High density left state
        RiemannState left_state;
        left_state.density = TWO;       // High density
        left_state.pressure = PRESSURE_EQUILIBRIUM;
        left_state.velocity = ZERO;
        left_state.sound_speed = std::sqrt(GAMMA * left_state.pressure / left_state.density);
        
        // Low density right state
        RiemannState right_state;
        right_state.density = ONE;      // Low density
        right_state.pressure = PRESSURE_EQUILIBRIUM;  // Same pressure
        right_state.velocity = ZERO;
        right_state.sound_speed = std::sqrt(GAMMA * right_state.pressure / right_state.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Interface pressure should be approximately maintained") {
                // HLL is diffusive at contacts but should preserve pressure reasonably
                EXPECT_NEAR(solution.pressure, PRESSURE_EQUILIBRIUM, 0.2);
            }
            
            AND("Interface velocity should be near zero") {
                // No pressure gradient means no flow
                EXPECT_NEAR(solution.velocity, ZERO, 0.1);
            }
        }
    }
}

SCENARIO(HLLSolver, ExtremeDensityRatio) {
    GIVEN("Extreme density ratio (low density right side)") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        constexpr real DENSITY_RATIO = 1e6;
        
        // Very high density left state
        RiemannState left_state;
        left_state.density = DENSITY_RATIO;
        left_state.pressure = ONE;
        left_state.velocity = ZERO;
        left_state.sound_speed = std::sqrt(GAMMA * left_state.pressure / left_state.density);
        
        // Very low density right state
        RiemannState right_state;
        right_state.density = ONE;
        right_state.pressure = ONE;
        right_state.velocity = ZERO;
        right_state.sound_speed = std::sqrt(GAMMA * right_state.pressure / right_state.density);
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Solution should remain stable") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
            }
            
            AND("Pressure should be positive") {
                EXPECT_GT(solution.pressure, ZERO);
            }
            
            AND("Solution should be valid") {
                EXPECT_TRUE(solution.is_valid());
            }
        }
    }
}

SCENARIO(HLLSolver, SonicPoint) {
    GIVEN("Left state with transonic flow") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        
        // Subsonic left state
        RiemannState left_state;
        left_state.density = ONE;
        left_state.pressure = ONE;
        left_state.velocity = ZERO;
        left_state.sound_speed = std::sqrt(GAMMA * left_state.pressure / left_state.density);
        
        // Exactly sonic right state (u = c)
        RiemannState right_state;
        right_state.density = ONE;
        right_state.pressure = ONE;
        right_state.sound_speed = std::sqrt(GAMMA * right_state.pressure / right_state.density);
        right_state.velocity = right_state.sound_speed;  // Exactly sonic!
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Solution should handle sonic point correctly") {
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
                EXPECT_GT(solution.pressure, ZERO);
            }
            
            AND("Velocity should be positive") {
                EXPECT_GT(solution.velocity, ZERO);
            }
        }
    }
}

SCENARIO(HLLSolver, SymmetricStates) {
    GIVEN("Identical left and right states") {
        constexpr real GAMMA = GAMMA_MONOATOMIC;
        
        RiemannState left_state;
        left_state.density = ONE;
        left_state.pressure = ONE;
        left_state.velocity = ZERO;
        left_state.sound_speed = std::sqrt(GAMMA * left_state.pressure / left_state.density);
        
        // Identical right state
        RiemannState right_state = left_state;
        
        WHEN("Solving for interface state") {
            HLLSolver solver;
            auto solution = solver.solve(left_state, right_state);
            
            THEN("Interface state should match input states") {
                EXPECT_NEAR(solution.pressure, left_state.pressure, 1e-10);
                EXPECT_NEAR(solution.velocity, left_state.velocity, 1e-10);
            }
        }
    }
}

SCENARIO(HLLSolver, InvalidInputHandling) {
    GIVEN("Invalid left state (negative density)") {
        RiemannState invalid_left;
        invalid_left.density = -ONE;  // Invalid!
        invalid_left.pressure = ONE;
        invalid_left.velocity = ZERO;
        invalid_left.sound_speed = ONE;
        
        RiemannState valid_right;
        valid_right.density = ONE;
        valid_right.pressure = ONE;
        valid_right.velocity = ZERO;
        valid_right.sound_speed = ONE;
        
        WHEN("Validating states") {
            THEN("Invalid state should be detected") {
                EXPECT_FALSE(invalid_left.is_valid());
                EXPECT_TRUE(valid_right.is_valid());
            }
        }
        
        WHEN("Solving with invalid state") {
            HLLSolver solver;
            auto solution = solver.solve(invalid_left, valid_right);
            
            THEN("Solver should return safe fallback") {
                // Should not crash, should return finite values
                EXPECT_TRUE(std::isfinite(solution.pressure));
                EXPECT_TRUE(std::isfinite(solution.velocity));
            }
        }
    }
}

} // FEATURE(HLLRiemannSolver)
