# Evrard Collapse Workflow - Quick Start Guide

## Complete Workflow with Energy Analysis

### One-Command Full Workflow

```bash
make full-clean
```

This will:
1. ✓ Clean all build artifacts
2. ✓ Build the Evrard plugin
3. ✓ Run simulation (t=0 to t=20.0, output every 1.0s)
4. ✓ Generate energy conservation plots
5. ✓ Generate animations (if scripts available)

### Individual Steps

#### 1. Build Only
```bash
make build
```

#### 2. Run Simulation Only
```bash
make run
```
- Runs from `t=0` to `t=20.0`
- Outputs snapshots every `1.0` time units
- Outputs energy data every `1.0` time units
- Creates: `build/output/evrard_collapse/energy.dat`

#### 3. Generate Energy Plots Only
```bash
make energy-analysis
```
- Requires `energy.dat` from simulation
- Creates 3 publication-quality plots:
  - `energy_evolution.png` - Components + conservation error
  - `energy_paper_style.png` - Normalized (Evrard 1988 style)  
  - `energy_normalized.png` - Energy fractions

#### 4. All Visualizations
```bash
make viz
```
- Energy analysis + animations

### Output Locations

After running `make full-clean`, all outputs are organized:

```
workflows/evrard_workflow/01_simulation/
├── build/output/evrard_collapse/
│   ├── energy.dat                    # Raw energy data
│   ├── energy_plots/                 # Energy conservation plots
│   │   ├── energy_evolution.png
│   │   ├── energy_paper_style.png
│   │   └── energy_normalized.png
│   └── snapshots/                    # Particle data snapshots
│       ├── snapshot_00000.csv
│       ├── snapshot_00001.csv
│       └── ...
├── results/                          # Copied results
│   └── analysis/
│       ├── plots/                    # All plots
│       └── animations/               # All animations
```

### Energy Data Format

`energy.dat` contains:
```
# time kinetic thermal potential total
0.000000000000000e+00 0.000000000000000e+00 4.999999999999957e-02 -6.610442948966810e-01 -6.110442948966814e-01
1.000000000000000e+00 ...
2.000000000000000e+00 ...
...
```

### Energy Conservation Metrics

The visualization script prints detailed statistics:

```
==================================================================
 ENERGY CONSERVATION ANALYSIS - Evrard Collapse Test
==================================================================

Energy Conservation Metrics:
  Maximum |ΔE/E₀|: X.XXX%
  Mean |ΔE/E₀|:    X.XXX%
  Final ΔE/E₀:     X.XXX%

Quality Assessment:
  ✓✓✓ Energy conservation: EXCELLENT  (< 1%)
  ✓✓  Energy conservation: GOOD       (< 5%)
  ✓   Energy conservation: ACCEPTABLE (< 10%)
  ✗   Energy conservation: POOR       (≥ 10%)
```

### Direct Python Script Usage

You can also run the energy analysis directly:

```bash
python3 visualize_evrard.py build/output/evrard_collapse
```

This provides more detailed console output and statistics.

### Makefile Targets

```
make help              # Show all available targets
make build             # Build plugin
make run               # Run simulation  
make energy-analysis   # Generate energy plots
make animate           # 2D animation (if available)
make animate-3d        # 3D animation (if available)
make viz               # All visualizations
make full              # Build + Run + Viz (incremental)
make full-clean        # Full workflow from scratch
make clean             # Clean build artifacts
make clean-outputs     # Clean all output files
make clean-all         # Clean everything
```

### Expected Results

For a well-behaved Evrard collapse simulation:

1. **Energy Conservation**: Total energy should remain constant within 1-5%
2. **Physical Behavior**:
   - t ~ 0-0.8: Gravitational collapse (PE → KE)
   - t ~ 0.8-1.5: Core bounce (KE → IE)
   - t > 1.5: Oscillations with damping

3. **Plots Show**:
   - Kinetic energy peak at core bounce
   - Thermal energy rise from shock heating
   - Potential energy minimum at maximum compression
   - Small oscillations in total energy (numerical error)

### Troubleshooting

**Build fails?**
```bash
make clean && make build
```

**Simulation slow?**
- Edit `src/plugin.cpp`: reduce end time or increase output interval
- Reduce particle count in initial conditions

**Energy not conserved?**
- Check CFL parameters in `plugin.cpp`
- Increase neighbor count
- Reduce tree opening angle (theta)

**No plots generated?**
```bash
# Install required Python packages
pip3 install numpy matplotlib

# Run visualization manually
python3 visualize_evrard.py build/output/evrard_collapse
```

### Quick Test

To verify everything works with a short simulation:

1. Edit `src/plugin.cpp`, change:
   ```cpp
   .with_time(
       /*start=*/0.0,
       /*end=*/2.0,      // Short test: just to first bounce
       /*output=*/0.5,
       /*energy=*/0.5
   )
   ```

2. Run:
   ```bash
   make full-clean
   ```

3. Check results:
   - Energy data: `build/output/evrard_collapse/energy.csv`
   - Energy plots: `results/analysis/plots/`

### References

See `README_ENERGY_ANALYSIS.md` for detailed physics background and energy conservation theory.
