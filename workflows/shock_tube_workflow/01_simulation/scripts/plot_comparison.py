#!/usr/bin/env python3
"""
Generate static comparison plots for GSPH and DISPH vs analytical solution
"""

import argparse
import sys
from pathlib import Path
import numpy as np
import matplotlib.pyplot as plt

# Import the analytical solution from animation utils
from shock_tube_animation_utils import SodShockTube


def load_snapshot(snapshot_file):
    """Load particle data from a snapshot CSV file"""
    try:
        data = np.loadtxt(snapshot_file, delimiter=',', skiprows=1)
        if data.size == 0:
            return None
        
        # Extract columns: x, density, velocity_x, pressure
        x = data[:, 0]
        density = data[:, 1]
        velocity_x = data[:, 2]
        pressure = data[:, 3]
        
        return x, density, velocity_x, pressure
    except Exception as e:
        print(f"Warning: Could not load {snapshot_file}: {e}")
        return None


def find_final_snapshot(result_dir):
    """Find the final snapshot file in the results directory"""
    result_path = Path(result_dir)
    if not result_path.exists():
        return None
    
    # Look for snapshot files (format: snapshot_NNNN.csv or just numbered files)
    snapshot_files = sorted(result_path.glob('*.csv'))
    if not snapshot_files:
        return None
    
    # Filter out energy.csv if present
    snapshot_files = [f for f in snapshot_files if 'energy' not in f.name.lower()]
    
    if not snapshot_files:
        return None
    
    # Return the last snapshot (highest number)
    return snapshot_files[-1]


def plot_comparison(gsph_dir, disph_dir, output_file, gamma=1.4, time=0.3):
    """
    Generate comparison plot of GSPH and DISPH vs analytical solution
    
    Args:
        gsph_dir: Directory containing GSPH results
        disph_dir: Directory containing DISPH results
        output_file: Output plot filename
        gamma: Adiabatic index
        time: Simulation time for analytical solution
    """
    # Load final snapshots
    gsph_file = find_final_snapshot(gsph_dir)
    disph_file = find_final_snapshot(disph_dir)
    
    if not gsph_file and not disph_file:
        print(f"Error: No snapshot files found in {gsph_dir} or {disph_dir}")
        sys.exit(1)
    
    gsph_data = load_snapshot(gsph_file) if gsph_file else None
    disph_data = load_snapshot(disph_file) if disph_file else None
    
    # Get analytical solution
    solver = SodShockTube(gamma=gamma)
    x_analytical = np.linspace(0, 1, 500)
    density_analytical, velocity_analytical, pressure_analytical, _ = solver.shock_tube_solution(
        x_analytical, time
    )
    
    # Create figure with 3 subplots
    fig, (ax1, ax2, ax3) = plt.subplots(3, 1, figsize=(10, 10))
    fig.suptitle(f'Sod Shock Tube at t={time}s: GSPH vs DISPH vs Analytical', fontsize=14, fontweight='bold')
    
    # Plot density
    ax1.plot(x_analytical, density_analytical, 'k-', linewidth=2, label='Analytical', zorder=1)
    if gsph_data:
        x_gsph, density_gsph, _, _ = gsph_data
        ax1.scatter(x_gsph, density_gsph, c='blue', s=20, alpha=0.6, label='GSPH', zorder=2)
    if disph_data:
        x_disph, density_disph, _, _ = disph_data
        ax1.scatter(x_disph, density_disph, c='red', s=20, alpha=0.6, label='DISPH', zorder=3)
    
    ax1.set_ylabel('Density (kg/m³)', fontsize=11)
    ax1.set_xlim(0, 1)
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc='best', fontsize=10)
    ax1.set_title('Density Comparison', fontsize=11)
    
    # Plot velocity
    ax2.plot(x_analytical, velocity_analytical, 'k-', linewidth=2, label='Analytical', zorder=1)
    if gsph_data:
        x_gsph, _, velocity_gsph, _ = gsph_data
        ax2.scatter(x_gsph, velocity_gsph, c='blue', s=20, alpha=0.6, label='GSPH', zorder=2)
    if disph_data:
        x_disph, _, velocity_disph, _ = disph_data
        ax2.scatter(x_disph, velocity_disph, c='red', s=20, alpha=0.6, label='DISPH', zorder=3)
    
    ax2.set_ylabel('Velocity (m/s)', fontsize=11)
    ax2.set_xlim(0, 1)
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='best', fontsize=10)
    ax2.set_title('Velocity Comparison', fontsize=11)
    
    # Plot pressure
    ax3.plot(x_analytical, pressure_analytical, 'k-', linewidth=2, label='Analytical', zorder=1)
    if gsph_data:
        x_gsph, _, _, pressure_gsph = gsph_data
        ax3.scatter(x_gsph, pressure_gsph, c='blue', s=20, alpha=0.6, label='GSPH', zorder=2)
    if disph_data:
        x_disph, _, _, pressure_disph = disph_data
        ax3.scatter(x_disph, pressure_disph, c='red', s=20, alpha=0.6, label='DISPH', zorder=3)
    
    ax3.set_xlabel('Position (m)', fontsize=11)
    ax3.set_ylabel('Pressure (Pa)', fontsize=11)
    ax3.set_xlim(0, 1)
    ax3.grid(True, alpha=0.3)
    ax3.legend(loc='best', fontsize=10)
    ax3.set_title('Pressure Comparison', fontsize=11)
    
    plt.tight_layout()
    
    # Save plot
    output_path = Path(output_file)
    if not output_path.is_absolute():
        output_path = Path('../results/analysis/plots') / output_path.name
        output_path.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Saving plot to {output_path}...")
    plt.savefig(str(output_path), dpi=150, bbox_inches='tight')
    print(f"Plot saved: {output_path}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Generate comparison plots for Sod shock tube simulations'
    )
    parser.add_argument('--gsph', default='../results/gsph',
                        help='GSPH results directory')
    parser.add_argument('--disph', default='../results/disph',
                        help='DISPH results directory')
    parser.add_argument('--output', default='comparison_plot.png',
                        help='Output plot filename')
    parser.add_argument('--gamma', type=float, default=1.4,
                        help='Adiabatic index (default: 1.4)')
    parser.add_argument('--time', type=float, default=0.3,
                        help='Simulation time for analytical solution (default: 0.3)')
    
    args = parser.parse_args()
    
    plot_comparison(args.gsph, args.disph, args.output, args.gamma, args.time)


if __name__ == '__main__':
    main()
