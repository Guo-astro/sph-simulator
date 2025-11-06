#!/usr/bin/env python3
"""
Create animated comparison of ALL 3D SPH methods overlapped
Shows slice views with all methods overlaid
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
from animate_3d_utils import load_sph_data_3d


def create_multi_method_animation_3d(result_dirs, output_file, gamma=1.4):
    """Create animated comparison with all 3D SPH methods overlapped on slices
    
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
    print(f"Creating 3D multi-method comparison animation with {n_frames} frames")
    print(f"Methods: {', '.join(available_methods.keys())}")
    
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
    
    # Create figure with 3 slice views
    fig, axes = plt.subplots(1, 3, figsize=(20, 6))
    fig.suptitle('3D Shock Tube: All Methods Comparison - Density Slices', 
                 fontsize=16, fontweight='bold')
    
    # Initialize scatter plots for each method on each slice
    scatters = {}
    for method in available_methods:
        scatters[method] = []
    
    for i, (ax, view, xlabel, ylabel) in enumerate([
        (axes[0], 'xy', 'x', 'y'),
        (axes[1], 'xz', 'x', 'z'),
        (axes[2], 'yz', 'y', 'z')
    ]):
        for method in available_methods:
            scatter = ax.scatter([], [], s=30, alpha=0.5,
                               label=method.upper(),
                               marker=markers.get(method, 'o'),
                               c=colors.get(method, 'gray'),
                               edgecolors='none')
            scatters[method].append(scatter)
        
        ax.set_xlabel(xlabel, fontsize=12)
        ax.set_ylabel(ylabel, fontsize=12)
        ax.set_title(f'{view.upper()} Plane (mid-slice)', fontsize=12, fontweight='bold')
        ax.set_aspect('equal')
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.legend(loc='upper right', fontsize=9)
        
        # Set limits
        if 'x' in xlabel.lower():
            ax.set_xlim(-0.5, 1.5)
        else:
            ax.set_xlim(0, 0.5)
        
        if 'x' in ylabel.lower():
            ax.set_ylim(-0.5, 1.5)
        else:
            ax.set_ylim(0, 0.5)
    
    time_text = fig.text(0.5, 0.96, '', ha='center', fontsize=14, fontweight='bold')
    
    def init():
        for method in available_methods:
            for scatter in scatters[method]:
                scatter.set_offsets(np.empty((0, 2)))
        time_text.set_text('')
        return [s for method in available_methods for s in scatters[method]] + [time_text]
    
    def animate(frame):
        # Get time from first method
        first_method = list(available_methods.keys())[0]
        t, _, _, _, _, _, _, _, _, _ = load_sph_data_3d(available_methods[first_method][frame])
        
        # Update all methods
        for method, files in available_methods.items():
            if frame < len(files):
                _, x, y, z, rho, vx, vy, vz, p, e = load_sph_data_3d(files[frame])
                
                # XY plane (mid z-slice)
                z_mid = (z.min() + z.max()) / 2
                mask_xy = np.abs(z - z_mid) < 0.05
                scatters[method][0].set_offsets(np.c_[x[mask_xy], y[mask_xy]])
                
                # XZ plane (mid y-slice)
                y_mid = (y.min() + y.max()) / 2
                mask_xz = np.abs(y - y_mid) < 0.05
                scatters[method][1].set_offsets(np.c_[x[mask_xz], z[mask_xz]])
                
                # YZ plane (mid x-slice)
                x_mid = (x.min() + x.max()) / 2
                mask_yz = np.abs(x - x_mid) < 0.1
                scatters[method][2].set_offsets(np.c_[y[mask_yz], z[mask_yz]])
        
        time_text.set_text(f't = {t:.5f}')
        return [s for method in available_methods for s in scatters[method]] + [time_text]
    
    print("Creating 3D multi-method animation...")
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
        description='Animate comparison of all 3D SPH methods overlapped')
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
    parser.add_argument('--output', default='all_methods_comparison_3d.mp4',
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
    
    create_multi_method_animation_3d(result_dirs, args.output, args.gamma)


if __name__ == '__main__':
    main()
