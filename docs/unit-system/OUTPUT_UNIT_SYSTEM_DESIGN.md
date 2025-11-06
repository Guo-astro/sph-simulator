# Output and Unit System Redesign

**Date:** 2025-11-06  
**Status:** Design Phase  
**Author:** Automated Design Assistant

## Executive Summary

This document outlines a comprehensive redesign of the SPH simulation output and unit systems. The new design introduces:

1. **Multiple Output Formats**: CSV (human-readable) and Protocol Buffers (compact, efficient)
2. **Metadata System**: JSON metadata files with unit system information, simulation parameters, and versioning
3. **Flexible Unit Systems**: Support for Galactic, SI, and CGS unit systems with runtime selection

## Current System Analysis

### New Output System Requirements

The new system addresses key limitations:

1. **Multiple formats**: Support both human-readable (CSV) and efficient binary (Protobuf)
2. **Unit metadata**: Explicit unit system with automatic conversions
3. **Schema versioning**: Future-proof with versioned metadata
4. **Flexible fields**: Easy to extend with new particle properties
5. **Efficient storage**: Compressed binary format for large simulations
6. **Complete metadata**: Full parameter snapshot and provenance information

**Output Particle Data Fields:**
```cpp
pos[Dim]      // Position
vel[Dim]      // Velocity
acc[Dim]      // Acceleration
mass          // Mass
dens          // Density
pres          // Pressure
ene           // Internal energy
sml           // Smoothing length
id            // Particle ID
neighbor      // Neighbor count
alpha         // AV coefficient
gradh         // Grad-h term
type          // Particle type (REAL/GHOST)
sound         // Sound speed
balsara       // Balsara switch
phi           // Gravitational potential
```

## Design Overview

### Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────┐
│                     Simulation Core                              │
│  (SPHParticle<Dim>, Simulation<Dim>, Output<Dim>)               │
└────────────────────┬────────────────────────────────────────────┘
                     │
                     ▼
┌─────────────────────────────────────────────────────────────────┐
│                 OutputCoordinator                                │
│  - Manages output timing, file naming                           │
│  - Creates metadata                                              │
│  - Delegates to writers                                          │
└────────────┬─────────────────┬─────────────────┬────────────────┘
             │                 │                 │
             ▼                 ▼                 ▼
    ┌────────────────┐ ┌──────────────┐ ┌──────────────────┐
    │  CSVWriter     │ │ ProtobufWriter│ │ MetadataWriter   │
    │                │ │                │ │                  │
    │ .csv format    │ │ .pb format     │ │ .json metadata   │
    └────────┬───────┘ └───────┬────────┘ └────────┬─────────┘
             │                 │                   │
             └─────────────────┴───────────────────┘
                              │
                              ▼
                    ┌──────────────────┐
                    │   UnitSystem     │
                    │  (interface)     │
                    └────────┬─────────┘
                             │
              ┌──────────────┼──────────────┐
              ▼              ▼              ▼
      ┌─────────────┐ ┌──────────┐ ┌──────────┐
      │  Galactic   │ │   SI     │ │   CGS    │
      │ UnitSystem  │ │UnitSystem│ │UnitSystem│
      └─────────────┘ └──────────┘ └──────────┘
```

## Component Design

### 1. Unit System Framework

#### 1.1 UnitSystem Interface

**File:** `include/core/unit_system.hpp`

```cpp
namespace sph {

enum class UnitSystemType {
    GALACTIC,  // parsec, solar mass, km/s
    SI,        // meter, kilogram, second
    CGS        // centimeter, gram, second
};

/**
 * @brief Base class for unit system definitions
 * 
 * Provides conversion factors from internal (code) units to output units.
 * Internal units are assumed to be normalized (e.g., G=1, typical length=1).
 */
class UnitSystem {
public:
    virtual ~UnitSystem() = default;
    
    virtual UnitSystemType get_type() const = 0;
    virtual std::string get_name() const = 0;
    
    // Fundamental units
    virtual real get_length_unit() const = 0;      // [output_length] / [code_length]
    virtual real get_mass_unit() const = 0;        // [output_mass] / [code_mass]
    virtual real get_time_unit() const = 0;        // [output_time] / [code_time]
    
