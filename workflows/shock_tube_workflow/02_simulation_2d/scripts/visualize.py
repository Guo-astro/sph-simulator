#!/usr/bin/env python3
"""
2D Shock Tube Visualization Script

Creates density, pressure, and velocity plots for 2D shock tube results.
Compares with 1D analytical solution along centerline.
"""

import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

def read_sph_data(filename):
    """Read SPH output file (CSV format)
    
    CSV columns: pos_x, pos_y, vel_x, vel_y, acc_x, acc_y, mass, density, pressure, 
                 energy, sound_speed, smoothing_length, gradh, balsara, alpha, 
                 potential, id, neighbors, type
    """
    import pandas as pd
    df = pd.read_csv(filename)
    
    # Filter real particles (type == 0)
    df_real = df[df['type'] == 0]
    
    return {
        'x': df_real['pos_x'].values,
        'y': df_real['pos_y'].values,
        'vx': df_real['vel_x'].values,
        'vy': df_real['vel_y'].values,
        'rho': df_real['density'].values,
        'p': df_real['pressure'].values,
        'e': df_real['energy'].values
    }

def plot_2d_field(x, y, field, title, output_file):
    """Create 2D scatter plot of a field"""
    plt.figure(figsize=(14, 4))
    # Adjust point size based on region (smaller in sparse right region)
    point_sizes = np.where(x < 0.5, 20, 5)
    scatter = plt.scatter(x, y, c=field, cmap='viridis', s=point_sizes, alpha=0.8)
    plt.colorbar(scatter, label=title)
    plt.xlabel('x')
    plt.ylabel('y')
    plt.title(f'{title} - 2D Distribution')
    plt.xlim(-0.5, 1.5)
    plt.ylim(0, 0.5)
    plt.gca().set_aspect('equal')  # Equal aspect ratio
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    plt.close()

def plot_centerline(x, y, field, title, output_file):
    """Plot field along centerline (y ≈ 0.25)"""
    # Find particles near centerline
    y_center = 0.25
    tol = 0.05
    mask = np.abs(y - y_center) < tol
    
    x_center = x[mask]
    field_center = field[mask]
    
    # Sort by x
    idx = np.argsort(x_center)
    x_sorted = x_center[idx]
    field_sorted = field_center[idx]
    
    plt.figure(figsize=(10, 6))
    plt.plot(x_sorted, field_sorted, 'o-', markersize=4, label='SPH')
    plt.xlabel('x')
    plt.ylabel(title)
    plt.title(f'{title} along centerline (y≈{y_center})')
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    plt.close()

def main():
    parser = argparse.ArgumentParser(description='Visualize 2D shock tube results')
    parser.add_argument('--results-dir', default='results/gsph',
                       help='Results directory')
    parser.add_argument('--time-step', type=int, default=-1,
                       help='Time step to visualize (-1 for latest)')
    args = parser.parse_args()
    
    results_dir = Path(args.results_dir)
    output_dir = results_dir / 'plots'
    output_dir.mkdir(exist_ok=True)
    
    # Find output files (00000.dat, 00001.dat, etc.)
    output_files = sorted([f for f in results_dir.glob('*.dat') if f.stem.isdigit()])
    if not output_files:
        print(f"No output files found in {results_dir}")
        return
    
    # Select file
    if args.time_step >= 0:
        target_file = results_dir / f'{args.time_step:05d}.dat'
        if not target_file.exists():
            print(f"File not found: {target_file}")
            return
        data_file = target_file
    else:
        data_file = output_files[-1]
    
    print(f"Visualizing: {data_file}")
    
    # Read data
    data = read_sph_data(data_file)
    
    # Create 2D plots
    plot_2d_field(data['x'], data['y'], data['rho'], 
                  'Density', output_dir / 'density_2d.png')
    plot_2d_field(data['x'], data['y'], data['p'], 
                  'Pressure', output_dir / 'pressure_2d.png')
    plot_2d_field(data['x'], data['y'], data['vx'], 
                  'Velocity (x)', output_dir / 'velocity_2d.png')
    
    # Create centerline plots
    plot_centerline(data['x'], data['y'], data['rho'],
                   'Density', output_dir / 'density_1d.png')
    plot_centerline(data['x'], data['y'], data['p'],
                   'Pressure', output_dir / 'pressure_1d.png')
    plot_centerline(data['x'], data['y'], data['vx'],
                   'Velocity', output_dir / 'velocity_1d.png')
    
    print(f"Plots saved to {output_dir}")

if __name__ == '__main__':
    main()
