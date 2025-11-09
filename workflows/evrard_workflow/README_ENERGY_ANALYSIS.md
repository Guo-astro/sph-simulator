# Evrard Collapse Energy Conservation Analysis

## Overview

This workflow includes comprehensive energy conservation analysis for the Evrard collapse test, following the methodology from:
- Evrard, A. E. (1988). "Beyond N-body: 3D cosmological gas dynamics"
- Springel, V. (2005). "The cosmological simulation code GADGET-2"

## Energy Output

The simulation now outputs energy data at regular intervals (default: every 1.0 time units) to verify energy conservation during the gravitational collapse.

### Energy File Format

The `energy.csv` file is in standard CSV format with headers:
```csv
# SPH Simulation Energy Conservation Data
# All energies in code units
# Kinetic: sum(0.5 * m * v^2)
# Thermal: sum(m * u)  (internal energy)
# Potential: 0.5 * sum(m * phi)  (gravitational)
# Total: Kinetic + Thermal + Potential
#
time,kinetic,thermal,potential,total
0.000000000000000e+00,0.000000000000000e+00,4.999999999999957e-02,-6.610442948966810e-01,-6.110442948966814e-01
1.000000000000000e+00,...
```

Where:
- **Kinetic Energy (KE)**: `Σ 0.5 * m_i * v_i²`
- **Thermal Energy (IE)**: `Σ m_i * u_i`  (Internal energy)
- **Potential Energy (PE)**: `0.5 * Σ m_i * φ_i`  (Gravitational potential)
- **Total Energy**: `KE + IE + PE`

## Running the Simulation

### 1. Build the simulation
```bash
cd workflows/evrard_workflow/01_simulation
make clean && make -j8
```

### 2. Run the simulation
```bash
cd build
./sph3d ../lib/libevrard_plugin.dylib
```

The simulation will:
- Run from t=0 to t=20.0
- Output particle snapshots every 1.0 time units
- Output energy data every 1.0 time units
- Create output in `build/output/evrard_collapse/`

## Energy Analysis and Visualization

### Using the Energy Analyzer

The included Python script `visualize_evrard.py` provides comprehensive energy analysis:

```bash
cd workflows/evrard_workflow/01_simulation
python3 visualize_evrard.py build/output/evrard_collapse
```

### Generated Plots

The script generates three types of energy plots:

#### 1. **Energy Evolution Plot** (`energy_evolution.png`)
- **Top panel**: Shows all energy components (KE, IE, PE) and total energy vs time
- **Bottom panel**: Shows relative energy conservation error: `ΔE/E₀ = (E_total - E_initial) / |E_initial|`
- Includes statistics: maximum and final energy errors

#### 2. **Paper-Style Energy Plot** (`energy_paper_style.png`)
- Reproduces publication-quality plots similar to Evrard (1988) Figure 2
- Shows normalized energy components: `E_i / |E_0|`
- Clearly shows energy conversion during collapse:
  - Gravitational potential energy → Kinetic energy (during collapse)
  - Kinetic energy → Thermal energy (at bounce)

#### 3. **Normalized Energy Components** (`energy_normalized.png`)
- Shows each energy component as a fraction of total energy
- Useful for understanding energy budget throughout simulation

### Console Output

The script also prints detailed statistics:
```
==================================================================
 ENERGY CONSERVATION ANALYSIS - Evrard Collapse Test
==================================================================

Simulation Time Range: t = 0.0000 to 20.0000
Number of energy outputs: 21

Initial Energies (t = 0.0000):
  Kinetic:   0.000000e+00
  Thermal:   5.000000e-02
  Potential: -6.610443e-01
  Total:     -6.110443e-01

Final Energies (t = 20.0000):
  Kinetic:   <value>
  Thermal:   <value>
  Potential: <value>
  Total:     <value>

Energy Conservation Metrics:
  Maximum |ΔE/E₀|: <value>%
  Mean |ΔE/E₀|:    <value>%
  Final ΔE/E₀:     <value>%

Quality Assessment:
  ✓✓✓ Energy conservation: EXCELLENT
  Target: |ΔE/E₀| < 1% for publication quality
==================================================================
```

## Expected Physical Behavior

### Phase 1: Gravitational Collapse (t ≈ 0 to 0.8)
- Potential energy (PE) becomes more negative
- Kinetic energy (KE) increases as particles accelerate inward
- Thermal energy (IE) remains relatively constant
- Total energy should be conserved

