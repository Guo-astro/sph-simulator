#!/usr/bin/env python3
"""
3D Animation script for Evrard collapse simulation.
Generates 3D visualization of the collapse process.
"""

import numpy as np
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.animation import FuncAnimation, FFMpegWriter
import pandas as pd
import glob
import os
import sys

# Configuration
OUTPUT_DIR = "../output/evrard_collapse/snapshots"
ANIMATIONS_DIR = "../results/analysis/animations"
OUTPUT_FILE = os.path.join(ANIMATIONS_DIR, "evrard_collapse_3d.mp4")

def load_snapshot(filename):
    """Load a single snapshot CSV file."""
    try:
        # Skip comment lines starting with '#'
        df = pd.read_csv(filename, comment='#')
        
        return {
            'x': df['pos_x'].values,
            'y': df['pos_y'].values,
            'z': df['pos_z'].values,
            'rho': df['density'].values,
            'P': df['pressure'].values,
            'h': df['smoothing_length'].values,
            'm': df['mass'].values,
            'u': df['energy'].values
        }
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def extract_time_from_energy_file():
    """Extract time information from energy.csv file."""
    energy_file = "../output/evrard_collapse/energy.csv"
    try:
        df = pd.read_csv(energy_file)
        if 'time' in df.columns:
            return df['time'].values
        return None
    except Exception as e:
        print(f"Could not load energy file: {e}")
        return None

