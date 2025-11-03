# SPH Simulation Plugin Architecture

## Overview

The SPH (Smoothed Particle Hydrodynamics) code implements a modular plugin architecture that allows simulation cases to be developed as self-contained, dynamically loadable modules. This architecture provides excellent separation of concerns, reproducibility, and extensibility.

## Core Components

### 1. SimulationPlugin Interface

The `SimulationPlugin` class defines the contract that all simulation plugins must implement:

```cpp
class SimulationPlugin {
public:
    virtual ~SimulationPlugin() = default;

    // Metadata
    virtual std::string get_name() const = 0;
    virtual std::string get_description() const = 0;
    virtual std::string get_version() const = 0;

    // Core functionality
    virtual void initialize(std::shared_ptr<Simulation> sim,
                          std::shared_ptr<SPHParameters> params) = 0;

    // Reproducibility
    virtual std::vector<std::string> get_source_files() const = 0;
};
```

### 2. Plugin Implementation Pattern

Each simulation case implements the interface:

```cpp
class ShockTubePlugin : public SimulationPlugin {
public:
    std::string get_name() const override { return "shock_tube"; }
    std::string get_description() const override { return "Sod shock tube problem"; }
    std::string get_version() const override { return "2.0.0"; }

    void initialize(std::shared_ptr<Simulation> sim,
                   std::shared_ptr<SPHParameters> param) override {
        // Set up initial conditions, particles, etc.
    }

    std::vector<std::string> get_source_files() const override {
        return {"shock_tube.cpp"};
    }
};
```

### 3. Export Mechanism

Plugins export C functions for dynamic loading:

```cpp
#define DEFINE_SIMULATION_PLUGIN(ClassName) \
    EXPORT_PLUGIN_API sph::SimulationPlugin* create_plugin() { \
        return new ClassName(); \
    } \
    EXPORT_PLUGIN_API void destroy_plugin(sph::SimulationPlugin* plugin) { \
        delete plugin; \
    }

DEFINE_SIMULATION_PLUGIN(sph::ShockTubePlugin)
```

## Loading System Architecture

### PluginLoader (Dynamic Loading)
- Uses `dlopen()`/`dlsym()` for runtime plugin loading
- Loads `.dylib` (macOS) or `.so` (Linux) files
- Manages plugin lifecycle (create/destroy)

### SampleRegistry (Built-in Samples)
- Singleton registry for function pointers
- Backward compatibility with old sample system
- Macros for registration: `REGISTER_SAMPLE()`, `REGISTER_SAMPLE_WITH_SOURCE()`

### SimulationLoader (Unified Interface)
- Automatically detects plugin vs registry mode
- Plugin mode: if path ends with `.dylib`/`.so`
- Registry mode: for built-in sample names

## Workflow Structure

Each plugin-based simulation follows this structure:

```
simulations/workflows/[workflow_name]/
├── 01_simulation/
│   ├── src/plugin.cpp           # Plugin implementation
│   ├── build/                   # Build artifacts
│   │   └── lib[plugin]_plugin.dylib
│   ├── config/                  # JSON configs
│   ├── scripts/                 # Analysis scripts
│   ├── output/                  # Raw outputs
│   ├── results/                 # Processed results
│   └── docs/                    # Documentation
├── shared_data/                 # Common data files
├── workflow_results/            # Aggregated results
└── workflow_logs/               # Execution logs
```

## Key Benefits

✅ **Self-contained**: Each simulation includes source, config, and results
✅ **Reproducible**: Automatic source code archiving
✅ **Hot-reloadable**: Recompile plugin without rebuilding main executable
✅ **Extensible**: Easy to add new simulation cases
✅ **Versioned**: Each plugin has version metadata
✅ **Multi-format**: Supports both plugin and registry loading

## Usage Examples

```bash
# Plugin mode (recommended)
./sph1d ../simulations/shock_tube/build/libshock_tube_plugin.dylib config.json

# Registry mode (legacy)
./sph1d shock_tube config.json

# List available simulations
./sph1d --list-samples
```

## Build System Integration

Plugins are built as shared libraries:

