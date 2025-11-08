#!/usr/bin/env python3
"""
Analyze whether particles in Evrard collapse are gravitationally bound or escaping.

A particle is bound if its specific orbital energy E = (1/2)*v^2 - G*M/r < 0
where for this simulation G=1, M=1 (total mass).

If E > 0, the particle has enough kinetic energy to escape to infinity.
"""

import pandas as pd
import numpy as np
import sys

def analyze_snapshot(filepath):
    """Analyze binding energy for all particles in a snapshot."""
    
    df = pd.read_csv(filepath, comment='#')
    
    # Extract time from header
    with open(filepath) as f:
        for line in f:
            if 'Time:' in line:
                time = float(line.split(':')[1].strip())
                break
    
    # Calculate quantities
    df['r'] = np.sqrt(df['pos_x']**2 + df['pos_y']**2 + df['pos_z']**2)
    df['v'] = np.sqrt(df['vel_x']**2 + df['vel_y']**2 + df['vel_z']**2)
    
    # Radial velocity (positive = outward)
    df['v_radial'] = (df['pos_x']*df['vel_x'] + df['pos_y']*df['vel_y'] + df['pos_z']*df['vel_z']) / df['r']
    
    # Specific orbital energy: E = (1/2)*v^2 - G*M/r
    # For this simulation: G = 1, M = 1
    G = 1.0
    M = 1.0
    df['E_specific'] = 0.5 * df['v']**2 - G * M / (df['r'] + 1e-10)
    
    # Total energy (kinetic + potential for all particles)
    E_kin_total = 0.5 * (df['mass'] * df['v']**2).sum()
    E_pot_total = -(G * df['mass'] / (df['r'] + 1e-10)).sum()  # Simplified (ignores particle-particle)
    E_total = E_kin_total + E_pot_total
    
    # Statistics
    escaping = (df['E_specific'] > 0).sum()
    bound = (df['E_specific'] < 0).sum()
    outward = (df['v_radial'] > 0).sum()
    
    print(f"\n{'='*60}")
    print(f"SNAPSHOT: {filepath}")
    print(f"TIME: t = {time:.4f}")
    print(f"{'='*60}")
    print(f"\nPARTICLE BINDING STATE:")
    print(f"  Gravitationally bound (E<0):   {bound:5d} / {len(df)} ({100*bound/len(df):.1f}%)")
    print(f"  Unbound/escaping (E>0):        {escaping:5d} / {len(df)} ({100*escaping/len(df):.1f}%)")
    print(f"  Moving outward (v_r>0):        {outward:5d} / {len(df)} ({100*outward/len(df):.1f}%)")
    
    print(f"\nENERGY STATISTICS:")
    print(f"  Max specific energy:     {df['E_specific'].max():10.4f}")
    print(f"  Min specific energy:     {df['E_specific'].min():10.4f}")
    print(f"  Total kinetic energy:    {E_kin_total:10.4f}")
    print(f"  Total potential energy:  {E_pot_total:10.4f}")
    print(f"  Total energy (approx):   {E_total:10.4f}")
    
    print(f"\nSPATIAL DISTRIBUTION:")
    print(f"  Max radius:              {df['r'].max():10.4f}")
    print(f"  Max velocity:            {df['v'].max():10.4f}")
    print(f"  Max density:             {df['density'].max():10.2f}")
    
    if escaping > 0:
        print(f"\n⚠️  WARNING: {escaping} particles are UNBOUND and will escape!")
        print(f"   This indicates a problem with energy conservation.")
        
        # Show worst offenders
        worst = df.nlargest(5, 'E_specific')[['id', 'r', 'v', 'E_specific']]
        print(f"\n   Top 5 most unbound particles:")
        for _, row in worst.iterrows():
            print(f"     ID {int(row['id']):4d}: r={row['r']:6.3f}, v={row['v']:6.3f}, E={row['E_specific']:8.4f}")
    else:
        print(f"\n✓ All particles are gravitationally bound (physically correct)")
        if outward > len(df) * 0.1:
            print(f"  {outward} particles moving outward is NORMAL during rebound phase")
    
    return {
        'time': time,
        'escaping': escaping,
        'bound': bound,
        'E_max': df['E_specific'].max(),
        'r_max': df['r'].max()
    }

if __name__ == '__main__':
    if len(sys.argv) < 2:
        print("Usage: python analyze_bound_state.py <snapshot.csv>")
        sys.exit(1)
    
    analyze_snapshot(sys.argv[1])
