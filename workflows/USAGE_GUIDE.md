# SPH Simulation Workflows - Complete Guide

This directory contains self-contained workflow directories for different SPH simulation problems. Each workflow is organized as a complete simulation pipeline including initial conditions, configuration, execution, and visualization.

## 📁 Directory Structure

```
workflows/
├── visualize.py                    # General visualization script
├── regenerate_cmake.sh            # Regenerate CMakeLists.txt for all workflows
├── shock_tube_workflow/           # 1D Sod shock tube
│   ├── 01_simulation/
│   │   ├── src/plugin.cpp         # Initial condition plugin
│   │   ├── CMakeLists.txt         # Build configuration
│   │   ├── lib/                   # Built plugin library
│   │   └── build/                 # Build directory
│   ├── 02_output/                 # Simulation output files
│   ├── 03_analysis/               # Analysis results
│   ├── config/
│   │   ├── production.json        # Full-scale parameters
│   │   └── test.json              # Quick test parameters (create manually)
│   └── compare_shock_tube.py      # Compare with analytic solution
├── evrard_workflow/               # 3D gravitational collapse
├── khi_workflow/                  # 2D Kelvin-Helmholtz instability
├── gresho_chan_vortex_workflow/   # 2D vortex test
├── hydrostatic_workflow/          # 2D hydrostatic equilibrium
└── pairing_instability_workflow/  # 2D pairing instability test
```

## 🧪 Workflow List

| Workflow | Dimension | Description | Analytic Solution |
|----------|-----------|-------------|-------------------|
| shock_tube | 1D | Sod shock tube | ✅ Yes |
| evrard | 3D | Gravitational collapse | ❌ No |
| khi | 2D | Kelvin-Helmholtz instability | ❌ No |
| gresho_chan_vortex | 2D | Vortex in pressure equilibrium | ✅ Semi-analytic |
| hydrostatic | 2D | Hydrostatic equilibrium test | ✅ Yes |
| pairing_instability | 2D | SPH pairing instability test | ❌ No |

## 🚀 Quick Start

### 1. Build Shock Tube Plugin

```bash
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/01_simulation
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

The plugin will be built as `lib/libshock_tube_plugin.dylib`

### 2. Create Visualizations

```bash
# Create 1D animation
python3 ../../visualize.py 02_output --dim 1 --animate --output shock_tube.mp4

# Create snapshot at frame 10
python3 ../../visualize.py 02_output --dim 1 --snapshot 10
```

### 3. Compare Methods (Shock Tube)

```bash
cd shock_tube_workflow
python3 compare_shock_tube.py --disph 02_output/disph --gsph 02_output/gsph --time 0.2
```

## 📊 Visualization Tools

### General Visualizer (`visualize.py`)

Located in `/workflows/visualize.py`, supports 1D and 2D simulations.

**1D Animations:**
```bash
python3 visualize.py <output_dir> --dim 1 --animate --output animation.mp4
```

**2D Animations:**
```bash
# Density field
python3 visualize.py <output_dir> --dim 2 --animate --quantity density

# Pressure field
python3 visualize.py <output_dir> --dim 2 --animate --quantity pressure

# Velocity magnitude
python3 visualize.py <output_dir> --dim 2 --animate --quantity velocity
```

**Snapshots:**
```bash
# 1D snapshot at frame 15
python3 visualize.py <output_dir> --dim 1 --snapshot 15 --output snap15.png

# 2D pressure snapshot
python3 visualize.py <output_dir> --dim 2 --snapshot 20 --quantity pressure
```

### Shock Tube Comparison (`compare_shock_tube.py`)

Compare DISPH and GSPH methods against exact Riemann solution:

```bash
python3 shock_tube_workflow/compare_shock_tube.py \
    --disph path/to/disph/output \
    --gsph path/to/gsph/output \
    --time 0.2 \
    --gamma 1.4
