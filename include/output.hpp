#pragma once

#include <fstream>
#include <memory>
#include <vector>

#include "defines.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/simulation/simulation.hpp"
#include "core/parameters/output_parameters.hpp"

// Forward declarations
namespace sph {
    template<int Dim> class OutputCoordinator;
    class UnitSystem;
}

namespace sph
{

/**
 * @brief Unit conversion mode for output
 * 
 * Controls whether output values are converted to physical units or remain
 * in dimensionless code units.
 */
enum class UnitConversionMode {
    CODE_UNITS,     ///< No conversion - output raw simulation values (default)
    GALACTIC_UNITS, ///< Convert to Galactic units (pc, M_sol, km/s)
    SI_UNITS,       ///< Convert to SI units (m, kg, s)
    CGS_UNITS       ///< Convert to CGS units (cm, g, s)
};

/**
 * @brief Legacy Output class - now wraps OutputCoordinator
 * 
 * This class maintains backward compatibility while using the new
 * output system internally. It automatically creates an OutputCoordinator
 * with CSV writer by default.
 */
template<int Dim>
class Output {
    int m_count;
    std::unique_ptr<OutputCoordinator<Dim>> coordinator;
    std::shared_ptr<UnitSystem> unit_system;
    std::vector<OutputFormat> pending_formats_;
    UnitConversionMode unit_mode_;
    
    // Lazy initialization helper
    void initialize_coordinator(std::shared_ptr<Simulation<Dim>> sim);
    
public:
    Output(int count = 0);
    ~Output();
    void output_particle(std::shared_ptr<Simulation<Dim>> sim);
    void output_energy(std::shared_ptr<Simulation<Dim>> sim);
    
    // Unit conversion control (type-safe)
    void set_unit_conversion(UnitConversionMode mode);
    UnitConversionMode get_unit_conversion() const { return unit_mode_; }
    
    // Legacy methods for configuration
    void set_unit_system(UnitSystemType type);
    void set_output_formats(const std::vector<OutputFormat>& formats);
};

// Type aliases
using Output1D = Output<1>;
using Output2D = Output<2>;
using Output3D = Output<3>;

}

#include "output.tpp"
