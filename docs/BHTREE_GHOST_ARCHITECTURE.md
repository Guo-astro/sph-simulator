# BHTree and Ghost Particle Architecture

## Overview

The BHTree (Barnes-Hut Tree) is a spatial data structure used for efficient neighbor search in SPH simulations. This document describes its integration with ghost particles for boundary condition handling.

## Core Components

### 1. BHTree Structure

**File**: `include/core/bhtree.hpp`, `include/core/bhtree.tpp`

```
BHTree<Dim>
├── BHNode (nested class)
│   ├── Spatial hierarchy (oct-tree/quad-tree/binary-tree)
│   ├── Particle linked lists (via SPHParticle::next)
│   └── Methods: create_tree(), neighbor_search(), calc_force()
│
├── Particle Container Management
│   ├── m_particles_ptr: Pointer to particle vector used for tree construction
│   └── Ensures consistency between tree build and neighbor search
│
└── Neighbor Search
    ├── Traverses tree to find particles within search radius
    ├── Returns neighbor indices that reference m_particles_ptr
    └── Validates and sorts neighbors by distance
```

### 2. Ghost Particle System

**File**: `include/core/ghost_particle_manager.hpp`

**Purpose**: Creates mirror/periodic copies of real particles near boundaries to handle boundary conditions.

**Key Characteristics**:
- Ghost particles are **copies** of real particles
- Inherit source particle's `id` initially
- Must be **renumbered** when combined with real particles
- Only used for neighbor search, not time integration

### 3. Simulation Particle Management

**File**: `include/core/simulation.hpp`, `include/core/simulation.tpp`

**Two Particle Containers**:

1. **`particles`** (vector<SPHParticle>)
   - Contains **only real particles**
   - Size = `particle_num`
   - Used for time integration, force calculation
   - Particle IDs: 0 to (particle_num - 1)

2. **`cached_search_particles`** (vector<SPHParticle>)
   - Contains **real + ghost particles**
   - Size = particle_num + ghost_count
   - Used exclusively for neighbor search
   - Particle IDs: 0 to (total_count - 1)
   - **CRITICAL**: Ghost IDs must be renumbered to match their index

## Ghost Particle ID Management (Critical Section)

### The Problem (ROOT CAUSE of segfault)

When ghost particles are created, they initially have the `id` of their source real particle:

```cpp
// Ghost created from particle 42
ghost_particle.id = 42  // ← BUG: Wrong ID for position 450+
```

When inserted at position 450+ in the combined vector, this creates a mismatch:
- **Position in vector**: 450
- **Particle ID**: 42
- **Tree stores ID in neighbor_list**: 42
- **Access `particles[42]`** when should access `cached_search_particles[450]` → **CRASH**

### The Solution

**Function**: `Simulation<Dim>::get_all_particles_for_search()`

```cpp
std::vector<SPHParticle<Dim>> Simulation<Dim>::get_all_particles_for_search() const {
    if (ghost_manager && ghost_manager->has_ghosts()) {
        std::vector<SPHParticle<Dim>> all_particles = particles;
        const auto& ghosts = ghost_manager->get_ghost_particles();
        
        all_particles.reserve(particles.size() + ghosts.size());
        
        // CRITICAL: Renumber ghost IDs to match their position
        const int ghost_id_offset = static_cast<int>(particles.size());
        for (size_t i = 0; i < ghosts.size(); ++i) {
            SPHParticle<Dim> ghost = ghosts[i];
            ghost.id = ghost_id_offset + static_cast<int>(i);  // ← FIX
            all_particles.push_back(ghost);
        }
        
        return all_particles;
    }
    return particles;
}
```

**Result**:
- Real particles: ID 0-449, positions 0-449
- Ghost particles: ID 450-533, positions 450-533
- **ID = Position** (consistency guaranteed)

## BHTree Build and Search Flow

### Phase 1: Tree Construction

**Function**: `BHTree<Dim>::make(particles, particle_num)`

```
1. Clear previous tree state
   └── m_particles_ptr = nullptr

2. Build spatial hierarchy
   ├── Link particles via SPHParticle::next
   ├── Recursively divide space (create_tree)
   └── Assign particles to leaf nodes

3. Store particle container pointer
   └── m_particles_ptr = &particles  ← Records source container
```

**Invariant**: `m_particles_ptr` points to the **exact** container used to build the tree.

### Phase 2: Neighbor Search

**Function**: `BHTree<Dim>::neighbor_search(p_i, neighbor_list, particles, is_ij)`

```
1. Traverse tree to find neighbors
   ├── BHNode::neighbor_search() recursively visits nodes
   ├── Stores particle->id in neighbor_list
   └── Returns n_neighbor count

2. Validate neighbor indices
   ├── Use m_particles_ptr (not function parameter!)
   ├── Filter out indices >= size()
   └── Log warnings for invalid indices

3. Sort neighbors by distance
   ├── Access particles via (*m_particles_ptr)[idx]
   ├── Calculate distances with periodic boundaries
   └── Return sorted neighbor count
```

