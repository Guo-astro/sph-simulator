# Implementation Roadmap: Output and Unit System Redesign

**Created:** 2025-11-06  
**Estimated Total Time:** 14-20 days  
**Status:** Ready for Implementation

## Overview

This document provides a detailed, step-by-step implementation plan for the output and unit system redesign. Each task includes specific files, code snippets, and acceptance criteria.

---

## Phase 1: Unit System Foundation (2-3 days)

### Task 1.1: Create UnitSystem Base Interface

**Files to Create:**
- `include/core/unit_system.hpp`
- `src/core/unit_system.cpp`

**Steps:**

1. Create header file with base interface
2. Define `UnitSystemType` enum
3. Declare virtual methods for unit conversions
4. Implement default derived unit calculations
5. Add JSON serialization support

**Acceptance Criteria:**
- [ ] Interface compiles without errors
- [ ] All virtual methods declared
- [ ] Doxygen documentation complete
- [ ] Follows C++ coding rules (see `.github/instructions/coding_rules.instructions.md`)

**Code Template:**

```cpp
// include/core/unit_system.hpp
#ifndef SPH_CORE_UNIT_SYSTEM_HPP
#define SPH_CORE_UNIT_SYSTEM_HPP

#include "defines.hpp"
#include <nlohmann/json.hpp>
#include <string>
#include <memory>

namespace sph {

enum class UnitSystemType {
    GALACTIC,
    SI,
    CGS
};

class UnitSystem {
public:
    virtual ~UnitSystem() = default;
    
    // Type identification
    virtual UnitSystemType get_type() const = 0;
    virtual std::string get_name() const = 0;
    
    // Fundamental conversion factors
    virtual real get_length_unit() const = 0;
    virtual real get_mass_unit() const = 0;
    virtual real get_time_unit() const = 0;
    
    // Derived conversions (with default implementations)
    virtual real get_velocity_unit() const;
    virtual real get_acceleration_unit() const;
    virtual real get_density_unit() const;
    virtual real get_pressure_unit() const;
    virtual real get_energy_unit() const;
    virtual real get_energy_density_unit() const;
    
    // Unit name strings
    virtual std::string get_length_unit_name() const = 0;
    virtual std::string get_mass_unit_name() const = 0;
    virtual std::string get_time_unit_name() const = 0;
    virtual std::string get_velocity_unit_name() const = 0;
    virtual std::string get_density_unit_name() const = 0;
    virtual std::string get_pressure_unit_name() const = 0;
    virtual std::string get_energy_unit_name() const = 0;
    
    // Conversion methods
    real convert_length(real value) const;
    real convert_mass(real value) const;
    real convert_time(real value) const;
    real convert_velocity(real value) const;
    real convert_density(real value) const;
    real convert_pressure(real value) const;
    real convert_energy(real value) const;
    
    // Serialization
    virtual nlohmann::json to_json() const;
    
protected:
    UnitSystem() = default;
};

} // namespace sph

#endif // SPH_CORE_UNIT_SYSTEM_HPP
```

**Estimated Time:** 2-3 hours

---

### Task 1.2: Implement Concrete Unit Systems

**Files to Create:**
- `include/core/galactic_unit_system.hpp`
- `src/core/galactic_unit_system.cpp`
- `include/core/si_unit_system.hpp`
- `src/core/si_unit_system.cpp`
- `include/core/cgs_unit_system.hpp`
- `src/core/cgs_unit_system.cpp`

**Steps for Each Unit System:**

1. Define conversion constants
2. Implement all pure virtual methods
3. Calculate derived units from fundamentals
4. Add unit name strings
5. Test compilation

**GalacticUnitSystem Constants:**
```cpp
namespace {
    constexpr real kParsecToCm = 3.0857e18;
    constexpr real kSolarMassToGram = 1.989e33;
    constexpr real kGravConstCgs = 6.674e-8;
    
    // Time unit derived from G=1 assumption
    constexpr real kTimeUnitGalactic = 
        std::sqrt(kParsecToCm * kParsecToCm * kParsecToCm / 
                  (kGravConstCgs * kSolarMassToGram));
}
```

**Acceptance Criteria:**
- [ ] All three unit systems compile
- [ ] Conversion factors verified against references
- [ ] Unit tests pass (see Task 1.4)

