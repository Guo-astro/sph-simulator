# Output and Unit System Redesign - Quick Reference

**Created:** 2025-11-06  
**Related Documents:**
- [Design Document](OUTPUT_UNIT_SYSTEM_DESIGN.md) - Full architecture and design
- [Implementation Roadmap](IMPLEMENTATION_ROADMAP.md) - Step-by-step implementation plan

---

## What's Being Added

### 1. Multiple Output Formats
- **CSV**: Human-readable, with headers, easy debugging
- **Protocol Buffers**: Compact binary, 40-50% smaller, 5-10x faster parsing

### 2. Unit Systems
Three built-in unit systems with automatic conversions:

| System | Length | Mass | Time | Velocity |
|--------|--------|------|------|----------|
| **Galactic** | parsec (pc) | solar mass (M☉) | Myr | km/s |
| **SI** | meter (m) | kilogram (kg) | second (s) | m/s |
| **CGS** | centimeter (cm) | gram (g) | second (s) | cm/s |

### 3. Metadata System
JSON file containing:
- Unit system information
- Simulation parameters
- Physics settings
- Schema version (for backward compatibility)
- Timestamp

---

## File Structure

```
output/
├── metadata.json              # Simulation metadata
├── snapshots/
│   ├── 00000.csv             # CSV snapshots
│   ├── 00000.pb              # Protobuf snapshots
│   └── ...
├── energy.csv                # Energy time series
└── energy.pb
```

---

## Configuration Examples

### Minimal (Default Behavior)
```json
{
  "output": {
    "directory": "output",
    "particle_interval": 10,
    "energy_interval": 1
  }
}
```
**Result:** CSV output only, code units (CGS-like), metadata written

### Full Featured
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
**Result:** Both CSV and Protobuf, galactic units, metadata written

### Python-Friendly
```json
{
  "output": {
    "directory": "results",
    "particle_interval": 5,
    "energy_interval": 1,
    "formats": ["protobuf"],
    "unit_system": "SI",
    "write_metadata": true
  }
}
```
**Result:** Protobuf only (smaller, faster), SI units, metadata

---

## Python Usage

### Reading Data

```python
from sph_output import Metadata, read_snapshot

# Load metadata
metadata = Metadata.from_file("output/metadata.json")
print(f"Units: {metadata.unit_system.name}")

# Read snapshot (auto-detects format)
df = read_snapshot("output/snapshots/00000.csv", metadata)
# or
df = read_snapshot("output/snapshots/00000.pb", metadata)

# Access particle data
print(df['position_x'])  # Already in output units (e.g., parsecs)
print(df['velocity_x'])  # Already in output units (e.g., km/s)
```

### Energy Evolution

```python
from sph_output import read_energy

energy = read_energy("output/energy.csv", metadata)
# or
energy = read_energy("output/energy.pb", metadata)

import matplotlib.pyplot as plt
plt.plot(energy['time'], energy['total_energy'])
plt.xlabel(f"Time ({metadata.unit_system.time_unit})")
plt.ylabel(f"Energy ({metadata.unit_system.energy_unit})")
plt.show()
```

### Format Conversion

```python
from sph_output import convert_format

# Convert CSV to Protobuf
convert_format(
    "output/snapshots/00000.csv",
    "output/snapshots/00000.pb",
    metadata
)

# Convert Protobuf to CSV
convert_format(
    "output/snapshots/00000.pb",
    "output/snapshots/00000_converted.csv",
    metadata
)
```

---

## Architecture Overview

```
Simulation → OutputCoordinator → OutputWriter(s) → Files
                    ↓
              MetadataWriter → metadata.json
                    ↓
              UnitSystem → Conversions
```

### Key Classes

- **`UnitSystem`**: Base class for unit conversions
  - `GalacticUnitSystem`
  - `SIUnitSystem`
  - `CGSUnitSystem`

- **`OutputWriter<Dim>`**: Base class for output formats
  - `CSVWriter<Dim>`
  - `ProtobufWriter<Dim>`

- **`OutputCoordinator<Dim>`**: Manages multiple writers

- **`MetadataWriter`**: Creates metadata.json

---

## CSV Format Specification

### Header
```
# SPH Simulation Snapshot
# Time: 1.234567
# Dimension: 2
# Unit System: galactic
# Schema Version: 1.0
# Fields: pos_x,pos_y,vel_x,vel_y,...
```

### Data
```csv
pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,mass,density,pressure,energy,...
1.0,2.0,0.5,0.3,0.0,0.0,1.0,1.0,0.1,2.5,...
...
```

All values already converted to output units.

---

## Protocol Buffers Schema

### Particle Message
```protobuf
message Particle {
    repeated double position = 1;      // [Dim]
    repeated double velocity = 2;      // [Dim]
    repeated double acceleration = 3;  // [Dim]
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
```

### Snapshot Message
```protobuf
message Snapshot {
    int32 timestep = 1;
    double time = 2;
    int32 dimension = 3;
    repeated Particle particles = 4;
    repeated Particle ghost_particles = 5;
}
```

---

