#pragma once

#include "bhtree.hpp"
#include "../parameters.hpp"
#include "../exception.hpp"
#include "../openmp.hpp"
#include "periodic.hpp"
#include <cassert>
#include <algorithm>

namespace sph {

// Hernquist & Katz (1989) gravitational softening functions
inline real f(const real r, const real h) {
    const real e = h * 0.5;
    const real u = r / e;
    
    if (u < 1.0) {
        return (-0.5 * u * u * (1.0 / 3.0 - 3.0 / 20 * u * u + u * u * u / 20) + 1.4) / e;
    } else if (u < 2.0) {
        return -1.0 / (15 * r) + (-u * u * (4.0 / 3.0 - u + 0.3 * u * u - u * u * u / 30) + 1.6) / e;
    } else {
        return 1 / r;
    }
}

inline real g(const real r, const real h) {
    const real e = h * 0.5;
    const real u = r / e;
    
    if (u < 1.0) {
        return (4.0 / 3.0 - 1.2 * u * u + 0.5 * u * u * u) / (e * e * e);
    } else if (u < 2.0) {
        return (-1.0 / 15 + 8.0 / 3 * u * u * u - 3 * u * u * u * u + 1.2 * u * u * u * u * u - u * u * u * u * u * u / 6.0) / (r * r * r);
    } else {
        return 1 / (r * r * r);
    }
}

// ============================================================================
// BHTree<Dim> implementation
// ============================================================================

template<int Dim>
void BHTree<Dim>::initialize(std::shared_ptr<SPHParameters> param) {
    m_max_level = param->tree.max_level;
    m_leaf_particle_num = param->tree.leaf_particle_num;
    m_root.clear();
    m_root.level = 1;
    m_is_periodic = param->periodic.is_valid;
    
    if (m_is_periodic) {
        for (int i = 0; i < Dim; ++i) {
            m_range_max[i] = param->periodic.range_max[i];
            m_range_min[i] = param->periodic.range_min[i];
        }
        m_root.center = (m_range_max + m_range_min) * 0.5;
        auto range = m_range_max - m_range_min;
        real l = 0.0;
        for (int i = 0; i < Dim; ++i) {
            if (l < range[i]) {
                l = range[i];
            }
        }
        m_root.edge = l;
    }
    
    m_periodic = std::make_shared<Periodic<Dim>>();
    m_periodic->initialize(param);

    if (param->gravity.is_valid) {
        m_g_constant = param->gravity.constant;
        m_theta = param->gravity.theta;
        m_theta2 = m_theta * m_theta;
    }
}

template<int Dim>
void BHTree<Dim>::resize(const int particle_num, const int tree_size) {
    assert(m_nodes.get() == nullptr);

    m_node_size = particle_num * tree_size;
    m_nodes = std::shared_ptr<BHNode>(new BHNode[m_node_size], std::default_delete<BHNode[]>());

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < m_node_size; ++i) {
        m_nodes.get()[i].clear();
    }
}

template<int Dim>
void BHTree<Dim>::make(std::vector<SPHParticle<Dim>>& particles, const int particle_num) {
    m_root.root_clear();
    // Clear stored particle pointer until the tree is fully rebuilt
    m_particles_ptr = nullptr;
    
    // Clear all nodes from previous tree build to prevent stale pointers
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < m_node_size; ++i) {
        m_nodes.get()[i].clear();
    }

    if (!m_is_periodic) {
        omp_real r_min[Dim];
        omp_real r_max[Dim];
        for (int i = 0; i < Dim; ++i) {
            r_min[i].get() = std::numeric_limits<real>::max();
            r_max[i].get() = std::numeric_limits<real>::lowest();
        }

#ifdef _OPENMP
#pragma omp parallel for
#endif
        for (int i = 0; i < particle_num; ++i) {
            auto& r_i = particles[i].pos;
            for (int j = 0; j < Dim; ++j) {
                if (r_min[j].get() > r_i[j]) {
                    r_min[j].get() = r_i[j];
                }
                if (r_max[j].get() < r_i[j]) {
                    r_max[j].get() = r_i[j];
                }
            }
        }

        Vector<Dim> range_min, range_max;
        for (int i = 0; i < Dim; ++i) {
            range_min[i] = r_min[i].min();
            range_max[i] = r_max[i].max();
        }

        m_root.center = (range_max + range_min) * 0.5;
        auto range = range_max - range_min;
        real l = 0.0;
        for (int i = 0; i < Dim; ++i) {
            if (l < range[i]) {
                l = range[i];
            }
        }
        m_root.edge = l;
    }