**Estimated Time:** 4-5 hours

---

### Task 1.3: Create Unit System Factory

**Files to Create:**
- `include/core/unit_system_factory.hpp`
- `src/core/unit_system_factory.cpp`

**Implementation:**

```cpp
class UnitSystemFactory {
public:
    static std::unique_ptr<UnitSystem> create(UnitSystemType type);
    static std::unique_ptr<UnitSystem> create_from_string(const std::string& name);
    static std::unique_ptr<UnitSystem> create_from_json(const nlohmann::json& json);
    
private:
    UnitSystemFactory() = delete;
};
```

**Acceptance Criteria:**
- [ ] Factory creates all three unit systems
- [ ] String parsing works ("galactic", "SI", "cgs")
- [ ] JSON parsing includes error handling
- [ ] Invalid inputs throw appropriate exceptions

**Estimated Time:** 2 hours

---

### Task 1.4: Unit System Tests

**Files to Create:**
- `tests/unit/test_unit_system.cpp`

**Test Cases:**

1. **Test conversion factors**
   - Verify galactic conversions match expected values
   - Verify SI conversions are identity
   - Verify CGS conversions are identity

2. **Test derived units**
   - velocity = length / time
   - density = mass / length^3
   - pressure = mass / (length * time^2)
   - energy = mass * length^2 / time^2

3. **Test factory**
   - Create from enum
   - Create from string
   - Create from JSON
   - Handle invalid inputs

4. **Test JSON serialization**
   - Round-trip conversion
   - Schema validation

**Estimated Time:** 3-4 hours

---

## Phase 2: Protocol Buffers Schema (1-2 days)

### Task 2.1: Define Protocol Buffer Messages

**Files to Create:**
- `proto/particle_data.proto`
- `proto/metadata.proto`

**Steps:**

1. Create `proto/` directory
2. Define particle message structure
3. Define snapshot message structure
4. Define metadata messages
5. Add field documentation

**particle_data.proto:**
```protobuf
syntax = "proto3";

package sph.output;

message Particle {
    repeated double position = 1;
    repeated double velocity = 2;
    repeated double acceleration = 3;
    double mass = 4;
    double density = 5;
    double pressure = 6;
    double internal_energy = 7;
    double sound_speed = 8;
    double smoothing_length = 9;
    double gradh_term = 10;
    double balsara_switch = 11;
    double alpha_coefficient = 12;
    double gravitational_potential = 13;
    int32 id = 14;
    int32 neighbor_count = 15;
    int32 type = 16;
}

message Snapshot {
    int32 timestep = 1;
    double time = 2;
    int32 dimension = 3;
    repeated Particle particles = 4;
    repeated Particle ghost_particles = 5;
}

message EnergySnapshot {
    double time = 1;
    double kinetic_energy = 2;
    double thermal_energy = 3;
    double potential_energy = 4;
    double total_energy = 5;
}

message EnergySeries {
    repeated EnergySnapshot snapshots = 1;
}
```

**Acceptance Criteria:**
- [ ] Proto files have correct syntax
- [ ] All SPHParticle fields mapped
- [ ] Field numbers assigned (important for versioning)
- [ ] Documentation comments added

**Estimated Time:** 2-3 hours

---

### Task 2.2: Update Build System for Protobuf

**Files to Modify:**
- `conanfile.txt`
- `CMakeLists.txt`

**Steps:**

1. Add protobuf to Conan dependencies
2. Add CMake protobuf compilation
3. Configure include paths
4. Test build

**conanfile.txt update:**
```ini
[requires]
boost/1.84.0
nlohmann_json/3.11.3
gtest/1.14.0
protobuf/3.21.12

[generators]
CMakeDeps
CMakeToolchain
```

**CMakeLists.txt addition:**
```cmake
# Protocol Buffers
find_package(Protobuf REQUIRED)

# Generate proto files
file(GLOB PROTO_FILES "${CMAKE_SOURCE_DIR}/proto/*.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILES})

# Add to include paths
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Add proto sources to library
target_sources(sph_core PRIVATE ${PROTO_SRCS})
target_link_libraries(sph_core PUBLIC protobuf::libprotobuf)
```

