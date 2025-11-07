# Adding Heating/Cooling to SPH Simulation - Design Guide

## Overview

This guide demonstrates how to add **radiative heating/cooling** calculations using the same type-safe, declarative architecture as the particle cache refactoring.

## Design Principles

### 1. Module Pattern (Same as GravityForce)

Following the existing `GravityForce` pattern, create a dedicated module:

```cpp
template<int Dim>
class RadiativeCooling : public Module<Dim> {
public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
    
private:
    bool m_is_valid;
    CoolingFunction m_cooling_function;  // e.g., Grackle, TabCool, etc.
    real m_metallicity;
    real m_uv_background_intensity;
};
```

### 2. Declarative Cooling Function Strategy Pattern

Instead of hardcoding cooling physics, use **Strategy Pattern** for flexibility:

```cpp
// Abstract cooling function interface
class CoolingFunction {
public:
    virtual ~CoolingFunction() = default;
    
    /**
     * @brief Calculate cooling rate (energy loss per unit time)
     * 
     * @param density Gas density
     * @param temperature Gas temperature
     * @param metallicity Metal abundance
     * @return Cooling rate (erg/s/cm³)
     */
    virtual real calculate_cooling_rate(
        real density,
        real temperature,
        real metallicity
    ) const = 0;
    
    /**
     * @brief Calculate heating rate (energy gain per unit time)
     * 
     * @param density Gas density
     * @param uv_intensity UV background intensity
     * @return Heating rate (erg/s/cm³)
     */
    virtual real calculate_heating_rate(
        real density,
        real uv_intensity
    ) const = 0;
};
```

### 3. Concrete Cooling Function Implementations

```cpp
// Simple analytical cooling (Katz et al. 1996)
class KatzCooling : public CoolingFunction {
public:
    real calculate_cooling_rate(
        real density,
        real temperature,
        real metallicity
    ) const override {
        // Λ(T, Z) = Λ₀ * (T/T₀)^α * (1 + Z/Z_solar)
        constexpr real lambda_0 = 1e-23;  // erg cm³ s⁻¹
        constexpr real T_0 = 1e4;         // K
        constexpr real alpha = -0.7;
        
        const real lambda = lambda_0 * 
            std::pow(temperature / T_0, alpha) *
            (1.0 + metallicity);
            
        return lambda * density * density;  // n² Λ(T)
    }
    
    real calculate_heating_rate(
        real density,
        real uv_intensity
    ) const override {
        // Photoheating: Γ = ε * n * J_UV
        constexpr real epsilon = 2e-26;  // erg
        return epsilon * density * uv_intensity;
    }
};

// Tabulated cooling (more accurate)
class TabulatedCooling : public CoolingFunction {
public:
    explicit TabulatedCooling(const std::string& table_file) {
        load_cooling_table(table_file);
    }
    
    real calculate_cooling_rate(
        real density,
        real temperature,
        real metallicity
    ) const override {
        // Interpolate from table
        return interpolate_2d(cooling_table_, temperature, metallicity) *
               density * density;
    }
    
    real calculate_heating_rate(
        real density,
        real uv_intensity
    ) const override {
        return interpolate_1d(heating_table_, density) * uv_intensity;
    }
    
private:
    void load_cooling_table(const std::string& filename);
    real interpolate_2d(const Table2D& table, real x, real y) const;
    real interpolate_1d(const Table1D& table, real x) const;
    
    Table2D cooling_table_;  // Λ(T, Z)
    Table1D heating_table_;  // Γ(n)
};

// Grackle integration (most accurate, external library)
class GrackleCooling : public CoolingFunction {
public:
    explicit GrackleCooling(const GrackleConfig& config);
    
    real calculate_cooling_rate(
        real density,
        real temperature,
        real metallicity
    ) const override {
        // Call Grackle chemistry network
        return grackle_calculate_cooling_rate(density, temperature, metallicity);
    }
    
    real calculate_heating_rate(
        real density,
        real uv_intensity
    ) const override {
        return grackle_calculate_heating_rate(density, uv_intensity);
    }
    
private:
    GrackleDataHandle grackle_data_;
};
```

## TDD/BDD Test Structure

### Step 1: Write Tests First (Red Phase)