    // Derived units (computed from fundamentals)
    virtual real get_velocity_unit() const;        // length/time
    virtual real get_acceleration_unit() const;    // length/time^2
    virtual real get_density_unit() const;         // mass/length^3
    virtual real get_pressure_unit() const;        // mass/(length*time^2)
    virtual real get_energy_unit() const;          // mass*length^2/time^2
    virtual real get_energy_density_unit() const;  // mass/(length*time^2) or energy/length^3
    
    // Unit names for documentation
    virtual std::string get_length_unit_name() const = 0;
    virtual std::string get_mass_unit_name() const = 0;
    virtual std::string get_time_unit_name() const = 0;
    virtual std::string get_velocity_unit_name() const = 0;
    virtual std::string get_density_unit_name() const = 0;
    virtual std::string get_pressure_unit_name() const = 0;
    virtual std::string get_energy_unit_name() const = 0;
    
    // Convert value from code units to output units
    virtual real convert_length(real value) const;
    virtual real convert_mass(real value) const;
    virtual real convert_time(real value) const;
    virtual real convert_velocity(real value) const;
    virtual real convert_density(real value) const;
    virtual real convert_pressure(real value) const;
    virtual real convert_energy(real value) const;
    
    // Metadata for JSON output
    virtual nlohmann::json to_json() const;
};

} // namespace sph
```

#### 1.2 Concrete Unit Systems

##### GalacticUnitSystem

```cpp
class GalacticUnitSystem : public UnitSystem {
public:
    UnitSystemType get_type() const override { return UnitSystemType::GALACTIC; }
    std::string get_name() const override { return "galactic"; }
    
    // Fundamental units
    real get_length_unit() const override { return kParsecToCm; }  // pc
    real get_mass_unit() const override { return kSolarMassToCgs; } // M_sun
    real get_time_unit() const override;  // derived from G=1 assumption
    
    // Unit names
    std::string get_length_unit_name() const override { return "pc"; }
    std::string get_mass_unit_name() const override { return "M_sun"; }
    std::string get_time_unit_name() const override { return "Myr"; }
    std::string get_velocity_unit_name() const override { return "km/s"; }
    std::string get_density_unit_name() const override { return "M_sun/pc^3"; }
    std::string get_pressure_unit_name() const override { return "M_sun/(pc*Myr^2)"; }
    std::string get_energy_unit_name() const override { return "M_sun*pc^2/Myr^2"; }

private:
    static constexpr real kParsecToCm = 3.0857e18;  // cm/pc
    static constexpr real kSolarMassToCgs = 1.989e33; // g/M_sun
};
```

##### SIUnitSystem

```cpp
class SIUnitSystem : public UnitSystem {
public:
    UnitSystemType get_type() const override { return UnitSystemType::SI; }
    std::string get_name() const override { return "SI"; }
    
    real get_length_unit() const override { return 1.0; }  // m
    real get_mass_unit() const override { return 1.0; }    // kg
    real get_time_unit() const override { return 1.0; }    // s
    
    std::string get_length_unit_name() const override { return "m"; }
    std::string get_mass_unit_name() const override { return "kg"; }
    std::string get_time_unit_name() const override { return "s"; }
    std::string get_velocity_unit_name() const override { return "m/s"; }
    std::string get_density_unit_name() const override { return "kg/m^3"; }
    std::string get_pressure_unit_name() const override { return "Pa"; }
    std::string get_energy_unit_name() const override { return "J"; }
};
```

##### CGSUnitSystem

```cpp
class CGSUnitSystem : public UnitSystem {
public:
    UnitSystemType get_type() const override { return UnitSystemType::CGS; }
    std::string get_name() const override { return "cgs"; }
    
    real get_length_unit() const override { return 1.0; }  // cm
    real get_mass_unit() const override { return 1.0; }    // g
    real get_time_unit() const override { return 1.0; }    // s
    