**Acceptance Criteria:**
- [ ] Conan installs protobuf
- [ ] CMake generates .pb.cc and .pb.h files
- [ ] Build succeeds with proto files
- [ ] Generated headers accessible from code

**Estimated Time:** 3-4 hours

---

### Task 2.3: Test Protobuf Integration

**Files to Create:**
- `tests/unit/test_protobuf.cpp`

**Test Cases:**

1. Create and serialize Particle message
2. Deserialize and verify fields
3. Test Snapshot with multiple particles
4. Test EnergySeries with time series data
5. Measure serialization performance

**Acceptance Criteria:**
- [ ] Can create proto messages
- [ ] Serialization/deserialization works
- [ ] Binary output is valid protobuf format
- [ ] Performance acceptable (< 1ms per particle)

**Estimated Time:** 2 hours

---

## Phase 3: Output Writers (3-4 days)

### Task 3.1: Create OutputWriter Interface

**Files to Create:**
- `include/core/output_writer.hpp`

**Implementation:**

```cpp
namespace sph {

enum class OutputFormat {
    CSV,
    PROTOBUF
};

template<int Dim>
class OutputWriter {
public:
    virtual ~OutputWriter() = default;
    
    virtual void write_snapshot(
        const SPHParticle<Dim>* particles,
        int num_particles,
        const std::vector<SPHParticle<Dim>>* ghost_particles,
        real time,
        int timestep
    ) = 0;
    
    virtual void write_energy(
        real time,
        real kinetic,
        real thermal,
        real potential
    ) = 0;
    
    virtual std::string get_extension() const = 0;
    virtual OutputFormat get_format() const = 0;
    virtual void set_unit_system(std::shared_ptr<UnitSystem> units) = 0;
    
protected:
    OutputWriter() = default;
};

} // namespace sph
```

**Acceptance Criteria:**
- [ ] Interface compiles
- [ ] Template instantiation works for Dim=1,2,3
- [ ] Doxygen documentation complete

**Estimated Time:** 1-2 hours

---

### Task 3.2: Implement CSV Writer

**Files to Create:**
- `include/core/csv_writer.hpp`
- `src/core/csv_writer.cpp`
- `include/core/csv_writer.tpp` (template implementation)

**Key Methods:**

```cpp
template<int Dim>
class CSVWriter : public OutputWriter<Dim> {
public:
    explicit CSVWriter(const std::string& output_dir, bool with_header = true);
    
    void write_snapshot(...) override;
    void write_energy(...) override;
    
private:
    void write_header(std::ofstream& out);
    void write_particle(std::ofstream& out, const SPHParticle<Dim>& p);
    std::string format_filename(int timestep) const;
    
    std::string output_dir_;
    bool with_header_;
    std::shared_ptr<UnitSystem> unit_system_;
    std::ofstream energy_file_;
};
```

**Header Format:**
```
# SPH Simulation Snapshot
# Time: 1.234
# Dimension: 2
# Unit System: galactic
pos_x,pos_y,vel_x,vel_y,...
```

**Acceptance Criteria:**
- [ ] Writes valid CSV files
- [ ] Header includes metadata
- [ ] Unit conversions applied correctly
- [ ] Ghost particles included if present
- [ ] Energy file created and updated

**Estimated Time:** 4-5 hours

---

### Task 3.3: Implement Protobuf Writer

**Files to Create:**
- `include/core/protobuf_writer.hpp`
- `src/core/protobuf_writer.cpp`
- `include/core/protobuf_writer.tpp`

**Key Methods:**

```cpp
template<int Dim>
class ProtobufWriter : public OutputWriter<Dim> {
public:
    explicit ProtobufWriter(const std::string& output_dir);
    
    void write_snapshot(...) override;
    void write_energy(...) override;
    
private:
    void particle_to_proto(
        const SPHParticle<Dim>& p, 
        sph::output::Particle* proto
    );
    
    std::string output_dir_;
    std::shared_ptr<UnitSystem> unit_system_;
    sph::output::EnergySeries energy_series_;
};
```

**Acceptance Criteria:**
- [ ] Writes valid protobuf binary files
- [ ] Unit conversions applied
- [ ] File sizes smaller than CSV
- [ ] Can be read by protobuf tools
- [ ] Energy series accumulated correctly

**Estimated Time:** 4-5 hours

---

### Task 3.4: Test Output Writers

