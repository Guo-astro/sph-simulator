#!/usr/bin/env python3
"""
Create animated visualization of 2D shock tube simulation

Shows:
1. 2D density field evolution
2. Centerline comparison (y ≈ 0.25) with analytical 1D solution
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
import argparse
from scipy.optimize import fsolve


class SodShockTube:
    """Analytical solution for Sod shock tube problem (1D)
    
    Standard Sod shock tube initial conditions:
    - Left state (x < 0.5):  ρ=1.0, P=1.0, u=0.0
    - Right state (x ≥ 0.5): ρ=0.125, P=0.1, u=0.0
    - Discontinuity at x=0.5
    - Domain: [0, 1.0]
    
    For SPH comparison:
    - Use SPH-computed initial densities (from particle spacing)
    - Pressures are prescribed initial conditions
    """
    
    def __init__(self, gamma=1.4):
        self.gamma = gamma
        self.rho_L = 1.0
        self.p_L = 1.0
        self.u_L = 0.0
        self.rho_R = 0.125  # Standard Sod (will be overridden by SPH-computed value)
        self.p_R = 0.1
        self.u_R = 0.0
        self.x_discontinuity = 0.5  # Standard Sod discontinuity position
    
    def sound_speed(self, p, rho):
        return np.sqrt(self.gamma * p / rho)
    
    def shock_tube_solution(self, x, t):
        """Compute analytical solution at positions x and time t"""
        if t <= 0:
            density = np.where(x < self.x_discontinuity, self.rho_L, self.rho_R)
            velocity = np.where(x < self.x_discontinuity, self.u_L, self.u_R)
            pressure = np.where(x < self.x_discontinuity, self.p_L, self.p_R)
        else:
            p_star, u_star, rho_3, rho_4 = self._solve_riemann()
            
            c_L = self.sound_speed(self.p_L, self.rho_L)
            c_R = self.sound_speed(self.p_R, self.rho_R)
            c_3 = self.sound_speed(p_star, rho_3)
            
            shock_speed = self.u_R + c_R * np.sqrt((self.gamma + 1) / (2 * self.gamma) *
                                                     (p_star / self.p_R) +
                                                     (self.gamma - 1) / (2 * self.gamma))
            head_speed = self.u_L - c_L
            tail_speed = u_star - c_3
            contact_speed = u_star
            
            xi = (x - self.x_discontinuity) / t
            
            density = np.zeros_like(x)
            velocity = np.zeros_like(x)
            pressure = np.zeros_like(x)
            
            mask1 = xi < head_speed
            density[mask1] = self.rho_L
            velocity[mask1] = self.u_L
            pressure[mask1] = self.p_L
            
            mask2 = (xi >= head_speed) & (xi < tail_speed)
            if np.any(mask2):
                c_fan = (2 / (self.gamma + 1)) * (c_L + (self.gamma - 1) / 2 * (self.u_L - xi[mask2]))
                density[mask2] = self.rho_L * (c_fan / c_L)**(2 / (self.gamma - 1))
                velocity[mask2] = (2 / (self.gamma + 1)) * ((self.gamma - 1) / 2 * self.u_L + c_L + xi[mask2])
                pressure[mask2] = self.p_L * (c_fan / c_L)**(2 * self.gamma / (self.gamma - 1))
            
            mask3 = (xi >= tail_speed) & (xi < contact_speed)
            density[mask3] = rho_3
            velocity[mask3] = u_star
            pressure[mask3] = p_star
            
            mask4 = (xi >= contact_speed) & (xi < shock_speed)
            density[mask4] = rho_4
            velocity[mask4] = u_star
            pressure[mask4] = p_star
            
            mask5 = xi >= shock_speed
            density[mask5] = self.rho_R
            velocity[mask5] = self.u_R
            pressure[mask5] = self.p_R
        
        energy = pressure / ((self.gamma - 1) * density)
        return density, velocity, pressure, energy
    
    def _solve_riemann(self):
        """Solve Riemann problem for star region"""
        def equations(p):
            c_L = self.sound_speed(self.p_L, self.rho_L)
            c_R = self.sound_speed(self.p_R, self.rho_R)
            
            if p > self.p_L:
                f_L = (p - self.p_L) * np.sqrt((2 / (self.gamma + 1) / self.rho_L) /
                                               (p + (self.gamma - 1) / (self.gamma + 1) * self.p_L))
            else:
                f_L = (2 * c_L / (self.gamma - 1)) * ((p / self.p_L)**((self.gamma - 1) / (2 * self.gamma)) - 1)
            
            if p > self.p_R:
                f_R = (p - self.p_R) * np.sqrt((2 / (self.gamma + 1) / self.rho_R) /
                                               (p + (self.gamma - 1) / (self.gamma + 1) * self.p_R))
            else:
                f_R = (2 * c_R / (self.gamma - 1)) * ((p / self.p_R)**((self.gamma - 1) / (2 * self.gamma)) - 1)
            
            return f_L + f_R + self.u_R - self.u_L
        
        p_star = fsolve(equations, 0.5 * (self.p_L + self.p_R))[0]
        
        c_L = self.sound_speed(self.p_L, self.rho_L)
        c_R = self.sound_speed(self.p_R, self.rho_R)
        
        u_star = self.u_L - (2 * c_L / (self.gamma - 1)) * ((p_star / self.p_L)**((self.gamma - 1) / (2 * self.gamma)) - 1)
        
        rho_3 = self.rho_L * (p_star / self.p_L)**(1 / self.gamma)
        rho_4 = self.rho_R * ((p_star / self.p_R + (self.gamma - 1) / (self.gamma + 1)) /
                               ((self.gamma - 1) / (self.gamma + 1) * (p_star / self.p_R) + 1))
        
        return p_star, u_star, rho_3, rho_4


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
        'mass': df_real['mass'].values,
        'rho': df_real['density'].values,
        'p': df_real['pressure'].values,
        'e': df_real['energy'].values
    }


def extract_centerline(x, y, field, y_center=0.05, tol=0.03):
    """Extract field values along centerline
    
    For planar 2D domain [0,1] × [0,0.1], centerline is at y=0.05
    """
    mask = np.abs(y - y_center) < tol
    x_center = x[mask]
    field_center = field[mask]
    
    idx = np.argsort(x_center)
    return x_center[idx], field_center[idx]


def create_animation(results_dir, output_file, gamma=1.4, method_name='SPH'):
    """Create animation of 2D shock tube simulation"""
    results_path = Path(results_dir)
    
    # Check for snapshots subdirectory first
    snapshots_path = results_path / 'snapshots'
    if snapshots_path.exists() and snapshots_path.is_dir():
        results_path = snapshots_path
    
    # Match numeric CSV files: 00000.csv, 00001.csv, etc. (not energy.csv)
    output_files = sorted([f for f in results_path.glob('*.csv') 
                          if f.stem.isdigit() and 'energy' not in f.name])
    
    if not output_files:
        print(f"No output files found in {results_dir}")
        return
    
    print(f"Found {len(output_files)} output files")
    print(f"Creating animation: {output_file}")
    
    # Read initial conditions to verify SPH-computed densities
    initial_data = read_sph_data(output_files[0])
    rho_left_actual = np.mean(initial_data['rho'][initial_data['x'] < 0.5])
    rho_right_actual = np.mean(initial_data['rho'][initial_data['x'] > 0.5])
    p_left_actual = np.mean(initial_data['p'][initial_data['x'] < 0.5])
    p_right_actual = np.mean(initial_data['p'][initial_data['x'] > 0.5])
    
    print(f"SPH-computed initial state:")
    print(f"  Left:  ρ={rho_left_actual:.4f}, P={p_left_actual:.4f}")
    print(f"  Right: ρ={rho_right_actual:.4f}, P={p_right_actual:.4f}")
    print(f"  Density ratio: {rho_left_actual/rho_right_actual:.2f}:1 (target: 8.0:1)")
    print(f"  Pressure ratio: {p_left_actual/p_right_actual:.2f}:1 (target: 10.0:1)")
    
    # Initialize analytical solution using EXACT Sod initial conditions
    # This is the correct approach: analytical solution uses prescribed ICs,
    # and we compare SPH results (which compute density from spacing) against it
    sod = SodShockTube(gamma)
    # Standard Sod conditions (do NOT override with SPH-computed values):
    # sod.rho_L = 1.0, sod.p_L = 1.0, sod.rho_R = 0.125, sod.p_R = 0.1 (already set in __init__)
    x_analytical = np.linspace(0.0, 1.0, 500)  # Standard Sod domain [0,1]
    
    # Create figure with 2x3 subplots (added energy and velocity)
    fig = plt.figure(figsize=(18, 12))
    gs = fig.add_gridspec(2, 3, hspace=0.3, wspace=0.3)
    
    ax_2d = fig.add_subplot(gs[0, :])  # Top row: 2D density field (spans all columns)
    ax_density = fig.add_subplot(gs[1, 0])  # Bottom row, left: density
    ax_pressure = fig.add_subplot(gs[1, 1])  # Bottom row, middle: pressure
    ax_velocity = fig.add_subplot(gs[1, 2])  # Bottom row, right: velocity (NEW)
    # ax_energy will replace 2D field or be added as third row if needed
    
    def init():
        """Initialize animation"""
        ax_2d.clear()
        ax_density.clear()
        ax_pressure.clear()
        ax_velocity.clear()
        return []
    
    def animate(frame_idx):
        """Animation update function"""
        ax_2d.clear()
        ax_density.clear()
        ax_pressure.clear()
        ax_velocity.clear()
        
        # Read SPH data
        data_file = output_files[frame_idx]
        data = read_sph_data(data_file)
        
        # Extract time from filename
        time = frame_idx * 0.01  # Assuming 0.01 time interval
        
        # 2D density field - use aspect ratio to show proper spacing
        # Adjust point size based on local particle spacing (8:1 ratio)
        point_sizes = np.where(data['x'] < 0.5, 10, 2)  # Smaller points in sparse right region
        scatter = ax_2d.scatter(data['x'], data['y'], c=data['rho'], 
                               cmap='viridis', s=point_sizes, vmin=0, vmax=1.2, alpha=0.8)
        ax_2d.set_xlabel('x', fontsize=12)
        ax_2d.set_ylabel('y', fontsize=12)
        ax_2d.set_title(f'{method_name} 2D Density Field - t = {time:.3f}s', fontsize=14)
        ax_2d.set_xlim(0.0, 1.0)  # Standard Sod domain
        ax_2d.set_ylim(0, 0.1)    # Planar 2D height
        ax_2d.set_aspect('equal')  # Equal aspect ratio to show true spacing
        plt.colorbar(scatter, ax=ax_2d, label='Density')
        
        # Extract centerline data
        x_center, rho_center = extract_centerline(data['x'], data['y'], data['rho'])
        _, p_center = extract_centerline(data['x'], data['y'], data['p'])
        _, vx_center = extract_centerline(data['x'], data['y'], data['vx'])
        _, e_center = extract_centerline(data['x'], data['y'], data['e'])
        
        # Analytical solution
        rho_analytical, u_analytical, p_analytical, e_analytical = sod.shock_tube_solution(x_analytical, time)
        
        # Density comparison
        ax_density.plot(x_analytical, rho_analytical, 'k-', linewidth=2, label='Analytical')
        ax_density.plot(x_center, rho_center, 'ro', markersize=4, alpha=0.6, label=f'{method_name} (centerline)')
        ax_density.set_xlabel('x', fontsize=12)
        ax_density.set_ylabel('Density', fontsize=12)
        ax_density.set_title('Density (Centerline vs Analytical)', fontsize=13)
        ax_density.set_xlim(0.0, 1.0)  # Standard Sod domain
        ax_density.set_ylim(0, 1.2)
        ax_density.grid(True, alpha=0.3)
        ax_density.legend(fontsize=10)
        
        # Pressure comparison
        ax_pressure.plot(x_analytical, p_analytical, 'k-', linewidth=2, label='Analytical')
        ax_pressure.plot(x_center, p_center, 'bo', markersize=4, alpha=0.6, label=f'{method_name} (centerline)')
        ax_pressure.set_xlabel('x', fontsize=12)
        ax_pressure.set_ylabel('Pressure', fontsize=12)
        ax_pressure.set_title('Pressure (Centerline vs Analytical)', fontsize=13)
        ax_pressure.set_xlim(0.0, 1.0)  # Standard Sod domain
        ax_pressure.set_ylim(0, 1.2)
        ax_pressure.grid(True, alpha=0.3)
        ax_pressure.legend(fontsize=10)
        
        # Velocity comparison (NEW)
        ax_velocity.plot(x_analytical, u_analytical, 'k-', linewidth=2, label='Analytical')
        ax_velocity.plot(x_center, vx_center, 'go', markersize=4, alpha=0.6, label=f'{method_name} (centerline)')
        ax_velocity.set_xlabel('x', fontsize=12)
        ax_velocity.set_ylabel('Velocity', fontsize=12)
        ax_velocity.set_title('Velocity (Centerline vs Analytical)', fontsize=13)
        ax_velocity.set_xlim(0.0, 1.0)  # Standard Sod domain
        ax_velocity.set_ylim(-0.1, 1.0)
        ax_velocity.grid(True, alpha=0.3)
        ax_velocity.legend(fontsize=10)
        
        return []
    
    # Create animation
    anim = animation.FuncAnimation(
        fig, animate, init_func=init,
        frames=len(output_files), interval=100, blit=True
    )
    
    # Save animation
    output_path = Path(results_dir).parent / 'analysis' / 'animations' / output_file
    output_path.parent.mkdir(parents=True, exist_ok=True)
    
    Writer = animation.writers['ffmpeg']
    writer = Writer(fps=10, metadata=dict(artist='SPH'), bitrate=1800)
    anim.save(str(output_path), writer=writer)
    
    print(f"Animation saved: {output_path}")
    plt.close()


def main():
    parser = argparse.ArgumentParser(description='Animate 2D shock tube simulation')
    parser.add_argument('--results-dir', default='../results/gsph',
                       help='Results directory')
    parser.add_argument('--output', default='2d_shock_tube.mp4',
                       help='Output animation file')
    parser.add_argument('--gamma', type=float, default=1.4,
                       help='Adiabatic index')
    parser.add_argument('--method', default='GSPH',
                       help='Method name for labels')
    args = parser.parse_args()
    
    create_animation(args.results_dir, args.output, args.gamma, args.method)


if __name__ == '__main__':
    main()