    std::string get_length_unit_name() const override { return "cm"; }
    std::string get_mass_unit_name() const override { return "g"; }
    std::string get_time_unit_name() const override { return "s"; }
    std::string get_velocity_unit_name() const override { return "cm/s"; }
    std::string get_density_unit_name() const override { return "g/cm^3"; }
    std::string get_pressure_unit_name() const override { return "dyn/cm^2"; }
    std::string get_energy_unit_name() const override { return "erg"; }
};
```

#### 1.3 Unit System Factory

```cpp
class UnitSystemFactory {
public:
    static std::unique_ptr<UnitSystem> create(UnitSystemType type);
    static std::unique_ptr<UnitSystem> create_from_string(const std::string& name);
    static std::unique_ptr<UnitSystem> create_from_json(const nlohmann::json& json);
};
```

### 2. Protocol Buffers Schema

#### 2.1 Particle Data Schema

**File:** `proto/particle_data.proto`

```protobuf
syntax = "proto3";

package sph.output;

// Single particle state
message Particle {
    // Kinematic properties
    repeated double position = 1;       // Dim components
    repeated double velocity = 2;       // Dim components
    repeated double acceleration = 3;   // Dim components
    
    // Thermodynamic properties
    double mass = 4;
    double density = 5;
    double pressure = 6;
    double internal_energy = 7;
    double sound_speed = 8;
    
    // SPH properties
    double smoothing_length = 9;
    double gradh_term = 10;
    
    // Artificial viscosity
    double balsara_switch = 11;
    double alpha_coefficient = 12;
    
    // Optional properties
    double gravitational_potential = 13;
    
    // Particle metadata
    int32 id = 14;
    int32 neighbor_count = 15;
    int32 type = 16;  // 0=REAL, 1=GHOST
}

// Complete snapshot at one timestep
message Snapshot {
    // Metadata
    int32 timestep = 1;
    double time = 2;
    int32 dimension = 3;
    
    // Particle data
    repeated Particle particles = 4;
    
    // Optional: ghost particles stored separately
    repeated Particle ghost_particles = 5;
}

// Energy data
message EnergySnapshot {
    double time = 1;
    double kinetic_energy = 2;
    double thermal_energy = 3;
    double potential_energy = 4;
    double total_energy = 5;
}

// Time series of energy data
message EnergySeries {
    repeated EnergySnapshot snapshots = 1;
}
```

#### 2.2 Metadata Schema

**File:** `proto/metadata.proto`

```protobuf
syntax = "proto3";

package sph.output;

// Metadata for a simulation run
message SimulationMetadata {
    // Version information
    string schema_version = 1;
    string code_version = 2;
    
    // Timestamp
    string creation_time = 3;  // ISO 8601 format
    
    // Dimension
    int32 dimension = 4;
    
    // Unit system
    UnitSystemInfo unit_system = 5;
    
    // Physics parameters
    PhysicsParams physics = 6;
    
    // Computational parameters
    ComputationalParams computational = 7;
    
    // Output parameters
    OutputParams output = 8;
    
    // SPH algorithm
    string sph_algorithm = 9;  // "SSPH", "GSPH", "DISPH", etc.
}

message UnitSystemInfo {
    string name = 1;  // "galactic", "SI", "cgs"
    string length_unit = 2;
    string mass_unit = 3;
    string time_unit = 4;
    string velocity_unit = 5;
    string density_unit = 6;
    string pressure_unit = 7;
    string energy_unit = 8;
    
    // Conversion factors (for reference)
    double length_factor = 9;
    double mass_factor = 10;
    double time_factor = 11;
}

message PhysicsParams {
    double gamma = 1;
    int32 neighbor_number = 2;
    ArtificialViscosityParams artificial_viscosity = 3;
    bool has_gravity = 4;
    bool has_periodic_boundary = 5;
}

message ArtificialViscosityParams {
    double alpha = 1;
    bool use_balsara_switch = 2;
    bool use_time_dependent = 3;
}

message ComputationalParams {
    string kernel_type = 1;
    bool iterative_smoothing_length = 2;
    double kernel_ratio = 3;
}

message OutputParams {
    int32 particle_interval = 1;
    int32 energy_interval = 2;
    repeated string output_formats = 3;  // e.g., ["csv", "protobuf"]
}
```

### 3. Output Writer Interface

#### 3.1 OutputWriter Base Class

**File:** `include/core/output_writer.hpp`

```cpp
namespace sph {

enum class OutputFormat {
    CSV,
    PROTOBUF
};

/**
 * @brief Abstract interface for writing particle data in various formats
 */
template<int Dim>
class OutputWriter {
public:
    virtual ~OutputWriter() = default;
    