```

Outputs:
- Comparison plots (density, velocity, pressure, energy)
- L2 error metrics
- PNG file: `shock_tube_comparison_t0.200.png`

## 🔬 Workflow Details

### Shock Tube (1D)

**Physics:** Sod shock tube - Riemann problem with discontinuous initial conditions

**Initial Conditions:**
- Left state: ρ=1.0, P=1.0, v=0, x ∈ [-0.5, 0)
- Right state: ρ=0.125, P=0.1, v=0, x ∈ [0, 0.5]
- Discontinuity at x=0

**Features:**
- Shock wave propagating right
- Rarefaction wave propagating left
- Contact discontinuity
- Exact analytic solution available

**References:** Sod (1978), Toro (2009)

---

### Evrard (3D)

**Physics:** Adiabatic collapse of self-gravitating polytropic sphere

**Initial Conditions:**
- Density profile: ρ(r) = M/(2πr) for r < 1
- Total mass: M = 1.0
- Initial radius: R = 1.0
- Pressure: Cold cloud (P ≈ 0)
- Angular momentum: None (spherical collapse)

**Features:**
- Self-gravity via Barnes-Hut tree
- Energy conservation test
- Bounce and re-expansion

**References:** Evrard (1988)

---

### Kelvin-Helmholtz Instability (2D)

**Physics:** Shear flow instability from velocity discontinuity

**Initial Conditions:**
- Domain: [0, 1] × [0, 1]
- Upper layer (y > 0.5): vy = +0.5, ρ = 2.0
- Lower layer (y < 0.5): vy = -0.5, ρ = 1.0
- Sinusoidal perturbation: vy' = A sin(2πx)
- Pressure: Uniform P = 2.5

**Features:**
- Vortex roll-up
- Mixing at interface
- Tests artificial viscosity and diffusion

**References:** McNally et al. (2012)

---

### Gresho-Chan Vortex (2D)

**Physics:** Steady vortex in pressure equilibrium

**Initial Conditions:**
- Azimuthal velocity:
  - r < 0.2: v_θ = 5r
  - 0.2 ≤ r < 0.4: v_θ = 2 - 5r
  - r ≥ 0.4: v_θ = 0
- Pressure: Radial profile for equilibrium
- Density: ρ = 1 (uniform)

**Features:**
- Tests pressure gradient accuracy
- Vorticity preservation
- Long-term stability

**References:** Gresho & Chan (1990)

---

### Hydrostatic (2D)

**Physics:** Static equilibrium with density contrast

**Initial Conditions:**
- Inner square [-0.25, 0.25]²: ρ = 4.0, P = 2.5
- Outer region [-0.5, 0.5]²: ρ = 1.0, P = 2.5
- Velocity: v = 0 everywhere

**Features:**
- Constant pressure test
- Density contrast stability
- Hydrostatic equilibrium maintenance

**References:** Saitoh & Makino (2013)

---

### Pairing Instability (2D)

**Physics:** Uniform grid with random perturbations

**Initial Conditions:**
- 32×32 regular grid
- Random perturbations: ±5% of particle spacing
- Uniform: ρ = 1, P = 1, v = 0
- RNG seed: 12345

**Features:**
- Tests artificial particle clustering
- Should maintain uniform distribution
- Detects kernel deficiencies

**References:** Schuessler & Schmitt (1981), Monaghan (2002)

## 🏗️ Plugin Architecture

Each workflow implements the `SimulationPlugin` interface:

```cpp
#include "core/simulation_plugin.hpp"

class MyPlugin : public SimulationPlugin {
public:
    std::string get_name() const override {
        return "my_simulation";
    }
    
    std::string get_description() const override {
        return "Brief description";
    }
    
    std::string get_version() const override {
        return "1.0.0";
    }
    
    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // Set up particles
        // Configure parameters
    }
    
    std::vector<std::string> get_source_files() const override {
        return {"plugin.cpp"};
    }
};

