#include "core/parameters/sph_parameters_builder_base.hpp"
#include "core/parameters/ssph_parameters_builder.hpp"
#include "core/parameters/disph_parameters_builder.hpp"
#include "core/parameters/gsph_parameters_builder.hpp"
#include <gtest/gtest.h>
#include <memory>

using namespace sph;

// ==================== Test Fixture ====================

class AlgorithmParametersBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset for each test
    }
    
    // Helper to create base parameters with all required fields
    SPHParametersBuilderBase create_valid_base() {
        return SPHParametersBuilderBase()
            .with_time(0.0, 1.0, 0.1)
            .with_cfl(0.3, 0.25)
            .with_physics(50, 1.4)
            .with_kernel("cubic_spline");
    }
};

// ==================== SSPH Builder Tests ====================

TEST_F(AlgorithmParametersBuilderTest, SSPH_RequiresArtificialViscosity) {
    // Given: Valid base parameters
    auto base = create_valid_base();
    
    // When: Transitioning to SSPH
    auto ssph_builder = base.as_ssph();
    
    // Then: Cannot build without setting artificial viscosity
    EXPECT_THROW(ssph_builder.build(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, SSPH_BuildSucceedsWithViscosity) {
    // Given: SSPH builder with viscosity set
    auto base = create_valid_base();
    auto ssph_builder = base.as_ssph()
        .with_artificial_viscosity(1.0);
    
    // When: Building parameters
    auto params = ssph_builder.build();
    
    // Then: Build succeeds and type is SSPH
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->type, SPHType::SSPH);
    EXPECT_DOUBLE_EQ(params->av.alpha, 1.0);
    EXPECT_TRUE(params->av.use_balsara_switch);
}

TEST_F(AlgorithmParametersBuilderTest, SSPH_ArtificialViscosityWithAllOptions) {
    // Given: SSPH builder with all viscosity options
    auto base = create_valid_base();
    auto ssph_builder = base.as_ssph()
        .with_artificial_viscosity(
            2.0,    // alpha
            false,  // no Balsara
            true,   // time-dependent
            3.0,    // alpha_max
            0.05,   // alpha_min
            0.15    // epsilon
        );
    
    // When: Building parameters
    auto params = ssph_builder.build();
    
    // Then: All options are set correctly
    EXPECT_DOUBLE_EQ(params->av.alpha, 2.0);
    EXPECT_FALSE(params->av.use_balsara_switch);
    EXPECT_TRUE(params->av.use_time_dependent_av);
    EXPECT_DOUBLE_EQ(params->av.alpha_max, 3.0);
    EXPECT_DOUBLE_EQ(params->av.alpha_min, 0.05);
    EXPECT_DOUBLE_EQ(params->av.epsilon, 0.15);
}

TEST_F(AlgorithmParametersBuilderTest, SSPH_ArtificialConductivity) {
    // Given: SSPH builder with viscosity and conductivity
    auto base = create_valid_base();
    auto ssph_builder = base.as_ssph()
        .with_artificial_viscosity(1.0)
        .with_artificial_conductivity(1.5);
    
    // When: Building parameters
    auto params = ssph_builder.build();
    
    // Then: Conductivity is set
    EXPECT_TRUE(params->ac.is_valid);
    EXPECT_DOUBLE_EQ(params->ac.alpha, 1.5);
}

TEST_F(AlgorithmParametersBuilderTest, SSPH_InvalidViscosityAlpha) {
    // Given: SSPH builder with negative alpha
    auto base = create_valid_base();
    auto ssph_builder = base.as_ssph()
        .with_artificial_viscosity(-1.0);
    
    // When/Then: Build fails with error
    EXPECT_THROW(ssph_builder.build(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, SSPH_InvalidTimeDependentParams) {
    // Given: Time-dependent viscosity with alpha_max < alpha_min
    auto base = create_valid_base();
    auto ssph_builder = base.as_ssph()
        .with_artificial_viscosity(
            1.0,    // alpha
            true,   // Balsara
            true,   // time-dependent
            0.1,    // alpha_max (smaller!)
            2.0     // alpha_min (larger!)
        );
    
    // When/Then: Build fails
    EXPECT_THROW(ssph_builder.build(), std::runtime_error);
}

// ==================== DISPH Builder Tests ====================

TEST_F(AlgorithmParametersBuilderTest, DISPH_RequiresArtificialViscosity) {
    // Given: Valid base parameters
    auto base = create_valid_base();
    
    // When: Transitioning to DISPH
    auto disph_builder = base.as_disph();
    
    // Then: Cannot build without setting artificial viscosity
    EXPECT_THROW(disph_builder.build(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, DISPH_BuildSucceedsWithViscosity) {
    // Given: DISPH builder with viscosity set
    auto base = create_valid_base();
    auto disph_builder = base.as_disph()
        .with_artificial_viscosity(1.0);
    
    // When: Building parameters
    auto params = disph_builder.build();
    
    // Then: Build succeeds and type is DISPH
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->type, SPHType::DISPH);
    EXPECT_DOUBLE_EQ(params->av.alpha, 1.0);
}

// ==================== GSPH Builder Tests ====================

TEST_F(AlgorithmParametersBuilderTest, GSPH_BuildSucceedsWithoutViscosity) {
    // Given: Valid base parameters
    auto base = create_valid_base();
    
    // When: Transitioning to GSPH and building
    auto params = base.as_gsph().build();
    
    // Then: Build succeeds without needing artificial viscosity
    ASSERT_NE(params, nullptr);
    EXPECT_EQ(params->type, SPHType::GSPH);
}

TEST_F(AlgorithmParametersBuilderTest, GSPH_2ndOrderMUSCL) {
    // Given: GSPH builder with 2nd order enabled
    auto base = create_valid_base();
    auto gsph_builder = base.as_gsph()
        .with_2nd_order_muscl(true);
    
    // When: Building parameters
    auto params = gsph_builder.build();
    
    // Then: 2nd order flag is set
    EXPECT_TRUE(params->gsph.is_2nd_order);
}

TEST_F(AlgorithmParametersBuilderTest, GSPH_1stOrderDefault) {
    // Given: GSPH builder without calling with_2nd_order_muscl
    auto base = create_valid_base();
    auto gsph_builder = base.as_gsph();
    
    // When: Building parameters
    auto params = gsph_builder.build();
    
    // Then: 1st order by default (more stable)
    EXPECT_FALSE(params->gsph.is_2nd_order);
}

TEST_F(AlgorithmParametersBuilderTest, GSPH_Disable2ndOrder) {
    // Given: GSPH builder explicitly disabling 2nd order
    auto base = create_valid_base();
    auto gsph_builder = base.as_gsph()
        .with_2nd_order_muscl(false);
    
    // When: Building parameters
    auto params = gsph_builder.build();
    
    // Then: 2nd order is disabled
    EXPECT_FALSE(params->gsph.is_2nd_order);
}

// ==================== GSPH Does NOT Have Artificial Viscosity ====================

// NOTE: The following test would NOT compile if uncommented:
// This is EXACTLY what we want - compile-time type safety!
//
// TEST_F(AlgorithmParametersBuilderTest, GSPH_CannotSetArtificialViscosity) {
//     auto base = create_valid_base();
//     auto gsph_builder = base.as_gsph()
//         .with_artificial_viscosity(1.0);  // ← COMPILE ERROR!
//     
//     auto params = gsph_builder.build();
// }
//
// Compilation would fail with:
// "error: no member named 'with_artificial_viscosity' in 
//  'sph::AlgorithmParametersBuilder<sph::GSPHTag>'"

// ==================== Base Builder Tests ====================

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_RequiresTime) {
    // Given: Base builder missing time
    auto base = SPHParametersBuilderBase()
        .with_cfl(0.3, 0.25)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline");
    
    // When/Then: Cannot transition to algorithm without time
    EXPECT_THROW(base.as_gsph(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_RequiresCFL) {
    // Given: Base builder missing CFL
    auto base = SPHParametersBuilderBase()
        .with_time(0.0, 1.0, 0.1)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline");
    
    // When/Then: Cannot transition to algorithm without CFL
    EXPECT_THROW(base.as_gsph(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_RequiresPhysics) {
    // Given: Base builder missing physics
    auto base = SPHParametersBuilderBase()
        .with_time(0.0, 1.0, 0.1)
        .with_cfl(0.3, 0.25)
        .with_kernel("cubic_spline");
    
    // When/Then: Cannot transition to algorithm without physics
    EXPECT_THROW(base.as_gsph(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_RequiresKernel) {
    // Given: Base builder missing kernel
    auto base = SPHParametersBuilderBase()
        .with_time(0.0, 1.0, 0.1)
        .with_cfl(0.3, 0.25)
        .with_physics(50, 1.4);
    
    // When/Then: Cannot transition to algorithm without kernel
    EXPECT_THROW(base.as_gsph(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_ValidatesTimeRange) {
    // Given: Base builder with end <= start
    auto base = SPHParametersBuilderBase()
        .with_time(1.0, 0.5, 0.1)  // end < start!
        .with_cfl(0.3, 0.25)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline");
    
    // When/Then: Validation fails
    EXPECT_THROW(base.as_gsph(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_ValidatesCFLRange) {
    // Given: Base builder with CFL > 1.0
    auto base = SPHParametersBuilderBase()
        .with_time(0.0, 1.0, 0.1)
        .with_cfl(1.5, 0.25)  // CFL_sound > 1.0!
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline");
    
    // When/Then: Validation fails
    EXPECT_THROW(base.as_gsph(), std::runtime_error);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_Gravity) {
    // Given: Base builder with gravity
    auto base = create_valid_base()
        .with_gravity(9.81, 0.7);
    
    // When: Building GSPH parameters
    auto params = base.as_gsph().build();
    
    // Then: Gravity is set
    EXPECT_TRUE(params->gravity.is_valid);
    EXPECT_DOUBLE_EQ(params->gravity.constant, 9.81);
    EXPECT_DOUBLE_EQ(params->gravity.theta, 0.7);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_PeriodicBoundary) {
    // Given: Base builder with periodic boundary
    std::array<real, 3> min = {-0.5, -0.5, -0.5};
    std::array<real, 3> max = {1.5, 1.5, 1.5};
    
    auto base = create_valid_base()
        .with_periodic_boundary(min, max);
    
    // When: Building parameters
    auto params = base.as_gsph().build();
    
    // Then: Periodic boundary is set
    EXPECT_TRUE(params->periodic.is_valid);
    EXPECT_EQ(params->periodic.range_min, min);
    EXPECT_EQ(params->periodic.range_max, max);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_TreeParams) {
    // Given: Base builder with custom tree parameters
    auto base = create_valid_base()
        .with_tree_params(15, 4);
    
    // When: Building parameters
    auto params = base.as_gsph().build();
    
    // Then: Tree parameters are set
    EXPECT_EQ(params->tree.max_level, 15);
    EXPECT_EQ(params->tree.leaf_particle_num, 4);
}

TEST_F(AlgorithmParametersBuilderTest, BaseBuilder_IterativeSmoothingLength) {
    // Given: Base builder disabling iterative sml
    auto base = create_valid_base()
        .with_iterative_smoothing_length(false);
    
    // When: Building parameters
    auto params = base.as_gsph().build();
    
    // Then: Iterative sml is disabled
    EXPECT_FALSE(params->iterative_sml);
}

// ==================== Integration Tests ====================

TEST_F(AlgorithmParametersBuilderTest, Integration_ShockTubeGSPH) {
    // Given: Full shock tube configuration
    auto params = SPHParametersBuilderBase()
        .with_time(0.0, 0.15, 0.01)
        .with_cfl(0.3, 0.25)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline")
        .with_tree_params(20, 1)
        .with_iterative_smoothing_length(true)
        .as_gsph()
        .with_2nd_order_muscl(false)
        .build();
    
    // Then: All parameters are correctly set
    EXPECT_EQ(params->type, SPHType::GSPH);
    EXPECT_DOUBLE_EQ(params->time.start, 0.0);
    EXPECT_DOUBLE_EQ(params->time.end, 0.15);
    EXPECT_DOUBLE_EQ(params->cfl.sound, 0.3);
    EXPECT_EQ(params->physics.neighbor_number, 50);
    EXPECT_DOUBLE_EQ(params->physics.gamma, 1.4);
    EXPECT_FALSE(params->gsph.is_2nd_order);
}

TEST_F(AlgorithmParametersBuilderTest, Integration_DamBreakSSPH) {
    // Given: Full dam break configuration
    auto params = SPHParametersBuilderBase()
        .with_time(0.0, 5.0, 0.1)
        .with_cfl(0.25, 0.2)
        .with_physics(40, 7.0)
        .with_kernel("wendland")
        .with_gravity(9.81)
        .as_ssph()
        .with_artificial_viscosity(0.01, true, false)
        .build();
    
    // Then: All parameters are correctly set
    EXPECT_EQ(params->type, SPHType::SSPH);
    EXPECT_TRUE(params->gravity.is_valid);
    EXPECT_DOUBLE_EQ(params->gravity.constant, 9.81);
    EXPECT_DOUBLE_EQ(params->av.alpha, 0.01);
    EXPECT_TRUE(params->av.use_balsara_switch);
}

// ==================== Error Message Quality Tests ====================

TEST_F(AlgorithmParametersBuilderTest, ErrorMessage_MissingViscositySSPH) {
    // Given: SSPH without viscosity
    auto base = create_valid_base();
    auto ssph_builder = base.as_ssph();
    
    // When: Building fails
    try {
        ssph_builder.build();
        FAIL() << "Expected exception";
    } catch (const std::runtime_error& e) {
        // Then: Error message mentions artificial viscosity
        std::string msg(e.what());
        EXPECT_TRUE(msg.find("artificial viscosity") != std::string::npos);
        EXPECT_TRUE(msg.find("SSPH") != std::string::npos);
    }
}

TEST_F(AlgorithmParametersBuilderTest, ErrorMessage_MissingTime) {
    // Given: Base without time
    auto base = SPHParametersBuilderBase()
        .with_cfl(0.3, 0.25)
        .with_physics(50, 1.4)
        .with_kernel("cubic_spline");
    
    // When: Transition fails
    try {
        base.as_gsph();
        FAIL() << "Expected exception";
    } catch (const std::runtime_error& e) {
        // Then: Error message mentions time
        std::string msg(e.what());
        EXPECT_TRUE(msg.find("time") != std::string::npos ||
                    msg.find("Time") != std::string::npos);
    }
}

// Main provided by GTest::gtest_main

