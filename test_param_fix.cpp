/**
 * @file test_param_fix.cpp  
 * @brief Verify the parameter estimation fixes for anisotropic distributions
 */

#include <iostream>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// OLD VERSION
int suggest_neighbor_number_OLD(double particle_spacing, double kernel_support, int dimension) {
    int neighbor_num = 0;
    
    if (dimension == 2) {
        double area = M_PI * kernel_support * kernel_support;
        neighbor_num = static_cast<int>(area * 4.0);  // Safety factor
    }
    
    const int min_safe = 12;
    const int max_reasonable = 100;
    neighbor_num = std::max(min_safe, std::min(max_reasonable, neighbor_num));
    
    return neighbor_num;
}

// NEW VERSION
int suggest_neighbor_number_NEW(double particle_spacing, double kernel_support, int dimension) {
    int neighbor_num = 0;
    constexpr double SAFETY_FACTOR = 1.2;
    
    if (dimension == 2) {
        double theoretical = M_PI * kernel_support * kernel_support;
        neighbor_num = static_cast<int>(theoretical * SAFETY_FACTOR);
    }
    
    const int min_safe = 12;
    const int max_reasonable = 50;
    neighbor_num = std::max(min_safe, std::min(max_reasonable, neighbor_num));
    
    return neighbor_num;
}

// Calculate resulting smoothing length
double calc_initial_sml(int neighbor_num, double mass, double dens, int Dim) {
    const double A = M_PI;  // For 2D
    return std::pow(neighbor_num * mass / (dens * A), 1.0 / Dim);
}

int main() {
    // 2D shock tube parameters
    const double dx_left = 0.005;
    const double dy = 0.05;
    const double dx_right = 0.04;
    
    const double mass = 0.125 * dx_right * dy;  // 0.00025
    const double dens = 1.0;
    const double kernel_support = 2.0;
    const int Dim = 2;
    
    // OLD: minimum spacing
    const double old_spacing = dx_left;  // 0.005
    
    // NEW: geometric mean spacing
    const double new_spacing = std::sqrt(dx_left * dy);  // sqrt(0.00025) = 0.0158
    
    std::cout << "=== 2D Shock Tube Parameter Estimation ===\n\n";
    
    std::cout << "Physical setup:\n";
    std::cout << "  dx_left = " << dx_left << "\n";
    std::cout << "  dy      = " << dy << "\n";
    std::cout << "  mass    = " << mass << "\n";
    std::cout << "  dens    = " << dens << "\n";
    std::cout << "  Anisotropy ratio = " << dy/dx_left << ":1\n\n";
    
    // OLD APPROACH
    int old_neighbor_num = suggest_neighbor_number_OLD(old_spacing, kernel_support, Dim);
    double old_sml = calc_initial_sml(old_neighbor_num, mass, dens, Dim);
    
    std::cout << "OLD (minimum spacing):\n";
    std::cout << "  Spacing   = " << old_spacing << "\n";
    std::cout << "  Neighbors = " << old_neighbor_num << "\n";
    std::cout << "  Initial sml = " << old_sml << "\n";
    std::cout << "  sml / dy ratio = " << old_sml / dy << " (>1.0 means catastrophic!)\n\n";
    
    // NEW APPROACH
    int new_neighbor_num = suggest_neighbor_number_NEW(new_spacing, kernel_support, Dim);
    double new_sml = calc_initial_sml(new_neighbor_num, mass, dens, Dim);
    
    std::cout << "NEW (geometric mean spacing):\n";
    std::cout << "  Spacing   = " << new_spacing << "\n";
    std::cout << "  Neighbors = " << new_neighbor_num << "\n";
    std::cout << "  Initial sml = " << new_sml << "\n";
    std::cout << "  sml / dy ratio = " << new_sml / dy << " (should be <0.5)\n\n";
    
    std::cout << "IMPROVEMENT:\n";
    std::cout << "  Neighbor reduction: " << old_neighbor_num << " → " << new_neighbor_num 
              << " (" << ((old_neighbor_num - new_neighbor_num) * 100.0 / old_neighbor_num) << "% decrease)\n";
    std::cout << "  sml reduction: " << old_sml << " → " << new_sml 
              << " (" << ((old_sml - new_sml) * 100.0 / old_sml) << "% decrease)\n";
    
    // Estimate ghost particle reduction
    // OLD: sml=0.063 → kernel_support_radius = 0.126 > dy/2 (0.025) → ghosts from Y boundaries
    //      All particles within 0.126 of boundaries → ~4500 Y ghosts
    // NEW: sml=0.038 → kernel_support_radius = 0.076 > dy/2 (0.025) → still some Y ghosts
    //      But fewer particles affected
    
    double old_kernel_radius = old_sml * 2.0;
    double new_kernel_radius = new_sml * 2.0;
    
    std::cout << "\nKernel support radius (for ghost generation):\n";
    std::cout << "  OLD: " << old_kernel_radius << " (vs domain Y=" << dy << ")\n";
    std::cout << "  NEW: " << new_kernel_radius << " (vs domain Y=" << dy << ")\n";
    
    if (new_kernel_radius > dy) {
        std::cout << "  ⚠ WARNING: Still larger than Y domain! Further tuning needed.\n";
    } else {
        std::cout << "  ✓ Now properly sized for domain.\n";
    }
    
    return 0;
}