#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < particle_num - 1; ++i) {
        particles[i].next = &particles[i + 1];
    }
    particles[particle_num - 1].next = nullptr;
    m_root.first = &particles[0];

    int remaind = m_node_size;
    auto* p = m_nodes.get();
    m_root.create_tree(p, remaind, m_max_level, m_leaf_particle_num);
    // Record the pointer to the particle vector used to build the tree so
    // neighbor_search uses the exact same container for sorting and bounds
    // checking. This avoids races where callers pass a different vector.
    m_particles_ptr = &particles;
}

template<int Dim>
void BHTree<Dim>::set_kernel() {
    m_root.set_kernel();
}

/**
 * @brief Find neighbors using declarative API (REFACTORED - old neighbor_search removed)
 * 
 * This is the new implementation that replaces the imperative neighbor_search.
 * Key improvements:
 * - No manual bounds checking needed (NeighborCollector handles it)
 * - Returns immutable result value
 * - Impossible to overflow by design
 * - Self-validating indices
 * - Automatic distance-based sorting
 */
template<int Dim>
NeighborSearchResult BHTree<Dim>::find_neighbors(const SPHParticle<Dim>& p_i,
                                                  const NeighborSearchConfig& config) {
    // Create RAII collector with capacity enforcement
    NeighborCollector collector(config.max_neighbors);
    
    // Recursive tree traversal - collector prevents overflow automatically
    m_root.find_neighbors_recursive(p_i, collector, config, m_periodic.get());
    
    // Extract result with move semantics
    auto result = std::move(collector).finalize();
    
    // Use the particle array pointer recorded during make() for validation & sorting
    if (!m_particles_ptr) {
        WRITE_LOG << "ERROR: find_neighbors called before make() - no particle array available";
        return result;  // Return unsorted, caller should check
    }
    
    // Validate all neighbor indices are within bounds
    const int max_valid_index = static_cast<int>(m_particles_ptr->size()) - 1;
    std::vector<int> valid_indices;
    valid_indices.reserve(result.neighbor_indices.size());
    
    for (int idx : result.neighbor_indices) {
        if (idx >= 0 && idx <= max_valid_index) {
            valid_indices.push_back(idx);
        } else {
            #pragma omp critical
            {
                static bool warning_logged = false;
                if (!warning_logged) {
                    WRITE_LOG << "WARNING: find_neighbors found invalid index " << idx 
                              << " (max=" << max_valid_index << "), filtering out";
                    warning_logged = true;
                }
            }
        }
    }
    
    // Sort neighbors by distance (closest first)
    const auto& pos_i = p_i.pos;
    std::sort(valid_indices.begin(), valid_indices.end(), 
              [&](const int a, const int b) {
                  const Vector<Dim> r_ia = m_periodic->calc_r_ij(pos_i, (*m_particles_ptr)[a].pos);
                  const Vector<Dim> r_ib = m_periodic->calc_r_ij(pos_i, (*m_particles_ptr)[b].pos);
                  return abs2(r_ia) < abs2(r_ib);
              });
    
    // Return final result with sorted, validated indices
    return NeighborSearchResult{
        .neighbor_indices = std::move(valid_indices),
        .is_truncated = result.is_truncated,
        .total_candidates_found = result.total_candidates_found
    };
}

template<int Dim>
void BHTree<Dim>::tree_force(SPHParticle<Dim>& p_i) {
    p_i.phi = 0.0;
    m_root.calc_force(p_i, m_theta2, m_g_constant, m_periodic.get());
}

// ============================================================================
// BHNode implementation
// ============================================================================

template<int Dim>
void BHTree<Dim>::BHNode::create_tree(BHNode*& nodes, int& remaind, const int max_level, const int leaf_particle_num) {
    std::fill(childs, childs + nchild<Dim>(), nullptr);

    auto* pp = first;
    do {
        auto* pnext = pp->next;
        assign(pp, nodes, remaind);
        pp = pnext;
    } while (pp != nullptr);

    // Process child nodes
    for (int i = 0; i < nchild<Dim>(); ++i) {
        auto* child = childs[i];
        if (child) {
            child->m_center /= child->mass;

            if (child->num > leaf_particle_num && level < max_level) {
                child->create_tree(nodes, remaind, max_level, leaf_particle_num);
            } else {
                child->is_leaf = true;
            }
        }
    }
}

template<int Dim>
void BHTree<Dim>::BHNode::assign(SPHParticle<Dim>* particle, BHNode*& nodes, int& remaind) {
    auto& p_i = *particle;
    const auto& pos = p_i.pos;

    int index = 0;
    int mask = 1;
    for (int i = 0; i < Dim; ++i) {
        if (pos[i] > center[i]) {
            index |= mask;
        }
        mask <<= 1;
    }

    auto* child = childs[index];
    if (!child) {
        if (remaind < 0) {
            THROW_ERROR("There is no free node.");
        }
        childs[index] = nodes;
        child = childs[index];
        ++nodes;
        --remaind;
        child->clear();
        child->level = level + 1;
        child->edge = edge * 0.5;

        int a = 1;
        real b = 2.0;
        for (int i = 0; i < Dim; ++i) {
            child->center[i] = center[i] + ((index & a) * b - 1.0) * edge * 0.25;
            a <<= 1;
            b *= 0.5;
        }
    }

    child->num++;
    child->mass += p_i.mass;
    child->m_center += pos * p_i.mass;
    p_i.next = child->first;
    child->first = particle;
}

