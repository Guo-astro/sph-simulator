#!/usr/bin/env python3
"""
Shared utilities for shock tube animation scripts
"""

import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
from scipy.optimize import fsolve


class SodShockTube:
    """Analytic solution for Sod shock tube problem

    References:
    - Sod, G. A. (1978). A survey of several finite difference methods for
      systems of nonlinear hyperbolic conservation laws. Journal of
      Computational Physics, 27(1), 1-31.
    - Toro, E. F. (2009). Riemann Solvers and Numerical Methods for Fluid
      Dynamics: A Practical Introduction. Springer.
    """

    def __init__(self, gamma=1.4):
        self.gamma = gamma
        # Initial conditions (Sod 1978, standard test case)
        self.rho_L = 1.0      # Left state density
        self.p_L = 1.0        # Left state pressure
        self.u_L = 0.0        # Left state velocity
        self.rho_R = 0.125    # Right state density (Sod 1978: p5 = 0.125)
        self.p_R = 0.1        # Right state pressure (Sod 1978: p5 = 0.1)
        self.u_R = 0.0        # Right state velocity
        self.x_discontinuity = 0.5  # Discontinuity location

    def sound_speed(self, p, rho):
        return np.sqrt(self.gamma * p / rho)

    def shock_tube_solution(self, x, t):
        """Compute analytical solution at positions x and time t

        Implements the exact Riemann solver for Euler equations.
        Regions (Toro 2009, Chapter 4):
        1: Left state (pre-shock)
        2: Rarefaction fan (left expansion wave)
        3: Contact discontinuity region
        4: Post-shock region (right compression)
        5: Right state (unshocked)
        """
        if t <= 0:
            # Initial condition (t=0) - Sod (1978)
            density = np.where(x < self.x_discontinuity, self.rho_L, self.rho_R)
            velocity = np.where(x < self.x_discontinuity, self.u_L, self.u_R)
            pressure = np.where(x < self.x_discontinuity, self.p_L, self.p_R)
        else:
            # Solve Riemann problem for post-discontinuity states
            p_star, u_star, rho_3, rho_4 = self._solve_riemann()

            # Sound speeds in star states (Toro 2009, Eq. 4.8)
            c_L = self.sound_speed(self.p_L, self.rho_L)
            c_R = self.sound_speed(self.p_R, self.rho_R)
            c_3 = self.sound_speed(p_star, rho_3)

            # Wave speeds (Toro 2009, Chapter 4)
            # Shock speed (right-moving shock, Rankine-Hugoniot conditions)
            # For shock moving into right state: S = u_R + c_R * sqrt(...)
            shock_speed = self.u_R + c_R * np.sqrt((self.gamma + 1) / (2 * self.gamma) *
                                                     (p_star / self.p_R) +
                                                     (self.gamma - 1) / (2 * self.gamma))
            # Rarefaction wave speeds (Eq. 4.56-4.57)
            head_speed = self.u_L - c_L  # Left edge of rarefaction fan
            tail_speed = u_star - c_3    # Right edge of rarefaction fan
            contact_speed = u_star       # Contact discontinuity speed

            # Characteristic variable ξ = x/t (Toro 2009, Eq. 4.12)
            xi = (x - self.x_discontinuity) / t

            # Initialize solution arrays
            density = np.zeros_like(x)
            velocity = np.zeros_like(x)
            pressure = np.zeros_like(x)

            # Region 1: Left of rarefaction (unshocked left state)
            mask1 = xi < head_speed
            density[mask1] = self.rho_L
            velocity[mask1] = self.u_L
            pressure[mask1] = self.p_L

            # Region 2: Rarefaction fan (Toro 2009, Eqs. 4.63-4.68)
            mask2 = (xi >= head_speed) & (xi < tail_speed)
            if np.any(mask2):
                # Inside rarefaction fan: self-similar solution
                # Local sound speed (depends on position ξ = (x - x0)/t)
                c_fan = (2 / (self.gamma + 1)) * (c_L + (self.gamma - 1) / 2 * (self.u_L - xi[mask2]))
                # Density in rarefaction (isentropic relation)
                density[mask2] = self.rho_L * (c_fan / c_L)**(2 / (self.gamma - 1))
                # Velocity in rarefaction
                velocity[mask2] = (2 / (self.gamma + 1)) * ((self.gamma - 1) / 2 * self.u_L + c_L + xi[mask2])
                # Pressure in rarefaction (isentropic relation)
                pressure[mask2] = self.p_L * (c_fan / c_L)**(2 * self.gamma / (self.gamma - 1))

            # Region 3: Between rarefaction tail and contact (left post-contact state)
            mask3 = (xi >= tail_speed) & (xi < contact_speed)
            density[mask3] = rho_3
            velocity[mask3] = u_star
            pressure[mask3] = p_star

            # Region 4: Between contact and shock (right post-contact state)
            mask4 = (xi >= contact_speed) & (xi < shock_speed)
            density[mask4] = rho_4
            velocity[mask4] = u_star
            pressure[mask4] = p_star

            # Region 5: Right of shock (unshocked right state)
            mask5 = xi >= shock_speed
            density[mask5] = self.rho_R
            velocity[mask5] = self.u_R
            pressure[mask5] = self.p_R

        # Specific internal energy for ideal gas (Toro 2009, Eq. 3.1)
        energy = pressure / ((self.gamma - 1) * density)
        return density, velocity, pressure, energy

    def _solve_riemann(self):
        """Solve the Riemann problem for shock tube initial conditions

        Finds the star region values (p*, u*, ρ3*, ρ4*) that satisfy the
        Rankine-Hugoniot conditions and isentropic relations.

        Returns: p_star, u_star, rho_3, rho_4
        """
        def equations(p):
            """Riemann solver equations (Toro 2009, Eq. 4.35)

            Solve f_L(p) + f_R(p) + u_R - u_L = 0 for p = p*
            """
            c_L = self.sound_speed(self.p_L, self.rho_L)
            c_R = self.sound_speed(self.p_R, self.rho_R)

            # Left wave function (Toro 2009, Eq. 4.36-4.37)
            if p > self.p_L:
                # Left shock wave (Eq. 4.36)
                f_L = (p - self.p_L) * np.sqrt((2 / (self.gamma + 1) / self.rho_L) /
                                               (p + (self.gamma - 1) / (self.gamma + 1) * self.p_L))
            else:
                # Left rarefaction wave (Eq. 4.37)
                f_L = (2 * c_L / (self.gamma - 1)) * ((p / self.p_L)**((self.gamma - 1) / (2 * self.gamma)) - 1)

            # Right wave function (Toro 2009, Eq. 4.38-4.39)
            if p > self.p_R:
                # Right shock wave (Eq. 4.38)
                f_R = (p - self.p_R) * np.sqrt((2 / (self.gamma + 1) / self.rho_R) /
                                               (p + (self.gamma - 1) / (self.gamma + 1) * self.p_R))
            else:
                # Right rarefaction wave (Eq. 4.39)
                f_R = (2 * c_R / (self.gamma - 1)) * ((p / self.p_R)**((self.gamma - 1) / (2 * self.gamma)) - 1)

            # Riemann equation: f_L + f_R + (u_R - u_L) = 0 (Eq. 4.35)
            return f_L + f_R + (self.u_R - self.u_L)

        # Solve for p* using nonlinear equation solver
        p_star = fsolve(equations, (self.p_L + self.p_R) / 2)[0]

        # Compute u* from left wave (Toro 2009, Eq. 4.42)
        c_L = self.sound_speed(self.p_L, self.rho_L)

        if p_star > self.p_L:
            # Left shock: u* = u_L - f_L (Eq. 4.42 with shock)
            f_L = (p_star - self.p_L) * np.sqrt((2 / (self.gamma + 1) / self.rho_L) /
                                                (p_star + (self.gamma - 1) / (self.gamma + 1) * self.p_L))
        else:
            # Left rarefaction: u* = u_L - f_L (Eq. 4.42 with rarefaction)
            f_L = (2 * c_L / (self.gamma - 1)) * ((p_star / self.p_L)**((self.gamma - 1) / (2 * self.gamma)) - 1)

        u_star = self.u_L - f_L  # Note: sign convention matches Toro (2009)

        # Compute star region densities (Toro 2009, Eqs. 4.45-4.50)
        if p_star > self.p_L:
            # Left shock: ρ3* from Rankine-Hugoniot (Eq. 4.45)
            rho_3 = self.rho_L * ((p_star / self.p_L + (self.gamma - 1) / (self.gamma + 1)) /
                                  ((self.gamma - 1) / (self.gamma + 1) * p_star / self.p_L + 1))
        else:
            # Left rarefaction: ρ3* from isentropic relation (Eq. 4.46)
            rho_3 = self.rho_L * (p_star / self.p_L)**(1 / self.gamma)

        # Right shock: ρ4* from Rankine-Hugoniot (Eq. 4.50)
        rho_4 = self.rho_R * ((p_star / self.p_R + (self.gamma - 1) / (self.gamma + 1)) /
                              ((self.gamma - 1) / (self.gamma + 1) * p_star / self.p_R + 1))

        return p_star, u_star, rho_3, rho_4