    /**
     * @brief Write a single snapshot
     * @param particles Real particles
     * @param num_particles Number of real particles
     * @param ghost_particles Ghost particles (may be nullptr)
     * @param time Current simulation time
     * @param timestep Current timestep number
     */
    virtual void write_snapshot(
        const SPHParticle<Dim>* particles,
        int num_particles,
        const std::vector<SPHParticle<Dim>>* ghost_particles,
        real time,
        int timestep
    ) = 0;
    
    /**
     * @brief Write energy data
     */
    virtual void write_energy(
        real time,
        real kinetic,
        real thermal,
        real potential
    ) = 0;
    
    /**
     * @brief Get the file extension for this format
     */
    virtual std::string get_extension() const = 0;
    
    /**
     * @brief Get the format type
     */
    virtual OutputFormat get_format() const = 0;
    
    /**
     * @brief Set the unit system for conversions
     */
    virtual void set_unit_system(std::shared_ptr<UnitSystem> units) = 0;
};

} // namespace sph
```

#### 3.2 CSV Writer

**File:** `include/core/csv_writer.hpp`

```cpp
template<int Dim>
class CSVWriter : public OutputWriter<Dim> {
public:
    CSVWriter(const std::string& output_dir, bool with_header = true);
    ~CSVWriter() override = default;
    
    void write_snapshot(
        const SPHParticle<Dim>* particles,
        int num_particles,
        const std::vector<SPHParticle<Dim>>* ghost_particles,
        real time,
        int timestep
    ) override;
    
    void write_energy(real time, real kinetic, real thermal, real potential) override;
    
    std::string get_extension() const override { return "csv"; }
    OutputFormat get_format() const override { return OutputFormat::CSV; }
    void set_unit_system(std::shared_ptr<UnitSystem> units) override;

private:
    void write_header(std::ofstream& out);
    void write_particle(std::ofstream& out, const SPHParticle<Dim>& p);
    
    std::string output_dir_;
    bool with_header_;
    std::shared_ptr<UnitSystem> unit_system_;
    std::ofstream energy_file_;
};
```

#### 3.3 Protobuf Writer

**File:** `include/core/protobuf_writer.hpp`

```cpp
template<int Dim>
class ProtobufWriter : public OutputWriter<Dim> {
public:
    ProtobufWriter(const std::string& output_dir);
    ~ProtobufWriter() override = default;
    
    void write_snapshot(
        const SPHParticle<Dim>* particles,
        int num_particles,
        const std::vector<SPHParticle<Dim>>* ghost_particles,
        real time,
        int timestep
    ) override;
    
    void write_energy(real time, real kinetic, real thermal, real potential) override;
    
    std::string get_extension() const override { return "pb"; }
    OutputFormat get_format() const override { return OutputFormat::PROTOBUF; }
    void set_unit_system(std::shared_ptr<UnitSystem> units) override;

private:
    void particle_to_proto(const SPHParticle<Dim>& p, sph::output::Particle* proto);
    
    std::string output_dir_;
    std::shared_ptr<UnitSystem> unit_system_;
    sph::output::EnergySeries energy_series_;
};
```

### 4. Metadata Writer

**File:** `include/core/metadata_writer.hpp`

```cpp
namespace sph {

/**
 * @brief Writes JSON metadata for simulation runs
 */
class MetadataWriter {
public:
    MetadataWriter(const std::string& output_dir);
    
    /**
     * @brief Write metadata JSON file
     * @param params Complete simulation parameters
     * @param unit_system Active unit system
     * @param output_formats List of output formats being used
     */
    void write_metadata(
        const SimulationParameters& params,
        const UnitSystem& unit_system,
        const std::vector<OutputFormat>& output_formats
    );
    
    /**
     * @brief Get the metadata filename
     */
    static std::string get_metadata_filename() { return "metadata.json"; }

private:
    nlohmann::json create_metadata_json(
        const SimulationParameters& params,
        const UnitSystem& unit_system,
        const std::vector<OutputFormat>& output_formats
    );
    
    std::string output_dir_;
};

} // namespace sph
```

### 5. Output Coordinator

**File:** `include/core/output_coordinator.hpp`

```cpp
template<int Dim>
class OutputCoordinator {
public:
    OutputCoordinator(
        const std::string& output_dir,
        const SimulationParameters& params
    );
    
