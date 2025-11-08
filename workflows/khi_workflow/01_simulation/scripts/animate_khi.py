#!/usr/bin/env python3
"""
Animation script for Kelvin-Helmholtz Instability (KHI) simulation (2D).
Generates visualization of the shear layer instability development.
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
OUTPUT_FILE = os.path.join(ANIMATIONS_DIR, "khi.mp4")

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
    """Generate animation for Kelvin-Helmholtz instability."""
    
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
    fig, ((ax1, ax2), (ax3, ax4)) = plt.subplots(2, 2, figsize=(16, 12))
    
    # Find global min/max for consistent scaling
    all_x = np.concatenate([s['x'] for s in snapshots])
    all_y = np.concatenate([s['y'] for s in snapshots])
    all_rho = np.concatenate([s['rho'] for s in snapshots])
    all_vy = np.concatenate([s['vy'] for s in snapshots])
    
    x_range = [all_x.min() * 1.05, all_x.max() * 1.05]
    y_range = [all_y.min() * 1.05, all_y.max() * 1.05]
    rho_range = [all_rho.min(), all_rho.max()]
    vy_range = [all_vy.min(), all_vy.max()]
    
    def update(frame):
        """Update function for animation."""
        ax1.clear()
        ax2.clear()
        ax3.clear()
        ax4.clear()
        
        data = snapshots[frame]
        time = times[frame]
        
        # Calculate vorticity (curl of velocity)
        # Simplified as vy gradient - for visualization
        vorticity = np.abs(data['vy'])
        
        # Plot density field
        sc1 = ax1.scatter(data['x'], data['y'], c=data['rho'], 
                         s=5, cmap='viridis', alpha=0.8,
                         vmin=rho_range[0], vmax=rho_range[1])
        ax1.set_xlabel('x')
        ax1.set_ylabel('y')
        ax1.set_title(f'Density (t = {time:.3f})')
        ax1.set_xlim(x_range)
        ax1.set_ylim(y_range)
        ax1.set_aspect('equal')
        plt.colorbar(sc1, ax=ax1, label='Density')
        
        # Plot vertical velocity (shear visualization)
        sc2 = ax2.scatter(data['x'], data['y'], c=data['vy'], 
                         s=5, cmap='RdBu_r', alpha=0.8,
                         vmin=vy_range[0], vmax=vy_range[1])
        ax2.set_xlabel('x')
        ax2.set_ylabel('y')
        ax2.set_title(f'Vertical Velocity (t = {time:.3f})')
        ax2.set_xlim(x_range)
        ax2.set_ylim(y_range)
        ax2.set_aspect('equal')
        plt.colorbar(sc2, ax=ax2, label='vy')
        
        # Plot vorticity proxy
        sc3 = ax3.scatter(data['x'], data['y'], c=vorticity, 
                         s=5, cmap='hot', alpha=0.8)
        ax3.set_xlabel('x')
        ax3.set_ylabel('y')
        ax3.set_title(f'Vorticity Proxy (t = {time:.3f})')
        ax3.set_xlim(x_range)
        ax3.set_ylim(y_range)
        ax3.set_aspect('equal')
        plt.colorbar(sc3, ax=ax3, label='|vy|')
        
        # Plot velocity field (quiver)
        skip = max(1, len(data['x']) // 1000)
        v_mag = np.sqrt(data['vx']**2 + data['vy']**2)
        ax4.quiver(data['x'][::skip], data['y'][::skip], 
                  data['vx'][::skip], data['vy'][::skip],
                  v_mag[::skip], cmap='coolwarm', alpha=0.7)
        ax4.set_xlabel('x')
        ax4.set_ylabel('y')
        ax4.set_title(f'Velocity Field (t = {time:.3f})')
        ax4.set_xlim(x_range)
        ax4.set_ylim(y_range)
        ax4.set_aspect('equal')
        
        fig.suptitle(f'Kelvin-Helmholtz Instability - Frame {frame+1}/{len(snapshots)}', 
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