DEFINE_SIMULATION_PLUGIN(MyPlugin)
```

## 🔧 Building Workflows

### Regenerate All CMakeLists.txt

```bash
cd /Users/guo/sph-simulation/workflows
./regenerate_cmake.sh
```

### Build Individual Workflow

```bash
cd <workflow_name>/01_simulation
rm -rf build lib bin
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j4
```

### Build Configuration

- **Build Type:** Debug (matches Conan dependencies)
- **Compiler:** AppleClang 15.0
- **C++ Standard:** C++14
- **Dependencies:** Boost, nlohmann_json, OpenMP
- **Output:** Shared library in `lib/`

## 📦 Dependencies

### Build Dependencies
- CMake 3.13+
- C++14 compatible compiler
- Conan 2.0+
- OpenMP support

### Visualization Dependencies
```bash
pip3 install numpy matplotlib scipy
brew install ffmpeg
```

## 🧮 SPH Methods

### Supported Formulations

1. **SSPH (Standard SPH)**
   - Classic formulation
   - Simple and robust
   - Higher numerical viscosity

2. **DISPH (Hopkins 2013)**
   - Pressure-energy formulation
   - Reduced E0 and E1 errors
   - Better conservation properties

3. **GSPH (Cha & Whitworth 2003)**
   - Godunov SPH with Riemann solver
   - HLL approximate solver
   - MUSCL reconstruction
   - Superior shock capturing

### Method Comparison

Best demonstrated on shock_tube:
```bash
# Run with DISPH
./sph config_disph.json

# Run with GSPH
./sph config_gsph.json

# Compare results
python3 compare_shock_tube.py --disph output_disph --gsph output_gsph --time 0.2
```

## 📝 Configuration Files

Each workflow contains `config/production.json`:

```json
{
    "simulation": {
        "endTime": 3.0,
        "outputInterval": 0.1,
        "dimension": 2
    },
    "sph": {
        "neighborNumber": 50,
        "kernelType": "cubic_spline"
    },
    "physics": {
        "gamma": 1.4,
        "gravity": false
    },
    "cfl": {
        "sound": 0.3,
        "force": 0.25
    }
}
```

Create `config/test.json` for quick tests:
```json
{
    "extends": "production.json",
    "simulation": {
        "endTime": 0.1,
        "particleNumber": 100
    }
}
```

## 🧹 Old Code Removal

The old sample infrastructure has been removed:
- ✅ `/src/sample/` directory deleted
- ✅ `/sample/` directory deleted
- ✅ All samples converted to plugins

## 🚧 Troubleshooting

### Build Issues

**CMake Configuration Error:**
```
Please set CMAKE_BUILD_TYPE=<build_type>
```
Solution: Add `-DCMAKE_BUILD_TYPE=Debug`

**Boost Headers Not Found:**
```
fatal error: 'boost/format.hpp' file not found
```
Solution: Use `Boost::headers` target (already configured)

**OpenMP Warning:**
```
clang: warning: no such sysroot directory: 'macosx'
```
Solution: Safe to ignore (doesn't affect compilation)

### Visualization Issues

**ffmpeg not found:**
```bash
brew install ffmpeg
```

**SciPy import error:**
```bash
pip3 install scipy
```

**No data files:**
Check that simulation wrote JSON output to correct directory

## 📚 References

1. Sod, G. A. (1978). "A survey of several finite difference methods for systems of nonlinear hyperbolic conservation laws"
2. Hopkins, P. F. (2013). "A general class of Lagrangian smoothed particle hydrodynamics methods"
3. Cha, S.-H., & Whitworth, A. P. (2003). "Implementations and tests of Godunov-type particle hydrodynamics"
4. Evrard, A. E. (1988). "Beyond N-body: 3D cosmological gas dynamics"
5. McNally, C. P., et al. (2012). "A well-posed Kelvin-Helmholtz instability test"
6. Gresho, P. M., & Chan, S. T. (1990). "On the theory of semi-implicit projection methods"
7. Saitoh, T. R., & Makino, J. (2013). "A density-independent formulation of SPH"
8. Monaghan, J. J. (2002). "SPH compressible turbulence"
9. Toro, E. F. (2009). "Riemann Solvers and Numerical Methods for Fluid Dynamics"

## ✅ Status

- [x] All 6 workflows converted to plugins
- [x] Build system configured
- [x] Old sample directories removed
- [x] Visualization scripts created
- [x] Shock tube comparison tool created
- [ ] Run test simulations
- [ ] Generate benchmark results
- [ ] Create full documentation