    ~OutputCoordinator() = default;
    
    /**
     * @brief Add an output writer
     */
    void add_writer(std::unique_ptr<OutputWriter<Dim>> writer);
    
    /**
     * @brief Set the unit system for all writers
     */
    void set_unit_system(std::shared_ptr<UnitSystem> units);
    
    /**
     * @brief Write particle snapshot to all registered writers
     */
    void write_particles(std::shared_ptr<Simulation<Dim>> sim);
    
    /**
     * @brief Write energy data to all registered writers
     */
    void write_energy(std::shared_ptr<Simulation<Dim>> sim);
    
    /**
     * @brief Write metadata JSON (called once at start)
     */
    void write_metadata();
    
    /**
     * @brief Get current snapshot count
     */
    int get_count() const { return count_; }

private:
    std::vector<std::unique_ptr<OutputWriter<Dim>>> writers_;
    std::shared_ptr<UnitSystem> unit_system_;
    MetadataWriter metadata_writer_;
    SimulationParameters params_;
    int count_;
};
```

### 6. Updated Output Parameters

**File:** `include/core/output_parameters.hpp` (updated)

```cpp
namespace sph {

struct OutputParameters {
    std::string directory;
    int particle_interval;
    int energy_interval;
    
    // New fields
    std::vector<OutputFormat> formats;  // Multiple formats supported
    UnitSystemType unit_system;
    bool write_metadata;
};

class OutputParametersBuilder {
public:
    // ... existing methods ...
    
    // New methods
    OutputParametersBuilder& with_format(OutputFormat format);
    OutputParametersBuilder& with_formats(const std::vector<OutputFormat>& formats);
    OutputParametersBuilder& with_unit_system(UnitSystemType unit_system);
    OutputParametersBuilder& with_metadata(bool write_metadata);
    
    // Parse from JSON with new fields
    OutputParametersBuilder& from_json(const nlohmann::json& json);
    
