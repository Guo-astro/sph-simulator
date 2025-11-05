# Neighbor Search Refactoring Proposal

**Date:** 2025-11-05  
**Goal:** Refactor from imperative to declarative design with TDD/BDD approach  
**Status:** ✅ COMPLETED - See NEIGHBOR_SEARCH_REFACTORING_CHANGELOG.md for details  
**Integration Test:** ✅ PASSED - shock_tube workflow runs successfully

## Current Problem Analysis

### What's Wrong with Current Implementation?

```cpp
// IMPERATIVE - Hidden state, manual bounds checking, easy to miss
void neighbor_search(const SPHParticle<Dim>& p_i, 
                    std::vector<int>& neighbor_list,  // Mutable output parameter
                    int& n_neighbor,                   // Manual counter
                    const int max_neighbors,           // Easy to forget
                    const bool is_ij, 
                    const Periodic<Dim>* periodic) {
    // Recursive traversal with manual bounds checking
    if (n_neighbor >= max_neighbors) return;  // Band-aid fix
    neighbor_list[n_neighbor] = p->id;
    ++n_neighbor;
}
```

**Issues:**
1. **Mutable state everywhere** - `n_neighbor` passed by reference, modified in recursion
2. **Manual memory management** - Easy to overflow, no automatic bounds
3. **Implicit contracts** - Caller must allocate right size, no enforcement
4. **Hard to test** - Must set up complex state, difficult to verify correctness
5. **Error-prone** - Heap overflow happened because bounds checking was forgotten
6. **No type safety** - `std::vector<int>` could contain invalid indices

---

## Proposed Declarative Design

### 1. Value-Based Neighbor List (Immutable Result)

```cpp
// DECLARATIVE - Returns value, no hidden state
struct NeighborSearchResult {
    std::vector<int> neighbor_indices;
    bool is_truncated;           // Did we hit max neighbors?
    int total_candidates_found;  // For diagnostics
    
    // BDD: Given a result, when I check validity, then it should meet constraints
    [[nodiscard]] bool is_valid() const {
        return std::all_of(neighbor_indices.begin(), neighbor_indices.end(),
                          [](int idx) { return idx >= 0; });
    }
    
    [[nodiscard]] size_t size() const { return neighbor_indices.size(); }
    [[nodiscard]] bool empty() const { return neighbor_indices.empty(); }
};
```

### 2. RAII Neighbor Collector (Automatic Bounds)

```cpp
// BDD: Given a max capacity, when collecting neighbors, then never overflow
class NeighborCollector {
private:
    std::vector<int> indices_;
    const size_t max_capacity_;
    size_t total_candidates_ = 0;
    
public:
    explicit NeighborCollector(size_t max_capacity) 
        : max_capacity_(max_capacity) {
        indices_.reserve(max_capacity);  // Pre-allocate
    }
    
    // BDD: Given space available, when adding valid neighbor, then succeed
    [[nodiscard]] bool try_add(int neighbor_id) {
        ++total_candidates_;
        
        if (indices_.size() >= max_capacity_) {
            return false;  // Capacity reached, cannot add
        }
        
        if (neighbor_id < 0) {
            return false;  // Invalid ID, reject
        }
        
        indices_.push_back(neighbor_id);
        return true;
    }
    
    [[nodiscard]] bool is_full() const { 
        return indices_.size() >= max_capacity_; 
    }
    
    [[nodiscard]] NeighborSearchResult finalize() && {
        return NeighborSearchResult{
            .neighbor_indices = std::move(indices_),
            .is_truncated = (total_candidates_ > indices_.size()),
            .total_candidates_found = static_cast<int>(total_candidates_)
        };
    }
};
```

### 3. Functional Tree Traversal

```cpp
// DECLARATIVE - Pure function, no side effects
template<int Dim>
class BHNode {
public:
    // BDD: Given particle and search params, when searching, then return all neighbors
    [[nodiscard]] NeighborSearchResult find_neighbors(
        const SPHParticle<Dim>& p_i,
        const NeighborSearchConfig& config,
        const Periodic<Dim>& periodic) const {
        
        NeighborCollector collector(config.max_neighbors);
        find_neighbors_recursive(p_i, config, periodic, collector);
        return std::move(collector).finalize();
    }
    
private:
    void find_neighbors_recursive(
        const SPHParticle<Dim>& p_i,
        const NeighborSearchConfig& config,
        const Periodic<Dim>& periodic,
        NeighborCollector& collector) const {
        
        // Early exit if full
        if (collector.is_full()) {
            return;
        }
        
        // Check if this node is relevant
        if (!is_within_search_radius(p_i, config, periodic)) {
            return;
        }
        
        if (is_leaf) {
            // Collect leaf particles
            for (const auto* p = first; p != nullptr; p = p->next) {
                if (is_neighbor(p_i, *p, config, periodic)) {
                    if (!collector.try_add(p->id)) {
                        return;  // Hit capacity, stop
                    }
                }
            }
        } else {
            // Recurse into children
            for (int i = 0; i < nchild<Dim>(); ++i) {
                if (childs[i]) {
                    childs[i]->find_neighbors_recursive(p_i, config, periodic, collector);
                }
            }
        }
    }
    
    // BDD: Given two particles, when checking distance, then determine if neighbors
    [[nodiscard]] bool is_neighbor(
        const SPHParticle<Dim>& p_i,
        const SPHParticle<Dim>& p_j,
        const NeighborSearchConfig& config,
        const Periodic<Dim>& periodic) const {
        
        const Vector<Dim> r_ij = periodic.calc_r_ij(p_i.pos, p_j.pos);
        const real r2 = abs2(r_ij);
        const real h = config.use_max_kernel 
            ? std::max(p_i.sml, kernel_size) 
            : p_i.sml;
        return r2 < h * h;
    }
};
```

