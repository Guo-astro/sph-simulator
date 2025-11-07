#!/usr/bin/env python3
"""
Animation script for Evrard collapse simulation (3D).
Generates visualization of the collapse process.
"""

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.animation import FuncAnimation, FFMpegWriter
import glob
import os
import sys

# Configuration
RESULTS_DIR = "../results"
ANIMATIONS_DIR = "../results/analysis/animations"
OUTPUT_FILE = os.path.join(ANIMATIONS_DIR, "evrard_collapse.mp4")

def load_snapshot(filename):
    """Load a single snapshot CSV file."""
    try:
        data = np.loadtxt(filename, delimiter=',', skiprows=1)
        if data.size == 0:
            return None
        
        # Expected columns: x, y, z, vx, vy, vz, rho, P, h, m, u
        if data.ndim == 1:
            data = data.reshape(1, -1)
        
        return {
            'x': data[:, 0],
            'y': data[:, 1],
            'z': data[:, 2],
            'rho': data[:, 6],
            'P': data[:, 7],
            'h': data[:, 8],
            'm': data[:, 9],
            'u': data[:, 10] if data.shape[1] > 10 else np.zeros(len(data))
        }
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def main():
    """Generate animation for Evrard collapse."""
    
    # Create output directory
    os.makedirs(ANIMATIONS_DIR, exist_ok=True)
    
    # Find snapshot files
    snapshot_files = sorted(glob.glob(os.path.join(RESULTS_DIR, "snapshot_*.csv")))
    
    if not snapshot_files:
        print(f"Error: No snapshot files found in {RESULTS_DIR}")
        print("Please run the simulation first: make run")
        sys.exit(1)
    
    print(f"Found {len(snapshot_files)} snapshots")
    
    # Load all snapshots
    snapshots = []
    times = []
    for i, filename in enumerate(snapshot_files):
        data = load_snapshot(filename)
        if data is not None:
            snapshots.append(data)
            # Extract time from filename or use index
            times.append(i * 0.01)  # Adjust based on actual output frequency
    
    if not snapshots:
        print("Error: Failed to load any snapshots")
        sys.exit(1)
    
    print(f"Loaded {len(snapshots)} valid snapshots")
    
    # Set up the figure for 3D projection in 2D (x-y plane)
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 6))
    
    # Find global min/max for consistent scaling
    all_x = np.concatenate([s['x'] for s in snapshots])
    all_y = np.concatenate([s['y'] for s in snapshots])
    all_rho = np.concatenate([s['rho'] for s in snapshots])
    all_P = np.concatenate([s['P'] for s in snapshots])
    
    x_range = [all_x.min() * 1.1, all_x.max() * 1.1]
    y_range = [all_y.min() * 1.1, all_y.max() * 1.1]
    rho_range = [all_rho.min(), all_rho.max()]
    P_range = [all_P.min(), all_P.max()]
    
    def update(frame):
        """Update function for animation."""
        ax1.clear()
        ax2.clear()
        
        data = snapshots[frame]
        time = times[frame]
        
        # Plot density (x-y projection)
        sc1 = ax1.scatter(data['x'], data['y'], c=data['rho'], 
                         s=20, cmap='viridis', alpha=0.6,
                         vmin=rho_range[0], vmax=rho_range[1])
        ax1.set_xlabel('x')
        ax1.set_ylabel('y')
        ax1.set_title(f'Density (t = {time:.3f})')
        ax1.set_xlim(x_range)
        ax1.set_ylim(y_range)
        ax1.set_aspect('equal')
        plt.colorbar(sc1, ax=ax1, label='Density')
        
        # Plot pressure (x-y projection)
        sc2 = ax2.scatter(data['x'], data['y'], c=data['P'], 
                         s=20, cmap='plasma', alpha=0.6,
                         vmin=P_range[0], vmax=P_range[1])
        ax2.set_xlabel('x')
        ax2.set_ylabel('y')
        ax2.set_title(f'Pressure (t = {time:.3f})')
        ax2.set_xlim(x_range)
        ax2.set_ylim(y_range)
        ax2.set_aspect('equal')
        plt.colorbar(sc2, ax=ax2, label='Pressure')
        
        fig.suptitle(f'Evrard Collapse Simulation - Frame {frame+1}/{len(snapshots)}', 
                    fontsize=14, fontweight='bold')
    
    # Create animation
    print("Creating animation...")
    anim = FuncAnimation(fig, update, frames=len(snapshots), 
                        interval=100, repeat=True)
    
    # Save animation
    print(f"Saving animation to {OUTPUT_FILE}...")
    writer = FFMpegWriter(fps=10, bitrate=1800)
    anim.save(OUTPUT_FILE, writer=writer, dpi=100)
    
    print(f"✓ Animation saved: {OUTPUT_FILE}")
    plt.close()

if __name__ == "__main__":
    main()
