#!/usr/bin/env python3
"""Quick analysis of 2D shock tube output"""

import numpy as np
import glob
import sys

files = sorted([f for f in glob.glob("output/shock_tube_2d/*.dat") if f.split('/')[-1][0].isdigit()])
if not files:
    print("No output files found!")
    sys.exit(1)

print(f"Found {len(files)} output files\n")

# Analyze first and last
for label, fname in [("INITIAL", files[0]), ("FINAL", files[-1])]:
    data = np.loadtxt(fname)
    
    print(f"{label}: {fname}")
    print(f"  Particles: {len(data)}")
    print(f"  X range: [{data[:,0].min():.3f}, {data[:,0].max():.3f}]")
    print(f"  Y range: [{data[:,1].min():.3f}, {data[:,1].max():.3f}]")
    print(f"  Density: [{data[:,2].min():.4f}, {data[:,2].max():.4f}]")
    print(f"  Pressure: [{data[:,3].min():.4f}, {data[:,3].max():.4f}]")
    print(f"  Vx: [{data[:,4].min():.4f}, {data[:,4].max():.4f}]")
    print(f"  Vy: [{data[:,5].min():.4f}, {data[:,5].max():.4f}]")
    print()

# Check if shock developed
initial_data = np.loadtxt(files[0])
final_data = np.loadtxt(files[-1])

vx_changed = np.abs(final_data[:,4]).max() > 0.01
dens_spread = final_data[:,2].std() > 0.05

if vx_changed:
    print("✓ Velocities have developed (shock is propagating)")
else:
    print("✗ NO velocity change (shock NOT developing!)")

if dens_spread:
    print("✓ Density field has evolved")
else:
    print("✗ Density unchanged (problem!)")

print(f"\nMax velocity: {np.abs(final_data[:,4]).max():.6f}")
print(f"Density std dev: {final_data[:,2].std():.6f}")