## Metadata JSON Schema

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
    "energy_unit": "M_sun*pc^2/Myr^2"
  },
  "physics": {
    "gamma": 1.4,
    "neighbor_number": 50,
    "artificial_viscosity": {...}
  },
  "computational": {...},
  "output": {...},
  "sph_algorithm": "DISPH"
}
```

---

## Performance Expectations

### File Sizes (10,000 particles, 2D)
- Current .dat: ~2.5 MB
- New CSV: ~3.0 MB (with headers)
- Protobuf: ~1.5 MB (40% smaller)
- CSV gzipped: ~0.5 MB
- Protobuf gzipped: ~0.3 MB

### Read/Write Speed
- Protobuf: 5-10x faster than CSV for parsing
- CSV: Better for human inspection
- Both: Stream-based, low memory

---

## Python Usage

### Reading Data with Modern API

```python
from sph_output import Metadata, read_snapshot

metadata = Metadata.from_file("output/metadata.json")
df = read_snapshot("output/snapshots/00000.csv", metadata)
x = df['position_x']  # Pandas DataFrame, named columns
```

**Benefits:**
- Self-documenting (column names, units)
- Type-safe
- Unit-aware
- Format-independent (works with CSV or Protobuf)

---

## Implementation Timeline

| Phase | Description | Time | Status |
|-------|-------------|------|--------|
| 1 | Unit System Framework | 2-3 days | Not Started |
| 2 | Protocol Buffers Schema | 1-2 days | Not Started |
| 3 | Output Writers | 3-4 days | Not Started |
| 4 | Metadata System | 1-2 days | Not Started |
| 5 | Integration | 2-3 days | Not Started |
| 6 | Python Utilities | 2-3 days | Not Started |
| 7 | Documentation | 1-2 days | Not Started |
| 8 | Testing & Validation | 2-3 days | Not Started |
| **Total** | | **14-20 days** | |

---

## Testing Strategy

### Unit Tests
- Unit system conversions
- Protobuf serialization
- CSV writing
- Metadata generation

### Integration Tests
- Full output pipeline
- Multiple formats
- Different unit systems
- Round-trip conversions

### Validation Tests
- Energy conservation
- Dimensional analysis
- Format compatibility
- Performance benchmarks

---

## Dependencies

### C++ Side
- Protocol Buffers (`protobuf/3.21.12` via Conan)
- nlohmann_json (already included)
- Boost (already included)

### Python Side
- `pandas` - DataFrame support
- `protobuf` - Protobuf reading
- `numpy` - Numerical arrays
- `matplotlib` - Plotting (optional)

---

## Key Design Decisions

1. **Modern C++ Design**: Clean, RAII-based implementation
2. **Template Design**: OutputWriter<Dim> for dimension flexibility
3. **Factory Pattern**: UnitSystemFactory for extensibility
4. **Strategy Pattern**: Multiple OutputWriter implementations
5. **Metadata First**: Always write metadata before data
6. **Unit Conversion**: Applied during output, not in simulation
7. **Format Independence**: Same API for all formats

---

## Future Extensions

1. **HDF5 Format**: For very large simulations
2. **Built-in Compression**: Gzip for CSV files
3. **Streaming Output**: Real-time analysis
4. **Custom Fields**: Plugin-defined output
5. **Parallel I/O**: MPI support
6. **Restart Files**: Checkpoint integration

---

## Getting Help

- **Design Questions**: See [OUTPUT_UNIT_SYSTEM_DESIGN.md](OUTPUT_UNIT_SYSTEM_DESIGN.md)
- **Implementation Details**: See [IMPLEMENTATION_ROADMAP.md](IMPLEMENTATION_ROADMAP.md)
- **API Reference**: See `docs/OUTPUT_API.md` (to be created)
- **Examples**: Check workflow configurations and Python scripts

---

## Recommended Configuration

### Standard Setup
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
#!/usr/bin/env python3
"""Analyze shock tube simulation results"""

from sph_output import Metadata, read_snapshot, read_energy
import matplotlib.pyplot as plt

# Load metadata
meta = Metadata.from_file("output/metadata.json")
print(f"Simulation: {meta.dimension}D {meta.sph_algorithm}")
print(f"Units: {meta.unit_system.name}")

# Read all snapshots
snapshots = []
for i in range(0, 100, 10):
    filename = f"output/snapshots/{i:05d}.pb"
    df = read_snapshot(filename, meta)
    snapshots.append(df)

# Read energy
energy = read_energy("output/energy.pb", meta)

# Plot energy conservation
plt.figure(figsize=(10, 6))
plt.plot(energy['time'], energy['total_energy'])
plt.xlabel(f"Time ({meta.unit_system.time_unit})")
plt.ylabel(f"Energy ({meta.unit_system.energy_unit})")
plt.title("Energy Conservation")
plt.grid(True)
plt.savefig("energy_evolution.png")

# Plot final state
final = snapshots[-1]
plt.figure(figsize=(10, 6))
plt.scatter(final['position_x'], final['position_y'], 
            c=final['density'], s=1, cmap='viridis')
plt.colorbar(label=f"Density ({meta.unit_system.density_unit})")
plt.xlabel(f"X ({meta.unit_system.length_unit})")
plt.ylabel(f"Y ({meta.unit_system.length_unit})")
plt.title(f"Final State at t={energy['time'].iloc[-1]:.2f} {meta.unit_system.time_unit}")
plt.savefig("final_state.png")
```

---

**End of Quick Reference**

For complete details, see the full design document and implementation roadmap.