**Files to Create:**
- `tests/unit/test_csv_writer.cpp`
- `tests/unit/test_protobuf_writer.cpp`

**Test Cases for Each Writer:**

1. Write single particle snapshot
2. Write multiple particles
3. Write with ghost particles
4. Verify unit conversions
5. Test energy output
6. Test file naming
7. Performance benchmark

**Acceptance Criteria:**
- [ ] All tests pass
- [ ] CSV files can be read by Python/R
- [ ] Protobuf files can be parsed
- [ ] Unit conversions verified
- [ ] Performance acceptable

**Estimated Time:** 3-4 hours

---

## Phase 4: Metadata System (1-2 days)

### Task 4.1: Implement MetadataWriter

**Files to Create:**
- `include/core/metadata_writer.hpp`
- `src/core/metadata_writer.cpp`

**Implementation:**

```cpp
class MetadataWriter {
public:
    explicit MetadataWriter(const std::string& output_dir);
    
    void write_metadata(
        const SimulationParameters& params,
        const UnitSystem& unit_system,
        const std::vector<OutputFormat>& output_formats
    );
    
    static std::string get_metadata_filename() { return "metadata.json"; }
    
private:
    nlohmann::json create_metadata_json(...);
    nlohmann::json create_unit_system_json(const UnitSystem& units);
    nlohmann::json create_physics_json(const PhysicsParameters& physics);
    
    std::string output_dir_;
};
```

**JSON Schema:**
```json
{
  "schema_version": "1.0.0",
  "code_version": "string",
  "creation_time": "ISO8601",
  "dimension": 1-3,
  "unit_system": {...},
  "physics": {...},
  "computational": {...},
  "output": {...},
  "sph_algorithm": "string"
}
```

**Acceptance Criteria:**
- [ ] Creates valid JSON
- [ ] All parameters included
- [ ] Timestamp in ISO 8601 format
- [ ] JSON schema validated
- [ ] Pretty-printed output

**Estimated Time:** 3-4 hours

---

### Task 4.2: Test Metadata Writer

**Files to Create:**
- `tests/unit/test_metadata_writer.cpp`

**Test Cases:**

1. Write metadata with all parameters
2. Verify JSON structure
3. Test round-trip parsing
4. Test with different unit systems
5. Validate schema

**Estimated Time:** 2 hours

---

## Phase 5: Integration (2-3 days)

### Task 5.1: Create OutputCoordinator

**Files to Create:**
- `include/core/output_coordinator.hpp`
- `src/core/output_coordinator.cpp`
- `include/core/output_coordinator.tpp`

**Implementation:**

```cpp
template<int Dim>
class OutputCoordinator {
public:
    OutputCoordinator(
        const std::string& output_dir,
        const SimulationParameters& params
    );
    
    void add_writer(std::unique_ptr<OutputWriter<Dim>> writer);
    void set_unit_system(std::shared_ptr<UnitSystem> units);
    void write_particles(std::shared_ptr<Simulation<Dim>> sim);
    void write_energy(std::shared_ptr<Simulation<Dim>> sim);
    void write_metadata();
    
    int get_count() const { return count_; }
    
private:
    std::vector<std::unique_ptr<OutputWriter<Dim>>> writers_;
    std::shared_ptr<UnitSystem> unit_system_;
    MetadataWriter metadata_writer_;
    SimulationParameters params_;
    int count_;
};
```

**Acceptance Criteria:**
- [ ] Manages multiple writers
- [ ] Coordinates output timing
- [ ] Writes metadata once at start
- [ ] Increments snapshot counter
- [ ] Exception-safe

**Estimated Time:** 3-4 hours

---

### Task 5.2: Update Output<Dim> Class

**Files to Replace:**
- `include/output.hpp`
- `include/output.tpp`

**New Implementation:**

Replace the entire Output class with a clean implementation using OutputCoordinator.

**New Design:**
```cpp
template<int Dim>
class Output {
public:
    explicit Output(const OutputParameters& params);
    ~Output() = default;
    
    void write_particles(std::shared_ptr<Simulation<Dim>> sim);
    void write_energy(std::shared_ptr<Simulation<Dim>> sim);
    
    int get_count() const { return coordinator_->get_count(); }
    
private:
    std::unique_ptr<OutputCoordinator<Dim>> coordinator_;
};
```