**Key Design Decision**: Use `m_particles_ptr` instead of function parameter `particles` to ensure consistency, because:
- Tree was built with a specific container
- Neighbor IDs reference that container
- Caller might pass a different container (leading to crashes)

## Solver Integration

### Initialization Flow

**Function**: `Solver::initialize()`

```
1. Initialize real particles only
   └── Build tree with particles (no ghosts)

2. Calculate smoothing lengths
   └── PreInteraction::calculation() uses real particles

3. Generate ghost particles
   ├── Find max smoothing length
   ├── Set kernel support radius
   ├── GhostManager::generate_ghosts()
   └── Update cached_search_particles (real + ghost with renumbered IDs)

4. Rebuild tree with combined particles
   └── tree->make(cached_search_particles, total_count)
```

### Time Integration Flow

**Function**: `Solver::integrate()`

```
For each timestep:

1. Update ghost properties
   ├── Recalculate kernel support
   └── GhostManager::update_ghosts()

2. Build combined particle list
   ├── get_all_particles_for_search()
   └── Renumber ghost IDs

3. Update cached_search_particles
   ├── Reserve capacity (no reallocation!)
   ├── Copy combined list
   └── Clear particle->next pointers

4. Rebuild tree
   └── make_tree() with cached_search_particles

5. Neighbor search and forces
   ├── PreInteraction: uses cached_search_particles
   ├── FluidForce: uses cached_search_particles
   └── Only accesses real particles for updates

6. Time step
   └── predict(), correct() use only particles (real)
```

## Container Consistency Rules

### Rule 1: Build-Search Consistency
**The tree must be rebuilt whenever `cached_search_particles` changes.**

Violation leads to: Stale pointers, invalid indices, segfaults

### Rule 2: ID-Position Consistency
**Particle ID must equal its index in the container used for tree building.**

Violation leads to: Out-of-bounds access, wrong neighbor relationships

### Rule 3: Container Separation
**Real-only operations use `particles`, search operations use `cached_search_particles`.**

Operations on real particles only:
- Time integration (predict, correct)
- Property updates (velocity, energy)
- Output writing

Operations using real + ghost:
- Neighbor search
- Density calculation
- Force calculation (ghost particles contribute but aren't updated)

### Rule 4: Tree Rebuild Timing
**Tree must be rebuilt immediately after cached_search_particles is modified.**

```cpp
// CORRECT
cached_search_particles = get_all_particles_for_search();
make_tree();  // ← Immediate rebuild

// WRONG
cached_search_particles = get_all_particles_for_search();
// ... other operations ...
make_tree();  // ← Too late, may use stale data
```

## Common Pitfalls and Solutions

### Pitfall 1: Passing Wrong Container to neighbor_search

**Problem**: Caller passes `particles` (real-only) but tree was built with `cached_search_particles` (real+ghost).

**Solution**: BHTree stores and uses `m_particles_ptr` internally.

### Pitfall 2: Ghost IDs Not Renumbered

**Problem**: Ghost particle.id still refers to source particle.

**Solution**: `get_all_particles_for_search()` renumbers explicitly.

### Pitfall 3: Using particles Array with Ghost Neighbor Indices

**Problem**: `neighbor_list[n] = 450` but `particles.size() = 450`.

**Solution**: All code paths accessing neighbors must use `cached_search_particles`.

### Pitfall 4: Tree Not Rebuilt After Ghost Update

**Problem**: Ghosts change position/count but tree still reflects old state.

**Solution**: Always call `make_tree()` after modifying `cached_search_particles`.

## Testing Recommendations

### Critical Test Cases

1. **ID Renumbering Test**
   - Given: 100 real particles, 20 ghost particles
   - When: Combined into search container
   - Then: Ghost IDs are 100-119

2. **Container Consistency Test**
   - Given: Tree built with container A
   - When: neighbor_search called with container B
   - Then: Uses container A internally (m_particles_ptr)

3. **Bounds Validation Test**
   - Given: Neighbor index >= container size
   - When: Sorting neighbors
   - Then: Invalid index filtered out, warning logged

4. **Ghost Update Test**
   - Given: Particles move, ghosts need updating
   - When: integrate() called
   - Then: Tree rebuilt with updated ghost positions/count

5. **Edge Cases**
   - Zero ghost particles
   - All particles at boundaries (max ghosts)
   - Ghost count changes significantly between steps

## Performance Considerations

### Memory Management

- Reserve capacity in `cached_search_particles` to avoid reallocations
- Clear `particle->next` pointers after copying to avoid stale references
- Tree nodes pre-allocated based on particle count estimate

### Parallelization

- Tree building can be partially parallelized
- Neighbor search is thread-safe (read-only)
- PreInteraction parallelized over real particles only

## References

- Original bug fix: Ghost particle ID renumbering (2025-11-05)
- Related: `docs/GHOST_PARTICLES_IMPLEMENTATION.md`
- Related: `docs/BOUNDARY_SYSTEM.md`