    // ... rest of existing interface ...
};

} // namespace sph
```

### 7. JSON Configuration Format

Example configuration file:

```json
{
  "description": "2D Shock Tube with new output system",
  "physics": {
    "dim": 2,
    "neighbor_number": 50,
    "gamma": 1.4
  },
  "time": {
    "dt": 0.0001,
    "max_iteration": 1000,
    "output_interval": 10
  },
  "output": {
    "directory": "output",
    "particle_interval": 10,
    "energy_interval": 1,
    "formats": ["csv", "protobuf"],
    "unit_system": "galactic",
    "write_metadata": true
  },
  "sph": {
    "type": "DISPH",
    "kernel": "cubic_spline",
    "iterative_sml": true
  }
}
```

### 8. Output File Structure

```
output/
├── metadata.json              # Simulation metadata
├── snapshots/
│   ├── 00000.csv             # CSV format (if enabled)
│   ├── 00000.pb              # Protobuf format (if enabled)
│   ├── 00001.csv
│   ├── 00001.pb
│   └── ...
├── energy.csv                # Energy time series (CSV)
└── energy.pb                 # Energy time series (Protobuf)
```

## Implementation Plan

### Phase 1: Unit System Foundation (2-3 days)

1. **Create unit system framework**
   - `include/core/unit_system.hpp` - Base interface
   - `src/core/unit_system.cpp` - Default implementations
   - `include/core/galactic_unit_system.hpp`
   - `include/core/si_unit_system.hpp`
   - `include/core/cgs_unit_system.hpp`
   - `src/core/unit_system_factory.cpp`

2. **Unit tests**
   - Test conversion factors
   - Test unit system factory
   - Test JSON serialization

### Phase 2: Protocol Buffers Schema (1-2 days)

1. **Define .proto files**
   - `proto/particle_data.proto`
   - `proto/metadata.proto`

2. **Update build system**
   - Add protobuf to `conanfile.txt`
   - Update `CMakeLists.txt` to compile .proto files
   - Generate C++ headers

3. **Test proto compilation**
   - Verify proto messages can be created
   - Test serialization/deserialization

### Phase 3: Output Writers (3-4 days)

1. **Create OutputWriter interface**
   - `include/core/output_writer.hpp`

2. **Implement CSV writer**
   - `include/core/csv_writer.hpp`
   - `src/core/csv_writer.cpp`
   - With header support
   - Unit conversion integration

3. **Implement Protobuf writer**
   - `include/core/protobuf_writer.hpp`
   - `src/core/protobuf_writer.cpp`
   - Binary serialization
   - Efficient batch writing

4. **Unit tests**
   - Test both writers independently
   - Test unit conversions in output
   - Verify file format correctness

### Phase 4: Metadata System (1-2 days)

1. **Create MetadataWriter**
   - `include/core/metadata_writer.hpp`
   - `src/core/metadata_writer.cpp`
   - JSON generation from simulation parameters

2. **Unit tests**
   - Test metadata JSON structure
   - Test round-trip parsing

### Phase 5: Integration (2-3 days)

1. **Create OutputCoordinator**
   - `include/core/output_coordinator.hpp`
   - `src/core/output_coordinator.cpp`
   - Manages multiple writers
   - Coordinates metadata writing

2. **Update Output<Dim> class**
   - Refactor to use OutputCoordinator
   - Maintain backward compatibility
   - Update template implementations

3. **Update OutputParameters**
   - Add new fields
   - Update builder pattern
   - JSON parsing updates

4. **Integration tests**
   - Test full output pipeline
   - Test multiple format outputs
   - Test metadata generation

### Phase 6: Python Utilities (2-3 days)

1. **Create Python module** (`workflows/scripts/sph_output/`)
   - `__init__.py`
   - `csv_reader.py` - CSV parsing
   - `protobuf_reader.py` - Protobuf reading
   - `metadata.py` - Metadata parsing
   - `converter.py` - Format conversion utilities
   - `units.py` - Unit conversion helpers

2. **Example scripts**
   - `read_snapshot.py` - Read and display snapshot
   - `convert_format.py` - Convert between formats
   - `plot_energy.py` - Plot energy evolution

3. **Tests**
   - Python unit tests
   - Integration tests with actual output files

### Phase 7: Documentation (1-2 days)

1. **User documentation**
   - Update workflows/README.md
   - Create OUTPUT_FORMATS.md guide
   - Create UNIT_SYSTEMS.md guide
   - Migration guide for existing workflows

2. **Code documentation**
   - Doxygen comments
   - Example configurations
   - API documentation

3. **Example workflows**
   - Update shock_tube_workflow config
   - Add examples for each unit system
   - Demonstrate format selection

### Phase 8: Testing & Validation (2-3 days)

1. **Comprehensive testing**
   - End-to-end workflow tests
   - Performance benchmarks
   - File size comparisons
   - Read/write speed tests

2. **Validation**
   - Verify physical correctness with unit conversions
   - Compare CSV vs Protobuf outputs
   - Validate metadata accuracy

3. **Bug fixes and optimization**

## Configuration

### Recommended Configuration

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

### Analysis Scripts

```python
from sph_output import read_snapshot, Metadata

metadata = Metadata.from_file("output/metadata.json")
snapshot = read_snapshot("output/snapshots/00000.csv", metadata)
# or
snapshot = read_snapshot("output/snapshots/00000.pb", metadata)
```

## Performance Considerations

### File Size Comparison (estimated)

For a typical 2D simulation with 10,000 particles:

| Format | File Size | Compression | Read Speed |
|--------|-----------|-------------|------------|
| Current .dat | ~2.5 MB | None | Fast |
| CSV with header | ~3.0 MB | Gzip: ~0.5 MB | Medium |
| Protobuf | ~1.5 MB | Native | Very Fast |

### Memory Usage

- CSV: Stream-based, minimal memory overhead
- Protobuf: Message-based, requires message construction but efficient serialization

### I/O Performance

- CSV: Human-readable, good for debugging, slower parsing
- Protobuf: Binary format, 5-10x faster parsing, smaller files

## Default Configuration

1. **Default format**: Both CSV and Protobuf (dual output)
2. **Default units**: Galactic system for astrophysics simulations
3. **Metadata**: Always written for documentation and reproducibility
4. **Schema versioning**: metadata.json includes schema_version for future compatibility

## Security Considerations

1. **Input validation**: Validate all JSON configuration parameters
2. **Path safety**: Sanitize output directory paths
3. **Resource limits**: Limit file sizes, prevent disk exhaustion
4. **Protobuf safety**: Use validated protobuf parsing, check message sizes

## Future Extensions

1. **HDF5 format**: Add HDF5 writer for very large simulations
2. **Compression**: Add built-in compression for CSV files
3. **Streaming output**: Support streaming for real-time analysis
4. **Checkpoint/restart**: Integrate with checkpoint system
5. **Custom fields**: Allow plugins to add custom output fields
6. **Parallel I/O**: MPI-based parallel output for distributed simulations

## References

1. **Protocol Buffers**: https://protobuf.dev/
2. **Unit systems in astrophysics**: Binney & Tremaine, "Galactic Dynamics"
3. **SPH output formats**: Springel 2005, GADGET-2 paper
4. **JSON Schema**: https://json-schema.org/

## Appendix A: Conversion Factors

### Galactic Units

```cpp
// Fundamental
constexpr real PARSEC_TO_CM = 3.0857e18;      // cm
constexpr real SOLAR_MASS_TO_G = 1.989e33;    // g
constexpr real G_CGS = 6.674e-8;              // cm^3 g^-1 s^-2

