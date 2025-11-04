# Shock Tube Workflow

This workflow simulates and compares 1D Sod shock tube problem using different SPH methods.

## Directory Structure

```
shock_tube_workflow/
├── 01_simulation/           # Simulation plugin and configuration
│   ├── Makefile            # Build and run automation
│   ├── CMakeLists.txt      # Build configuration
│   ├── src/                # Plugin source code
│   │   └── plugin_enhanced.cpp  # Physics-based parameter plugin
│   ├── config/             # Simulation configurations
│   │   ├── gsph.json       # Godunov SPH config
│   │   ├── disph.json      # Density Independent SPH config
│   │   └── ssph.json       # Standard SPH config
│   ├── lib/                # Built plugin library
│   │   └── libshock_tube_plugin.dylib
│   └── results/            # Simulation output data
│       ├── gsph/           # GSPH results (.dat files)
│       └── disph/          # DISPH results (.dat files)
├── scripts/                # Python visualization scripts
│   ├── animate_comparison.py     # Create animated comparison
│   ├── plot_final_comparison.py  # Create final timestep plot
│   └── compare_shock_tube.py     # General comparison utility
├── plots/                  # Generated comparison plots
│   └── shock_tube_final_comparison.png
└── animations/             # Generated animations
    └── shock_tube_comparison.mp4
```

## Quick Start

### Using Makefile (Recommended)

```bash
cd 01_simulation

# Show available targets
make help

# Build plugin
make build

# Run GSPH simulation
make run-gsph

# Run DISPH simulation
make run-disph

# Run both simulations
make run-all

# Generate comparison plot
make plot

# Generate animation
make animate

# Generate all visualizations
make viz

# Full workflow (clean, build, run all, visualize)
make full
```

### Manual Workflow

```bash
# 1. Build the plugin
cd 01_simulation
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make
cd ..

# 2. Run simulations
cd ../../..  # Go to SPH root
./build/sph workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin.dylib \
               workflows/shock_tube_workflow/01_simulation/config/gsph.json

./build/sph workflows/shock_tube_workflow/01_simulation/lib/libshock_tube_plugin.dylib \
               workflows/shock_tube_workflow/01_simulation/config/disph.json

# 3. Generate visualizations
cd workflows/shock_tube_workflow/scripts
python3 plot_final_comparison.py
python3 animate_comparison.py
```

## Physics-Based Parameters

This workflow demonstrates the **physics-based parameter system**:

### Parameter Estimation
- **CFL coefficients** calculated from von Neumann stability analysis
  - `CFL_sound` from: `dt = CFL_sound * h / (c_s + |v|)`
  - `CFL_force` from: `dt = CFL_force * sqrt(h / |a|)`
  - Literature values: Monaghan (2005), Morris (1997)
- **Neighbor number** from kernel support volume
- Adapts to resolution and physics automatically

### Validation
- Parameters validated against actual particle configuration
- Catches mismatches that would cause simulation blow-up
- Provides descriptive error messages with fix suggestions

### Example Output
```
--- Physics-Based Parameter Estimation ---
  CFL_sound = 0.3 (from dt = CFL * h / (c_s + |v|))
  CFL_force = 0.25 (from dt = CFL * sqrt(h / |a|))
  neighbor_number = 6 (from kernel support volume)

--- Parameter Validation ---
✓ All parameters validated - SAFE to run!

Expected timestep:
  dt_sound = 0.000634
  dt_force = inf
  dt_actual = 0.000634
```

## Initial Conditions

### Sod Shock Tube Problem
- **Domain**: x ∈ [-0.5, 1.5]
- **Discontinuity**: x = 0.5
- **Left state** (x < 0.5): ρ=1.0, P=1.0, v=0
- **Right state** (x > 0.5): ρ=0.25, P=0.1795, v=0
- **Adiabatic index**: γ = 1.4
- **Simulation time**: t = 0.2

### SPH Parameters
- **Particles**: 500 (125 left, 375 right)
- **Particle spacing**: dx_left = 0.0025, dx_right = 0.01
- **Kernel**: Cubic spline
- **Boundary**: Periodic

## Comparison Results

The workflow compares three solutions:
1. **GSPH** - Godunov SPH (2nd order Riemann solver)
2. **DISPH** - Density Independent SPH
3. **Analytical** - Exact Riemann solution

### Typical L2 Errors (Density)
- GSPH: ~0.208
- DISPH: ~0.207
- Both methods show excellent agreement with analytical solution

## Output Files

### Simulation Data (.dat format)
Each timestep produces a `.dat` file with columns:
```
x y vx vy rho p sml energy id neighbor balsara flag
```

### Visualizations
- `plots/shock_tube_final_comparison.png` - 4-panel comparison at t=0.2
  - Density, velocity, pressure, energy
  - Overlays GSPH, DISPH, and analytical solutions
- `animations/shock_tube_comparison.mp4` - Time evolution animation
  - 21 frames from t=0 to t=0.2
  - Shows shock propagation, rarefaction wave, contact discontinuity

## Troubleshooting

### Build Issues
```bash
# Clean and rebuild
make clean
make build
```

### Missing Dependencies
```bash
# Check ffmpeg for animations
which ffmpeg
# Install if needed: brew install ffmpeg
```

### Animation Errors
```bash
# Run manually with verbose output
cd scripts
python3 animate_comparison.py --gsph ../01_simulation/results/gsph \
                               --disph ../01_simulation/results/disph
```

## References

- **CFL Theory**: See `docs/CFL_THEORY.md` in SPH root
- **Plugin Architecture**: See `src/plugin_enhanced.cpp`
- **Parameter System**: See `include/core/parameter_*.hpp`
