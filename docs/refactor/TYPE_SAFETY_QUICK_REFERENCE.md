# Type-Safe Neighbor Access - Quick Reference

## ✅ CORRECT Usage Pattern

```cpp
// 1. Create accessor from simulation
auto neighbor_accessor = sim->create_neighbor_accessor();

// 2. Perform neighbor search
auto result = tree->find_neighbors(p_i, search_config);

// 3. Type-safe iteration
for(auto neighbor_idx : result) {
    // Access neighbor through accessor
    const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);
    
    // Use p_j in calculations
    const Vector<Dim> r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
    // ...
}
```

## ❌ WRONG Patterns (Won't Compile)

```cpp
// ❌ Direct array access with neighbor index
particles[neighbor_idx];  // ERROR: NeighborIndex cannot index vector

// ❌ Wrong array type to accessor
NeighborAccessor<Dim> bad{sim->get_real_particles()};  // ERROR: deleted constructor

// ❌ Raw int from neighbor search
int j = result.neighbor_indices[0];
accessor.get_neighbor(j);  // ERROR: requires NeighborIndex

// ❌ Implicit conversion
NeighborIndex idx = 5;  // ERROR: explicit constructor required
```

## 🔍 Common Scenarios

### Scenario 1: Density Calculation
```cpp
auto neighbor_accessor = sim->create_neighbor_accessor();
auto result = tree->find_neighbors(p_i, search_config);

real dens_i = 0.0;
for(auto neighbor_idx : result) {
    const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);
    const Vector<Dim> r_ij = periodic->calc_r_ij(p_i.pos, p_j.pos);
    const real r = abs(r_ij);
    
    if(r >= p_i.sml) break;
    
    dens_i += p_j.mass * kernel->w(r, p_i.sml);
}
```

### Scenario 2: Force Calculation
```cpp
auto neighbor_accessor = sim->create_neighbor_accessor();
auto result = tree->find_neighbors(p_i, search_config);

Vector<Dim> acc{};
for(auto neighbor_idx : result) {
    const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);
    
    // Compute force contribution
    const Vector<Dim> dw_i = kernel->dw(r_ij, r, h_i);
    acc -= dw_i * (p_j.mass * /* ... */);
}
```

### Scenario 3: Limited Iteration (first N neighbors)
```cpp
auto neighbor_accessor = sim->create_neighbor_accessor();
auto result = tree->find_neighbors(p_i, search_config);

int n_neighbor = 0;
for(auto neighbor_idx : result) {
    const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);
    
    if(abs(r_ij) >= p_i.sml) break;
    ++n_neighbor;
    // ... calculation
}

// Later: iterate first n_neighbor elements
auto it = result.begin();
for(int n = 0; n < n_neighbor; ++n, ++it) {
    const auto neighbor_idx = *it;
    const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);
    // ... second pass calculation
}
```

### Scenario 4: Self-Exclusion Check
```cpp
for(auto neighbor_idx : result) {
    const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);
    
    // Compare particle index with neighbor index
    if(i != neighbor_idx()) {  // Use operator() to get int value
        // Compute velocity signal
        const real v_sig = p_i.sound + p_j.sound - /* ... */;
    }
}
```

## 🛡️ Debug Mode Features

In debug builds (NDEBUG not defined):
- Bounds checking on `get_neighbor()` throws `std::out_of_range`
- Particle array validation throws `std::logic_error`

```cpp
sim->validate_particle_arrays();  // Check invariants in debug builds
```

## 📊 Simulation Interface Methods

```cpp
// Get typed particle arrays
RealParticleArray<Dim> real_arr = sim->get_real_particles();
SearchParticleArray<Dim> search_arr = sim->get_search_particles();

// Create accessor (recommended)
NeighborAccessor<Dim> accessor = sim->create_neighbor_accessor();

// Validate invariants (debug only)
sim->validate_particle_arrays();
```

## 🎯 Type Conversions

```cpp
// NeighborIndex construction (explicit only)
NeighborIndex idx{5};        // ✅ OK
NeighborIndex bad = 5;       // ❌ ERROR

// Extract int value (explicit)
int value = idx();           // ✅ OK
int bad = idx;               // ❌ ERROR
```

## 📝 Migration Checklist

When refactoring legacy code:

1. ✅ Replace `auto & search_particles = sim->cached_search_particles;`
   with `auto neighbor_accessor = sim->create_neighbor_accessor();`

2. ✅ Replace `for(int n = 0; n < result.neighbor_indices.size(); ++n)`
   with `for(auto neighbor_idx : result)`

3. ✅ Replace `int const j = result.neighbor_indices[n];`
   Remove this line entirely

4. ✅ Replace `auto & p_j = search_particles[j];`
   with `const auto& p_j = neighbor_accessor.get_neighbor(neighbor_idx);`

5. ✅ Replace comparisons `i != j` with `i != neighbor_idx()`

## 🔬 Concepts (C++20)

For generic programming:

```cpp
template<int Dim, NeighborProvider<Dim> Accessor>
void my_function(Accessor& accessor) {
    // Compile-time guarantee: accessor provides get_neighbor()
}
```

## ⚠️ Important Notes

1. **Accessor lifetime:** Create one accessor per loop, not per neighbor
2. **Ghost particles:** Accessor handles both real and ghost automatically
3. **OpenMP:** Each thread should create its own accessor (value type, cheap)
4. **Performance:** Zero overhead in release builds (NDEBUG defined)

## 📚 References

- Architecture: `docs/refactor/TYPE_SAFETY_ARCHITECTURE.md`
- Implementation: `TYPE_SAFETY_IMPLEMENTATION_SUMMARY.md`
- Checklist: `docs/refactor/IMPLEMENTATION_CHECKLIST.md`
