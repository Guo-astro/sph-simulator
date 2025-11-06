#!/usr/bin/env python3
"""
Shared utilities for 3D shock tube animation scripts
For 3D simulations, we visualize slice views and statistical summaries
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path


def load_sph_data_3d(csv_file):
    """Load 3D SPH simulation data from CSV file
    
    Args:
        csv_file: Path to the CSV data file
    
    Returns:
        time, x, y, z, rho, vx, vy, vz, p, e: Simulation data arrays
    """
    import pandas as pd
    
    # Read CSV file
    df = pd.read_csv(csv_file)
    
    # Filter real particles (type == 0)
    df_real = df[df['type'] == 0]
    
    # Extract time from filename (snapshot number * timestep)
    snapshot_num = int(csv_file.stem)
    time = snapshot_num * 0.01  # Assuming 0.01 timestep
    
    # Map CSV columns to variables
    x = df_real['pos_x'].values
    y = df_real['pos_y'].values
    z = df_real['pos_z'].values
    vx = df_real['vel_x'].values
    vy = df_real['vel_y'].values
    vz = df_real['vel_z'].values
    rho = df_real['density'].values
    p = df_real['pressure'].values
    e = df_real['energy'].values
    
    return time, x, y, z, rho, vx, vy, vz, p, e


def create_animation_3d(sph_dir, output_file, gamma=1.4, method_name='SPH'):
    """Create animated visualization of 3D SPH simulation
    
    Shows slice views (xy, xz, yz planes) of the density field
    
    Args:
        sph_dir: Directory containing SPH output CSV files
        output_file: Output animation filename
        gamma: Adiabatic index (default 1.4)
        method_name: Name of SPH method for plot labels
    """
    sph_files = sorted([f for f in Path(sph_dir).glob("*.csv") 
                       if f.stem.isdigit() and 'energy' not in f.name])
    
    if len(sph_files) == 0:
        print(f"Error: No data files found in {sph_dir}")
        return
    
    n_frames = len(sph_files)
    print(f"Creating 3D {method_name} animation with {n_frames} frames")
    
    # Create figure with 3 slice views
    fig, axes = plt.subplots(1, 3, figsize=(18, 6))
    fig.suptitle(f'3D Shock Tube: {method_name} - Density Slices', 
                 fontsize=16, fontweight='bold')
    
    # Initialize scatter plots for each slice
    scatters = []
    for i, (ax, view, xlabel, ylabel) in enumerate([
        (axes[0], 'xy', 'x', 'y'),
        (axes[1], 'xz', 'x', 'z'),
        (axes[2], 'yz', 'y', 'z')
    ]):
        scatter = ax.scatter([], [], c=[], s=20, cmap='viridis',
                           vmin=0, vmax=1.2, alpha=0.8)
        scatters.append(scatter)
        
        ax.set_xlabel(xlabel, fontsize=12)
        ax.set_ylabel(ylabel, fontsize=12)
        ax.set_title(f'{view.upper()} Plane (mid-slice)', fontsize=12, fontweight='bold')
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.3, linestyle='--')
        
        # Set limits based on typical shock tube domain
        if 'x' in xlabel.lower():
            ax.set_xlim(-0.5, 1.5)
        else:
            ax.set_xlim(0, 0.5)
        
        if 'x' in ylabel.lower():
            ax.set_ylim(-0.5, 1.5)
        else:
            ax.set_ylim(0, 0.5)
    
    # Add colorbars
    for i, scatter in enumerate(scatters):
        cbar = plt.colorbar(scatter, ax=axes[i], label='Density')
    
    time_text = fig.text(0.5, 0.95, '', ha='center', fontsize=14, fontweight='bold')
    
    def init():
        for scatter in scatters:
            scatter.set_offsets(np.empty((0, 2)))
            scatter.set_array(np.array([]))
        time_text.set_text('')
        return scatters + [time_text]
    
    def animate(frame):
        t, x, y, z, rho, vx, vy, vz, p, e = load_sph_data_3d(sph_files[frame])
        
        # XY plane (mid z-slice)
        z_mid = (z.min() + z.max()) / 2
        mask_xy = np.abs(z - z_mid) < 0.05
        scatters[0].set_offsets(np.c_[x[mask_xy], y[mask_xy]])
        scatters[0].set_array(rho[mask_xy])
        
        # XZ plane (mid y-slice)
        y_mid = (y.min() + y.max()) / 2
        mask_xz = np.abs(y - y_mid) < 0.05
        scatters[1].set_offsets(np.c_[x[mask_xz], z[mask_xz]])
        scatters[1].set_array(rho[mask_xz])
        
        # YZ plane (mid x-slice)
        x_mid = (x.min() + x.max()) / 2
        mask_yz = np.abs(x - x_mid) < 0.1
        scatters[2].set_offsets(np.c_[y[mask_yz], z[mask_yz]])
        scatters[2].set_array(rho[mask_yz])
        
        time_text.set_text(f't = {t:.5f}')
        return scatters + [time_text]
    
    print("Creating 3D animation...")
    anim = animation.FuncAnimation(fig, animate, init_func=init,
                                   frames=n_frames, interval=100, blit=True)
    
    output_path = Path(output_file)
    if not output_path.is_absolute():
        output_path = Path('../results/analysis/animations') / output_path.name
        output_path.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Saving to {output_path}...")
    Writer = animation.writers['ffmpeg']
    writer = Writer(fps=10, bitrate=3000, codec='libx264')
    anim.save(str(output_path), writer=writer, dpi=150)
    print(f"Animation saved: {output_path}")
    plt.close()
