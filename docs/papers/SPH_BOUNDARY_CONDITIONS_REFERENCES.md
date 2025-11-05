# SPH Boundary Conditions - Essential Papers Reference

**Created:** 2025-11-05  
**Purpose:** Reference list for correct ghost particle implementation in SPH simulations

---

## Critical Papers to Download

### 1. **Monaghan (1994) - Simulating Free Surface Flows with SPH**
- **Full Title:** "Simulating Free Surface Flows with SPH"
- **Author:** J.J. Monaghan
- **Journal:** Journal of Computational Physics, Vol. 110, pp. 399-406
- **Year:** 1994
- **DOI:** 10.1006/jcph.1994.1034
- **Key Content:** 
  - First rigorous treatment of boundary conditions in SPH
  - Ghost particle method for solid walls
  - Mirror particle positioning
- **Search:** Google Scholar: `Monaghan 1994 "Simulating Free Surface Flows with SPH"`

### 2. **Morris et al. (1997) - Modeling Low Reynolds Number Incompressible Flows**
- **Full Title:** "Modeling Low Reynolds Number Incompressible Flows Using SPH"
- **Authors:** J.P. Morris, P.J. Fox, Y. Zhu
- **Journal:** Journal of Computational Physics, Vol. 136, pp. 214-226
- **Year:** 1997
- **DOI:** 10.1006/jcph.1997.5776
- **Key Content:**
  - Virtual/ghost particles at boundaries
  - **CRITICAL:** Distance between ghost and wall = distance between real particle and wall
  - Velocity treatment for no-slip vs free-slip
- **Search:** Google Scholar: `Morris 1997 "Modeling Low Reynolds Number Incompressible Flows Using SPH"`

### 3. **Takeda et al. (1994) - Numerical Simulation of Viscous Flow by SPH**
- **Full Title:** "Numerical Simulation of Viscous Flow by Smoothed Particle Hydrodynamics"
- **Authors:** H. Takeda, S.M. Miyama, M. Sekiya
- **Journal:** Progress of Theoretical Physics, Vol. 92, No. 5, pp. 939-960
- **Year:** 1994
- **DOI:** 10.1143/ptp/92.5.939
- **Key Content:**
  - Wall boundary treatment
  - Force calculation at boundaries
- **Search:** Google Scholar: `Takeda 1994 "Numerical Simulation of Viscous Flow by Smoothed Particle Hydrodynamics"`

### 4. **Monaghan (2005) - Smoothed Particle Hydrodynamics (Review)**
- **Full Title:** "Smoothed Particle Hydrodynamics"
- **Author:** J.J. Monaghan
- **Journal:** Reports on Progress in Physics, Vol. 68, pp. 1703-1759
- **Year:** 2005
- **DOI:** 10.1088/0034-4885/68/8/R01
- **Key Content:**
  - Comprehensive review of SPH methods
  - Section on boundary conditions (Section 5.3)
  - Summary of best practices
- **Search:** Google Scholar: `Monaghan 2005 "Smoothed Particle Hydrodynamics" review`

### 5. **Adami et al. (2012) - Generalized Wall Boundary Condition**
- **Full Title:** "A generalized wall boundary condition for smoothed particle hydrodynamics"
- **Authors:** S. Adami, X.Y. Hu, N.A. Adams
- **Journal:** Journal of Computational Physics, Vol. 231, pp. 7057-7075
- **Year:** 2012
- **DOI:** 10.1016/j.jcp.2012.05.005
- **Key Content:**
  - Modern treatment of wall boundaries
  - Ghost particle positioning and extrapolation
  - **HIGHLY RELEVANT** for current implementation
- **Search:** Google Scholar: `Adami 2012 "generalized wall boundary condition smoothed particle hydrodynamics"`

### 6. **Libersky & Petschek (1991) - High Strain Lagrangian Hydrodynamics**
- **Full Title:** "Smooth Particle Hydrodynamics with Strength of Materials"
- **Authors:** L.D. Libersky, A.G. Petschek
- **Conference:** Advances in the Free-Lagrange Method, Springer, pp. 248-257
- **Year:** 1991
- **Key Content:**
  - Ghost particles for solid mechanics
  - Mirror image method
- **Search:** Google Scholar: `Libersky Petschek 1991 "Smooth Particle Hydrodynamics with Strength of Materials"`