```cpp
// tests/unit/core/thermodynamics/radiative_cooling_test.cpp

class RadiativeCoolingTest : public ::testing::Test {
protected:
    std::shared_ptr<RadiativeCooling<3>> cooling_module;
    std::shared_ptr<Simulation<3>> sim;
    
    void SetUp() override {
        // Create test simulation
        auto param = create_test_parameters();
        sim = std::make_shared<Simulation<3>>(param);
        
        // Create cooling module
        cooling_module = std::make_shared<RadiativeCooling<3>>();
    }
};

TEST_F(RadiativeCoolingTest, GivenKatzCooling_WhenCalculate_ThenEnergyDecreases) {
    // GIVEN: Particles with initial thermal energy
    GIVEN("Hot gas particles at T=10^6 K") {
        auto particles = create_hot_gas_particles(100, 1e6);
        sim->particles = particles;
        
        // Configure Katz cooling
        auto cooling_params = std::make_shared<SPHParameters>();
        cooling_params->cooling.type = CoolingType::KATZ;
        cooling_params->cooling.metallicity = 0.1;  // 0.1 Z_solar
        
        cooling_module->initialize(cooling_params);
    }
    
    // WHEN: Cooling calculation performed
    WHEN("Radiative cooling is calculated") {
        const real initial_energy = sim->particles[0].ene;
        cooling_module->calculation(sim);
    }
    
    // THEN: Energy decreases due to cooling
    THEN("Thermal energy decreased") {
        const real final_energy = sim->particles[0].ene;
        EXPECT_LT(final_energy, initial_energy);
        
        // Energy change should be physical
        const real energy_change = final_energy - initial_energy;
        EXPECT_GT(energy_change, -initial_energy);  // Not all energy lost
        EXPECT_LT(energy_change, 0.0);               // Some energy lost
    }
}

TEST_F(RadiativeCoolingTest, GivenLowDensity_WhenCalculate_ThenSlowCooling) {
    // GIVEN: Low density gas
    GIVEN("Low density gas (n = 0.01 cm⁻³)") {
        auto particles = create_gas_particles(100, 0.01, 1e6);
        sim->particles = particles;
    }
    
    // WHEN: Calculate cooling at two different densities
    WHEN("Compare cooling at low vs high density") {
        cooling_module->calculation(sim);
        const real low_density_rate = sim->particles[0].dene;
        
        // Increase density 100x
        for (auto& p : sim->particles) {
            p.dens *= 100.0;
        }
        cooling_module->calculation(sim);
        const real high_density_rate = sim->particles[0].dene;
        
        // THEN: Cooling scales as n²
        THEN("Cooling rate scales quadratically with density") {
            EXPECT_NEAR(high_density_rate / low_density_rate, 10000.0, 100.0);
        }
    }
}

TEST_F(RadiativeCoolingTest, GivenUVBackground_WhenCalculate_ThenHeatingBalancesCooling) {
    // Scenario: Ionization equilibrium
    GIVEN("Gas in UV background") {
        auto particles = create_gas_particles(100, 1.0, 1e4);
        sim->particles = particles;
        
        auto params = std::make_shared<SPHParameters>();
        params->cooling.type = CoolingType::KATZ;
        params->cooling.uv_intensity = 1e-21;  // erg/s/cm²
        cooling_module->initialize(params);
    }
    
    WHEN("Heating and cooling reach equilibrium") {
        // Run for multiple timesteps
        for (int i = 0; i < 100; ++i) {
            cooling_module->calculation(sim);
        }
    }
    
    THEN("Temperature stabilizes at equilibrium value") {
        const real T_final = calculate_temperature(sim->particles[0]);
        const real T_equilibrium = 1e4;  // Expected equilibrium T
        
        EXPECT_NEAR(T_final, T_equilibrium, 0.1 * T_equilibrium);
    }
}
```

### Step 2: Implement (Green Phase)

```cpp
// include/core/thermodynamics/radiative_cooling.hpp

#pragma once

#include "../module.hpp"
#include "cooling_function.hpp"
#include <memory>

namespace sph {

template<int Dim>
class RadiativeCooling : public Module<Dim> {
public:
    void initialize(std::shared_ptr<SPHParameters> param) override;
    void calculation(std::shared_ptr<Simulation<Dim>> sim) override;
    
private:
    bool m_is_valid = false;
    std::unique_ptr<CoolingFunction> m_cooling_function;
    real m_metallicity = 0.0;
    real m_uv_intensity = 0.0;
    
    /**
     * @brief Convert internal energy to temperature
     * 
     * T = (γ - 1) * μ * m_p * u / k_B
     * where μ is mean molecular weight
     */
    real energy_to_temperature(real internal_energy) const;
    
    /**
     * @brief Calculate energy change rate
     * 
     * du/dt = (Γ - Λ) / (ρ * n)
     */
    real calculate_energy_rate(
        real density,
        real temperature,
        real metallicity,
        real uv_intensity
    ) const;
};

} // namespace sph

#include "radiative_cooling.tpp"
```

