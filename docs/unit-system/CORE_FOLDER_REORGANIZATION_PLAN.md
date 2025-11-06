# Core Folder Reorganization Plan

**Date:** 2025-11-06  
**Status:** Proposed Reorganization

## Current State Analysis

The `/include/core` directory currently contains **52 files** in a flat structure, making it difficult to navigate and understand the codebase organization. Files span multiple concerns including particles, kernels, parameters, output, boundaries, trees, and plugins.

## Proposed Directory Structure

```
include/core/
├── particles/              # Particle representations
│   ├── sph_particle.hpp
│   ├── sph_particle_2_5d.hpp
│   └── sph_types.hpp
│
├── kernels/                # Kernel functions
│   ├── kernel_function.hpp
│   ├── cubic_spline.hpp
│   ├── cubic_spline_2_5d.hpp
│   └── wendland_kernel.hpp
│
├── parameters/             # Parameter structures and builders
│   ├── simulation_parameters.hpp
│   ├── physics_parameters.hpp
│   ├── computational_parameters.hpp
│   ├── output_parameters.hpp
│   ├── parameter_validator.hpp
│   ├── parameter_validator.tpp
│   ├── parameter_estimator.hpp
│   ├── parameter_estimator.tpp
│   ├── sph_parameters_builder_base.hpp
│   ├── ssph_parameters_builder.hpp
│   ├── gsph_parameters_builder.hpp
│   └── disph_parameters_builder.hpp
│
├── output/                 # Output system (NEW)
│   ├── units/
│   │   ├── unit_system.hpp
│   │   ├── unit_system_factory.hpp
│   │   ├── galactic_unit_system.hpp
│   │   ├── si_unit_system.hpp
│   │   └── cgs_unit_system.hpp
│   ├── writers/
│   │   ├── output_writer.hpp
│   │   ├── csv_writer.hpp
│   │   ├── csv_writer.tpp
│   │   ├── protobuf_writer.hpp
│   │   ├── protobuf_writer.tpp
│   │   └── metadata_writer.hpp
│   └── output_coordinator.hpp
│
├── boundaries/             # Boundary conditions
│   ├── boundary_types.hpp
│   ├── boundary_builder.hpp
│   ├── boundary_config_helper.hpp
│   ├── periodic.hpp
│   ├── ghost_particle_manager.hpp
│   ├── ghost_particle_manager.tpp
│   ├── ghost_particle_coordinator.hpp
│   └── ghost_particle_coordinator.tpp
│
├── spatial/                # Spatial data structures
│   ├── bhtree.hpp
│   ├── bhtree.tpp
│   ├── bhtree_2_5d.hpp
│   ├── spatial_tree_coordinator.hpp
│   ├── spatial_tree_coordinator.tpp
│   ├── neighbor_search_config.hpp
│   ├── neighbor_search_result.hpp
│   └── neighbor_collector.hpp
│
├── simulation/             # Core simulation logic
│   ├── simulation.hpp
│   ├── simulation.tpp
│   ├── simulation_2_5d.hpp
│   └── simulation_2_5d.tpp
│
├── algorithms/             # Algorithm registry and tags
│   ├── algorithm_tags.hpp
│   └── sph_algorithm_registry.hpp
│
├── plugins/                # Plugin system
│   ├── plugin_loader.hpp
│   └── simulation_plugin.hpp
│
└── utilities/              # General utilities
    └── vector.hpp
```

## Categorization Rationale

### 1. **particles/** - Particle Data Structures
- **Purpose:** Contains all particle representations
- **Files:** 3 files
- **Rationale:** Particle structures are fundamental data types used throughout

### 2. **kernels/** - Kernel Functions
- **Purpose:** SPH kernel functions and interpolation
- **Files:** 4 files
- **Rationale:** Kernels are a distinct mathematical component of SPH

### 3. **parameters/** - Configuration and Parameters
- **Purpose:** All parameter structures, builders, and validators
- **Files:** 12 files
- **Rationale:** Centralized parameter management with builders following builder pattern

### 4. **output/** - Output System (NEW MODULAR DESIGN)
- **Purpose:** Complete output infrastructure
- **Subdirectories:**
  - `units/` - Unit system framework (5 files)
  - `writers/` - Format-specific writers (6 files)
  - Root: Output coordination (1 file)