**Implementation Details:**
- Remove all legacy file writing code
- Use OutputCoordinator exclusively
- Clean, modern interface
- Delegate all operations to coordinator

**Acceptance Criteria:**
- [ ] Clean implementation using OutputCoordinator
- [ ] All output formats work correctly
- [ ] Metadata written properly
- [ ] Unit conversions applied
- [ ] No legacy code remaining

**Estimated Time:** 3-4 hours

---

### Task 5.3: Implement OutputParameters

**Files to Create:**
- `include/core/output_parameters.hpp`
- `src/core/output_parameters.cpp`

**Complete Structure:**
```cpp
struct OutputParameters {
    std::string directory;
    int particle_interval;
    int energy_interval;
    std::vector<OutputFormat> formats = {OutputFormat::CSV, OutputFormat::PROTOBUF};
    UnitSystemType unit_system = UnitSystemType::GALACTIC;
    bool write_metadata = true;
};
```

**Builder Implementation:**
```cpp
OutputParametersBuilder& with_directory(const std::string& dir);
OutputParametersBuilder& with_particle_output_interval(int interval);
OutputParametersBuilder& with_energy_output_interval(int interval);
OutputParametersBuilder& with_format(OutputFormat format);
OutputParametersBuilder& with_formats(const std::vector<OutputFormat>& formats);
OutputParametersBuilder& with_unit_system(UnitSystemType unit_system);
OutputParametersBuilder& with_metadata(bool write_metadata);
OutputParametersBuilder& from_json(const nlohmann::json& json);
```

**JSON Configuration:**
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

**Acceptance Criteria:**
- [ ] All fields properly initialized with good defaults
- [ ] JSON parsing complete and robust
- [ ] Builder pattern fully implemented
- [ ] Comprehensive validation
- [ ] Format and unit system validation

**Estimated Time:** 3 hours

---

### Task 5.4: Integration Tests

**Files to Create:**
- `tests/integration/test_output_pipeline.cpp`

**Test Scenarios:**

1. **Single format output**
   - CSV only
   - Protobuf only

2. **Dual format output**
   - CSV + Protobuf
   - Verify both created
   - Verify contents match

3. **Different unit systems**
   - Galactic units
   - SI units
   - CGS units
   - Verify conversions correct

4. **Complete workflow**
   - Initialize simulation
   - Run a few timesteps
   - Output snapshots
   - Verify metadata
   - Read back and verify

**Acceptance Criteria:**
- [ ] All integration tests pass
- [ ] Files created in correct locations
- [ ] Metadata accurate
- [ ] Unit conversions verified

**Estimated Time:** 4-5 hours

---

## Phase 6: Python Utilities (2-3 days)

### Task 6.1: Create Python Module Structure

**Directory Structure:**
```
workflows/scripts/sph_output/
├── __init__.py
├── metadata.py
├── csv_reader.py
├── protobuf_reader.py
├── converter.py
├── units.py
└── tests/
    ├── test_metadata.py
    ├── test_readers.py
    └── test_converter.py
```

**Estimated Time:** 1 hour

---

### Task 6.2: Implement Metadata Parser

**File:** `workflows/scripts/sph_output/metadata.py`

**Classes:**

```python
class UnitSystem:
    """Unit system information from metadata"""
    def __init__(self, json_data):
        self.name = json_data['name']
        self.length_unit = json_data['length_unit']
        # ... other fields
        
    def convert_length(self, value):
        """Convert from code units to output units"""
        return value * self.length_factor

class Metadata:
    """Simulation metadata"""
    def __init__(self, json_data):
        self.schema_version = json_data['schema_version']
        self.dimension = json_data['dimension']
        self.unit_system = UnitSystem(json_data['unit_system'])
        # ... other fields
    
    @classmethod
    def from_file(cls, filepath):
        """Load metadata from JSON file"""
        with open(filepath) as f:
            return cls(json.load(f))
```

**Acceptance Criteria:**
- [ ] Parses all metadata fields
- [ ] Validates schema version
- [ ] Provides unit conversion methods
- [ ] Good error messages

**Estimated Time:** 3 hours

---

### Task 6.3: Implement CSV Reader

**File:** `workflows/scripts/sph_output/csv_reader.py`

**Functions:**

