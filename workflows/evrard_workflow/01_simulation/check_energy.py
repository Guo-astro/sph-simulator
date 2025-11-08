#!/usr/bin/env python3
"""
Check energy conservation in Evrard collapse test.
Computes E_total = KE + IE + PE at each snapshot.
"""

import numpy as np
import glob
import os

def compute_total_energy(filepath):
    """
    Compute total energy from a CSV snapshot.
    E_total = sum(0.5*m*v^2) + sum(m*u) + 0.5*sum(m*phi)
    
    CSV format (from csv_writer.tpp):
    id,pos_x,pos_y,pos_z,vel_x,vel_y,vel_z,acc_x,acc_y,acc_z,
    mass,density,pressure,energy,smoothing_length,sound_speed,potential,neighbors
    """
    # Skip header lines starting with # AND the column name line
    with open(filepath, 'r') as f:
        lines = [line for line in f if not line.startswith('#')]
    
    # First non-comment line is column headers, skip it
    data = np.loadtxt(lines[1:], delimiter=',')
    
    # Extract columns (0-indexed)
    # id=0, pos=(1,2,3), vel=(4,5,6), acc=(7,8,9), 
    # mass=10, density=11, pressure=12, energy=13, 
    # sml=14, sound=15, potential=16, neighbors=17
    
    mass = data[:, 10]
    vel_x = data[:, 4]
    vel_y = data[:, 5]
    vel_z = data[:, 6]
    ene = data[:, 13]  # Specific internal energy u
    phi = data[:, 16]  # Gravitational potential
    
    # Kinetic energy
    v_squared = vel_x**2 + vel_y**2 + vel_z**2
    KE = np.sum(0.5 * mass * v_squared)
    
    # Internal energy
    IE = np.sum(mass * ene)
    
    # Gravitational potential energy
    # Factor of 0.5 to avoid double counting (each pair counted twice)
    PE = 0.5 * np.sum(mass * phi)
    
    E_total = KE + IE + PE
    
    return KE, IE, PE, E_total

def main():
    # Find all snapshot files
    snapshot_dir = "output/evrard_collapse/snapshots"
    if not os.path.exists(snapshot_dir):
        print(f"Error: Directory {snapshot_dir} not found")
        return
    
    files = sorted(glob.glob(f"{snapshot_dir}/*.csv"))
    
    if not files:
        print(f"No CSV files found in {snapshot_dir}")
        return
    
    print("Analyzing energy conservation:")
    print(f"{'File':<15} {'KE':>12} {'IE':>12} {'PE':>12} {'E_total':>12} {'ΔE/E0':>10}")
    print("=" * 85)
    
    E_initial = None
    
    for i, filepath in enumerate(files):
        try:
            KE, IE, PE, E_total = compute_total_energy(filepath)
            
            if E_initial is None:
                E_initial = E_total
                delta_frac = 0.0
            else:
                delta_frac = (E_total - E_initial) / E_initial * 100.0
            
            filename = os.path.basename(filepath)
            print(f"{filename:<15} {KE:>12.6f} {IE:>12.6f} {PE:>12.6f} {E_total:>12.6f} {delta_frac:>9.3f}%")
            
        except Exception as e:
            print(f"Error processing {filepath}: {e}")
    
    print("\nEnergy conservation summary:")
    print(f"  Initial energy E_0 = {E_initial:.6f}")
    if len(files) > 1:
        KE_f, IE_f, PE_f, E_final = compute_total_energy(files[-1])
        print(f"  Final energy   E_f = {E_final:.6f}")
        print(f"  Relative error ΔE/E_0 = {(E_final - E_initial)/E_initial * 100:.3f}%")
        
        if abs((E_final - E_initial)/E_initial) < 0.01:
            print("\n✓ Energy conserved to within 1% - GOOD!")
        else:
            print("\n✗ Energy NOT conserved - integration error")

if __name__ == "__main__":
    main()