- **Files:** 12 files total
- **Rationale:** 
  - Clean separation of concerns
  - Unit systems are reusable
  - Writers are pluggable
  - Easy to extend with new formats or unit systems

### 5. **boundaries/** - Boundary Conditions
- **Purpose:** All boundary-related code including ghost particles
- **Files:** 8 files
- **Rationale:** Boundary handling is a cohesive subsystem

### 6. **spatial/** - Spatial Data Structures
- **Purpose:** Trees, neighbor search, and spatial algorithms
- **Files:** 8 files
- **Rationale:** Spatial structures for efficient particle interactions

### 7. **simulation/** - Core Simulation
- **Purpose:** Main simulation classes and logic
- **Files:** 4 files
- **Rationale:** Top-level simulation orchestration

### 8. **algorithms/** - Algorithm Registry
- **Purpose:** SPH algorithm selection and registration
- **Files:** 2 files
- **Rationale:** Algorithm abstraction layer

### 9. **plugins/** - Plugin System
- **Purpose:** Dynamic plugin loading infrastructure
- **Files:** 2 files
- **Rationale:** Extensibility mechanism

### 10. **utilities/** - General Utilities
- **Purpose:** Reusable utility code
- **Files:** 1 file
- **Rationale:** Common utilities used across modules

## Migration Strategy

### Phase 1: Create Directory Structure
```bash
mkdir -p include/core/{particles,kernels,parameters,output/{units,writers},boundaries,spatial,simulation,algorithms,plugins,utilities}
```

### Phase 2: Move Files (Recommended Order)