```cmake
# CMakeLists.txt in plugin directory
add_library(${PLUGIN_NAME}_plugin SHARED src/plugin.cpp)
target_link_libraries(${PLUGIN_NAME}_plugin PRIVATE sph_core)
```

## Current Plugin Examples

### Shock Tube Plugin
- **Purpose**: Sod shock tube problem - 1D Riemann problem
- **Features**: Discontinuity at x=0, creates shock wave, contact discontinuity, and rarefaction wave
- **States**:
  - Left (x < 0): ρ=1.0, P=1.0, v=0
  - Right (x > 0): ρ=0.125, P=0.1, v=0

### Riemann Problems Plugin
- **Purpose**: Comprehensive 1D Riemann problems benchmark suite
- **Tests**: Sod shock tube, double rarefaction, strong shock, slow shock, vacuum generation
- **Multi-test**: Uses `TEST_CASE` environment variable for different configurations

### Other Plugins
- **sedov_taylor**: Sedov-Taylor blast wave (2D/3D)
- **hydrostatic**: Hydrostatic equilibrium tests
- **kelvin_helmholtz**: Kelvin-Helmholtz instability
- **razor_thin_hvcc**: Razor-thin disk simulations

## SPH Algorithm Support

The plugin architecture supports multiple SPH variants:

- **SSPH**: Standard SPH
- **DISPH**: Density-Independent SPH (Saitoh & Makino 2013 formulation)
- **GSPH**: Godunov SPH with Riemann solvers
- **GDISPH**: Godunov + DISPH hybrid

## Configuration System

Plugins work with JSON configuration files supporting:

- **Inheritance**: `"extends": "base_config.json"` for config reuse
- **SPH Parameters**: Algorithm-specific settings (GSPH shock detection, etc.)
- **Physical Parameters**: γ, CFL numbers, artificial viscosity
- **Output Control**: Formats, intervals, directories
- **Checkpointing**: Auto-save and resume functionality

## Development Workflow

1. **Create Plugin Structure**:
   ```bash
   mkdir -p simulations/my_simulation/01_simulation/src
   cp template/plugin.cpp simulations/my_simulation/01_simulation/src/
   ```

2. **Implement Simulation Logic**:
   - Override `initialize()` to set up particles and initial conditions
   - Set particle positions, masses, densities, pressures, velocities
   - Configure boundary conditions and domain

3. **Build Plugin**:
   ```bash
   cd simulations/my_simulation/01_simulation
   mkdir build && cd build
   cmake ..
   make
   ```

4. **Test and Run**:
   ```bash
   cd /path/to/sphcode/build
   ./sph1d ../simulations/my_simulation/01_simulation/build/libmy_simulation_plugin.dylib config.json
   ```

## Best Practices

### Plugin Development
- **Validation**: Check dimensional requirements (`#if DIM != 1`)
- **Documentation**: Provide clear descriptions and version info
- **Error Handling**: Use `THROW_ERROR` for invalid configurations
- **Reproducibility**: Return accurate source file lists

### Configuration
- **Inheritance**: Use base configs for common settings
- **Validation**: Enable appropriate SPH algorithm parameters
- **Units**: Specify unit systems for dimensional consistency

### Workflow Organization
- **Separation**: Keep simulation, analysis, and visualization separate
- **Automation**: Provide run scripts for complete workflows
- **Documentation**: Include READMEs and usage examples

## Architecture Advantages

1. **Modularity**: Clean separation between core SPH engine and simulation cases
2. **Reproducibility**: Complete simulation environments with all dependencies
3. **Collaboration**: Easy sharing of simulation setups
4. **Testing**: Isolated testing of individual simulation cases
5. **Maintenance**: Independent updates to simulations without affecting core code
6. **Performance**: Hot-reloading without full rebuilds

## Future Extensions

The plugin architecture enables easy addition of:

- **New Physics**: Gravity, MHD, radiation transport
- **Boundary Conditions**: Custom ghost particle strategies
- **Analysis Tools**: Specialized post-processing plugins
- **Multi-physics**: Coupled simulation capabilities
- **Optimization**: Algorithm-specific performance tuning

This architecture provides a solid foundation for extensible, maintainable SPH simulation development.