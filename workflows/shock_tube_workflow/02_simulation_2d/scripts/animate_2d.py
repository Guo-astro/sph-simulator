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
    """Analytical solution for Sod shock tube problem (1D)"""
    
    def __init__(self, gamma=1.4):
        self.gamma = gamma
        self.rho_L = 1.0
        self.p_L = 1.0
        self.u_L = 0.0
        self.rho_R = 0.125
        self.p_R = 0.1
        self.u_R = 0.0
        self.x_discontinuity = 0.5
    
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
    """Read SPH output file
    
    Format (for 2D): pos(2), vel(2), acc(2), mass, dens, pres, ene, sml, id, neighbor, alpha, gradh
    """
    data = np.loadtxt(filename)
    return {
        'x': data[:, 0],
        'y': data[:, 1],
        'vx': data[:, 2],
        'vy': data[:, 3],
        'rho': data[:, 7],   # dens
        'p': data[:, 8],      # pres
        'e': data[:, 9]       # ene
    }


def extract_centerline(x, y, field, y_center=0.25, tol=0.05):
    """Extract field values along centerline"""
    mask = np.abs(y - y_center) < tol
    x_center = x[mask]
    field_center = field[mask]
    
    idx = np.argsort(x_center)
    return x_center[idx], field_center[idx]


def create_animation(results_dir, output_file, gamma=1.4, method_name='SPH'):
    """Create animation of 2D shock tube simulation"""
    results_path = Path(results_dir)
    # Match numeric output files: 00000.dat, 00001.dat, etc. (not energy.dat)
    output_files = sorted([f for f in results_path.glob('*.dat') if f.stem.isdigit()])
    
    if not output_files:
        print(f"No output files found in {results_dir}")
        return
    
    print(f"Found {len(output_files)} output files")
    print(f"Creating animation: {output_file}")
    
    # Initialize analytical solution
    sod = SodShockTube(gamma)
    x_analytical = np.linspace(-0.5, 1.5, 500)
    
    # Create figure with 2x2 subplots
    fig = plt.figure(figsize=(16, 12))
    gs = fig.add_gridspec(2, 2, hspace=0.3, wspace=0.3)
    
    ax_2d = fig.add_subplot(gs[0, :])  # Top: 2D density field
    ax_density = fig.add_subplot(gs[1, 0])  # Bottom left: density
    ax_pressure = fig.add_subplot(gs[1, 1])  # Bottom right: pressure
    
    def init():
        """Initialize animation"""
        ax_2d.clear()
        ax_density.clear()
        ax_pressure.clear()
        return []
    
    def animate(frame_idx):
        """Animation update function"""
        ax_2d.clear()
        ax_density.clear()
        ax_pressure.clear()
        
        # Read SPH data
        data_file = output_files[frame_idx]
        data = read_sph_data(data_file)
        
        # Extract time from filename
        time = frame_idx * 0.01  # Assuming 0.01 time interval
        
        # 2D density field
        scatter = ax_2d.scatter(data['x'], data['y'], c=data['rho'], 
                               cmap='viridis', s=10, vmin=0, vmax=1.2)
        ax_2d.set_xlabel('x', fontsize=12)
        ax_2d.set_ylabel('y', fontsize=12)
        ax_2d.set_title(f'{method_name} 2D Density Field - t = {time:.3f}s', fontsize=14)
        ax_2d.set_xlim(-0.5, 1.5)
        ax_2d.set_ylim(0, 0.5)
        plt.colorbar(scatter, ax=ax_2d, label='Density')
        
        # Extract centerline data
        x_center, rho_center = extract_centerline(data['x'], data['y'], data['rho'])
        _, p_center = extract_centerline(data['x'], data['y'], data['p'])
        
        # Analytical solution
        rho_analytical, _, p_analytical, _ = sod.shock_tube_solution(x_analytical, time)
        
        # Density comparison
        ax_density.plot(x_analytical, rho_analytical, 'k-', linewidth=2, label='Analytical')
        ax_density.plot(x_center, rho_center, 'ro', markersize=4, alpha=0.6, label=f'{method_name} (centerline)')
        ax_density.set_xlabel('x', fontsize=12)
        ax_density.set_ylabel('Density', fontsize=12)
        ax_density.set_title('Density (Centerline vs Analytical)', fontsize=13)
        ax_density.set_xlim(-0.5, 1.5)
        ax_density.set_ylim(0, 1.2)
        ax_density.grid(True, alpha=0.3)
        ax_density.legend(fontsize=10)
        
        # Pressure comparison
        ax_pressure.plot(x_analytical, p_analytical, 'k-', linewidth=2, label='Analytical')
        ax_pressure.plot(x_center, p_center, 'bo', markersize=4, alpha=0.6, label=f'{method_name} (centerline)')
        ax_pressure.set_xlabel('x', fontsize=12)
        ax_pressure.set_ylabel('Pressure', fontsize=12)
        ax_pressure.set_title('Pressure (Centerline vs Analytical)', fontsize=13)
        ax_pressure.set_xlim(-0.5, 1.5)
        ax_pressure.set_ylim(0, 1.2)
        ax_pressure.grid(True, alpha=0.3)
        ax_pressure.legend(fontsize=10)
        
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