### 4. Configuration Object (Explicit Contract)

```cpp
// BDD: Given search parameters, when creating config, then validate constraints
struct NeighborSearchConfig {
    size_t max_neighbors;        // Hard limit
    bool use_max_kernel;         // For is_ij searches
    
    static NeighborSearchConfig create(int neighbor_number, bool is_ij) {
        constexpr int SAFETY_FACTOR = 20;
        
        if (neighbor_number <= 0) {
            throw std::invalid_argument("neighbor_number must be positive");
        }
        
        return NeighborSearchConfig{
            .max_neighbors = static_cast<size_t>(neighbor_number * SAFETY_FACTOR),
            .use_max_kernel = is_ij
        };
    }
    
    // BDD: Given config, when checking validity, then ensure reasonable limits
    [[nodiscard]] bool is_valid() const {
        return max_neighbors > 0 && max_neighbors < 100000;  // Sanity check
    }
};
```

---

## BDD Test Cases

### Test Structure (Given-When-Then)

```cpp
// tests/core/neighbor_search_test.cpp

#include <catch2/catch_test_macros.hpp>
#include "core/bhtree.hpp"

SCENARIO("Neighbor search with capacity limits", "[neighbor_search][bdd]") {
    
    GIVEN("A particle with many potential neighbors") {
        SPHParticle<1> target_particle = create_test_particle(/*pos=*/0.5);
        auto tree = create_test_tree_with_particles(100);  // 100 particles nearby
        
        WHEN("Searching with max_neighbors = 50") {
            auto config = NeighborSearchConfig{.max_neighbors = 50, .use_max_kernel = false};
            auto result = tree.find_neighbors(target_particle, config);
            
            THEN("Result should be truncated at 50 neighbors") {
                REQUIRE(result.size() <= 50);
                REQUIRE(result.is_truncated == true);
                REQUIRE(result.total_candidates_found > 50);
            }
            
            AND_THEN("All returned indices should be valid") {
                REQUIRE(result.is_valid());
                for (int idx : result.neighbor_indices) {
                    REQUIRE(idx >= 0);
                    REQUIRE(idx < 100);
                }
            }
        }
        
        WHEN("Searching with max_neighbors = 200") {
            auto config = NeighborSearchConfig{.max_neighbors = 200, .use_max_kernel = false};
            auto result = tree.find_neighbors(target_particle, config);
            
            THEN("Result should contain all actual neighbors") {
                REQUIRE(result.size() <= 200);
                REQUIRE(result.is_truncated == false);
            }
        }
    }
}

SCENARIO("Neighbor collector prevents buffer overflow", "[neighbor_collector][bdd]") {
    
    GIVEN("A collector with capacity 5") {
        NeighborCollector collector(5);
        
        WHEN("Adding 3 valid neighbors") {
            bool success1 = collector.try_add(10);
            bool success2 = collector.try_add(20);
            bool success3 = collector.try_add(30);
            
            THEN("All additions should succeed") {
                REQUIRE(success1);
                REQUIRE(success2);
                REQUIRE(success3);
                REQUIRE_FALSE(collector.is_full());
            }
        }
        
        WHEN("Adding 6 neighbors (exceeds capacity)") {
            for (int i = 0; i < 6; ++i) {
                collector.try_add(i * 10);
            }
            
            THEN("Only first 5 should be added, 6th should fail") {
                auto result = std::move(collector).finalize();
                REQUIRE(result.size() == 5);
                REQUIRE(result.is_truncated == true);
                REQUIRE(result.total_candidates_found == 6);
            }
        }
        
        WHEN("Adding negative index") {
            bool success = collector.try_add(-1);
            
            THEN("Addition should fail") {
                REQUIRE_FALSE(success);
            }
        }
    }
}

SCENARIO("Neighbor search result validation", "[neighbor_search][bdd]") {
    
    GIVEN("A valid search result") {
        NeighborSearchResult result{
            .neighbor_indices = {0, 5, 10, 15},
            .is_truncated = false,
            .total_candidates_found = 4
        };
        
        WHEN("Checking validity") {
            THEN("Should be valid") {
                REQUIRE(result.is_valid());
                REQUIRE(result.size() == 4);
            }
        }
    }
    
    GIVEN("A result with invalid index") {
        NeighborSearchResult result{
            .neighbor_indices = {0, -1, 10},  // -1 is invalid
            .is_truncated = false,
            .total_candidates_found = 3
        };
        
        WHEN("Checking validity") {
            THEN("Should be invalid") {
                REQUIRE_FALSE(result.is_valid());
            }
        }
    }
}
```