```python
def read_snapshot_csv(filepath, metadata=None):
    """
    Read CSV snapshot file
    
    Args:
        filepath: Path to .csv file
        metadata: Optional Metadata object for unit info
        
    Returns:
        pandas.DataFrame with particle data
    """
    # Read CSV with pandas
    # Parse header comments
    # Apply unit conversions if metadata provided
    pass

def read_energy_csv(filepath, metadata=None):
    """Read energy time series CSV"""
    pass
```

**Acceptance Criteria:**
- [ ] Reads CSV files correctly
- [ ] Handles headers
- [ ] Returns pandas DataFrame
- [ ] Unit conversions optional
- [ ] Good documentation

**Estimated Time:** 2-3 hours

---

### Task 6.4: Implement Protobuf Reader

**File:** `workflows/scripts/sph_output/protobuf_reader.py`

**Requirements:**
- Install protobuf Python package
- Use generated Python proto files

**Functions:**

```python
def read_snapshot_pb(filepath, metadata=None):
    """
    Read protobuf snapshot file
    
    Args:
        filepath: Path to .pb file
        metadata: Optional Metadata object
        
    Returns:
        pandas.DataFrame with particle data
    """
    from sph_output.proto import particle_data_pb2
    
    snapshot = particle_data_pb2.Snapshot()
    with open(filepath, 'rb') as f:
        snapshot.ParseFromString(f.read())
    
    # Convert to DataFrame
    # Apply unit conversions
    pass
```

**Acceptance Criteria:**
- [ ] Reads protobuf files
- [ ] Converts to DataFrame
- [ ] Same interface as CSV reader
- [ ] Unit conversions work

**Estimated Time:** 2-3 hours

---

### Task 6.5: Create Example Scripts

**Files to Create:**
- `workflows/scripts/examples/read_snapshot.py`
- `workflows/scripts/examples/convert_format.py`
- `workflows/scripts/examples/plot_energy.py`
- `workflows/scripts/examples/compare_units.py`

**read_snapshot.py:**
```python
#!/usr/bin/env python3
"""Example: Read and display snapshot data"""

from sph_output import Metadata, read_snapshot

# Load metadata
metadata = Metadata.from_file("output/metadata.json")
print(f"Simulation: {metadata.dimension}D, {metadata.unit_system.name} units")

# Read snapshot (auto-detects format)
df = read_snapshot("output/snapshots/00000.csv", metadata)
print(df.describe())

# Or read protobuf
df_pb = read_snapshot("output/snapshots/00000.pb", metadata)
assert df.equals(df_pb)  # Should be identical
```

**Acceptance Criteria:**
- [ ] Examples run without errors
- [ ] Clear, documented code
- [ ] Cover common use cases
- [ ] Include visualization examples

**Estimated Time:** 3-4 hours

---

### Task 6.6: Python Tests

**Files to Create:**
- `workflows/scripts/sph_output/tests/test_*.py`

**Test Coverage:**
- Metadata parsing
- CSV reading
- Protobuf reading
- Format conversion
- Unit conversions

**Use pytest:**
```python
def test_read_csv_snapshot():
    """Test CSV snapshot reading"""
    # Create test CSV file
    # Read it
    # Verify contents
    pass
```

**Estimated Time:** 3-4 hours

---

## Phase 7: Documentation (1-2 days)

### Task 7.1: Create User Guides

**Files to Create:**

1. **`docs/OUTPUT_FORMATS.md`**
   - Description of CSV format
   - Description of Protobuf format
   - When to use each
   - Examples

2. **`docs/UNIT_SYSTEMS.md`**
   - Galactic units explained
   - SI units
   - CGS units
   - Conversion examples
   - How to choose

3. **`docs/MIGRATION_GUIDE.md`**
   - Updating from old format
   - Config file changes
   - Python script updates
   - Common issues

4. **`docs/OUTPUT_API.md`**
   - OutputWriter interface
   - Creating custom writers
   - Python API reference

**Estimated Time:** 4-5 hours

---

### Task 7.2: Update Existing Documentation

**Files to Modify:**
- `workflows/README.md`
- `README.md`
- `docs/QUICKSTART.md`

**Add Sections:**
- Output system overview
- Link to detailed docs
- Quick start examples

**Estimated Time:** 2 hours

