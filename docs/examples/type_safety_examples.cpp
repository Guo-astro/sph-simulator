/**
 * @file type_safety_examples.cpp
 * @brief DEPRECATED: Examples showing compile-time safety of V2 phase-aware plugin interface
 * 
 * NOTE: This file documents the historical V2 approach using phantom types.
 * The current standard is V3 (pure functional interface).
 * See workflows/USAGE_GUIDE.md for V3 documentation.
 * 
 * This file documents what compile errors you get when trying to access
 * uninitialized state. These examples are intentionally commented out
 * because they would fail to compile.
 */

#include "core/plugins/simulation_plugin_v2.hpp"  // DEPRECATED - V2 interface
#include "core/simulation/simulation_phase_view.hpp"  // DEPRECATED - V2 helper
#include <iostream>

using namespace sph;

// ============================================================================
// EXAMPLE 1: Trying to access ghost_manager in Uninitialized phase
// ============================================================================

void example_1_premature_ghost_access() {
    std::shared_ptr<Simulation<2>> sim = std::make_shared<Simulation<2>>(nullptr);
    UninitializedSimulation<2> uninit_sim = 
        UninitializedSimulation<2>::create_uninitialized(sim);
    
    // ❌ COMPILE ERROR: ghost_manager() requires Initialized phase
    // 
    // Error message:
    //   "no matching member function for call to 'ghost_manager'"
    //   note: candidate template ignored: requirement 
    //   'allows_ghost_operations<sph::phase::Uninitialized>::value' was not satisfied
    //
    // auto ghosts = uninit_sim.ghost_manager();
    
    // ✅ CORRECT: Wait until Solver transitions to Initialized phase
    std::cout << "Cannot access ghost_manager until initialized\n";
}

// ============================================================================
// EXAMPLE 2: Trying to build tree in Uninitialized phase
// ============================================================================

void example_2_premature_tree_build() {
    std::shared_ptr<Simulation<2>> sim = std::make_shared<Simulation<2>>(nullptr);
    UninitializedSimulation<2> uninit_sim = 
        UninitializedSimulation<2>::create_uninitialized(sim);
    
    // ❌ COMPILE ERROR: make_tree() requires Initialized phase
    //
    // Error message:
    //   "no matching member function for call to 'make_tree'"
    //   note: candidate template ignored: requirement
    //   'allows_tree_operations<sph::phase::Uninitialized>::value' was not satisfied
    //
    // uninit_sim.make_tree();
    
    std::cout << "Cannot build tree until smoothing lengths computed\n";
}

// ============================================================================
// EXAMPLE 3: Accessing smoothing length before computation
// ============================================================================

void example_3_accessing_uninitialized_sml() {
    std::shared_ptr<Simulation<2>> sim = std::make_shared<Simulation<2>>(nullptr);
    UninitializedSimulation<2> uninit_sim = 
        UninitializedSimulation<2>::create_uninitialized(sim);
    
    // Create some particles
    std::vector<SPHParticle<2>> particles(100);
    uninit_sim.particles() = std::move(particles);
    
    // ⚠️ RUNTIME ERROR (not caught at compile time - particle field access)
    // This is why plugins should NOT access computed fields directly:
    //
    // for (const auto& p : uninit_sim.particles()) {
    //     real sml = p.sml;  // UNDEFINED BEHAVIOR - sml not initialized!
    // }
    //
    // This is a limitation - we can't prevent direct field access through
    // the particles() reference. Best practice: don't access computed fields
    // in plugin code.
    
    std::cout << "Direct particle field access not type-checked - use with care\n";
}

// ============================================================================
// EXAMPLE 4: Correct usage - phase transition
// ============================================================================

void example_4_correct_phase_transition() {
    std::shared_ptr<Simulation<2>> sim = std::make_shared<Simulation<2>>(nullptr);
    
    // Start in Uninitialized phase
    UninitializedSimulation<2> uninit_sim = 
        UninitializedSimulation<2>::create_uninitialized(sim);
    
    // ✅ SAFE: Set up particles
    std::vector<SPHParticle<2>> particles(100);
    uninit_sim.particles() = std::move(particles);
    
    // Solver computes smoothing lengths, densities, forces...
    // (In real code, this is done by Solver::initialize())
    
    // Transition to Initialized phase (UNSAFE - caller promises init is done)
    InitializedSimulation<2> init_sim = 
        uninit_sim.unsafe_transition_to_initialized();
    
    // ✅ NOW SAFE: Access ghost_manager, build tree, etc.
    auto ghost_mgr = init_sim.ghost_manager();
    init_sim.make_tree();
    
    std::cout << "Phase transition successful - all operations now safe\n";
}

// ============================================================================
// EXAMPLE 5: Plugin interface enforces Uninitialized phase
// ============================================================================

class ExamplePlugin : public SimulationPluginV2<2> {
public:
    std::string get_name() const override { return "example"; }
    std::string get_description() const override { return "Example"; }
    std::string get_version() const override { return "1.0"; }
    std::vector<std::string> get_source_files() const override { return {}; }
    
    void initialize(UninitializedSimulation<2> sim,
                   std::shared_ptr<SPHParameters> params) override {
        
        // ✅ SAFE: These operations are allowed
        sim.particles().resize(100);
        sim.set_particle_num(100);
        
        // ❌ COMPILE ERROR: These operations are forbidden
        // sim.ghost_manager();  // Error: requires Initialized phase
        // sim.make_tree();      // Error: requires Initialized phase
        
        // The type system ENFORCES that plugins cannot access uninitialized state
        std::cout << "Plugin can only perform safe operations\n";
    }
};

// ============================================================================
// Summary
// ============================================================================

/*
 * The type-safe plugin interface prevents:
 * 
 * ❌ Accessing ghost_manager before sml is computed
 * ❌ Building spatial tree before sml is computed
 * ❌ Syncing particle cache before initialization
 * ❌ Any operation marked with requires_initialized_phase
 * 
 * All prevented at COMPILE TIME with clear error messages.
 * 
 * Limitations:
 * - Cannot prevent direct field access through particles() reference
 * - Requires discipline to not access p.sml, p.acc directly in plugins
 * - Best practice: Only set initial conditions (pos, vel, mass, dens, pres)
 *   and let Solver compute derived quantities (sml, acc, sound)
 */

int main() {
    std::cout << "=== Type Safety Examples ===\n\n";
    
    example_1_premature_ghost_access();
    example_2_premature_tree_build();
    example_3_accessing_uninitialized_sml();
    example_4_correct_phase_transition();
    
    std::cout << "\n✓ Type safety prevents bugs at compile time\n";
    return 0;
}