---

## Migration Strategy (TDD)

### Phase 1: Add New API (No Breaking Changes)

1. **Write tests first** (BDD scenarios above)
2. Implement `NeighborCollector` class
3. Implement `NeighborSearchResult` struct
4. Add new `find_neighbors()` method alongside old `neighbor_search()`
5. All tests pass ✅

### Phase 2: Migrate Callers

```cpp
// OLD (imperative)
std::vector<int> neighbor_list(m_neighbor_number * 20);
int n_neighbor = 0;
tree->neighbor_search(p_i, neighbor_list, n_neighbor, is_ij);

// NEW (declarative)
auto config = NeighborSearchConfig::create(m_neighbor_number, is_ij);
auto result = tree->find_neighbors(p_i, config);
// result.neighbor_indices is ready to use
// result.is_truncated tells if we hit limit
```

### Phase 3: Remove Old API

Once all callers migrated, delete old `neighbor_search()` method.

---

## Benefits of Refactoring

### ✅ Safety
- **Impossible to overflow** - Collector enforces bounds automatically
- **Type safety** - Config object validates parameters
- **RAII** - Resources managed automatically

### ✅ Testability
- **Pure functions** - Easy to unit test
- **No hidden state** - All inputs/outputs explicit
- **BDD scenarios** - Clear behavior specification

### ✅ Maintainability
- **Self-documenting** - Types express intent
- **Compiler-enforced** - Can't forget to check bounds
- **Easy to reason about** - Functional style, no mutation

### ✅ Performance
- **Same efficiency** - Pre-allocated vectors
- **Better caching** - Move semantics, no copies
- **Compiler optimizations** - `[[nodiscard]]` enables RVO

---

## Implementation Checklist

- [ ] Write BDD tests for `NeighborCollector`
- [ ] Implement `NeighborCollector` class
- [ ] Write BDD tests for `NeighborSearchResult`
- [ ] Implement `NeighborSearchResult` struct
- [ ] Write BDD tests for `NeighborSearchConfig`
- [ ] Implement `NeighborSearchConfig` struct
- [ ] Write BDD tests for new `find_neighbors()` API
- [ ] Implement new `find_neighbors()` in BHTree
- [ ] Migrate `g_fluid_force.tpp` to new API
- [ ] Migrate `g_pre_interaction.tpp` to new API
- [ ] Verify all tests pass
- [ ] Remove old `neighbor_search()` API
- [ ] Update documentation

---

## Example: Complete Usage

```cpp
// In g_fluid_force.tpp - AFTER refactoring

template<int Dim>
void FluidForce<Dim>::calculation(std::shared_ptr<Simulation<Dim>> sim) {
    auto & particles = sim->particles;
    const int num = sim->particle_num;
    auto * tree = sim->tree.get();
    
    // Create config once (declarative)
    const auto search_config = NeighborSearchConfig::create(
        this->m_neighbor_number, 
        /*is_ij=*/true
    );
    
#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        auto & p_i = particles[i];
        
        // DECLARATIVE: Get neighbors (cannot overflow by design)
        auto neighbors = tree->find_neighbors(p_i, search_config, sim->periodic);
        
        // Check if we hit limit (for diagnostics)
        if (neighbors.is_truncated) {
            #pragma omp critical
            {
                static bool warned = false;
                if (!warned) {
                    WRITE_LOG << "WARNING: Particle " << i 
                              << " has more neighbors than capacity ("
                              << neighbors.total_candidates_found << " > "
                              << search_config.max_neighbors << ")";
                    warned = true;
                }
            }
        }
        
        // Process neighbors (clean, functional)
        Vector<Dim> acc{};
        real dene = 0.0;
        
        for (int j : neighbors.neighbor_indices) {
            auto & p_j = search_particles[j];
            // ... calculate forces ...
        }
        
        p_i.acc = acc;
        p_i.dene = dene;
    }
}
```

**Compare to current imperative style:**
- ✅ No manual `n_neighbor` tracking
- ✅ No forgotten bounds checks
- ✅ Clear intent and behavior
- ✅ Compiler-enforced safety
- ✅ Easily testable

---

## Conclusion

The refactoring transforms:
- **Imperative → Declarative**: "What to do" instead of "how to do it"
- **Mutable → Immutable**: Values returned, not modified in place
- **Error-prone → Safe**: Impossible to overflow by design
- **Hard to test → Easy to test**: Pure functions with clear contracts
- **Implicit → Explicit**: All contracts expressed in types

**Next Steps:**
1. Review and approve this design
2. Set up Catch2 testing framework (if not already present)
3. Implement Phase 1 (new API) with TDD
4. Migrate callers incrementally
5. Remove old API once migration complete

This approach ensures **the heap buffer overflow bug category is eliminated by design**, not just fixed reactively.