---

### Task 7.3: Code Documentation

**Tasks:**
- Add Doxygen comments to all public APIs
- Generate Doxygen documentation
- Review and fix warnings

**Estimated Time:** 2-3 hours

---

## Phase 8: Testing & Validation (2-3 days)

### Task 8.1: End-to-End Workflow Tests

**Create Test Workflows:**

1. **1D Shock Tube**
   - Configure with galactic units
   - Output CSV + Protobuf
   - Verify results identical

2. **2D Sod Test**
   - Configure with SI units
   - Output both formats
   - Compare file sizes
   - Verify unit conversions

3. **3D Test**
   - CGS units
   - Verify metadata
   - Test Python scripts

**Acceptance Criteria:**
- [ ] All workflows complete successfully
- [ ] Outputs readable by Python
- [ ] Unit conversions verified
- [ ] Performance acceptable

**Estimated Time:** 4-5 hours

---

### Task 8.2: Performance Benchmarks

**Benchmarks to Run:**

1. **Write performance**
   - CSV vs Protobuf
   - Different particle counts (100, 1000, 10000, 100000)
   - With/without unit conversions

2. **File sizes**
   - CSV vs Protobuf
   - Compression ratios

3. **Read performance**
   - Python CSV vs Protobuf
   - NumPy array conversion speed

**Create:** `docs/PERFORMANCE.md`

**Estimated Time:** 3-4 hours

---

### Task 8.3: Validation Tests

**Physics Validation:**

1. Verify energy conservation with unit conversions
2. Compare results across unit systems
3. Verify dimensional analysis

**Format Validation:**

1. CSV parseable by external tools (Excel, R, etc.)
2. Protobuf parseable by standard tools
3. Metadata JSON schema valid

**Estimated Time:** 3-4 hours

---

### Task 8.4: Bug Fixes and Polish

**Review and Fix:**
- Compiler warnings
- Clang-tidy issues
- Memory leaks (valgrind)
- Code style violations
- Documentation errors

**Estimated Time:** 4-6 hours

---

## Final Checklist

### Code Quality
- [ ] All files follow coding rules
- [ ] No compiler warnings
- [ ] Clang-tidy passes
- [ ] No memory leaks
- [ ] Exception safety verified

### Testing
- [ ] All unit tests pass
- [ ] All integration tests pass
- [ ] Python tests pass
- [ ] Performance benchmarks complete
- [ ] Validation tests pass

### Documentation
- [ ] All public APIs documented
- [ ] User guides complete
- [ ] Examples work
- [ ] README updated

### Integration
- [ ] Workflows updated
- [ ] Build system works
- [ ] Dependencies resolved
- [ ] All formats functional

---

## Post-Implementation Tasks

1. **Code Review**
   - Peer review all changes
   - Address feedback
   - Final testing

2. **Release Preparation**
   - Update CHANGELOG
   - Tag version
   - Create release notes

3. **User Communication**
   - Announce new features
   - Provide examples
   - Offer support

---

## Risk Mitigation

### Potential Issues

1. **Protobuf build complexity**
   - Mitigation: Test early, provide clear build docs
   - Solution: Ensure proper Conan configuration and CMake setup

2. **Performance degradation**
   - Mitigation: Benchmark early, optimize hot paths
   - Solution: Use efficient serialization, minimize conversions

3. **Implementation complexity**
   - Mitigation: Follow detailed roadmap, implement incrementally
   - Solution: Test each phase before proceeding

4. **Unit conversion errors**
   - Mitigation: Extensive testing, validation
   - Solution: Comprehensive test suite with known values

---

## Summary

**Total Estimated Time:** 14-20 working days

**Critical Path:**
1. Unit system (prerequisite for everything)
2. Protobuf setup (needed for writer)
3. Output writers (core functionality)
4. Integration (ties it together)
5. Testing (validation)

**Can Be Parallelized:**
- Python utilities (independent)
- Documentation (can start early)
- Examples (as features complete)

**Deliverables:**
1. Complete unit system framework
2. Dual-format output system
3. Metadata infrastructure
4. Python analysis tools
5. Comprehensive documentation
6. Updated workflows
7. Test suite

---

**Next Steps:** Review this plan, adjust timeline as needed, then begin with Phase 1, Task 1.1.
