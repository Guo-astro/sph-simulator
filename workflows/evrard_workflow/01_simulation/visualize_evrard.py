#!/usr/bin/env python3
"""
Enhanced SPH Evrard Collapse Visualization with Energy Conservation
Reproduces energy profile plots as shown in Evrard (1988) and other SPH papers
"""

import numpy as np
import matplotlib
matplotlib.use('Agg')  # Use non-interactive backend
import matplotlib.pyplot as plt
from pathlib import Path
import sys

class EvrardEnergyAnalyzer:
    """Analyzes and visualizes energy conservation in Evrard collapse simulation"""
    
    def __init__(self, output_dir):
        """Initialize analyzer with output directory"""
        self.output_dir = Path(output_dir)
        self.energy_file = self.output_dir / 'energy.csv'
        
        if not self.energy_file.exists():
            raise FileNotFoundError(f"Energy file not found: {self.energy_file}")
    
    def load_energy_data(self):
        """Load energy data from energy.csv file"""
        # CSV format: time,kinetic,thermal,potential,total
        # Skip comment lines starting with #
        data = np.loadtxt(self.energy_file, delimiter=',', comments='#', skiprows=1)
        
        return {
            'time': data[:, 0],
            'kinetic': data[:, 1],
            'thermal': data[:, 2],
            'potential': data[:, 3],
            'total': data[:, 4]
        }
    
    def plot_energy_evolution(self, save_path='energy_evolution.png'):
        """
        Plot energy evolution over time - standard view
        Shows all energy components and total energy
        """
        energy = self.load_energy_data()
        
        fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(10, 10))
        
        # Top panel: All energy components
        ax1.plot(energy['time'], energy['kinetic'], 'r-', linewidth=2, label='Kinetic Energy')
        ax1.plot(energy['time'], energy['thermal'], 'g-', linewidth=2, label='Thermal Energy')
        ax1.plot(energy['time'], energy['potential'], 'b-', linewidth=2, label='Potential Energy')
        ax1.plot(energy['time'], energy['total'], 'k--', linewidth=2.5, label='Total Energy')
        
        ax1.axhline(y=0, color='gray', linestyle=':', linewidth=1)
        ax1.set_xlabel('Time', fontsize=14)
        ax1.set_ylabel('Energy', fontsize=14)
        ax1.set_title('Evrard Collapse - Energy Evolution', fontsize=16, fontweight='bold')
        ax1.legend(loc='best', fontsize=12, frameon=True)
        ax1.grid(True, alpha=0.3, linestyle='--')
        ax1.tick_params(labelsize=12)
        
        # Bottom panel: Energy conservation error
        E_initial = energy['total'][0]
        energy_error = (energy['total'] - E_initial) / np.abs(E_initial) * 100
        
        ax2.plot(energy['time'], energy_error, 'r-', linewidth=2)
        ax2.axhline(y=0, color='k', linestyle='-', linewidth=1)
        ax2.axhline(y=1, color='gray', linestyle='--', linewidth=1, alpha=0.5, label='±1%')
        ax2.axhline(y=-1, color='gray', linestyle='--', linewidth=1, alpha=0.5)
        
        ax2.set_xlabel('Time', fontsize=14)
        ax2.set_ylabel('Relative Energy Error (%)', fontsize=14)
        ax2.set_title('Energy Conservation', fontsize=14, fontweight='bold')
        ax2.grid(True, alpha=0.3, linestyle='--')
        ax2.tick_params(labelsize=12)
        ax2.legend(fontsize=11)
        
        # Add statistics text
        max_error = np.max(np.abs(energy_error))
        final_error = energy_error[-1]
        stats_text = f'Max |ΔE/E₀| = {max_error:.3f}%\nFinal ΔE/E₀ = {final_error:.3f}%'
        ax2.text(0.02, 0.98, stats_text, transform=ax2.transAxes,
                fontsize=11, verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"✓ Energy evolution plot saved: {save_path}")
        plt.close()
        
        return max_error, final_error
    
    def plot_energy_paper_style(self, save_path='energy_paper_style.png'):
        """
        Plot energy evolution in paper style
        Similar to Evrard (1988) Figure 2 and Springel (2005) energy plots
        Normalized energies to show conversion between energy types
        """
        energy = self.load_energy_data()
        
        # Normalize to initial total energy (or make absolute value to avoid negatives)
        E_norm = np.abs(energy['total'][0])
        
        fig, ax = plt.subplots(figsize=(12, 8))
        
        # Plot normalized energies
        ax.plot(energy['time'], energy['kinetic'] / E_norm, 'r-', 
                linewidth=2.5, label='Kinetic ($E_K$)')
        ax.plot(energy['time'], energy['thermal'] / E_norm, 'g-', 
                linewidth=2.5, label='Thermal ($E_{th}$)')
        ax.plot(energy['time'], energy['potential'] / E_norm, 'b-', 
                linewidth=2.5, label='Potential ($E_P$)')
        ax.plot(energy['time'], energy['total'] / E_norm, 'k--', 
                linewidth=3, label='Total ($E_{tot}$)', alpha=0.7)
        
        ax.axhline(y=0, color='gray', linestyle=':', linewidth=1)
        ax.set_xlabel('Time', fontsize=16, fontweight='bold')
        ax.set_ylabel('Energy / $|E_0|$', fontsize=16, fontweight='bold')
        ax.set_title('Evrard Collapse: Energy Components (Paper Style)', 
                    fontsize=18, fontweight='bold')
        ax.legend(loc='best', fontsize=14, frameon=True, shadow=True)
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.tick_params(labelsize=14)
        
        # Add shaded region for gravitational collapse phase
        # Typically occurs in first ~0.8 free-fall times
        if energy['time'][-1] > 1.0:
            ax.axvspan(0, 0.8, alpha=0.1, color='blue', label='Collapse Phase')
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"✓ Paper-style energy plot saved: {save_path}")
        plt.close()
    
    def plot_energy_components_normalized(self, save_path='energy_normalized.png'):
        """
        Plot energy components as fraction of total energy
        Shows energy conversion clearly
        """
        energy = self.load_energy_data()
        
        # Calculate fractions
        E_total_abs = np.abs(energy['total'])
        kinetic_frac = energy['kinetic'] / E_total_abs
        thermal_frac = energy['thermal'] / E_total_abs
        potential_frac = energy['potential'] / E_total_abs
        
        fig, ax = plt.subplots(figsize=(12, 8))
        
        ax.plot(energy['time'], kinetic_frac, 'r-', linewidth=2.5, label='Kinetic Fraction')
        ax.plot(energy['time'], thermal_frac, 'g-', linewidth=2.5, label='Thermal Fraction')
        ax.plot(energy['time'], potential_frac, 'b-', linewidth=2.5, label='Potential Fraction')
        
        ax.set_xlabel('Time', fontsize=16, fontweight='bold')
        ax.set_ylabel('Energy Fraction', fontsize=16, fontweight='bold')
        ax.set_title('Energy Component Fractions in Evrard Collapse', 
                    fontsize=18, fontweight='bold')
        ax.legend(loc='best', fontsize=14, frameon=True, shadow=True)
        ax.grid(True, alpha=0.3, linestyle='--')
        ax.tick_params(labelsize=14)
        
        plt.tight_layout()
        plt.savefig(save_path, dpi=150, bbox_inches='tight')
        print(f"✓ Normalized energy components plot saved: {save_path}")
        plt.close()
    
    def print_energy_statistics(self):
        """Print detailed energy conservation statistics"""
        energy = self.load_energy_data()
        
        E_initial = energy['total'][0]
        E_final = energy['total'][-1]
        
        print("\n" + "="*70)
        print(" ENERGY CONSERVATION ANALYSIS - Evrard Collapse Test")
        print("="*70)
        
        print(f"\nSimulation Time Range: t = {energy['time'][0]:.4f} to {energy['time'][-1]:.4f}")
        print(f"Number of energy outputs: {len(energy['time'])}")
        
        print(f"\nInitial Energies (t = {energy['time'][0]:.4f}):")
        print(f"  Kinetic:   {energy['kinetic'][0]:>12.6e}")
        print(f"  Thermal:   {energy['thermal'][0]:>12.6e}")
        print(f"  Potential: {energy['potential'][0]:>12.6e}")
        print(f"  Total:     {energy['total'][0]:>12.6e}")
        
        print(f"\nFinal Energies (t = {energy['time'][-1]:.4f}):")
        print(f"  Kinetic:   {energy['kinetic'][-1]:>12.6e}")
        print(f"  Thermal:   {energy['thermal'][-1]:>12.6e}")
        print(f"  Potential: {energy['potential'][-1]:>12.6e}")
        print(f"  Total:     {energy['total'][-1]:>12.6e}")
        
        print(f"\nEnergy Changes:")
        print(f"  ΔE_kinetic:   {energy['kinetic'][-1] - energy['kinetic'][0]:>12.6e}")
        print(f"  ΔE_thermal:   {energy['thermal'][-1] - energy['thermal'][0]:>12.6e}")
        print(f"  ΔE_potential: {energy['potential'][-1] - energy['potential'][0]:>12.6e}")
        
        # Energy conservation metrics
        energy_error = (energy['total'] - E_initial) / np.abs(E_initial) * 100
        max_error = np.max(np.abs(energy_error))
        mean_error = np.mean(np.abs(energy_error))
        final_error = energy_error[-1]
        
        print(f"\nEnergy Conservation Metrics:")
        print(f"  Maximum |ΔE/E₀|: {max_error:.4f}%")
        print(f"  Mean |ΔE/E₀|:    {mean_error:.4f}%")
        print(f"  Final ΔE/E₀:     {final_error:.4f}%")
        
        # Physical interpretation
        print(f"\nPhysical Process:")
        KE_change = energy['kinetic'][-1] / energy['kinetic'][0] if energy['kinetic'][0] != 0 else float('inf')
        IE_change = energy['thermal'][-1] / energy['thermal'][0] if energy['thermal'][0] != 0 else float('inf')
        print(f"  Kinetic energy changed by factor of {KE_change:.2f}")
        print(f"  Thermal energy changed by factor of {IE_change:.2f}")
        
        # Quality assessment
        print(f"\nQuality Assessment:")
        if max_error < 1.0:
            quality = "EXCELLENT"
            symbol = "✓✓✓"
        elif max_error < 5.0:
            quality = "GOOD"
            symbol = "✓✓"
        elif max_error < 10.0:
            quality = "ACCEPTABLE"
            symbol = "✓"
        else:
            quality = "POOR"
            symbol = "✗"
        
        print(f"  {symbol} Energy conservation: {quality}")
        print(f"  Target: |ΔE/E₀| < 1% for publication quality")
        
        print("="*70 + "\n")
        
        return max_error, mean_error, final_error
    
    def create_all_plots(self):
        """Generate all energy analysis plots"""
        print("\n" + "="*70)
        print(" Generating Energy Analysis Plots")
        print("="*70 + "\n")
        
        output_plots_dir = self.output_dir / 'energy_plots'
        output_plots_dir.mkdir(exist_ok=True)
        
        # Generate plots
        max_err, final_err = self.plot_energy_evolution(
            output_plots_dir / 'energy_evolution.png')
        
        self.plot_energy_paper_style(
            output_plots_dir / 'energy_paper_style.png')
        
        self.plot_energy_components_normalized(
            output_plots_dir / 'energy_normalized.png')
        
        # Print statistics
        self.print_energy_statistics()
        
        print(f"\n✓ All plots saved to: {output_plots_dir}")
        print("="*70 + "\n")


def main():
    """Main function"""
    
    if len(sys.argv) > 1:
        output_dir = sys.argv[1]
    else:
        # Default to build directory
        script_dir = Path(__file__).parent
        output_dir = script_dir / 'build' / 'output' / 'evrard_collapse'
    
    print("\n" + "="*70)
    print(" SPH EVRARD COLLAPSE - ENERGY CONSERVATION ANALYSIS")
    print("="*70)
    print(f"\nOutput directory: {output_dir}\n")
    
    try:
        analyzer = EvrardEnergyAnalyzer(output_dir)
        analyzer.create_all_plots()
        
    except FileNotFoundError as e:
        print(f"\n✗ Error: {e}")
        print("\nPlease run the simulation first to generate energy.dat file")
        print("Expected location: <output_dir>/energy.dat")
        sys.exit(1)
    except Exception as e:
        print(f"\n✗ Unexpected error: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)


if __name__ == '__main__':
    main()
