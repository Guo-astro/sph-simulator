#!/bin/bash
# Debug script to compare 2D shock tube outputs with analytical solution

set -e

echo "=== 2D Shock Tube Diagnostic ==="
echo

# Build the 2D shock tube plugin
echo "Building 2D shock tube plugin..."
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/02_simulation_2d
make clean
make all

# Run with minimal output
echo
echo "Running baseline 2D simulation..."
./shock_tube_baseline.out > run_baseline.log 2>&1 || {
    echo "ERROR: Baseline simulation failed"
    tail -50 run_baseline.log
    exit 1
}

echo "✓ Baseline simulation completed"

# Check if output exists
if [ ! -f "output_baseline/output/baseline_shock_tube_2d/particle_0000.dat" ]; then
    echo "ERROR: No output files generated"
    exit 1
fi

# Count particles in first and last output
initial_count=$(wc -l < output_baseline/output/baseline_shock_tube_2d/particle_0000.dat)
final_count=$(ls output_baseline/output/baseline_shock_tube_2d/particle_*.dat | tail -1 | xargs wc -l | awk '{print $1}')

echo
echo "Particle counts:"
echo "  Initial: $initial_count"
echo "  Final:   $final_count"

# Check for NaN or Inf values
echo
echo "Checking for numerical instabilities..."
if grep -q "nan\|inf" output_baseline/output/baseline_shock_tube_2d/particle_*.dat; then
    echo "WARNING: Found NaN or Inf values in output"
else
    echo "✓ No NaN/Inf detected"
fi

# Extract density values from middle timestep
mid_file=$(ls output_baseline/output/baseline_shock_tube_2d/particle_*.dat | sed -n '11p')
echo
echo "Analyzing $mid_file..."

# Column 3 should be density
awk '{print $3}' "$mid_file" > /tmp/densities.txt
min_dens=$(sort -n /tmp/densities.txt | head -1)
max_dens=$(sort -n /tmp/densities.txt | tail -1)

echo "Density range: [$min_dens, $max_dens]"
echo "Expected: [0.125, 1.0] initially"

# If density hasn't changed from initial conditions, shock didn't develop
if awk -v min="$min_dens" -v max="$max_dens" 'BEGIN {exit !(min > 0.12 && min < 0.13 && max > 0.99 && max < 1.01)}'; then
    echo "WARNING: Density distribution unchanged - shock may not have developed!"
    echo "This suggests:"
    echo "  1. Neighbor search may be failing"
    echo "  2. Force calculations may be zero/negligible"
    echo "  3. Boundary conditions may be suppressing evolution"
else
    echo "✓ Density distribution has evolved (shock developing)"
fi

echo
echo "=== Diagnostic Complete ==="
