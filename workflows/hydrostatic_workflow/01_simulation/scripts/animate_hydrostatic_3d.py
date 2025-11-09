#!/usr/bin/env python3
"""
3D-style Animation script for Hydrostatic equilibrium simulation (2D).
Generates enhanced 3D-perspective visualization of the 2D data.
"""

import numpy as np
import matplotlib
matplotlib.use('Agg')  # Non-interactive backend
import matplotlib.pyplot as plt
from mpl_toolkits.mplot3d import Axes3D
from matplotlib.animation import FuncAnimation, FFMpegWriter
import glob
import os
import sys

# Configuration
RESULTS_DIR = "../results"
ANIMATIONS_DIR = "../results/analysis/animations"
OUTPUT_FILE = os.path.join(ANIMATIONS_DIR, "hydrostatic_3d.mp4")

def load_snapshot(filename):
    """Load a single snapshot CSV file."""
    try:
        # Count header lines to skip and detect format
        skip_lines = 0
        has_id_column = False
        with open(filename) as f:
            for line in f:
                if line.startswith('#'):
                    skip_lines += 1
                else:
                    # First non-comment line is the header
                    skip_lines += 1
                    has_id_column = line.startswith('id,')
                    break
        
        data = np.loadtxt(filename, delimiter=',', skiprows=skip_lines)
        if data.size == 0:
            return None
        if data.ndim == 1:
            data = data.reshape(1, -1)
        
        if has_id_column:
            # New format: id,pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,mass,density,pressure,energy,smoothing_length,sound_speed,neighbors
            return {
                'x': data[:, 1],   # pos_x
                'y': data[:, 2],   # pos_y
                'vx': data[:, 3],  # vel_x
                'vy': data[:, 4],  # vel_y
                'rho': data[:, 8], # density
                'P': data[:, 9],   # pressure
                'h': data[:, 11],  # smoothing_length
                'm': data[:, 7],   # mass
                'u': data[:, 10]   # energy
            }
        else:
            # Old format: pos_x,pos_y,vel_x,vel_y,acc_x,acc_y,mass,density,pressure,energy,sound_speed,smoothing_length,...
            return {
                'x': data[:, 0],   # pos_x
                'y': data[:, 1],   # pos_y
                'vx': data[:, 2],  # vel_x
                'vy': data[:, 3],  # vel_y
                'rho': data[:, 7], # density
                'P': data[:, 8],   # pressure
                'h': data[:, 11],  # smoothing_length
                'm': data[:, 6],   # mass
                'u': data[:, 9]    # energy
            }
    except Exception as e:
        print(f"Error loading {filename}: {e}")
        return None

