#pragma once

#include <memory>
#include <vector>
#include <algorithm>
#include "vector.hpp"
#include "sph_particle.hpp"
#include "core/periodic.hpp"
#include "../defines.hpp"

namespace sph {

// Forward declaration
struct SPHParameters;

// Dimension-dependent child count
template<int Dim>
constexpr int nchild() {
    return Dim == 1 ? 2 : Dim == 2 ? 4 : 8;
}

template<int Dim>
class BHTree {
private:
    class BHNode {
    public:
        SPHParticle<Dim>* first;
        real mass;
        int num;
        BHNode* childs[nchild<Dim>()];
        Vector<Dim> center;
        Vector<Dim> m_center; // mass center
        real edge;
        int level;
        real kernel_size;
        bool is_leaf;

        void clear() {
            first = nullptr;
            mass = 0.0;
            num = 0;
            std::fill(childs, childs + nchild<Dim>(), nullptr);
            center = Vector<Dim>();  // Default constructor gives zero vector
            m_center = Vector<Dim>();
            edge = 0.0;
            level = 0;
            kernel_size = 0.0;
            is_leaf = false;
        }

        void root_clear() {
            first = nullptr;
            mass = 0.0;
            num = 0;
            std::fill(childs, childs + nchild<Dim>(), nullptr);
            m_center = Vector<Dim>();
            kernel_size = 0.0;
            is_leaf = false;
        }

        void create_tree(BHNode*& nodes, int& remaind, const int max_level, const int leaf_particle_num);
        void assign(SPHParticle<Dim>* particle, BHNode*& nodes, int& remaind);
        real set_kernel();
        void neighbor_search(const SPHParticle<Dim>& p_i, std::vector<int>& neighbor_list, 
                           int& n_neighbor, const int max_neighbors, const bool is_ij, const Periodic<Dim>* periodic);
        void calc_force(SPHParticle<Dim>& p_i, const real theta2, const real g_constant, 
                       const Periodic<Dim>* periodic);
    };

    int m_max_level;
    int m_leaf_particle_num;
    bool m_is_periodic;
    Vector<Dim> m_range_max;
    Vector<Dim> m_range_min;
    std::shared_ptr<Periodic<Dim>> m_periodic;
    BHNode m_root;
    std::shared_ptr<BHNode> m_nodes;
    int m_node_size;

    real m_g_constant;
    real m_theta;
    real m_theta2;
    // Pointer to the particle array used to build the tree. This ensures
    // neighbor_search sorts and bounds-check against the same container
    // that was passed to make(). It is set in make() and cleared when
    // the tree is rebuilt.
    const std::vector<SPHParticle<Dim>>* m_particles_ptr = nullptr;

public:
    void initialize(std::shared_ptr<SPHParameters> param);
    void resize(const int particle_num, const int tree_size = 5);
    void make(std::vector<SPHParticle<Dim>>& particles, const int particle_num);
    void set_kernel();
    int neighbor_search(const SPHParticle<Dim>& p_i, std::vector<int>& neighbor_list, 
                       const std::vector<SPHParticle<Dim>>& particles, const bool is_ij = false);
    void tree_force(SPHParticle<Dim>& p_i);
};

// Type aliases for convenience
using BHTree1D = BHTree<1>;
using BHTree2D = BHTree<2>;
using BHTree3D = BHTree<3>;

} // namespace sph

#include "bhtree.tpp"