```cpp
// include/core/thermodynamics/radiative_cooling.tpp

#pragma once

#include "radiative_cooling.hpp"
#include "../utilities/constants.hpp"

namespace sph {

template<int Dim>
void RadiativeCooling<Dim>::initialize(std::shared_ptr<SPHParameters> param) {
    m_is_valid = param->cooling.is_valid;
    
    if (!m_is_valid) {
        return;
    }
    
    m_metallicity = param->cooling.metallicity;
    m_uv_intensity = param->cooling.uv_intensity;
    
    // Factory pattern for cooling function selection
    switch (param->cooling.type) {
        case CoolingType::KATZ:
            m_cooling_function = std::make_unique<KatzCooling>();
            break;
            
        case CoolingType::TABULATED:
            m_cooling_function = std::make_unique<TabulatedCooling>(
                param->cooling.table_file
            );
            break;
            
        case CoolingType::GRACKLE:
            m_cooling_function = std::make_unique<GrackleCooling>(
                param->cooling.grackle_config
            );
            break;
            
        default:
            THROW_ERROR("Unknown cooling type");
    }
}

template<int Dim>
void RadiativeCooling<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim) {
    if (!m_is_valid) {
        return;
    }
    
    auto& particles = sim->particles;
    const int num = sim->particle_num;
    const real dt = sim->dt;
    
#pragma omp parallel for
    for (int i = 0; i < num; ++i) {
        auto& p_i = particles[i];
        
        // Convert internal energy to temperature
        const real temperature = energy_to_temperature(p_i.ene);
        
        // Calculate net energy rate (heating - cooling)
        const real dene_dt = calculate_energy_rate(
            p_i.dens,
            temperature,
            m_metallicity,
            m_uv_intensity
        );
        
        // Add to existing energy derivative (from shock heating, etc.)
        p_i.dene += dene_dt;
        
        // Optional: Limit cooling to prevent negative energy
        const real max_cooling = -p_i.ene / dt;
        if (p_i.dene < max_cooling) {
            p_i.dene = max_cooling * 0.9;  // Safety factor
        }
    }
}

template<int Dim>
real RadiativeCooling<Dim>::energy_to_temperature(real internal_energy) const {
    using namespace utilities::constants;
    
    constexpr real mean_molecular_weight = 0.6;  // Ionized gas
    constexpr real gamma = 5.0 / 3.0;
    
    // T = (γ - 1) * μ * m_p * u / k_B
    return (gamma - 1.0) * mean_molecular_weight * 
           PROTON_MASS * internal_energy / BOLTZMANN_CONSTANT;
}

template<int Dim>
real RadiativeCooling<Dim>::calculate_energy_rate(
    real density,
    real temperature,
    real metallicity,
    real uv_intensity
) const {
    // Heating rate (erg/s/cm³)
    const real heating = m_cooling_function->calculate_heating_rate(
        density,
        uv_intensity
    );
    
    // Cooling rate (erg/s/cm³)
    const real cooling = m_cooling_function->calculate_cooling_rate(
        density,
        temperature,
        metallicity
    );
    
    // Net rate per unit mass: (Γ - Λ) / ρ
    return (heating - cooling) / density;
}

} // namespace sph
```

## Integration with Solver (Declarative)

### Step 3: Update Solver to Use Cooling Module

```cpp
// src/solver.cpp

template<int Dim>
void Solver<Dim>::initialize() {
    // ... existing initialization ...
    
    m_gforce = std::make_shared<GravityForce<Dim>>();
    m_cooling = std::make_shared<RadiativeCooling<Dim>>();  // NEW
    
    // ... existing code ...
    
    m_gforce->initialize(m_param);
    m_cooling->initialize(m_param);  // NEW
    
    // ... existing code ...
    
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
    m_cooling->calculation(m_sim);  // NEW - After forces, before timestep
    
    m_timestep->calculation(m_sim);
}

template<int Dim>
void Solver<Dim>::integrate() {
    m_timestep->calculation(m_sim);
    
    predict();
    
    // Regenerate ghosts and sync cache
    if (m_sim->ghost_manager) {
        m_sim->ghost_manager->regenerate_ghosts(m_sim->particles);
        m_sim->extend_cache_with_ghosts();
    }
    m_sim->make_tree();
    
    // Calculate all physics
    m_pre->calculation(m_sim);
    m_sim->sync_particle_cache();  // CRITICAL: Sync after density update
    
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
    m_cooling->calculation(m_sim);  // NEW - Declarative cooling
    
    correct();
}
```

