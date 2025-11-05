#!/usr/bin/env python3
"""
Diagnostic script to analyze real and ghost particles near boundaries
"""

import numpy as np
import matplotlib.pyplot as plt


def load_all_particles(dat_file):
    """Load ALL particles including ghosts from .dat file
    
    Returns:
        time, real_data, ghost_data
    """
    data = np.loadtxt(dat_file, comments='#')
    with open(dat_file, 'r') as f:
        time = float(f.readline().replace('#', '').strip())
    
    # Column indices: 0=x, 1=v, 4=rho, 5=p, 6=e, 7=mass, 8=sml, -1=type
    is_ghost = data[:, -1] == 1
    
    real_data = data[~is_ghost]
    ghost_data = data[is_ghost]
    
    return time, real_data, ghost_data


def analyze_boundary_region(real_data, ghost_data, boundary_x, side='left', window=0.1):
    """Analyze particles near a boundary
    
    Args:
        real_data: Real particle data
        ghost_data: Ghost particle data  
        boundary_x: Boundary position
        side: 'left' or 'right'
        window: Distance from boundary to include
    """
    print(f"\n{'='*80}")
    print(f"ANALYZING {side.upper()} BOUNDARY (x = {boundary_x})")
    print(f"{'='*80}")
    
    # Column indices for 1D: 0=x, 1=vx, 2=ax, 3=mass, 4=dens, 5=pres, 6=ene, 7=sml, 8=id, 9=neighbor, 10=alpha, 11=gradh, 12=type
    
    # Extract positions
    real_x = real_data[:, 0]
    ghost_x = ghost_data[:, 0]
    
    # Find particles within window of boundary
    if side == 'left':
        real_near = real_data[(real_x >= boundary_x) & (real_x <= boundary_x + window)]
        ghost_near = ghost_data[(ghost_x >= boundary_x - window) & (ghost_x <= boundary_x)]
    else:  # right
        real_near = real_data[(real_x <= boundary_x) & (real_x >= boundary_x - window)]
        ghost_near = ghost_data[(ghost_x <= boundary_x + window) & (ghost_x >= boundary_x)]
    
    print(f"\nReal particles near boundary: {len(real_near)}")
    print(f"Ghost particles near boundary: {len(ghost_near)}")
    
    # Sort by position
    real_near = real_near[np.argsort(real_near[:, 0])]
    ghost_near = ghost_near[np.argsort(ghost_near[:, 0])]
    
    print(f"\n{'-'*80}")
    print(f"REAL PARTICLES (closest {min(10, len(real_near))} to boundary):")
    print(f"{'Idx':<4} {'x':>10} {'v':>10} {'rho':>10} {'p':>10} {'sml':>10} {'dist_to_bnd':>12}")
    print(f"{'-'*80}")
    
    for i, p in enumerate(real_near[:10]):
        x, v, rho, pres, sml = p[0], p[1], p[4], p[5], p[7]  # Fixed: sml is index 7
        dist = abs(x - boundary_x)
        print(f"{i:<4} {x:>10.6f} {v:>10.6f} {rho:>10.6f} {pres:>10.6f} {sml:>10.6f} {dist:>12.6f}")
    
    print(f"\n{'-'*80}")
    print(f"GHOST PARTICLES (closest {min(10, len(ghost_near))} to boundary):")
    print(f"{'Idx':<4} {'x':>10} {'v':>10} {'rho':>10} {'p':>10} {'sml':>10} {'dist_to_bnd':>12}")
    print(f"{'-'*80}")
    
    for i, p in enumerate(ghost_near[:10] if side == 'left' else ghost_near[-10:]):
        x, v, rho, pres, sml = p[0], p[1], p[4], p[5], p[7]  # Fixed: sml is index 7
        dist = abs(x - boundary_x)
        print(f"{i:<4} {x:>10.6f} {v:>10.6f} {rho:>10.6f} {pres:>10.6f} {sml:>10.6f} {dist:>12.6f}")
    
    # Check if ghosts mirror real particles correctly
    print(f"\n{'-'*80}")
    print(f"GHOST-REAL PAIRING ANALYSIS:")
    print(f"{'-'*80}")
    
    # For each ghost, find its mirrored real particle
    if len(ghost_near) > 0 and len(real_near) > 0:
        print(f"Checking if ghosts are correct mirrors of real particles...")
        for i, ghost in enumerate(ghost_near[:5]):
            ghost_x = ghost[0]
            # Expected real particle position: 2*boundary - ghost_x
            expected_real_x = 2 * boundary_x - ghost_x
            
            # Find closest real particle
            closest_idx = np.argmin(np.abs(real_near[:, 0] - expected_real_x))
            real_p = real_near[closest_idx]
            real_x = real_p[0]
            
            x_error = abs(real_x - expected_real_x)
            rho_match = abs(ghost[4] - real_p[4]) < 1e-6
            v_sign = (ghost[1] * real_p[1]) < 0 if (abs(ghost[1]) > 1e-10 or abs(real_p[1]) > 1e-10) else True
            
            print(f"\nGhost #{i}:")
            print(f"  Ghost pos: {ghost_x:>10.6f}")
            print(f"  Expected real pos: {expected_real_x:>10.6f}")
            print(f"  Actual real pos: {real_x:>10.6f}")
            print(f"  Position error: {x_error:>10.6f}")
            print(f"  Ghost rho: {ghost[4]:>10.6f}, Real rho: {real_p[4]:>10.6f} {'✓' if rho_match else '✗'}")
            print(f"  Ghost v: {ghost[1]:>10.6f}, Real v: {real_p[1]:>10.6f} {'✓' if v_sign else '✗'}")
            print(f"  Ghost sml: {ghost[7]:>10.6f}, Real sml: {real_p[7]:>10.6f}")
            
            # Check kernel support overlap
            kernel_support = 2.0 * real_p[7]  # 2*sml for cubic spline
            distance_between = abs(ghost_x - real_x)
            print(f"  Kernel support (2*sml): {kernel_support:>10.6f}")
            print(f"  Distance between ghost and real: {distance_between:>10.6f}")
            print(f"  Ghost within real's kernel? {'YES' if distance_between < kernel_support else 'NO'}")
    
    return real_near, ghost_near