def main():
    """Generate 3D animation for Evrard collapse."""
    
    # Create output directory
    os.makedirs(ANIMATIONS_DIR, exist_ok=True)
    
    # Find snapshot files
    snapshot_files = sorted(glob.glob(os.path.join(OUTPUT_DIR, "*.csv")))
    
    if not snapshot_files:
        print(f"Error: No snapshot files found in {OUTPUT_DIR}")
        print("Please run the simulation first: make run")
        sys.exit(1)
    
    print(f"Found {len(snapshot_files)} snapshots")
    
    # Try to get actual times from energy file
    times_from_energy = extract_time_from_energy_file()
    
    # Load all snapshots
    snapshots = []
    times = []
    for i, filename in enumerate(snapshot_files):
        data = load_snapshot(filename)
        if data is not None:
            snapshots.append(data)
            # Use actual time if available, otherwise estimate
            if times_from_energy is not None and i < len(times_from_energy):
                times.append(times_from_energy[i])
            else:
                times.append(i * 0.1)  # Estimate based on output frequency
    
    if not snapshots:
        print("Error: Failed to load any snapshots")
        sys.exit(1)
    
    print(f"Loaded {len(snapshots)} valid snapshots")
    
    # Set up the figure for 3D visualization
    fig = plt.figure(figsize=(16, 12))
    
    # Create 4 subplots: 3D view + 3 2D projections
    ax_3d = fig.add_subplot(221, projection='3d')
    ax_xy = fig.add_subplot(222)
    ax_xz = fig.add_subplot(223)
    ax_yz = fig.add_subplot(224)
    
    # Find global min/max for consistent scaling
    all_x = np.concatenate([s['x'] for s in snapshots])
    all_y = np.concatenate([s['y'] for s in snapshots])
    all_z = np.concatenate([s['z'] for s in snapshots])
    all_rho = np.concatenate([s['rho'] for s in snapshots])
    
    # Remove NaN and Inf values
    all_x = all_x[np.isfinite(all_x)]
    all_y = all_y[np.isfinite(all_y)]
    all_z = all_z[np.isfinite(all_z)]
    all_rho = all_rho[np.isfinite(all_rho)]
    
    if len(all_x) == 0 or len(all_y) == 0 or len(all_z) == 0:
        print("Error: No valid coordinate data found")
        sys.exit(1)
    
    coord_max = max(np.abs(all_x).max(), np.abs(all_y).max(), np.abs(all_z).max()) * 1.1
    rho_range = [all_rho.min(), all_rho.max()] if len(all_rho) > 0 else [0, 1]
    
    print(f"Coordinate range: ±{coord_max:.3f}")
    print(f"Density range: [{rho_range[0]:.6f}, {rho_range[1]:.6f}]")
    
    def update(frame):
        """Update function for animation."""
        # Clear all axes
        ax_3d.clear()
        ax_xy.clear()
        ax_xz.clear()
        ax_yz.clear()
        
        data = snapshots[frame]
        time = times[frame]
        
        # 3D scatter plot
        sc_3d = ax_3d.scatter(data['x'], data['y'], data['z'], 
                             c=data['rho'], s=10, cmap='hot', alpha=0.6,
                             vmin=rho_range[0], vmax=rho_range[1])
        ax_3d.set_xlabel('X')
        ax_3d.set_ylabel('Y')
        ax_3d.set_zlabel('Z')
        ax_3d.set_title(f'3D View (t = {time:.3f})', fontsize=12, fontweight='bold')
        ax_3d.set_xlim([-coord_max, coord_max])
        ax_3d.set_ylim([-coord_max, coord_max])
        ax_3d.set_zlim([-coord_max, coord_max])
        
        # Rotate view for better visualization
        ax_3d.view_init(elev=20, azim=45 + frame * 2)  # Slow rotation
        
        # XY projection
        sc_xy = ax_xy.scatter(data['x'], data['y'], c=data['rho'], 
                            s=15, cmap='hot', alpha=0.6,
                            vmin=rho_range[0], vmax=rho_range[1])
        ax_xy.set_xlabel('X')
        ax_xy.set_ylabel('Y')
        ax_xy.set_title('XY Projection', fontsize=10)
        ax_xy.set_xlim([-coord_max, coord_max])
        ax_xy.set_ylim([-coord_max, coord_max])
        ax_xy.set_aspect('equal')
        ax_xy.grid(True, alpha=0.3)
        
        # XZ projection
        sc_xz = ax_xz.scatter(data['x'], data['z'], c=data['rho'], 
                            s=15, cmap='hot', alpha=0.6,
                            vmin=rho_range[0], vmax=rho_range[1])
        ax_xz.set_xlabel('X')
        ax_xz.set_ylabel('Z')
        ax_xz.set_title('XZ Projection', fontsize=10)
        ax_xz.set_xlim([-coord_max, coord_max])
        ax_xz.set_ylim([-coord_max, coord_max])
        ax_xz.set_aspect('equal')
        ax_xz.grid(True, alpha=0.3)
        
        # YZ projection
        sc_yz = ax_yz.scatter(data['y'], data['z'], c=data['rho'], 
                            s=15, cmap='hot', alpha=0.6,
                            vmin=rho_range[0], vmax=rho_range[1])
        ax_yz.set_xlabel('Y')
        ax_yz.set_ylabel('Z')
        ax_yz.set_title('YZ Projection', fontsize=10)
        ax_yz.set_xlim([-coord_max, coord_max])
        ax_yz.set_ylim([-coord_max, coord_max])
        ax_yz.set_aspect('equal')
        ax_yz.grid(True, alpha=0.3)
        
        # Add colorbar to the last subplot
        if frame == 0:
            plt.colorbar(sc_yz, ax=ax_yz, label='Density', fraction=0.046, pad=0.04)
        
        # Main title
        fig.suptitle(f'Evrard Collapse - Frame {frame+1}/{len(snapshots)} - Time: {time:.3f}', 
                    fontsize=16, fontweight='bold')
        
        plt.tight_layout()
    
    # Create animation
    print("Creating 3D animation...")
    anim = FuncAnimation(fig, update, frames=len(snapshots), 
                        interval=200, repeat=True)
    
    # Save animation
    print(f"Saving animation to {OUTPUT_FILE}...")
    try:
        writer = FFMpegWriter(fps=5, bitrate=2400, codec='libx264')
        anim.save(OUTPUT_FILE, writer=writer, dpi=100)
        print(f"✓ Animation saved: {OUTPUT_FILE}")
    except Exception as e:
        print(f"Error saving animation: {e}")
        print("Make sure ffmpeg is installed: brew install ffmpeg")
        sys.exit(1)
    
    plt.close()
    
    # Print statistics
    print("\n=== Animation Statistics ===")
    print(f"Total frames: {len(snapshots)}")
    print(f"Time range: [{times[0]:.3f}, {times[-1]:.3f}]")
    print(f"Particles per frame: {len(snapshots[0]['x'])}")
    print(f"Output file: {OUTPUT_FILE}")
    print("===========================\n")

if __name__ == "__main__":
    main()
