#!/usr/bin/env python3
"""
Animation script for Hydrostatic equilibrium simulation (2D).
Generates visualization of the pressure and density profiles.
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
OUTPUT_FILE = os.path.join(ANIMATIONS_DIR, "hydrostatic.mp4")

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
    """Generate animation for hydrostatic equilibrium."""
    
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
            times.append(i * 0.02)  # Adjust based on actual output frequency
    
    if not snapshots:
        print("Error: Failed to load any snapshots")
        sys.exit(1)
    
    print(f"Loaded {len(snapshots)} valid snapshots")
    
    # Set up the figure
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(14, 12))
    
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
        ax3.clear()
        ax4.clear()
        
        data = snapshots[frame]
        time = times[frame]
        
        # Plot density field
        sc1 = ax1.scatter(data['x'], data['y'], c=data['rho'], 
                         s=10, cmap='viridis', alpha=0.7,
                         vmin=rho_range[0], vmax=rho_range[1])
        ax1.set_xlabel('x')
        ax1.set_ylabel('y')
        ax1.set_title(f'Density (t = {time:.3f})')
        ax1.set_xlim(x_range)
        ax1.set_ylim(y_range)
        ax1.set_aspect('equal')
        plt.colorbar(sc1, ax=ax1, label='Density')
        
        # Plot pressure field
        sc2 = ax2.scatter(data['x'], data['y'], c=data['P'], 
                         s=10, cmap='plasma', alpha=0.7,
                         vmin=P_range[0], vmax=P_range[1])
        ax2.set_xlabel('x')
        ax2.set_ylabel('y')
        ax2.set_title(f'Pressure (t = {time:.3f})')
        ax2.set_xlim(x_range)
        ax2.set_ylim(y_range)
        ax2.set_aspect('equal')
        plt.colorbar(sc2, ax=ax2, label='Pressure')
        
        # Plot density vs height (vertical profile)
        y_sorted_idx = np.argsort(data['y'])
        ax3.plot(data['y'][y_sorted_idx], data['rho'][y_sorted_idx], 'b-', alpha=0.5)
        ax3.set_xlabel('Height (y)')
        ax3.set_ylabel('Density')
        ax3.set_title(f'Density Profile (t = {time:.3f})')
        ax3.grid(True, alpha=0.3)
        
        # Plot pressure vs height (vertical profile)
        ax4.plot(data['y'][y_sorted_idx], data['P'][y_sorted_idx], 'r-', alpha=0.5)
        ax4.set_xlabel('Height (y)')
        ax4.set_ylabel('Pressure')
        ax4.set_title(f'Pressure Profile (t = {time:.3f})')
        ax4.grid(True, alpha=0.3)
        
        fig.suptitle(f'Hydrostatic Equilibrium - Frame {frame+1}/{len(snapshots)}', 
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