1. **Start with output/** (newest, cleanest code)
   - Minimal dependencies on other modules
   - Well-organized from the start
   
2. **Then utilities/** (least dependencies)

3. **Then particles/** (depended on by many)

4. **Then kernels/** (depends on particles)

5. **Then parameters/** (configuration)

6. **Then boundaries/** (depends on particles)

7. **Then spatial/** (depends on particles)

8. **Then simulation/** (depends on most others)

9. **Then algorithms/** (registry)

10. **Finally plugins/** (optional features)

### Phase 3: Update Include Paths

Update all `#include` statements:
- Old: `#include "core/sph_particle.hpp"`
- New: `#include "core/particles/sph_particle.hpp"`

### Phase 4: Update Build System

Update `include/core/CMakeLists.txt` to reference new subdirectories.

## Recommended Naming Conventions

### Current Good Names (Keep)
✅ `simulation.hpp` - Clear, describes what it is  
✅ `sph_particle.hpp` - Descriptive  
✅ `unit_system.hpp` - Clear abstraction  
✅ `output_coordinator.hpp` - Role-based naming  
✅ `metadata_writer.hpp` - Clear purpose

### Files to Consider Renaming

| Current Name | Recommended Name | Rationale |
|--------------|------------------|-----------|
| `bhtree.hpp` | `barnes_hut_tree.hpp` | More explicit (but longer) |
| `sml` references | Keep as is | Standard SPH notation |
| `neighbor_collector.hpp` | Keep | Clear purpose |

### Backup Files to Remove
❌ `ghost_particle_manager.tpp.bak2` - Should be removed or versioned properly

## Benefits of This Organization

1. **Discoverability:** New developers can quickly find relevant code
2. **Modularity:** Clear boundaries between subsystems
3. **Scalability:** Easy to add new kernels, writers, unit systems, etc.
4. **Testing:** Easier to write focused unit tests per module
5. **Compilation:** Potential for parallel compilation with proper dependencies
6. **Documentation:** Each directory can have its own README
7. **Maintenance:** Changes to one subsystem don't affect others

## Output System Highlights

The new output system deserves special attention:

```
output/
├── units/                  # Reusable unit conversion framework
│   ├── unit_system.hpp          # Base abstraction
│   ├── unit_system_factory.hpp  # Creation pattern
│   └── *_unit_system.hpp        # Concrete implementations
├── writers/                # Pluggable output formats
│   ├── output_writer.hpp        # Writer interface
│   ├── csv_writer.*             # CSV implementation
│   ├── protobuf_writer.*        # Protobuf implementation
│   └── metadata_writer.hpp      # Metadata generation
└── output_coordinator.hpp  # Orchestrates all writers
```

**Design Principles:**
- **Open/Closed:** Easy to add new formats without changing existing code
- **Single Responsibility:** Each writer handles one format
- **Dependency Inversion:** Writers depend on abstractions (OutputWriter)
- **Strategy Pattern:** Writers are interchangeable strategies

## Implementation Priority

### High Priority (Do First)
1. ✅ **output/** - Already well-organized, just needs moving
2. **particles/** - Foundation for everything else
3. **kernels/** - Independent, easy to move

### Medium Priority (Do Second)
4. **parameters/** - Many files but well-defined
5. **boundaries/** - Cohesive subsystem
6. **spatial/** - Tree structures

### Lower Priority (Can Wait)
7. **simulation/** - Top-level, fewer files
8. **algorithms/** - Registry pattern
9. **plugins/** - Optional features
10. **utilities/** - Minimal

## Backward Compatibility

To maintain compatibility during migration:

1. **Option 1: Gradual Migration**
   - Create symlinks from old to new locations
   - Deprecate old paths with warnings
   - Remove symlinks after 1-2 releases

2. **Option 2: Big Bang**
   - Move everything at once
   - Update all includes
   - Breaking change, document in CHANGELOG

**Recommendation:** Use **Option 1** for safer migration.

## Example Migration Script

```bash
#!/bin/bash
# migrate_output_system.sh

set -e

CORE_DIR="include/core"

# Create output directory structure
mkdir -p "$CORE_DIR/output/units"
mkdir -p "$CORE_DIR/output/writers"

# Move unit system files
mv "$CORE_DIR/unit_system.hpp" "$CORE_DIR/output/units/"
mv "$CORE_DIR/unit_system_factory.hpp" "$CORE_DIR/output/units/"
mv "$CORE_DIR/galactic_unit_system.hpp" "$CORE_DIR/output/units/"
mv "$CORE_DIR/si_unit_system.hpp" "$CORE_DIR/output/units/"
mv "$CORE_DIR/cgs_unit_system.hpp" "$CORE_DIR/output/units/"

# Move writer files
mv "$CORE_DIR/output_writer.hpp" "$CORE_DIR/output/writers/"
mv "$CORE_DIR/csv_writer.hpp" "$CORE_DIR/output/writers/"
mv "$CORE_DIR/csv_writer.tpp" "$CORE_DIR/output/writers/"
mv "$CORE_DIR/protobuf_writer.hpp" "$CORE_DIR/output/writers/"
mv "$CORE_DIR/protobuf_writer.tpp" "$CORE_DIR/output/writers/"
mv "$CORE_DIR/metadata_writer.hpp" "$CORE_DIR/output/writers/"

# Move coordinator
mv "$CORE_DIR/output_coordinator.hpp" "$CORE_DIR/output/"

echo "Output system migration complete!"
```

## Testing Strategy

After migration:

1. **Compilation Test:** Ensure all files compile
2. **Unit Tests:** Run existing tests
3. **Integration Tests:** Run full simulation workflows
4. **Documentation:** Update all documentation references

## Documentation Updates Needed

1. Update `README.md` with new structure
2. Create `include/core/README.md` explaining organization
3. Create `include/core/output/README.md` for output system
4. Update API documentation (Doxygen)
5. Update developer guide

## Conclusion

This reorganization will:
- ✅ Improve code discoverability
- ✅ Enable better modularity
- ✅ Make the codebase more maintainable
- ✅ Facilitate easier onboarding for new developers
- ✅ Support future extensions (new formats, unit systems, kernels, etc.)

The **output/** subdirectory demonstrates best practices for the rest of the codebase to follow.

---

## Immediate Next Steps

1. Review this plan with the team
2. Get approval for directory structure
3. Start with `output/` directory migration (lowest risk)
4. Create migration scripts for each category
5. Update build system incrementally
6. Update documentation progressively

**Estimated Effort:** 2-3 days for complete migration
**Risk Level:** Low (with gradual migration strategy)
**Impact:** High (significantly improved organization)