### 7. **Marrone et al. (2011) - δ-SPH Model**
- **Full Title:** "δ-SPH model for simulating violent impact flows"
- **Authors:** S. Marrone, M. Antuono, A. Colagrossi, G. Colicchio, D. Le Touzé, G. Graziani
- **Journal:** Computer Methods in Applied Mechanics and Engineering, Vol. 200, pp. 1526-1542
- **Year:** 2011
- **DOI:** 10.1016/j.cma.2010.12.016
- **Key Content:**
  - Density diffusion at boundaries
  - Ghost particle treatment for violent flows
- **Search:** Google Scholar: `Marrone 2011 "δ-SPH model for simulating violent impact flows"`

---

## Key Concepts from Literature

### Ghost Particle Positioning (from Morris et al. 1997)

**CRITICAL RULE:**
```
If real particle distance from wall = d
Then ghost particle distance from wall = d (on opposite side)
Total distance between real and ghost = 2d
```

**Mathematical Expression:**
```
x_ghost = x_wall - (x_real - x_wall)
x_ghost = 2*x_wall - x_real
```

### Velocity Treatment

**Free-Slip Wall (Mirror):**
```
v_ghost_normal = -v_real_normal    (reflect normal component)
v_ghost_tangent = v_real_tangent   (preserve tangential component)
```

**No-Slip Wall:**
```
v_ghost = -v_real                  (reflect all components)
```

**For 1D Shock Tube (Reflective/Free-Slip):**
```
At left wall (x = x_left):
  x_ghost = 2*x_left - x_real
  v_ghost = -v_real

At right wall (x = x_right):
  x_ghost = 2*x_right - x_real
  v_ghost = -v_real
```

### Thermodynamic Properties

**From Monaghan (1994, 2005):**
```
ρ_ghost = ρ_real        (density same)
p_ghost = p_real        (pressure same for free surface)
e_ghost = e_real        (internal energy same)
```

**For solid walls (Adami et al. 2012):**
Ghost pressure may be extrapolated to enforce boundary condition.

---

## Download Instructions

### Method 1: Google Scholar
1. Go to https://scholar.google.com
2. Search for each paper using the title or search terms provided
3. Look for [PDF] links on the right side
4. Some papers are freely available, others require institutional access

### Method 2: arXiv (for preprints)
Some authors post preprints at https://arxiv.org
Search by author name and year.

### Method 3: ResearchGate
1. Go to https://www.researchgate.net
2. Search for paper title
3. Authors often share PDFs directly

### Method 4: Institutional Library
If you have university access:
1. Use your library's journal database
2. Search by DOI (most reliable)
3. Download PDF directly

### Method 5: Author Websites
Many authors post PDFs on their personal/lab websites:
- Joe Monaghan: University of Melbourne
- Salvatore Marrone: CNR-INM Rome
- Xiangyu Hu: TU Munich

---

## Specific Issues in Current Implementation

Based on your shock tube workflow, check these aspects:

### 1. Ghost Particle Distance
**Current Issue:** Ghost particles may not maintain symmetric distance from wall

**Solution from Morris 1997:**
```cpp
// For particle at position x with wall at x_wall
double distance_to_wall = abs(x - x_wall);
double x_ghost = x_wall - distance_to_wall;  // Mirror position
// OR equivalently:
double x_ghost = 2.0 * x_wall - x;
```

**CRITICAL ADJUSTMENT - Domain Boundary Placement:**
```
The wall boundaries should NOT be placed at the particle layer positions.
Instead, walls should be offset by half the particle spacing:

For 1D domain with particle spacing dx:
  Left-most particle at:  x_left_particle
  Right-most particle at: x_right_particle
  
  Wall positions:
  x_wall_left  = x_left_particle - 0.5 * dx
  x_wall_right = x_right_particle + 0.5 * dx

This ensures ghost particles mirror correctly with proper spacing.

Example for shock tube with dx = 0.0025:
  Left particle layer:   x = 0.0
  Left wall position:    x = -0.00125  (0.0 - 0.5*0.0025)
  
  Right particle layer:  x = 1.0
  Right wall position:   x = 1.00125   (1.0 + 0.5*0.0025)
  
Then for a real particle at x = 0.0:
  Distance to wall = 0.0 - (-0.00125) = 0.00125
  Ghost position   = 2*(-0.00125) - 0.0 = -0.0025
  Ghost-to-real distance = 0.0025 = dx ✓ (correct spacing)
```

### 2. Velocity Sign
**Current Issue:** Uncertain if velocity should be flipped

**Solution from Monaghan 1994 (Free-slip/Reflective):**
```cpp
// For 1D, normal component is the only component
v_ghost = -v_real;  // Reflect velocity
```