def load_sph_data(dat_file):
    """Load SPH simulation data from .dat file

    Args:
        dat_file: Path to the data file

    Returns:
        time, x, rho, v, p, e: Simulation data arrays
    """
    data = np.loadtxt(dat_file, comments='#')
    with open(dat_file, 'r') as f:
        time = float(f.readline().replace('#', '').strip())

    x = data[:, 0]
    rho = data[:, 4]
    v = data[:, 1]
    p = data[:, 5]
    e = data[:, 6]  # Energy from file (column 6, not 7!)

    return time, x, rho, v, p, e


def create_animation(sph_dir, output_file, gamma=1.4, method_name='SPH'):
    """Create animated comparison of SPH vs analytical Sod shock tube solution

    Args:
        sph_dir: Directory containing SPH output .dat files
        output_file: Output animation filename
        gamma: Adiabatic index (default 1.4)
        method_name: Name of SPH method for plot labels
    """
    sph_files = sorted([f for f in Path(sph_dir).glob("*.dat") if f.name != 'energy.dat'])

    if len(sph_files) == 0:
        print(f"Error: No data files found in {sph_dir}")
        return

    n_frames = len(sph_files)
    print(f"Creating {method_name} animation with {n_frames} frames")

    _, x_sample, _, _, _, _ = load_sph_data(sph_files[0])
    x_min, x_max = x_sample.min(), x_sample.max()
    x_analytic = np.linspace(x_min, x_max, 1000)
    solver = SodShockTube(gamma=gamma)

    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle(f'Shock Tube: {method_name} vs Analytical', fontsize=16, fontweight='bold')

    lines_analytic = []
    scatters_sph = []
    titles = ['Density', 'Velocity', 'Pressure', 'Energy']
    ylabels = ['ρ', 'v', 'P', 'e']

    for i, ax in enumerate(axes.flat):
        line, = ax.plot([], [], 'k-', linewidth=2.5, label='Analytical', alpha=0.9)
        scatter = ax.scatter([], [], s=30, alpha=0.7, label=method_name, marker='s', edgecolors='none', c='C0')

        lines_analytic.append(line)
        scatters_sph.append(scatter)

        ax.set_xlabel('Position', fontsize=11)
        ax.set_ylabel(ylabels[i], fontsize=11)
        ax.set_title(titles[i], fontsize=12, fontweight='bold')
        ax.legend(loc='best', fontsize=10)
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.set_xlim(x_min, x_max)

    axes[0, 0].set_ylim(0, 1.2)
    axes[0, 1].set_ylim(-0.1, 1.0)
    axes[1, 0].set_ylim(0, 1.2)
    axes[1, 1].set_ylim(0, 3.0)  # Energy (specific internal energy)

    time_text = fig.text(0.5, 0.95, '', ha='center', fontsize=14, fontweight='bold')

    def init():
        for line in lines_analytic:
            line.set_data([], [])
        for scatter in scatters_sph:
            scatter.set_offsets(np.empty((0, 2)))
        time_text.set_text('')
        return lines_analytic + scatters_sph + [time_text]

    def animate(frame):
        t, x, rho, v, p, e = load_sph_data(sph_files[frame])
        rho_a, v_a, p_a, e_a = solver.shock_tube_solution(x_analytic, t)

        lines_analytic[0].set_data(x_analytic, rho_a)
        lines_analytic[1].set_data(x_analytic, v_a)
        lines_analytic[2].set_data(x_analytic, p_a)
        lines_analytic[3].set_data(x_analytic, e_a)

        scatters_sph[0].set_offsets(np.c_[x, rho])
        scatters_sph[1].set_offsets(np.c_[x, v])
        scatters_sph[2].set_offsets(np.c_[x, p])
        scatters_sph[3].set_offsets(np.c_[x, e])

        time_text.set_text(f't = {t:.5f}')
        return lines_analytic + scatters_sph + [time_text]

    print("Creating animation...")
    anim = animation.FuncAnimation(fig, animate, init_func=init,
                                   frames=n_frames, interval=100, blit=True)

    output_path = Path(output_file)
    if not output_path.is_absolute():
        output_path = Path('../results/analysis/animations') / output_path.name
        output_path.parent.mkdir(parents=True, exist_ok=True)

    print(f"Saving to {output_path}...")
    Writer = animation.writers['ffmpeg']
    writer = Writer(fps=10, bitrate=3000, codec='libx264')
    anim.save(str(output_path), writer=writer, dpi=150)
    print(f"Animation saved: {output_path}")
    plt.close()