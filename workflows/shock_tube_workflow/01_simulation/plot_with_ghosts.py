#!/usr/bin/env python3
"""
Visualize SPH simulation with ghost particles shown in different color
"""
import numpy as np
import matplotlib.pyplot as plt
import glob
import os

def load_sph_data(filename):
    """Load SPH data file and separate real vs ghost particles"""
    data = np.loadtxt(filename, skiprows=1)
    
    # For 1D: columns are pos[0], vel[0], acc[0], mass, dens, pres, ene, sml, id, neighbor, alpha, gradh, type
    positions = data[:, 0]
    densities = data[:, 4]
    pressures = data[:, 5]
    particle_types = data[:, 12].astype(int)
    
    # Separate real (type=0) and ghost (type=1) particles
    real_mask = (particle_types == 0)
    ghost_mask = (particle_types == 1)
    
    return {
        'real_pos': positions[real_mask],
        'real_dens': densities[real_mask],
        'real_pres': pressures[real_mask],
        'ghost_pos': positions[ghost_mask],
        'ghost_dens': densities[ghost_mask],
        'ghost_pres': pressures[ghost_mask],
        'n_real': np.sum(real_mask),
        'n_ghost': np.sum(ghost_mask)
    }

def plot_snapshot(filename, output_file):
    """Create a plot showing real and ghost particles with different colors"""
    data = load_sph_data(filename)
    
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Plot density
    ax1.scatter(data['real_pos'], data['real_dens'], 
                c='blue', s=30, alpha=0.7, label=f'Real particles (n={data["n_real"]})')
    ax1.scatter(data['ghost_pos'], data['ghost_dens'], 
                c='red', s=30, alpha=0.7, marker='x', label=f'Ghost particles (n={data["n_ghost"]})')
    ax1.axvline(-0.5, color='gray', linestyle='--', alpha=0.5, label='Domain boundary')
    ax1.axvline(1.5, color='gray', linestyle='--', alpha=0.5)
    ax1.set_ylabel('Density', fontsize=12)
    ax1.set_title('Shock Tube Simulation: Real vs Ghost Particles', fontsize=14)
    ax1.legend()
    ax1.grid(alpha=0.3)
    
    # Plot pressure
    ax2.scatter(data['real_pos'], data['real_pres'], 
                c='blue', s=30, alpha=0.7, label=f'Real particles')
    ax2.scatter(data['ghost_pos'], data['ghost_pres'], 
                c='red', s=30, alpha=0.7, marker='x', label=f'Ghost particles')
    ax2.axvline(-0.5, color='gray', linestyle='--', alpha=0.5, label='Domain boundary')
    ax2.axvline(1.5, color='gray', linestyle='--', alpha=0.5)
    ax2.set_xlabel('Position', fontsize=12)
    ax2.set_ylabel('Pressure', fontsize=12)
    ax2.legend()
    ax2.grid(alpha=0.3)
    
    plt.tight_layout()
    plt.savefig(output_file, dpi=150)
    print(f"Saved: {output_file}")
    plt.close()

def main():
    """Process all output files"""
    results_dir = "results/gsph"
    plot_dir = "plots"
    
    if not os.path.exists(plot_dir):
        os.makedirs(plot_dir)
    
    # Get all .dat files
    data_files = sorted(glob.glob(f"{results_dir}/*.dat"))
    
    print(f"Found {len(data_files)} output files")
    
    # Plot a few representative snapshots
    for i, filename in enumerate(data_files[::5]):  # Every 5th file
        output_file = f"{plot_dir}/snapshot_{i:03d}.png"
        plot_snapshot(filename, output_file)
    
    print(f"\nPlots saved to {plot_dir}/")
    print("Ghost particles (red 'x') show boundary condition implementation")
    print("Real particles (blue dots) show the physical simulation domain")

if __name__ == "__main__":
    main()
