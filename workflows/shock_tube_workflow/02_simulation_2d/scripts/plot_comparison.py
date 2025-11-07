#!/usr/bin/env python3
"""
Generate static comparison plots for SSPH, GSPH and DISPH vs analytical solution (2D)
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
        
        # For 2D: Extract columns: x, y, density, velocity_x, velocity_y, pressure
        # We'll average along y-direction to get 1D profiles for comparison
        x = data[:, 0]
        y = data[:, 1]
        density = data[:, 2]
        velocity_x = data[:, 3]
        velocity_y = data[:, 4]
        pressure = data[:, 5]
        
        # Average quantities along y-direction by binning x positions
        x_bins = np.linspace(x.min(), x.max(), 100)
        x_centers = 0.5 * (x_bins[:-1] + x_bins[1:])
        
        density_avg = np.zeros_like(x_centers)
        velocity_avg = np.zeros_like(x_centers)
        pressure_avg = np.zeros_like(x_centers)
        
        for i in range(len(x_centers)):
            mask = (x >= x_bins[i]) & (x < x_bins[i+1])
            if np.any(mask):
                density_avg[i] = np.mean(density[mask])
                velocity_avg[i] = np.mean(velocity_x[mask])
                pressure_avg[i] = np.mean(pressure[mask])
        
        return x_centers, density_avg, velocity_avg, pressure_avg
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


def plot_comparison(ssph_dir, gsph_dir, disph_dir, output_file, gamma=1.4, time=0.3):
    """
    Generate comparison plot of SSPH, GSPH and DISPH vs analytical solution
    
    Args:
        ssph_dir: Directory containing SSPH results
        gsph_dir: Directory containing GSPH results
        disph_dir: Directory containing DISPH results
        output_file: Output plot filename
        gamma: Adiabatic index
        time: Simulation time for analytical solution
    """
    # Load final snapshots
    ssph_file = find_final_snapshot(ssph_dir)
    gsph_file = find_final_snapshot(gsph_dir)
    disph_file = find_final_snapshot(disph_dir)
    
    if not ssph_file and not gsph_file and not disph_file:
        print(f"Error: No snapshot files found in {ssph_dir}, {gsph_dir} or {disph_dir}")
        sys.exit(1)
    
    ssph_data = load_snapshot(ssph_file) if ssph_file else None
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
    fig.suptitle(f'2D Sod Shock Tube at t={time}s: SSPH vs GSPH vs DISPH vs Analytical', 
                 fontsize=14, fontweight='bold')
    
    # Plot density
    ax1.plot(x_analytical, density_analytical, 'k-', linewidth=2, label='Analytical', zorder=1)
    if ssph_data:
        x_ssph, density_ssph, _, _ = ssph_data
        ax1.plot(x_ssph, density_ssph, 'g-', linewidth=1.5, alpha=0.7, label='SSPH', zorder=2)
    if gsph_data:
        x_gsph, density_gsph, _, _ = gsph_data
        ax1.plot(x_gsph, density_gsph, 'b-', linewidth=1.5, alpha=0.7, label='GSPH', zorder=3)
    if disph_data:
        x_disph, density_disph, _, _ = disph_data
        ax1.plot(x_disph, density_disph, 'r-', linewidth=1.5, alpha=0.7, label='DISPH', zorder=4)
    
    ax1.set_ylabel('Density (kg/m³)', fontsize=11)
    ax1.set_xlim(0, 1)
    ax1.grid(True, alpha=0.3)
    ax1.legend(loc='best', fontsize=10)
    ax1.set_title('Density Comparison (averaged along y)', fontsize=11)
    
    # Plot velocity
    ax2.plot(x_analytical, velocity_analytical, 'k-', linewidth=2, label='Analytical', zorder=1)
    if ssph_data:
        x_ssph, _, velocity_ssph, _ = ssph_data
        ax2.plot(x_ssph, velocity_ssph, 'g-', linewidth=1.5, alpha=0.7, label='SSPH', zorder=2)
    if gsph_data:
        x_gsph, _, velocity_gsph, _ = gsph_data
        ax2.plot(x_gsph, velocity_gsph, 'b-', linewidth=1.5, alpha=0.7, label='GSPH', zorder=3)
    if disph_data:
        x_disph, _, velocity_disph, _ = disph_data
        ax2.plot(x_disph, velocity_disph, 'r-', linewidth=1.5, alpha=0.7, label='DISPH', zorder=4)
    
    ax2.set_ylabel('Velocity (m/s)', fontsize=11)
    ax2.set_xlim(0, 1)
    ax2.grid(True, alpha=0.3)
    ax2.legend(loc='best', fontsize=10)
    ax2.set_title('Velocity Comparison (averaged along y)', fontsize=11)
    
    # Plot pressure
    ax3.plot(x_analytical, pressure_analytical, 'k-', linewidth=2, label='Analytical', zorder=1)
    if ssph_data:
        x_ssph, _, _, pressure_ssph = ssph_data
        ax3.plot(x_ssph, pressure_ssph, 'g-', linewidth=1.5, alpha=0.7, label='SSPH', zorder=2)
    if gsph_data:
        x_gsph, _, _, pressure_gsph = gsph_data
        ax3.plot(x_gsph, pressure_gsph, 'b-', linewidth=1.5, alpha=0.7, label='GSPH', zorder=3)
    if disph_data:
        x_disph, _, _, pressure_disph = disph_data
        ax3.plot(x_disph, pressure_disph, 'r-', linewidth=1.5, alpha=0.7, label='DISPH', zorder=4)
    
    ax3.set_xlabel('Position (m)', fontsize=11)
    ax3.set_ylabel('Pressure (Pa)', fontsize=11)
    ax3.set_xlim(0, 1)
    ax3.grid(True, alpha=0.3)
    ax3.legend(loc='best', fontsize=10)
    ax3.set_title('Pressure Comparison (averaged along y)', fontsize=11)
    
    plt.tight_layout()
    
    # Save plot
    output_path = Path(output_file)
    if not output_path.is_absolute():
        output_path = Path('../results/analysis/plots') / output_path.name
        output_path.parent.mkdir(parents=True, exist_ok=True)
    
    print(f"Saving plot to {output_path}...")
    plt.savefig(str(output_path), dpi=150, bbox_inches='tight')
    print(f"✓ Plot saved: {output_path}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description='Generate comparison plots for 2D Sod shock tube simulations'
    )
    parser.add_argument('--ssph', default='../results/ssph',
                        help='SSPH results directory')
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
    
    plot_comparison(args.ssph, args.gsph, args.disph, args.output, args.gamma, args.time)


if __name__ == '__main__':
    main()
