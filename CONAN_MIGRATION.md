# SPH Simulation - Conan Integration & Animation

## Summary of Changes

### 1. Removed Visual Studio Project Files
- Deleted `sph.sln`, `sph.vcxproj`, and `sph.vcxproj.filters`
- Project now uses CMake exclusively for cross-platform builds

### 2. Integrated Conan Package Management

#### Added Files:
- `conanfile.txt` - Conan dependency configuration

#### Dependencies Managed by Conan:
- Boost 1.84.0 (header-only)
- nlohmann_json 3.11.3

#### Modified Files:
- `CMakeLists.txt` - Added Conan toolchain integration
- `src/solver.cpp` - Fixed parameter file parsing to support JSON file paths

### 3. Build Instructions

#### Prerequisites:
```bash
pip3 install conan
```

#### Build Steps:
```bash
# Create build directory
mkdir -p build
cd build

# Install dependencies with Conan
conan profile detect --force
conan install .. --output-folder=. --build=missing

# Configure and build with CMake
cmake .. -DCMAKE_TOOLCHAIN_FILE=conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(sysctl -n hw.ncpu)
```

### 4. Running Simulations

```bash
# Run shock tube simulation
./build/sph sample/shock_tube/shock_tube.json

# Output will be written to sample/shock_tube/results/
```

### 5. Generating Animations

#### Prerequisites:
```bash
pip3 install matplotlib
brew install ffmpeg  # or equivalent for your system
```

#### Create Animation:
```bash
python3 animate_shock_tube.py
```

This will generate:
- `shock_tube_animation.mp4` - Full animation of the simulation
- `shock_tube_snapshots.png` - Grid of snapshots at key timesteps

### Bug Fixes

#### Fixed Segmentation Fault
**Problem**: When passing JSON file paths (e.g., `sample/shock_tube/shock_tube.json`) instead of sample names, the code would crash with a segmentation fault.

**Root Cause**: The parameter parsing code only recognized simple sample names like `"shock_tube"`. When given a full path, it would set `m_sample = Sample::DoNotUse` but not populate the required `m_sample_parameters["N"]` field, causing a crash when the sample initialization code tried to access it.

**Solution**: Modified `src/solver.cpp` to detect sample names within file paths using substring matching, so both formats work:
- Short form: `./sph shock_tube`
- Full path: `./sph sample/shock_tube/shock_tube.json`

## Output Files

### Simulation Data Format
Each `.dat` file in the results directory contains particle data with these columns (for DIM=1):
1. Position
2. Velocity  
3. Acceleration
4. Mass
5. Density
6. Pressure
7. Internal Energy
8. Smoothing Length
9. Particle ID
10. Neighbor Count
11. Artificial Viscosity Alpha
12. Grad-h Term

### Animation Details
- **FPS**: 20 frames per second
- **Duration**: ~5 seconds (101 frames)
- **Resolution**: 1200x1000 pixels (3 subplots)
- **Plots**: 
  - Density vs Position
  - Pressure vs Position
  - Velocity vs Position

## Notes

- The project now builds cleanly with Conan on macOS (Apple Silicon)
- OpenMP support is detected and configured automatically
- All compiler warnings are preserved (they relate to virtual destructors)
- The shock tube test completes in ~841ms on an M-series Mac
