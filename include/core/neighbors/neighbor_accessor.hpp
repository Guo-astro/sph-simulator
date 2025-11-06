#pragma once

#include "particle_array_types.hpp"
#include <stdexcept>
#include <string>

namespace sph {

/**
 * Strong type for neighbor indices.
 * 
 * Prevents accidental mixing of neighbor indices with arbitrary integers.
 * Forces explicit construction and prevents implicit conversions.
 * 
 * Design rationale:
 * - Explicit constructor: cannot accidentally assign raw int
 * - Deleted float/double constructors: prevent accidental numeric conversions
 * - Explicit operator(): must explicitly extract value when needed
 * 
 * Example:
 *   NeighborIndex idx{5};        // ✅ OK
 *   int val = idx();             // ✅ OK - explicit extraction
 *   NeighborIndex bad = 5;       // ❌ Compile error - no implicit conversion
 *   NeighborIndex bad2 = 5.0;    // ❌ Compile error - deleted constructor
 */
struct NeighborIndex {
    int value;
    
    /**
     * Explicit constructor from int.
     * Prevents implicit conversions: NeighborIndex idx = 5; won't compile.
     */
    explicit constexpr NeighborIndex(int v) : value(v) {}
    
    /**
     * Explicit value extraction.
     * Use as: int i = idx();
     */
    constexpr int operator()() const { return value; }
    
    /**
     * Deleted constructors prevent accidental numeric conversions.
     */
    NeighborIndex(double) = delete;
    NeighborIndex(float) = delete;
};

/**
 * Type-safe accessor for neighbor particles.
 * 
 * CRITICAL DESIGN: Only accepts SearchParticleArray (real + ghost particles).
 * Attempting to construct with RealParticleArray results in compile error.
 * 
 * This prevents the array index space mismatch bug:
 * - Neighbor indices are into the search space (real + ghost)
 * - Accessing real-only array with neighbor index = out of bounds bug
 * - Compile-time enforcement makes this bug impossible
 * 
 * Debug builds include bounds checking that throws exceptions.
 * Release builds optimize away the check for performance.
 * 
 * @tparam Dim Spatial dimension (2 or 3)
 */
template<int Dim>
class NeighborAccessor {
private:
    SearchParticleArray<Dim> m_search_particles;
    
public:
    /**
     * Constructor ONLY accepts SearchParticleArray.
     * 
     * This is the compile-time safety mechanism.
     * If you try to pass RealParticleArray, you get a deleted constructor error.
     */
    explicit NeighborAccessor(SearchParticleArray<Dim> search_particles)
        : m_search_particles(search_particles) {}
    
    /**
     * Type-safe neighbor access.
     * 
     * Takes NeighborIndex (not raw int) which forces caller to be explicit.
     * Returns const reference to prevent accidental modification of ghost particles.
     * 
     * In debug builds (NDEBUG not defined): bounds checking with exception
     * In release builds: no overhead, direct access
     * 
     * @param idx Neighbor index (must be explicitly constructed)
     * @return const reference to neighbor particle
     * @throws std::out_of_range if index out of bounds (debug builds only)
     */
    const SPHParticle<Dim>& get_neighbor(NeighborIndex idx) const {
        #ifndef NDEBUG
        if (idx.value < 0 || idx.value >= static_cast<int>(m_search_particles.size())) {
            throw std::out_of_range(
                "Neighbor index out of bounds: " + std::to_string(idx.value) + 
                " >= " + std::to_string(m_search_particles.size())
            );
        }
        #endif
        return m_search_particles[idx.value];
    }
    
    /**
     * Deleted constructor for RealParticleArray.
     * 
     * This causes a compile error if you try:
     *   NeighborAccessor<2> accessor{sim->get_real_particles()};  // ❌ ERROR
     * 
     * The correct usage is:
     *   NeighborAccessor<2> accessor{sim->get_search_particles()}; // ✅ OK
     */
    NeighborAccessor(RealParticleArray<Dim>) = delete;
    
    /**
     * Get total particle count in search space (real + ghost).
     */
    std::size_t particle_count() const { 
        return m_search_particles.size(); 
    }
    
    /**
     * Check if search space is empty.
     */
    bool empty() const {
        return m_search_particles.empty();
    }
};

}  // namespace sph