### 3. Smoothing Length
**From Literature:**
```cpp
h_ghost = h_real;  // Same smoothing length
```

### 4. Number of Ghost Layers
**From Morris 1997:**
- At least one layer of ghost particles
- Distance from wall ≤ 2h (kernel support radius)
- For cubic spline kernel: support radius = 2h
- Therefore: Need ghosts for all particles within 2h of wall

---

## Quick Reference Table

| Property | Value for Ghost Particle | Reference |
|----------|-------------------------|-----------|
| Position | x_ghost = 2*x_wall - x_real | Morris 1997 |
| Velocity (free-slip) | v_ghost = -v_real | Monaghan 1994 |
| Velocity (no-slip) | v_ghost = -v_real | Morris 1997 |
| Density | ρ_ghost = ρ_real | Monaghan 1994 |
| Pressure | p_ghost = p_real (or extrapolated) | Adami 2012 |
| Internal Energy | e_ghost = e_real | Monaghan 2005 |
| Smoothing Length | h_ghost = h_real | Morris 1997 |
| Mass | m_ghost = m_real | Monaghan 1994 |

---

## Recommended Reading Order

1. **Start with:** Monaghan (2005) - Section 5.3 on boundaries (comprehensive overview)
2. **Then:** Morris et al. (1997) - Detailed implementation
3. **Modern approach:** Adami et al. (2012) - Generalized method
4. **For review:** Monaghan (1994) - Original method

---

## Notes for Implementation

After downloading these papers, look specifically for:

1. **Diagrams** showing ghost particle positioning
2. **Equations** for mirror/virtual particle placement
3. **Algorithm boxes** with step-by-step implementation
4. **Test cases** (especially 1D shock tubes)
5. **Validation results** comparing with analytical solutions

The Morris 1997 paper is particularly important as it has clear diagrams and equations for exactly your use case (1D reflective boundaries).

---

## Related Topics to Search

- "SPH reflective boundary conditions"
- "SPH wall boundary treatment"
- "SPH ghost particle method"
- "SPH mirror particles"
- "SPH solid wall boundary"
- "SPH free-slip boundary"

---

## Detailed Implementation for 1D Shock Tube

### Correct Domain Setup

**Scenario:** 1D shock tube from x=0 to x=1 with particle spacing dx

**Step 1: Particle Initialization**
```cpp
const double dx = 0.0025;  // Particle spacing
const int n_left = 200;    // Particles in left region
const int n_right = 200;   // Particles in right region

// Initialize particle positions
for (int i = 0; i < n_left; ++i) {
    particles[i].pos[0] = i * dx;  // Left: 0.0, 0.0025, 0.005, ...
}
for (int i = 0; i < n_right; ++i) {
    particles[n_left + i].pos[0] = 0.5 + i * dx;  // Right: 0.5, 0.5025, ...
}
```

**Step 2: Wall Boundary Placement (CRITICAL)**
```cpp
// Domain boundaries are NOT at particle positions
// They are offset by half particle spacing

const double x_left_particle  = 0.0;           // Left-most particle
const double x_right_particle = 1.0 - dx;      // Right-most particle

// Wall positions (offset by half spacing)
const double x_wall_left  = x_left_particle - 0.5 * dx;   // -0.00125
const double x_wall_right = x_right_particle + 0.5 * dx;  // 1.00125

// Store in periodic/boundary structure
periodic.range_min[0] = x_wall_left;
periodic.range_max[0] = x_wall_right;
```

**Step 3: Ghost Particle Generation**
```cpp
void generate_mirror_ghosts_1d(
    const vector<SPHParticle<1>>& real_particles,
    vector<SPHParticle<1>>& ghost_particles,
    double x_wall_left,
    double x_wall_right,
    double kernel_support_radius  // Typically 2*h for cubic spline
) {
    ghost_particles.clear();
    
    for (const auto& p_real : real_particles) {
        const double x = p_real.pos[0];
        
        // Left wall ghosts (for particles near left boundary)
        if (x - x_wall_left < kernel_support_radius) {
            SPHParticle<1> ghost = p_real;
            
            // Mirror position across wall
            ghost.pos[0] = 2.0 * x_wall_left - x;
            
            // Reflect velocity (free-slip/reflective)
            ghost.vel[0] = -p_real.vel[0];
            
            // Same thermodynamic properties
            ghost.dens = p_real.dens;
            ghost.pres = p_real.pres;
            ghost.ene  = p_real.ene;
            ghost.sml  = p_real.sml;
            ghost.mass = p_real.mass;
            
            // Mark as ghost (negative ID or special flag)
            ghost.id = -(p_real.id + 1);  // Negative ID convention
            
            ghost_particles.push_back(ghost);
        }
        
        // Right wall ghosts (for particles near right boundary)
        if (x_wall_right - x < kernel_support_radius) {
            SPHParticle<1> ghost = p_real;
            
            // Mirror position across wall
            ghost.pos[0] = 2.0 * x_wall_right - x;
            
            // Reflect velocity
            ghost.vel[0] = -p_real.vel[0];
            
            // Same thermodynamic properties
            ghost.dens = p_real.dens;
            ghost.pres = p_real.pres;
            ghost.ene  = p_real.ene;
            ghost.sml  = p_real.sml;
            ghost.mass = p_real.mass;
            
            // Mark as ghost
            ghost.id = -(p_real.id + 1);
            
            ghost_particles.push_back(ghost);
        }
    }
}
```

