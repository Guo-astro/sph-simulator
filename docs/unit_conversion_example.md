# Unit Conversion in Output

## Type-Safe Unit Conversion Control

The SPH simulation now provides a type-safe way to control unit conversion in CSV output files.

### Usage Example

```cpp
#include "output.hpp"

// Create output system
Output<1> output;

// Option 1: Code units (default - no conversion)
// - Positions remain in simulation units (-0.5 to 1.5)
// - Densities remain dimensionless (0.125 to 1.0)
// - Best for comparison with analytical solutions
output.set_unit_conversion(UnitConversionMode::CODE_UNITS);

// Option 2: Galactic units
// - Positions converted to parsecs (pc)
// - Masses converted to solar masses (M_sol)
// - Velocities converted to km/s
output.set_unit_conversion(UnitConversionMode::GALACTIC_UNITS);

// Option 3: SI units
// - Positions converted to meters (m)
// - Masses converted to kilograms (kg)
// - Time converted to seconds (s)
output.set_unit_conversion(UnitConversionMode::SI_UNITS);

// Option 4: CGS units
// - Positions converted to centimeters (cm)
// - Masses converted to grams (g)
// - Time converted to seconds (s)
output.set_unit_conversion(UnitConversionMode::CGS_UNITS);
```

### Default Behavior

By default, the output system uses **CODE_UNITS** (no conversion). This ensures:
- Direct comparison with analytical solutions
- Compatibility with existing visualization scripts
- No confusion from large unit conversion factors

### Checking Current Mode

```cpp
auto mode = output.get_unit_conversion();
switch (mode) {
    case UnitConversionMode::CODE_UNITS:
        std::cout << "Using code units (no conversion)\n";
        break;
    case UnitConversionMode::GALACTIC_UNITS:
        std::cout << "Using Galactic units\n";
        break;
    // ...
}
```

### When to Use Each Mode

**CODE_UNITS** (default):
- Comparing with analytical solutions
- Debugging physics algorithms
- Visualization scripts expecting normalized values

**GALACTIC_UNITS**:
- Astrophysics applications
- Galactic-scale simulations
- Publishing results in astronomical units

**SI_UNITS**:
- Engineering applications
- Comparison with experimental data in SI
- General scientific communication

**CGS_UNITS**:
- Legacy astrophysics code compatibility
- Some physics journals prefer CGS

### Migration from Old Code

Old code that relied on automatic Galactic unit conversion:

```cpp
// OLD (before): Galactic units applied automatically
Output<1> output;
// Output was always in Galactic units
```

New code for same behavior:

```cpp
// NEW: Explicit unit conversion control
Output<1> output;
output.set_unit_conversion(UnitConversionMode::GALACTIC_UNITS);
```

Or use default code units for simpler visualization:

```cpp
// NEW: Code units (default - recommended)
Output<1> output;
// No set_unit_conversion() call needed
```

## Benefits

1. **Type Safety**: Enum prevents typos and invalid modes
2. **Explicit**: Clear intent in code
3. **Flexible**: Easy to switch between unit systems
4. **Backward Compatible**: Default behavior matches analytical solutions
5. **Self-Documenting**: Code clearly shows which units are being used
