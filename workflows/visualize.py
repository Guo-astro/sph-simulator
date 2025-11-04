#!/usr/bin/env python3
"""
Visualization and animation script for SPH simulations
Generates plots and animations from JSON output files
"""

import json
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
from pathlib import Path
import argparse
from typing import List, Dict, Tuple

class SPHVisualizer:
    def __init__(self, output_dir: str, dimension: int):
        self.output_dir = Path(output_dir)
        self.dimension = dimension
        self.files = sorted(self.output_dir.glob("*.json"))
        self.data = []
        self.load_data()
        
    def load_data(self):
        """Load all JSON output files"""
        print(f"Loading {len(self.files)} output files...")
        for file in self.files:
            with open(file, 'r') as f:
                self.data.append(json.load(f))
        print(f"Loaded {len(self.data)} timesteps")
    
    def get_particle_data(self, frame: int) -> Tuple[np.ndarray, ...]:
        """Extract particle data from a specific frame"""
        particles = self.data[frame]['particles']
        
        pos = np.array([p['position'] for p in particles])
        vel = np.array([p['velocity'] for p in particles])
        dens = np.array([p['density'] for p in particles])
        pres = np.array([p['pressure'] for p in particles])
        ene = np.array([p['energy'] for p in particles])
        
        return pos, vel, dens, pres, ene
    
    def plot_1d_snapshot(self, frame: int, save_path: str = None):
        """Plot 1D profiles at a specific frame"""
        pos, vel, dens, pres, ene = self.get_particle_data(frame)
        time = self.data[frame]['time']
        
        x = pos[:, 0]
        vx = vel[:, 0]
        
        fig, axes = plt.subplots(2, 2, figsize=(12, 10))
        fig.suptitle(f't = {time:.4f}', fontsize=14)
        
        axes[0, 0].scatter(x, dens, s=10, alpha=0.6)
        axes[0, 0].set_ylabel('Density')
        axes[0, 0].grid(True, alpha=0.3)
        
        axes[0, 1].scatter(x, vx, s=10, alpha=0.6, color='orange')
        axes[0, 1].set_ylabel('Velocity')
        axes[0, 1].grid(True, alpha=0.3)
        
        axes[1, 0].scatter(x, pres, s=10, alpha=0.6, color='green')
        axes[1, 0].set_ylabel('Pressure')
        axes[1, 0].set_xlabel('Position')
        axes[1, 0].grid(True, alpha=0.3)
        
        axes[1, 1].scatter(x, ene, s=10, alpha=0.6, color='red')
        axes[1, 1].set_ylabel('Energy')
        axes[1, 1].set_xlabel('Position')
        axes[1, 1].grid(True, alpha=0.3)
        
        plt.tight_layout()
        
        if save_path:
            plt.savefig(save_path, dpi=150)
            print(f"Saved: {save_path}")
        else:
            plt.show()
        
        plt.close()
    
    def plot_2d_snapshot(self, frame: int, quantity='density', save_path: str = None):
        """Plot 2D scatter plot at a specific frame"""
        pos, vel, dens, pres, ene = self.get_particle_data(frame)
        time = self.data[frame]['time']
        
        x = pos[:, 0]
        y = pos[:, 1]
        
        quantity_map = {
            'density': dens,
            'pressure': pres,
            'energy': ene,
            'velocity': np.sqrt(vel[:, 0]**2 + vel[:, 1]**2)
        }
        
        values = quantity_map.get(quantity, dens)
        
        fig, ax = plt.subplots(figsize=(10, 8))
        scatter = ax.scatter(x, y, c=values, s=20, cmap='viridis', alpha=0.8)
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_title(f'{quantity.capitalize()} at t = {time:.4f}')
        ax.set_aspect('equal')
        plt.colorbar(scatter, ax=ax, label=quantity.capitalize())
        
        if save_path:
            plt.savefig(save_path, dpi=150)
            print(f"Saved: {save_path}")
        else:
            plt.show()
        
        plt.close()
    
    def create_1d_animation(self, output_file: str = 'animation_1d.mp4'):
        """Create animation for 1D simulation"""
        print("Creating 1D animation...")
        
        fig, axes = plt.subplots(2, 2, figsize=(12, 10))
        
        # Initialize plots
        pos, vel, dens, pres, ene = self.get_particle_data(0)
        x = pos[:, 0]
        vx = vel[:, 0]
        
        xlim = (x.min() - 0.1, x.max() + 0.1)
        
        scatter_dens = axes[0, 0].scatter(x, dens, s=10, alpha=0.6)
        axes[0, 0].set_ylabel('Density')
        axes[0, 0].set_xlim(xlim)
        axes[0, 0].grid(True, alpha=0.3)
        
        scatter_vel = axes[0, 1].scatter(x, vx, s=10, alpha=0.6, color='orange')
        axes[0, 1].set_ylabel('Velocity')
        axes[0, 1].set_xlim(xlim)
        axes[0, 1].grid(True, alpha=0.3)
        
        scatter_pres = axes[1, 0].scatter(x, pres, s=10, alpha=0.6, color='green')
        axes[1, 0].set_ylabel('Pressure')
        axes[1, 0].set_xlabel('Position')
        axes[1, 0].set_xlim(xlim)
        axes[1, 0].grid(True, alpha=0.3)
        
        scatter_ene = axes[1, 1].scatter(x, ene, s=10, alpha=0.6, color='red')
        axes[1, 1].set_ylabel('Energy')
        axes[1, 1].set_xlabel('Position')
        axes[1, 1].set_xlim(xlim)
        axes[1, 1].grid(True, alpha=0.3)
        
        title = fig.suptitle('', fontsize=14)
        
        def update(frame):
            pos, vel, dens, pres, ene = self.get_particle_data(frame)
            time = self.data[frame]['time']
            
            x = pos[:, 0]
            vx = vel[:, 0]
            
            scatter_dens.set_offsets(np.c_[x, dens])
            scatter_vel.set_offsets(np.c_[x, vx])
            scatter_pres.set_offsets(np.c_[x, pres])
            scatter_ene.set_offsets(np.c_[x, ene])
            
            # Update y-limits
            axes[0, 0].set_ylim(dens.min() * 0.9, dens.max() * 1.1)
            axes[0, 1].set_ylim(vx.min() * 1.1 if vx.min() < 0 else vx.min() * 0.9, 
                               vx.max() * 1.1 if vx.max() > 0 else vx.max() * 0.9)
            axes[1, 0].set_ylim(pres.min() * 0.9, pres.max() * 1.1)
            axes[1, 1].set_ylim(ene.min() * 0.9, ene.max() * 1.1)
            
            title.set_text(f't = {time:.4f}')
            return scatter_dens, scatter_vel, scatter_pres, scatter_ene, title
        
        anim = animation.FuncAnimation(fig, update, frames=len(self.data),
                                      interval=100, blit=False)
        
        anim.save(output_file, writer='ffmpeg', fps=10, dpi=100)
        print(f"Animation saved: {output_file}")
        plt.close()
    
    def create_2d_animation(self, quantity='density', output_file: str = 'animation_2d.mp4'):
        """Create animation for 2D simulation"""
        print(f"Creating 2D animation for {quantity}...")
        
        fig, ax = plt.subplots(figsize=(10, 8))
        
        pos, vel, dens, pres, ene = self.get_particle_data(0)
        x, y = pos[:, 0], pos[:, 1]
        
        quantity_map = {
            'density': dens,
            'pressure': pres,
            'energy': ene,
            'velocity': np.sqrt(vel[:, 0]**2 + vel[:, 1]**2)
        }
        
        values = quantity_map.get(quantity, dens)
        
        scatter = ax.scatter(x, y, c=values, s=20, cmap='viridis', alpha=0.8)
        ax.set_xlabel('X')
        ax.set_ylabel('Y')
        ax.set_aspect('equal')
        cbar = plt.colorbar(scatter, ax=ax, label=quantity.capitalize())
        title = ax.set_title('')
        
        # Set fixed limits
        xlim = (x.min() - 0.05, x.max() + 0.05)
        ylim = (y.min() - 0.05, y.max() + 0.05)
        ax.set_xlim(xlim)
        ax.set_ylim(ylim)
        
        def update(frame):
            pos, vel, dens, pres, ene = self.get_particle_data(frame)
            time = self.data[frame]['time']
            
            x, y = pos[:, 0], pos[:, 1]
            values = quantity_map.get(quantity, dens)
            
            scatter.set_offsets(np.c_[x, y])
            scatter.set_array(values)
            scatter.set_clim(values.min(), values.max())
            
            title.set_text(f'{quantity.capitalize()} at t = {time:.4f}')
            return scatter, title
        
        anim = animation.FuncAnimation(fig, update, frames=len(self.data),
                                      interval=100, blit=False)
        
        anim.save(output_file, writer='ffmpeg', fps=10, dpi=100)
        print(f"Animation saved: {output_file}")
        plt.close()