## Parameter Configuration

### Step 4: Add Cooling Parameters

```cpp
// include/core/parameters/physics_parameters.hpp

struct CoolingParameters {
    bool is_valid = false;
    CoolingType type = CoolingType::NONE;
    real metallicity = 0.0;       // Z/Z_solar
    real uv_intensity = 0.0;      // erg/s/cm²
    std::string table_file = "";  // For tabulated cooling
    GrackleConfig grackle_config; // For Grackle integration
};

struct PhysicsParameters {
    // ... existing fields ...
    CoolingParameters cooling;  // NEW
};
```

```cpp
// src/core/parameters/physics_parameters.cpp

void PhysicsParameters::load_from_json(const nlohmann::json& j) {
    // ... existing code ...
    
    // Cooling parameters
    if (j.contains("cooling")) {
        const auto& cooling_json = j["cooling"];
        cooling.is_valid = true;
        cooling.type = parse_cooling_type(cooling_json["type"]);
        cooling.metallicity = cooling_json.value("metallicity", 0.0);
        cooling.uv_intensity = cooling_json.value("uv_intensity", 0.0);
        
        if (cooling.type == CoolingType::TABULATED) {
            cooling.table_file = cooling_json["table_file"];
        }
    }
}
```

## Plugin Configuration Example

```json
{
  "simulation": {
    "name": "cooling_collapse",
    "description": "Gas cloud with radiative cooling"
  },
  "physics": {
    "gamma": 1.66667,
    "neighbor_number": 50,
    "cooling": {
      "type": "katz",
      "metallicity": 0.1,
      "uv_intensity": 1e-21
    }
  },
  "gravity": {
    "enabled": true,
    "constant": 6.674e-8
  }
}
```

## Best Practices Summary

### 1. **Modularity**
- Cooling is a separate `Module<Dim>` like gravity
- Clear single responsibility
- Easy to enable/disable

### 2. **Strategy Pattern**
- Abstract `CoolingFunction` interface
- Multiple implementations (Katz, Tabulated, Grackle)
- Easy to add new cooling models

### 3. **Type Safety**
- Strongly typed parameters
- Compile-time checks
- Runtime validation

### 4. **Declarative API**
```cpp
m_cooling->calculation(m_sim);  // Clear intent
```

### 5. **TDD/BDD Testing**
- Tests define requirements
- Given-When-Then structure
- Physical validation

### 6. **Performance**
- OpenMP parallelization
- Virtual function overhead only at cooling function boundary
- Inlined temperature calculation

### 7. **Extensibility**
```cpp
// Easy to add new cooling model
class MyCustomCooling : public CoolingFunction {
    // Just implement the interface
};
```

## Migration Path

1. **Phase 1**: Implement `CoolingFunction` interface + Katz implementation
2. **Phase 2**: Add tests and verify with simple cases
3. **Phase 3**: Add `RadiativeCooling` module
4. **Phase 4**: Integrate into solver
5. **Phase 5**: Add advanced cooling (Tabulated, Grackle)

## Comparison with Gravity

| Aspect | Gravity | Cooling | Similarity |
|--------|---------|---------|------------|
| Base class | `Module<Dim>` | `Module<Dim>` | ✅ Same |
| Calculation | Force on particles | Energy rate | Different physics |
| Parallelization | OpenMP | OpenMP | ✅ Same |
| Tree dependency | Yes | No | Different |
| Parameters | Simple | Complex (tables, etc.) | Different complexity |
| Strategy pattern | No | Yes | Cooling more flexible |

## Key Differences from Gravity

1. **Cooling doesn't need neighbor search** - Local calculation per particle
2. **Cooling modifies `dene`, not `acc`** - Energy evolution, not motion
3. **Cooling needs temperature** - Convert from internal energy
4. **Cooling has multiple models** - Strategy pattern essential

This design maintains consistency with your architecture while being flexible for future physics additions!
