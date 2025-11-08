#pragma once

#include "defines.hpp"
#include "core/particles/sph_types.hpp"
#include <array>
#include <string>
#include <variant>
#include <optional>

namespace sph
{

// Forward declarations for friend access
class SPHParametersBuilderBase;
template<typename AlgorithmTag> class AlgorithmParametersBuilder;
struct SSPHTag;
struct DISPHTag;
struct GSPHTag;

/**
 * @brief SPH simulation parameters
 * 
 * CRITICAL: Direct mutation of parameters is FORBIDDEN at compile-time.
 * Members are private and can ONLY be set via the type-safe builder pattern:
 * 
 * ✅ CORRECT:
 *   auto params = SPHParametersBuilderBase()
 *       .with_time(0.0, 3.0, 0.1)
 *       .with_gravity(1.0, 0.5)
 *       .as_ssph()
 *       .with_artificial_viscosity(1.0)
 *       .build();
 * 
 * ❌ COMPILE ERROR (intentional):
 *   param->time.end = 3.0;           // Private member!
 *   param->gravity.is_valid = true;  // Private member!
 * 
 * This enforces:
 * - Type safety: Cannot set invalid parameter combinations
 * - Validation: All required parameters must be set
 * - Discoverability: IDE guides you to correct methods
 * - Immutability: Parameters cannot be changed after construction
 */
struct SPHParameters {
    // Grant exclusive write access to builder classes
    friend class SPHParametersBuilderBase;
    friend class AlgorithmParametersBuilder<SSPHTag>;
    friend class AlgorithmParametersBuilder<DISPHTag>;
    friend class AlgorithmParametersBuilder<GSPHTag>;

private:
    // ==================== PRIVATE MEMBERS - BUILDER ACCESS ONLY ====================
    
    // Spatial dimension (1, 2, or 3)
    int dimension = 1;

    struct Time {
        real start;
        real end;
        real output;
        real energy;
    } time;

    SPHType type;

    struct CFL {
        real sound;
        real force;
    } cfl;

    struct ArtificialViscosity {
        real alpha;
        bool use_balsara_switch;
        bool use_time_dependent_av;
        real alpha_max;
        real alpha_min;
        real epsilon; // tau = h / (epsilon * c)
    } av;

    struct ArtificialConductivity {
        real alpha;
        bool is_valid;
    } ac;

    struct Tree {
        int max_level;
        int leaf_particle_num;
    } tree;

    struct Physics {
        int neighbor_number;
        real gamma;
    } physics;

    KernelType kernel;

    bool iterative_sml;

public:
    // ==================== SMOOTHING LENGTH POLICY ====================
    
    /// Smoothing length minimum enforcement policy
    enum class SmoothingLengthPolicy {
        NO_MIN,          ///< No minimum - allow h to collapse naturally
        CONSTANT_MIN,    ///< Enforce constant h_min (for testing/debugging)
        PHYSICS_BASED    ///< Physics-based h_min = alpha * (m/rho_max)^(1/dim)
    };

private:
    struct SmoothingLength {
        SmoothingLengthPolicy policy;
        real h_min_constant;           ///< Used when policy == CONSTANT_MIN
        real expected_max_density;     ///< Used when policy == PHYSICS_BASED
        real h_min_coefficient;        ///< Coefficient α in h_min = α*(m/ρ_max)^(1/dim), typically 2.0
    } smoothing_length;

    // Legacy periodic boundary (maintained for backward compatibility)
    struct Periodic {
        bool is_valid;
        std::array<real, 3> range_max;  // Max size for 3D, unused elements for 1D/2D
        std::array<real, 3> range_min;
    } periodic;
    
    // New flexible boundary configuration
    struct Boundary {
        bool is_valid;
        std::array<std::string, 3> types;        // "periodic", "mirror", "none" per dimension
        std::array<bool, 3> enable_lower;         // Enable lower boundary [x, y, z]
        std::array<bool, 3> enable_upper;         // Enable upper boundary [x, y, z]
        std::array<real, 3> range_min;
        std::array<real, 3> range_max;
        std::array<std::string, 3> mirror_types;  // "no_slip", "free_slip"
    } boundary;

public:
    // ==================== TYPE-SAFE GRAVITY (std::variant) ====================
    
    /// No gravity - particles are non-interacting or use external potential
    struct NoGravity {};
    
    /// Newtonian self-gravity with Barnes-Hut tree
    struct NewtonianGravity {
        real constant;  // Gravitational constant G
        real theta;     // Barnes-Hut opening angle
        
        constexpr NewtonianGravity(real g, real t) : constant(g), theta(t) {}
    };
    
    /// Modified gravity models (future extension)
    struct ModifiedGravity {
        real constant;
        real theta;
        real alpha;  // Modification parameter
        
        constexpr ModifiedGravity(real g, real t, real a) 
            : constant(g), theta(t), alpha(a) {}
    };
    
    /// Type-safe gravity configuration - NO runtime boolean checks!
    /// Compiler enforces correct usage via pattern matching
    using GravityVariant = std::variant<NoGravity, NewtonianGravity, ModifiedGravity>;

private:
    GravityVariant gravity = NoGravity{};  // Default: no gravity

    struct GSPH {
        bool is_2nd_order;
    } gsph;
    
public:
    // ==================== PUBLIC READ-ONLY ACCESSORS ====================
    
    /// Read-only access to parameters - mutation forbidden
    const Time& get_time() const { return time; }
    const CFL& get_cfl() const { return cfl; }
    const Physics& get_physics() const { return physics; }
    const GravityVariant& get_gravity() const { return gravity; }
    const ArtificialViscosity& get_av() const { return av; }
    const ArtificialConductivity& get_ac() const { return ac; }
    const Tree& get_tree() const { return tree; }
    const Periodic& get_periodic() const { return periodic; }
    const Boundary& get_boundary() const { return boundary; }
    const GSPH& get_gsph() const { return gsph; }
    const SmoothingLength& get_smoothing_length() const { return smoothing_length; }
    
    int get_dimension() const { return dimension; }
    SPHType get_type() const { return type; }
    KernelType get_kernel() const { return kernel; }
    bool get_iterative_sml() const { return iterative_sml; }
    
    // ==================== TYPE-SAFE GRAVITY HELPERS ====================
    
    /// Check if gravity is enabled (type-safe pattern matching)
    bool has_gravity() const {
        return !std::holds_alternative<NoGravity>(gravity);
    }
    
    /// Get Newtonian gravity config (throws if not Newtonian)
    const NewtonianGravity& get_newtonian_gravity() const {
        return std::get<NewtonianGravity>(gravity);
    }
    
    /// Visit gravity configuration with type-safe pattern matching
    /// Example usage:
    ///   std::visit([](auto&& g) { 
    ///       if constexpr (std::is_same_v<std::decay_t<decltype(g)>, NewtonianGravity>) {
    ///           // Use g.constant, g.theta
    ///       }
    ///   }, params->get_gravity());
    template<typename Visitor>
    auto visit_gravity(Visitor&& visitor) const {
        return std::visit(std::forward<Visitor>(visitor), gravity);
    }
};

}