### Phase 2: Core Bounce (t ≈ 0.8 to 1.5)
- Maximum compression reached
- Kinetic energy converts to thermal energy (shock heating)
- Density and pressure reach maximum
- Core rebounds due to pressure gradient

### Phase 3: Oscillations (t > 1.5)
- System oscillates around equilibrium
- Energy exchanges between kinetic and thermal
- Artificial viscosity damps oscillations
- System gradually relaxes

## Energy Conservation Targets

For publication-quality SPH simulations:
- **Excellent**: `|ΔE/E₀| < 1%`
- **Good**: `|ΔE/E₀| < 5%`
- **Acceptable**: `|ΔE/E₀| < 10%`

Poor energy conservation indicates:
- Time step too large (reduce CFL numbers)
- Insufficient neighbor resolution
- Tree opening angle (θ) too large
- Numerical instabilities

## Simulation Parameters

Current configuration (in `plugin.cpp`):
```cpp
.with_time(
    /*start=*/0.0,
    /*end=*/20.0,      // End time
    /*output=*/1.0,     // Particle output interval
    /*energy=*/1.0      // Energy output interval
)

.with_cfl(
    /*sound=*/0.3,      // CFL for sound speed
    /*force=*/0.25      // CFL for force term
)

.with_physics(
    /*neighbor_number=*/50,  // Target neighbors
    /*gamma=*/5.0/3.0        // Adiabatic index
)

.with_gravity(
    /*constant=*/1.0,   // G
    /*theta=*/0.5       // Barnes-Hut opening angle
)
```

## Troubleshooting

### Energy not conserved well?
1. **Reduce time step**: Lower CFL numbers (sound=0.2, force=0.15)
2. **Increase neighbors**: Try `neighbor_number=100`
3. **Reduce tree opening angle**: Try `theta=0.3`
4. **Check h_min**: Ensure smoothing length doesn't collapse

### Simulation too slow?
1. **Reduce end time**: Change `end=10.0` for faster testing
2. **Reduce particles**: Lower resolution in initial conditions
3. **Increase output intervals**: `output=2.0`, `energy=0.5`

### No energy.csv file?
- Energy output is now properly implemented in `CSVWriter::write_energy()`
- File should appear in `output/evrard_collapse/energy.csv`
- Check that simulation reached at least one energy output time

## References

1. **Evrard, A. E. (1988)**
   - "Beyond N-body: 3D cosmological gas dynamics"
   - Monthly Notices RAS, 235, 911-934
   - Original Evrard collapse test definition

2. **Springel, V. (2005)**
   - "The cosmological simulation code GADGET-2"
   - MNRAS, 364, 1105-1134
   - Modern SPH implementation and energy conservation

3. **Monaghan, J. J. (1992)**
   - "Smoothed Particle Hydrodynamics"
   - Annual Review of Astronomy and Astrophysics, 30, 543-574
   - SPH fundamentals and energy conservation theory

## Code Implementation

Energy output is implemented in:
- **CSV Writer Header**: `include/core/output/writers/csv_writer.hpp`
- **CSV Writer Implementation**: `include/core/output/writers/csv_writer.tpp`
- **Output Coordinator**: `include/core/output/output_coordinator.hpp`
- **Visualization**: `workflows/evrard_workflow/01_simulation/visualize_evrard.py`

The energy calculation follows standard SPH conventions:
```cpp
real kinetic = 0.0;
real thermal = 0.0;
real potential = 0.0;

#pragma omp parallel for reduction(+: kinetic, thermal, potential)
for (int i = 0; i < num_particles; ++i) {
    const auto& p_i = particles[i];
    kinetic += 0.5 * p_i.mass * abs2(p_i.vel);
    thermal += p_i.mass * p_i.ene;
    potential += 0.5 * p_i.mass * p_i.phi;  // Factor 0.5 avoids double counting
}

real total = kinetic + thermal + potential;
```

## Next Steps

1. **Run full simulation** (t=20.0) to see complete evolution
2. **Generate energy plots** to verify conservation
3. **Compare with literature** - Evrard (1988) Figure 2
4. **Optimize parameters** if energy conservation is poor
5. **Create animations** showing density evolution alongside energy plots