template<int Dim>
real BHTree<Dim>::BHNode::set_kernel() {
    real kernel = 0.0;
    if (is_leaf) {
        auto* p = first;
        while (p) {
            const real h = p->sml;
            if (h > kernel) {
                kernel = h;
            }
            p = p->next;
        }
    } else {
        for (int i = 0; i < nchild<Dim>(); ++i) {
            auto* child = childs[i];
            if (child) {
                const real h = child->set_kernel();
                if (h > kernel) {
                    kernel = h;
                }
            }
        }
    }

    kernel_size = kernel;
    return kernel_size;
}

/**
 * @brief Recursive neighbor search using declarative collector (REFACTORED)
 * 
 * This method replaces the old imperative neighbor_search that used manual bounds checking.
 * Key improvements:
 * - Uses NeighborCollector::try_add() which enforces bounds automatically
 * - Early exit when collector is full (performance optimization)
 * - No possibility of buffer overflow
 * - Cleaner logic without manual n_neighbor tracking
 */
template<int Dim>
void BHTree<Dim>::BHNode::find_neighbors_recursive(const SPHParticle<Dim>& p_i,
                                                    NeighborCollector& collector,
                                                    const NeighborSearchConfig& config,
                                                    const Periodic<Dim>* periodic) {
    // Early exit if collector is full (optimization)
    if (collector.is_full()) {
        return;
    }
    
    // Check if this node is within search radius
    const Vector<Dim>& r_i = p_i.pos;
    const real h = config.use_max_kernel ? std::max(p_i.sml, kernel_size) : p_i.sml;
    const real h2 = h * h;
    const real l2 = sqr(edge * 0.5 + h);
    const Vector<Dim> d = periodic->calc_r_ij(r_i, center);
    
    real dx2_max = sqr(d[0]);
    for (int i = 1; i < Dim; ++i) {
        const real dx2 = sqr(d[i]);
        if (dx2 > dx2_max) {
            dx2_max = dx2;
        }
    }
    
    // Node is too far, skip
    if (dx2_max > l2) {
        return;
    }
    
    // Node is within range, process it
    if (is_leaf) {
        // Leaf node: check each particle
        auto* p = first;
        while (p) {
            const Vector<Dim>& r_j = p->pos;
            const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, r_j);
            const real r2 = abs2(r_ij);
            
            if (r2 < h2) {
                // Particle is within kernel radius - try to add
                // Collector handles bounds checking automatically
                if (!collector.try_add(p->id)) {
                    // Capacity reached - stop searching this branch
                    return;
                }
            }
            p = p->next;
        }
    } else {
        // Internal node: recurse into children
        for (int i = 0; i < nchild<Dim>(); ++i) {
            if (childs[i]) {
                childs[i]->find_neighbors_recursive(p_i, collector, config, periodic);
                
                // Early exit if collector filled during recursion
                if (collector.is_full()) {
                    return;
                }
            }
        }
    }
}

template<int Dim>
void BHTree<Dim>::BHNode::calc_force(SPHParticle<Dim>& p_i, const real theta2, const real g_constant,
                                      const Periodic<Dim>* periodic) {
    const Vector<Dim>& r_i = p_i.pos;
    const real l2 = edge * edge;
    const Vector<Dim> d = periodic->calc_r_ij(r_i, m_center);
    const real d2 = abs2(d);

    if (l2 > theta2 * d2) {
        if (is_leaf) {
            auto* p = first;
            while (p) {
                const Vector<Dim>& r_j = p->pos;
                const Vector<Dim> r_ij = periodic->calc_r_ij(r_i, r_j);
                const real r = abs(r_ij);
                p_i.phi -= g_constant * p->mass * (f(r, p_i.sml) + f(r, p->sml)) * 0.5;
                p_i.acc -= r_ij * (g_constant * p->mass * (g(r, p_i.sml) + g(r, p->sml)) * 0.5);
                p = p->next;
            }
        } else {
            for (int i = 0; i < nchild<Dim>(); ++i) {
                if (childs[i]) {
                    childs[i]->calc_force(p_i, theta2, g_constant, periodic);
                }
            }
        }
    } else {
        const real r_inv = 1.0 / std::sqrt(d2);
        p_i.phi -= g_constant * mass * r_inv;
        p_i.acc -= d * (g_constant * mass * pow3(r_inv));
    }
}

// Explicit template instantiations
template class BHTree<1>;
template class BHTree<2>;
template class BHTree<3>;

} // namespace sph
