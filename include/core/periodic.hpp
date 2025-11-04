#pragma once

#include <memory>
#include "vector.hpp"
#include "../defines.hpp"
#include "../parameters.hpp"

namespace sph {

template<int Dim>
class Periodic {
    bool m_is_valid;
    Vector<Dim> m_max;
    Vector<Dim> m_min;
    Vector<Dim> m_range;

public:
    Periodic() : m_is_valid(false) {}
    
    void initialize(std::shared_ptr<SPHParameters> param) {
        if (param->periodic.is_valid) {
            m_is_valid = true;
            for (int i = 0; i < Dim; ++i) {
                m_max[i] = param->periodic.range_max[i];
                m_min[i] = param->periodic.range_min[i];
            }
            m_range = m_max - m_min;
        } else {
            m_is_valid = false;
        }
    }

    Vector<Dim> calc_r_ij(const Vector<Dim>& r_i, const Vector<Dim>& r_j) const {
        if (m_is_valid) {
            Vector<Dim> r_ij = r_i - r_j;
            for (int i = 0; i < Dim; ++i) {
                if (r_ij[i] > m_range[i] * 0.5) {
                    r_ij[i] -= m_range[i];
                } else if (r_ij[i] < -m_range[i] * 0.5) {
                    r_ij[i] += m_range[i];
                }
            }
            return r_ij;
        } else {
            return r_i - r_j;
        }
    }

    void apply_periodic_condition(Vector<Dim>& pos) const {
        if (m_is_valid) {
            for (int i = 0; i < Dim; ++i) {
                if (pos[i] < m_min[i]) {
                    pos[i] += m_range[i];
                } else if (pos[i] > m_max[i]) {
                    pos[i] -= m_range[i];
                }
            }
        }
    }

    bool is_valid() const { return m_is_valid; }
    const Vector<Dim>& max() const { return m_max; }
    const Vector<Dim>& min() const { return m_min; }
    const Vector<Dim>& range() const { return m_range; }
};

// Type aliases for convenience
using Periodic1D = Periodic<1>;
using Periodic2D = Periodic<2>;
using Periodic3D = Periodic<3>;

} // namespace sph