def visualize_boundary(real_near, ghost_near, boundary_x, side, time):
    """Create visualization of particles near boundary"""
    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 8))
    
    # Position diagram
    ax1.axvline(boundary_x, color='red', linestyle='--', linewidth=2, label='Boundary', alpha=0.7)
    ax1.scatter(real_near[:, 0], real_near[:, 4], s=50, alpha=0.7, label='Real particles', marker='o')
    ax1.scatter(ghost_near[:, 0], ghost_near[:, 4], s=50, alpha=0.7, label='Ghost particles', marker='s')
    
    ax1.set_xlabel('Position x')
    ax1.set_ylabel('Density ρ')
    ax1.set_title(f'{side.capitalize()} Boundary - Particle Positions (t={time:.6f})')
    ax1.legend()
    ax1.grid(True, alpha=0.3)
    
    # Kernel support visualization
    ax2.axvline(boundary_x, color='red', linestyle='--', linewidth=2, label='Boundary', alpha=0.7)
    
    # Plot kernel support for first few real particles
    for i, p in enumerate(real_near[:5]):
        x, sml = p[0], p[7]  # Fixed: sml is index 7
        kernel_support = 2.0 * sml
        ax2.plot([x - kernel_support, x + kernel_support], [i, i], 'o-', alpha=0.6, label=f'Real p{i} support')
        ax2.scatter([x], [i], s=100, marker='o', zorder=3)
    
    # Plot ghost positions
    for i, p in enumerate(ghost_near[:5]):
        x = p[0]
        offset = len(real_near[:5]) + i
        ax2.scatter([x], [offset], s=100, marker='s', zorder=3, label=f'Ghost p{i}')
    
    ax2.set_xlabel('Position x')
    ax2.set_ylabel('Particle index')
    ax2.set_title('Kernel Support Regions')
    ax2.legend(bbox_to_anchor=(1.05, 1), loc='upper left')
    ax2.grid(True, alpha=0.3)
    
    plt.tight_layout()
    return fig


def main():
    import sys
    
    if len(sys.argv) > 1:
        dat_file = sys.argv[1]
    else:
        dat_file = '../output/shock_tube_enhanced/00000.dat'
    
    print(f"Analyzing: {dat_file}")
    
    time, real_data, ghost_data = load_all_particles(dat_file)
    
    print(f"\nTime: {time}")
    print(f"Total real particles: {len(real_data)}")
    print(f"Total ghost particles: {len(ghost_data)}")
    
    # Analyze left boundary
    left_real, left_ghost = analyze_boundary_region(
        real_data, ghost_data, -0.5, 'left', window=0.1
    )
    
    # Analyze right boundary
    right_real, right_ghost = analyze_boundary_region(
        real_data, ghost_data, 1.5, 'right', window=0.1
    )
    
    # Create visualizations
    fig1 = visualize_boundary(left_real, left_ghost, -0.5, 'left', time)
    plt.savefig('../results/analysis/boundary_diagnosis_left.png', dpi=150, bbox_inches='tight')
    print(f"\nSaved: ../results/analysis/boundary_diagnosis_left.png")
    
    fig2 = visualize_boundary(right_real, right_ghost, 1.5, 'right', time)
    plt.savefig('../results/analysis/boundary_diagnosis_right.png', dpi=150, bbox_inches='tight')
    print(f"\nSaved: ../results/analysis/boundary_diagnosis_right.png")
    
    plt.show()


if __name__ == '__main__':
    main()
