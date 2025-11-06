# SPH Simulation Documentation

Welcome to the SPH simulation codebase documentation.

## Quick Navigation

### Getting Started
- [Quickstart Guide](../workflows/README.md) - Get up and running quickly
- [Build Instructions](BUILD.md) - How to build the project
- [Parameter Builder Example](PARAMETER_BUILDER_EXAMPLE.md) - Configuration basics

### Core Architecture
- [Algorithm Architecture](ALGORITHM_ARCHITECTURE.md) - SPH algorithm design
- [Ghost Particles Quick Start](GHOST_PARTICLES_QUICKSTART.md) - Boundary particle system
- [Boundary System](BOUNDARY_SYSTEM.md) - Complete boundary conditions guide
- [2.5D Simulations](SPH_2_5D_SIMULATIONS.md) - Axisymmetric simulations

### Output and Analysis (NEW!)
- **[Output System Quick Reference](OUTPUT_UNIT_SYSTEM_QUICK_REF.md)** ⭐ Start here!
- **[Output & Unit System Design](OUTPUT_UNIT_SYSTEM_DESIGN.md)** - Complete design document
- **[Implementation Roadmap](IMPLEMENTATION_ROADMAP.md)** - Step-by-step implementation plan

### Development Guides
- [Code Quality](CODE_QUALITY.md) - Standards and best practices
- [Migration Guides](PARAMETER_BUILDER_MIGRATION.md) - Updating existing code

### Reference
- [BH-Tree Search Design](BHTREE_SEARCH_DESIGN.md) - Spatial tree algorithms
- [Neighbor Search Refactoring](NEIGHBOR_SEARCH_REFACTORING_PROPOSAL.md)
- [Vector Usage Guide](VECTOR_USAGE_GUIDE.md) - Mathematical operations

## Recent Updates

### 2025-11-06: Output and Unit System Redesign
A comprehensive redesign of the output system with:
- **Multiple Formats**: CSV (human-readable) and Protocol Buffers (compact, efficient)
- **Unit Systems**: Galactic, SI, and CGS units with automatic conversions
- **Metadata**: JSON metadata with full simulation parameters and provenance
- **Python Tools**: Easy-to-use Python utilities for analysis

**Key Documents:**
1. [Quick Reference](OUTPUT_UNIT_SYSTEM_QUICK_REF.md) - Overview and examples
2. [Design Document](OUTPUT_UNIT_SYSTEM_DESIGN.md) - Complete architecture
3. [Implementation Roadmap](IMPLEMENTATION_ROADMAP.md) - Development plan

### Configuration Example
```json
{
  "output": {
    "directory": "output",
    "particle_interval": 10,
    "energy_interval": 1,
    "formats": ["csv", "protobuf"],
    "unit_system": "galactic",
    "write_metadata": true
  }
}
```

### Python Analysis
```python
from sph_output import Metadata, read_snapshot

metadata = Metadata.from_file("output/metadata.json")
snapshot = read_snapshot("output/snapshots/00000.pb", metadata)
# Data already in chosen units (e.g., parsecs, M_sun, km/s)
```

## Directory Structure

```
docs/
├── README.md                              # This file
├── OUTPUT_UNIT_SYSTEM_QUICK_REF.md       # NEW: Output system overview
├── OUTPUT_UNIT_SYSTEM_DESIGN.md          # NEW: Complete design
├── IMPLEMENTATION_ROADMAP.md             # NEW: Implementation plan
├── ALGORITHM_ARCHITECTURE.md
├── BOUNDARY_SYSTEM.md
├── CODE_QUALITY.md
├── papers/                                # Research papers
└── archive/                               # Historical documents
```

## Common Tasks

### Running a Simulation
1. Create workflow: `./workflows/create_workflow.sh my_sim shock --dim=2`
2. Configure: Edit `config/production.json`
3. Build: `cmake -B build && cmake --build build`
4. Run: `./build/sph2d ./lib/my_sim_plugin.dylib`

### Analyzing Results
```python
from sph_output import Metadata, read_snapshot, read_energy

# Load metadata
meta = Metadata.from_file("output/metadata.json")

# Read snapshots
snapshot = read_snapshot("output/snapshots/00000.pb", meta)

# Read energy evolution
energy = read_energy("output/energy.pb", meta)
```

### Building the Project
See [BUILD.md](BUILD.md) or:
```bash
# Install dependencies
conan install . --output-folder=build --build=missing

# Configure
cmake --preset conan-debug

# Build
cmake --build build
```

## Architecture Overview

```
sph-simulation/
├── include/           # Public headers
│   ├── core/         # Core simulation components
│   ├── algorithms/   # SPH algorithms (SSPH, GSPH, DISPH)
│   ├── utilities/    # Helper utilities
│   └── ...
├── src/              # Implementation files
├── tests/            # Unit and integration tests
├── workflows/        # Example simulations
├── proto/            # Protocol Buffer schemas (NEW)
└── docs/             # Documentation
```

## Key Concepts

### SPH Algorithms
- **SSPH**: Standard SPH with artificial viscosity
- **GSPH**: Godunov SPH with Riemann solvers
- **DISPH**: Discontinuous SPH with MUSCL reconstruction

### Boundary Conditions
- **Periodic**: Wrap-around boundaries
- **Ghost**: Mirror particles at boundaries
- **Wall**: Reflective boundaries (in development)

### Output Formats (NEW)
- **CSV**: Human-readable, great for debugging
- **Protobuf**: Binary, compact, fast parsing
- Both include full metadata and unit information

### Unit Systems (NEW)
- **Galactic**: parsec, solar mass, km/s - for astrophysics
- **SI**: meter, kilogram, second - standard physics
- **CGS**: centimeter, gram, second - traditional SPH

## Contributing

1. Follow [Code Quality](CODE_QUALITY.md) guidelines
2. Write tests for new features
3. Update documentation
4. Follow C++ coding rules in `.github/instructions/coding_rules.instructions.md`

## Support

- Check relevant documentation first
- Review examples in `workflows/`
- See [Implementation Roadmap](IMPLEMENTATION_ROADMAP.md) for ongoing work

## License

See [LICENSE](../LICENSE) file in the root directory.

---

**Last Updated:** 2025-11-06
