#pragma once

#include <vector>
#include <cstddef>

namespace sph {

// Forward declaration - SPHParticle is defined as struct, not class
template<int Dim>
struct SPHParticle;

template<int Dim>
class NeighborAccessor;

/**
 * Tag types for compile-time differentiation of particle arrays.
 * These phantom types prevent accidental mixing of real-only and search (real + ghost) arrays.
 */
struct RealParticlesTag {};
struct SearchParticlesTag {};  // Real + Ghost particles

/**
 * Type-safe particle array wrapper using phantom type parameters.
 * 
 * Prevents array index space mismatch bugs by making RealParticleArray and SearchParticleArray
 * incompatible types at compile time.
 * 
 * Design rationale:
 * - Operator[] is private and only accessible by NeighborAccessor via friendship
 * - This enforces that neighbor indices can ONLY access SearchParticleArray
 * - Attempting to use wrong array type results in compile error, not runtime bug
 * 
 * @tparam Dim Spatial dimension (2 or 3)
 * @tparam ArrayTag Phantom type parameter (RealParticlesTag or SearchParticlesTag)
 */
template<int Dim, typename ArrayTag>
class TypedParticleArray {
private:
    std::vector<SPHParticle<Dim>>& m_particles;
    
public:
    /**
     * Construct typed wrapper around existing particle vector.
     * Wrapper does not own the data - reference semantics only.
     */
    explicit TypedParticleArray(std::vector<SPHParticle<Dim>>& particles)
        : m_particles(particles) {}
    
    // Allow copy construction for passing to functions
    TypedParticleArray(const TypedParticleArray&) = default;
    
    // Prevent assignment to avoid confusion about semantics
    TypedParticleArray& operator=(const TypedParticleArray&) = delete;
    
    /**
     * Get array size.
     * Public because size queries are safe.
     */
    std::size_t size() const { return m_particles.size(); }
    
    /**
     * Check if array is empty.
     */
    bool empty() const { return m_particles.empty(); }
    
private:
    /**
     * Private array access - only NeighborAccessor can use this.
     * 
     * This is the key safety mechanism: raw indexing is not publicly accessible.
     * Users must go through NeighborAccessor which enforces correct array type.
     */
    friend class NeighborAccessor<Dim>;
    
    const SPHParticle<Dim>& operator[](int idx) const {
        return m_particles[idx];
    }
    
    SPHParticle<Dim>& operator[](int idx) {
        return m_particles[idx];
    }
};

/**
 * Type alias for real particles only (no ghost particles).
 * Use this when you need to iterate over or update real particles.
 */
template<int Dim>
using RealParticleArray = TypedParticleArray<Dim, RealParticlesTag>;

/**
 * Type alias for search particles (real + ghost particles).
 * Neighbor search indices reference this array.
 * ALWAYS use NeighborAccessor to access elements by neighbor index.
 */
template<int Dim>
using SearchParticleArray = TypedParticleArray<Dim, SearchParticlesTag>;

}  // namespace sph