def main():
    """Generate 3D-style animation for hydrostatic equilibrium."""
    
    # Create output directory
    os.makedirs(ANIMATIONS_DIR, exist_ok=True)
    
    # Find snapshot files
    snapshot_files = sorted(glob.glob(os.path.join(RESULTS_DIR, "*.csv")))
    
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
            times.append(i * 0.1)  # Adjust based on actual output frequency
    
    if not snapshots:
        print("Error: Failed to load any snapshots")
        sys.exit(1)
    
    print(f"Loaded {len(snapshots)} valid snapshots")
    
    # Set up the figure for 3D-style visualization
    fig = plt.figure(figsize=(18, 10))
    
    # Create subplots: 3D surface view + density profile + pressure profile
    ax_3d = fig.add_subplot(131, projection='3d')
    ax_density = fig.add_subplot(132)
    ax_pressure = fig.add_subplot(133)
    
    # Find global min/max for consistent scaling
    all_x = np.concatenate([s['x'] for s in snapshots])
    all_y = np.concatenate([s['y'] for s in snapshots])
    all_rho = np.concatenate([s['rho'] for s in snapshots])
    all_P = np.concatenate([s['P'] for s in snapshots])
    
    # Remove NaN and Inf values
    all_x = all_x[np.isfinite(all_x)]
    all_y = all_y[np.isfinite(all_y)]
    all_rho = all_rho[np.isfinite(all_rho)]
    all_P = all_P[np.isfinite(all_P)]
    
    if len(all_x) == 0 or len(all_y) == 0:
        print("Error: No valid coordinate data found")
        sys.exit(1)
    
    x_range = [all_x.min() * 1.1, all_x.max() * 1.1]
    y_range = [all_y.min() * 1.1, all_y.max() * 1.1]
    rho_range = [all_rho.min(), all_rho.max()] if len(all_rho) > 0 else [0, 1]
    P_range = [all_P.min(), all_P.max()] if len(all_P) > 0 else [0, 1]
    
    print(f"X range: [{x_range[0]:.3f}, {x_range[1]:.3f}]")
    print(f"Y range: [{y_range[0]:.3f}, {y_range[1]:.3f}]")
    print(f"Density range: [{rho_range[0]:.6f}, {rho_range[1]:.6f}]")
    print(f"Pressure range: [{P_range[0]:.6f}, {P_range[1]:.6f}]")
    
    def update(frame):
        """Update function for animation."""
        # Clear all axes
        ax_3d.clear()
        ax_density.clear()
        ax_pressure.clear()
        
        data = snapshots[frame]
        time = times[frame]
        
        # 3D scatter plot with density as height
        sc_3d = ax_3d.scatter(data['x'], data['y'], data['rho'], 
                             c=data['rho'], s=20, cmap='viridis', alpha=0.7,
                             vmin=rho_range[0], vmax=rho_range[1])
        ax_3d.set_xlabel('X', fontsize=10)
        ax_3d.set_ylabel('Y', fontsize=10)
        ax_3d.set_zlabel('Density', fontsize=10)
        ax_3d.set_title(f'3D Density Surface (t = {time:.3f})', fontsize=12, fontweight='bold')
        ax_3d.set_xlim(x_range)
        ax_3d.set_ylim(y_range)
        ax_3d.set_zlim([rho_range[0], rho_range[1]])
        
        # Rotate view for better visualization
        ax_3d.view_init(elev=25, azim=45 + frame * 1.5)  # Slow rotation
        
        # Add colorbar to 3D plot
        if frame == 0:
            plt.colorbar(sc_3d, ax=ax_3d, label='Density', shrink=0.6, pad=0.1)
        
        # Density profile (spatial distribution)
        sc_density = ax_density.scatter(data['x'], data['y'], c=data['rho'], 
                                       s=30, cmap='viridis', alpha=0.7,
                                       vmin=rho_range[0], vmax=rho_range[1])
        ax_density.set_xlabel('X', fontsize=10)
        ax_density.set_ylabel('Y', fontsize=10)
        ax_density.set_title(f'Density Field (t = {time:.3f})', fontsize=12, fontweight='bold')
        ax_density.set_xlim(x_range)
        ax_density.set_ylim(y_range)
        ax_density.set_aspect('equal')
        ax_density.grid(True, alpha=0.3)
        
        if frame == 0:
            plt.colorbar(sc_density, ax=ax_density, label='Density', fraction=0.046, pad=0.04)
        
        # Pressure profile (spatial distribution)
        sc_pressure = ax_pressure.scatter(data['x'], data['y'], c=data['P'], 
                                         s=30, cmap='plasma', alpha=0.7,
                                         vmin=P_range[0], vmax=P_range[1])
        ax_pressure.set_xlabel('X', fontsize=10)
        ax_pressure.set_ylabel('Y', fontsize=10)
        ax_pressure.set_title(f'Pressure Field (t = {time:.3f})', fontsize=12, fontweight='bold')
        ax_pressure.set_xlim(x_range)
        ax_pressure.set_ylim(y_range)
        ax_pressure.set_aspect('equal')
        ax_pressure.grid(True, alpha=0.3)
        
        if frame == 0:
            plt.colorbar(sc_pressure, ax=ax_pressure, label='Pressure', fraction=0.046, pad=0.04)
        
        # Main title
        fig.suptitle(f'Hydrostatic Equilibrium - Frame {frame+1}/{len(snapshots)} - Time: {time:.3f}', 
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
        print(f"✓ 3D Animation saved: {OUTPUT_FILE}")
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
