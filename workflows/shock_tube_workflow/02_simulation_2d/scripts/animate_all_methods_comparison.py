#!/usr/bin/env python3
"""
Create animated comparison of ALL 2D SPH methods overlapped on one plot
Compares: baseline, modern, gsph, disph, ssph
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
from animate_2d import SodShockTube, read_sph_data


def create_multi_method_animation_2d(result_dirs, output_file, gamma=1.4):
    """Create animated comparison with all 2D SPH methods overlapped
    
    Args:
        result_dirs: Dict mapping method names to result directories
        output_file: Output animation filename
        gamma: Adiabatic index (default 1.4)
    """
    # Filter out methods that don't have data
    available_methods = {}
    for method, dir_path in result_dirs.items():
        sph_files = sorted([f for f in Path(dir_path).glob("*.dat") if f.name != 'energy.dat'])
        if len(sph_files) > 0:
            available_methods[method] = sph_files
        else:
            print(f"⚠ No data found for {method} in {dir_path} (skipping)")
    
    if len(available_methods) == 0:
        print("Error: No data files found for any method")
        return
    
    # Use the first available method to determine frame count
    first_method = list(available_methods.keys())[0]
    n_frames = len(available_methods[first_method])
    print(f"Creating 2D multi-method comparison animation with {n_frames} frames")
    print(f"Methods: {', '.join(available_methods.keys())}")
    
    # Get domain from first method's first file
    sample_data = read_sph_data(available_methods[first_method][0])
    x_sample = sample_data['x']
    y_sample = sample_data['y']
    
    # Color scheme for methods
    colors = {
        'baseline': '#1f77b4',  # Blue
        'modern': '#ff7f0e',    # Orange
        'gsph': '#2ca02c',      # Green
        'disph': '#d62728',     # Red
        'ssph': '#9467bd',      # Purple
    }
    
    markers = {
        'baseline': 'o',
        'modern': 's',
        'gsph': '^',
        'disph': 'v',
        'ssph': 'D',
    }
    
    # Create figure with 2D density field + centerline comparison
    fig = plt.figure(figsize=(16, 6))
    gs = fig.add_gridspec(1, 2, width_ratios=[1, 1.2])
    
    # Left: 2D density fields (stacked vertically for each method)
    ax_2d = fig.add_subplot(gs[0])
    
    # Right: Centerline density comparison
    ax_1d = fig.add_subplot(gs[1])
    
    fig.suptitle('2D Shock Tube: All Methods Comparison', 
                 fontsize=16, fontweight='bold')
    
    # Prepare analytical solution
    x_analytic = np.linspace(-0.5, 1.5, 1000)
    solver = SodShockTube(gamma=gamma)
    line_analytic, = ax_1d.plot([], [], 'k-', linewidth=2.5, label='Analytical (1D)', 
                                alpha=0.9, zorder=10)
    
    # SPH method scatters for centerline
    scatters_1d = {}
    for method in available_methods:
        scatter = ax_1d.scatter([], [], s=40, alpha=0.6, 
                               label=method.upper(), 
                               marker=markers.get(method, 'o'),
                               edgecolors='none',
                               c=colors.get(method, 'gray'))
        scatters_1d[method] = scatter
    
    ax_1d.set_xlabel('x', fontsize=12)
    ax_1d.set_ylabel('Density', fontsize=12)
    ax_1d.set_title('Centerline Density (y ≈ 0.25)', fontsize=13, fontweight='bold')
    ax_1d.legend(loc='best', fontsize=9, ncol=2)
    ax_1d.grid(True, alpha=0.3, linestyle='--')
    ax_1d.set_xlim(-0.5, 1.5)
    ax_1d.set_ylim(0, 1.2)
    
    # 2D density field - will show the first available method
    scatter_2d = ax_2d.scatter([], [], c=[], s=20, cmap='viridis', 
                              vmin=0, vmax=1.2, alpha=0.8)
    ax_2d.set_xlabel('x', fontsize=12)
    ax_2d.set_ylabel('y', fontsize=12)
    ax_2d.set_title('2D Density Field (First Method)', fontsize=13, fontweight='bold')
    ax_2d.set_aspect('equal')  # Equal aspect ratio for proper spacing visualization
    ax_2d.set_xlim(-0.5, 1.5)
    ax_2d.set_ylim(0, 0.5)
    
    cbar = plt.colorbar(scatter_2d, ax=ax_2d, label='Density')
    
    time_text = fig.text(0.5, 0.96, '', ha='center', fontsize=14, fontweight='bold')
    
    def init():
        line_analytic.set_data([], [])
        for scatter in scatters_1d.values():
            scatter.set_offsets(np.empty((0, 2)))
        scatter_2d.set_offsets(np.empty((0, 2)))
        scatter_2d.set_array(np.array([]))
        time_text.set_text('')
        return [line_analytic, scatter_2d] + list(scatters_1d.values()) + [time_text]
    
    def animate(frame):
        # Load data for first available method to get time (estimate from frame)
        first_method = list(available_methods.keys())[0]
        data_2d = read_sph_data(available_methods[first_method][frame])
        t = frame * 0.01  # Estimate time (assuming 0.01 time interval)
        
        # Update 2D density field (first method) with adaptive point sizes
        point_sizes = np.where(data_2d['x'] < 0.5, 20, 5)  # Smaller points in sparse right region
        scatter_2d.set_offsets(np.c_[data_2d['x'], data_2d['y']])
        scatter_2d.set_array(data_2d['rho'])
        scatter_2d.set_sizes(point_sizes)
        
        # Update analytical solution (1D)
        rho_a, _, _, _ = solver.shock_tube_solution(x_analytic, t)
        line_analytic.set_data(x_analytic, rho_a)
        
        # Update all SPH methods on centerline
        for method, files in available_methods.items():
            if frame < len(files):
                data = read_sph_data(files[frame])
                
                # Extract centerline particles (y ≈ 0.25)
                centerline_mask = np.abs(data['y'] - 0.25) < 0.05
                x_centerline = data['x'][centerline_mask]
                rho_centerline = data['rho'][centerline_mask]
                
                scatters_1d[method].set_offsets(np.c_[x_centerline, rho_centerline])
        
        time_text.set_text(f't = {t:.5f}')
        return [line_analytic, scatter_2d] + list(scatters_1d.values()) + [time_text]
    
    print("Creating 2D multi-method animation...")
    anim = animation.FuncAnimation(fig, animate, init_func=init,
                                   frames=n_frames, interval=100, blit=True)
    
    output_path = Path(output_file)
    if not output_path.is_absolute():
        output_path = Path('../results/analysis/animations') / output_path.name
        output_path.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Saving to {output_path}...")
    Writer = animation.writers['ffmpeg']
    writer = Writer(fps=10, bitrate=4000, codec='libx264')
    anim.save(str(output_path), writer=writer, dpi=150)
    print(f"Animation saved: {output_path}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Animate comparison of all 2D SPH methods overlapped')
    parser.add_argument('--baseline', default='../results/baseline',
                        help='Baseline output directory')
    parser.add_argument('--modern', default='../results/modern',
                        help='Modern output directory')
    parser.add_argument('--gsph', default='../results/gsph',
                        help='GSPH output directory')
    parser.add_argument('--disph', default='../results/disph',
                        help='DISPH output directory')
    parser.add_argument('--ssph', default='../results/ssph',
                        help='SSPH output directory')
    parser.add_argument('--output', default='all_methods_comparison_2d.mp4',
                        help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index')
    args = parser.parse_args()
    
    result_dirs = {
        'baseline': args.baseline,
        'modern': args.modern,
        'gsph': args.gsph,
        'disph': args.disph,
        'ssph': args.ssph,
    }
    
    create_multi_method_animation_2d(result_dirs, args.output, args.gamma)


if __name__ == '__main__':
    main()