def main():
    parser = argparse.ArgumentParser(description='SPH Simulation Visualizer')
    parser.add_argument('output_dir', help='Directory containing JSON output files')
    parser.add_argument('--dim', type=int, required=True, choices=[1, 2, 3],
                       help='Simulation dimension')
    parser.add_argument('--animate', action='store_true',
                       help='Create animation')
    parser.add_argument('--snapshot', type=int,
                       help='Create snapshot at specific frame')
    parser.add_argument('--quantity', default='density',
                       choices=['density', 'pressure', 'energy', 'velocity'],
                       help='Quantity to visualize (for 2D)')
    parser.add_argument('--output', default=None,
                       help='Output filename')
    
    args = parser.parse_args()
    
    viz = SPHVisualizer(args.output_dir, args.dim)
    
    if args.animate:
        output_file = args.output or f'animation_{args.dim}d.mp4'
        if args.dim == 1:
            viz.create_1d_animation(output_file)
        elif args.dim == 2:
            viz.create_2d_animation(args.quantity, output_file)
        else:
            print("3D animation not yet implemented")
    
    if args.snapshot is not None:
        output_file = args.output or f'snapshot_frame{args.snapshot}.png'
        if args.dim == 1:
            viz.plot_1d_snapshot(args.snapshot, output_file)
        elif args.dim == 2:
            viz.plot_2d_snapshot(args.snapshot, args.quantity, output_file)
        else:
            print("3D visualization not yet implemented")


if __name__ == '__main__':
    main()