// Derived (assuming G = 1 in code units)
constexpr real TIME_UNIT_GALACTIC = 
    sqrt(PARSEC_TO_CM * PARSEC_TO_CM * PARSEC_TO_CM / 
         (G_CGS * SOLAR_MASS_TO_G));
// ≈ 4.7e13 s ≈ 1.5 Myr

constexpr real VELOCITY_UNIT_GALACTIC = 
    PARSEC_TO_CM / TIME_UNIT_GALACTIC;
// ≈ 6.6e4 cm/s ≈ 0.66 km/s
```

### SI Units

All factors = 1.0 (base system)

### CGS Units

All factors = 1.0 (base system)

## Appendix B: Example Metadata JSON

```json
{
  "schema_version": "1.0.0",
  "code_version": "sph-simulation v2.0",
  "creation_time": "2025-11-06T10:30:00Z",
  "dimension": 2,
  "unit_system": {
    "name": "galactic",
    "length_unit": "pc",
    "mass_unit": "M_sun",
    "time_unit": "Myr",
    "velocity_unit": "km/s",
    "density_unit": "M_sun/pc^3",
    "pressure_unit": "M_sun/(pc*Myr^2)",
    "energy_unit": "M_sun*pc^2/Myr^2",
    "length_factor": 3.0857e18,
    "mass_factor": 1.989e33,
    "time_factor": 4.7e13
  },
  "physics": {
    "gamma": 1.4,
    "neighbor_number": 50,
    "artificial_viscosity": {
      "alpha": 1.0,
      "use_balsara_switch": true,
      "use_time_dependent": false
    },
    "has_gravity": true,
    "has_periodic_boundary": false
  },
  "computational": {
    "kernel_type": "cubic_spline",
    "iterative_smoothing_length": true,
    "kernel_ratio": 1.2
  },
  "output": {
    "particle_interval": 10,
    "energy_interval": 1,
    "output_formats": ["csv", "protobuf"]
  },
  "sph_algorithm": "DISPH"
}
```

## Appendix C: CSV Format Specification

```csv
# SPH Simulation Snapshot
# Time: 0.123456
# Dimension: 2
# Unit System: galactic
# Schema Version: 1.0
# Fields: pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,mass,density,pressure,energy,sound_speed,smoothing_length,gradh,balsara,alpha,potential,id,neighbors,type
pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,mass,density,pressure,energy,sound_speed,smoothing_length,gradh,balsara,alpha,potential,id,neighbors,type
1.0,2.0,0.5,0.3,0.0,0.0,1.0,1.0,0.1,2.5,1.0,0.1,1.0,0.5,1.0,0.0,0,50,0
...
```

---

**End of Design Document**

**Total Estimated Implementation Time:** 14-20 days

**Key Deliverables:**
1. Unit system framework with 3 unit systems
2. Protocol Buffers schema and integration
3. Dual-format output writers (CSV + Protobuf)
4. JSON metadata system
5. Python utilities for reading/converting
6. Comprehensive documentation
7. Migration guide
8. Updated example workflows
