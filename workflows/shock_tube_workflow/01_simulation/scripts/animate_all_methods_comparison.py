#!/usr/bin/env python3
"""
Create animated comparison of ALL SPH methods overlapped on one plot
Compares: baseline, modern, gsph, disph, ssph vs analytical solution
"""

import argparse
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
from shock_tube_animation_utils import SodShockTube, load_sph_data


def create_multi_method_animation(result_dirs, output_file, gamma=1.4):
    """Create animated comparison with all SPH methods overlapped
    
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
    
    # Use the first available method to determine frame count and domain
    first_method = list(available_methods.keys())[0]
    n_frames = len(available_methods[first_method])
    print(f"Creating multi-method comparison animation with {n_frames} frames")
    print(f"Methods: {', '.join(available_methods.keys())}")
    
    # Get domain from first method's first file
    _, x_sample, _, _, _, _ = load_sph_data(available_methods[first_method][0])
    x_min, x_max = x_sample.min(), x_sample.max()
    x_analytic = np.linspace(x_min, x_max, 1000)
    solver = SodShockTube(gamma=gamma)
    
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
    
    # Create figure with 2x2 subplots
    fig, axes = plt.subplots(2, 2, figsize=(16, 12))
    fig.suptitle('Shock Tube: All Methods Comparison vs Analytical', 
                 fontsize=16, fontweight='bold')
    
    # Analytical solution line
    line_analytic = []
    # SPH method scatters
    scatters_methods = {method: [] for method in available_methods}
    
    titles = ['Density', 'Velocity', 'Pressure', 'Energy']
    ylabels = ['ρ', 'v', 'P', 'e']
    
    for i, ax in enumerate(axes.flat):
        # Analytical line (black, thick)
        line, = ax.plot([], [], 'k-', linewidth=2.5, label='Analytical', 
                       alpha=0.9, zorder=10)
        line_analytic.append(line)
        
        # SPH methods (colored markers)
        for method in available_methods:
            scatter = ax.scatter([], [], s=40, alpha=0.6, 
                               label=method.upper(), 
                               marker=markers.get(method, 'o'),
                               edgecolors='none',
                               c=colors.get(method, 'gray'))
            scatters_methods[method].append(scatter)
        
        ax.set_xlabel('Position', fontsize=12)
        ax.set_ylabel(ylabels[i], fontsize=12)
        ax.set_title(titles[i], fontsize=13, fontweight='bold')
        ax.legend(loc='best', fontsize=9, ncol=2)
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.set_xlim(x_min, x_max)
    
    # Set y-axis limits
    axes[0, 0].set_ylim(0, 1.2)   # Density
    axes[0, 1].set_ylim(-0.1, 1.0)  # Velocity
    axes[1, 0].set_ylim(0, 1.2)   # Pressure
    axes[1, 1].set_ylim(0, 3.0)   # Energy
    
    time_text = fig.text(0.5, 0.96, '', ha='center', fontsize=14, fontweight='bold')
    
    def init():
        for line in line_analytic:
            line.set_data([], [])
        for method in available_methods:
            for scatter in scatters_methods[method]:
                scatter.set_offsets(np.empty((0, 2)))
        time_text.set_text('')
        return line_analytic + [s for method in available_methods 
                               for s in scatters_methods[method]] + [time_text]
    
    def animate(frame):
        # Load data for first available method to get time
        first_method = list(available_methods.keys())[0]
        t, _, _, _, _, _ = load_sph_data(available_methods[first_method][frame])
        
        # Update analytical solution
        rho_a, v_a, p_a, e_a = solver.shock_tube_solution(x_analytic, t)
        line_analytic[0].set_data(x_analytic, rho_a)
        line_analytic[1].set_data(x_analytic, v_a)
        line_analytic[2].set_data(x_analytic, p_a)
        line_analytic[3].set_data(x_analytic, e_a)
        
        # Update all SPH methods
        for method, files in available_methods.items():
            if frame < len(files):
                _, x, rho, v, p, e = load_sph_data(files[frame])
                scatters_methods[method][0].set_offsets(np.c_[x, rho])
                scatters_methods[method][1].set_offsets(np.c_[x, v])
                scatters_methods[method][2].set_offsets(np.c_[x, p])
                scatters_methods[method][3].set_offsets(np.c_[x, e])
        
        time_text.set_text(f't = {t:.5f}')
        return (line_analytic + 
                [s for method in available_methods for s in scatters_methods[method]] + 
                [time_text])
    
    print("Creating multi-method animation...")
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
        description='Animate comparison of all SPH methods overlapped')
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
    parser.add_argument('--output', default='all_methods_comparison.mp4',
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
    
    create_multi_method_animation(result_dirs, args.output, args.gamma)


if __name__ == '__main__':
    main()