**Step 4: Verification (Debug Output)**
```cpp
// After ghost generation, verify spacing
for (const auto& ghost : ghost_particles) {
    // Find corresponding real particle
    int real_id = -ghost.id - 1;
    const auto& real = particles[real_id];
    
    double ghost_to_real = abs(ghost.pos[0] - real.pos[0]);
    
    // Should be approximately dx (particle spacing)
    if (abs(ghost_to_real - dx) > 1e-10) {
        WRITE_LOG << "WARNING: Ghost-real spacing incorrect!\n"
                  << "  Real particle " << real_id << " at x=" << real.pos[0] << "\n"
                  << "  Ghost at x=" << ghost.pos[0] << "\n"
                  << "  Spacing=" << ghost_to_real << " (expected " << dx << ")";
    }
}
```

### Example Calculation

**Given:**
- Particle spacing: dx = 0.0025
- Left-most real particle: x = 0.0
- Right-most real particle: x = 0.9975
- Smoothing length: h = 0.003
- Kernel support: 2h = 0.006

**Wall positions:**
```
x_wall_left  = 0.0 - 0.5*0.0025 = -0.00125
x_wall_right = 0.9975 + 0.5*0.0025 = 0.99875
```

**Left boundary ghost (for particle at x=0.0):**
```
Real particle:
  x_real = 0.0
  v_real = 0.5 (moving right)

Distance to wall:
  d = 0.0 - (-0.00125) = 0.00125

Ghost particle:
  x_ghost = 2*(-0.00125) - 0.0 = -0.0025
  v_ghost = -0.5 (moving left, reflected)

Verification:
  Distance real-to-ghost = |0.0 - (-0.0025)| = 0.0025 = dx ✓
  Distance ghost-to-wall = |-0.0025 - (-0.00125)| = 0.00125 = d ✓
  Distance real-to-wall  = |0.0 - (-0.00125)| = 0.00125 = d ✓
  Symmetry preserved ✓
```

**Right boundary ghost (for particle at x=0.9975):**
```
Real particle:
  x_real = 0.9975
  v_real = -0.3 (moving left)

Distance to wall:
  d = 0.99875 - 0.9975 = 0.00125

Ghost particle:
  x_ghost = 2*0.99875 - 0.9975 = 1.0
  v_ghost = 0.3 (moving right, reflected)

Verification:
  Distance real-to-ghost = |1.0 - 0.9975| = 0.0025 = dx ✓
  Distance ghost-to-wall = |1.0 - 0.99875| = 0.00125 = d ✓
  Distance real-to-wall  = |0.99875 - 0.9975| = 0.00125 = d ✓
  Symmetry preserved ✓
```

### Common Mistakes to Avoid

❌ **WRONG - Wall at particle position:**
```cpp
x_wall_left = 0.0;  // Same as left-most particle
// Result: Ghost at x=-0.0 (overlaps with real particle!)
```

❌ **WRONG - No velocity reflection:**
```cpp
ghost.vel = real.vel;  // Don't copy velocity directly
// Result: Particles flow through wall
```

❌ **WRONG - Asymmetric mirroring:**
```cpp
ghost.pos[0] = x_wall - abs(x_real - x_wall);  // Using abs() breaks sign
// Result: All ghosts on same side of wall
```

✅ **CORRECT - Half-spacing offset:**
```cpp
x_wall_left = x_left_particle - 0.5 * dx;
ghost.pos[0] = 2.0 * x_wall_left - x_real;
ghost.vel[0] = -x_real;
// Result: Perfect symmetry, correct spacing
```

---

**Status:** Please download these papers and save them in this directory for reference.
The most critical ones are Morris (1997) and Monaghan (2005).
