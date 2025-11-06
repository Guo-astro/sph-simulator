#include <ctime>

#include "output.hpp"
#include "logger.hpp"
#include "defines.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/simulation/simulation.hpp"
#include "core/output/output_coordinator.hpp"
#include "core/output/writers/csv_writer.hpp"
#include "core/output/units/unit_system_factory.hpp"

// Only include protobuf writer if proto files are available
#ifdef SPH_ENABLE_PROTOBUF
#include "core/output/writers/protobuf_writer.hpp"
#endif

namespace sph
{

template<int Dim>
Output<Dim>::Output(int count)
    : m_count(count)
    , unit_mode_(UnitConversionMode::CODE_UNITS)  // Default: no conversion
{
    // Coordinator will be created lazily on first output call
    // when we have access to the simulation parameters
    
    // Default: no unit conversion (code units) for backward compatibility
    // Users can change via set_unit_conversion()
    unit_system = nullptr;
    
    WRITE_LOG << "Output system initialized";
    WRITE_LOG << "Unit conversion mode: CODE_UNITS (default)";
}

template<int Dim>
Output<Dim>::~Output()
{
    // Coordinator will clean up automatically
}

template<int Dim>
void Output<Dim>::set_unit_conversion(UnitConversionMode mode)
{
    unit_mode_ = mode;
    
    // Create appropriate unit system based on mode
    switch (mode) {
        case UnitConversionMode::CODE_UNITS:
            unit_system = nullptr;  // No conversion
            WRITE_LOG << "Unit conversion mode set to: CODE_UNITS";
            break;
        case UnitConversionMode::GALACTIC_UNITS:
            unit_system = UnitSystemFactory::create(UnitSystemType::GALACTIC);
            WRITE_LOG << "Unit conversion mode set to: GALACTIC_UNITS";
            break;
        case UnitConversionMode::SI_UNITS:
            unit_system = UnitSystemFactory::create(UnitSystemType::SI);
            WRITE_LOG << "Unit conversion mode set to: SI_UNITS";
            break;
        case UnitConversionMode::CGS_UNITS:
            unit_system = UnitSystemFactory::create(UnitSystemType::CGS);
            WRITE_LOG << "Unit conversion mode set to: CGS_UNITS";
            break;
    }
    
    // If coordinator already exists, update its unit system
    if (coordinator) {
        coordinator->set_unit_system(unit_system);
    }
}

template<int Dim>
void Output<Dim>::initialize_coordinator(std::shared_ptr<Simulation<Dim>> sim)
{
    if (coordinator) return;  // Already initialized
    
    const std::string dir_name = Logger::get_dir_name();
    
    // Create the coordinator without parameters (legacy mode)
    coordinator = std::make_unique<OutputCoordinator<Dim>>(dir_name);
    
    // Add requested writers or default to CSV
    if (!pending_formats_.empty()) {
        for (const auto& format : pending_formats_) {
            switch (format) {
                case OutputFormat::CSV:
                    coordinator->add_writer(std::make_unique<CSVWriter<Dim>>(dir_name, false));
                    WRITE_LOG << "Added CSV writer";
                    break;
                case OutputFormat::PROTOBUF:
#ifdef SPH_ENABLE_PROTOBUF
                    coordinator->add_writer(std::make_unique<ProtobufWriter<Dim>>(dir_name));
                    WRITE_LOG << "Added Protobuf writer";
#else
                    WRITE_LOG << "Warning: Protobuf support not enabled, skipping Protobuf writer";
#endif
                    break;
            }
        }
    } else {
        // Default: add CSV writer for backward compatibility
        coordinator->add_writer(std::make_unique<CSVWriter<Dim>>(dir_name, false));
    }
    
    // Apply unit system
    coordinator->set_unit_system(unit_system);
    
    WRITE_LOG << "OutputCoordinator initialized";
    WRITE_LOG << "Output directory: " << dir_name;
}

template<int Dim>
void Output<Dim>::output_particle(std::shared_ptr<Simulation<Dim>> sim)
{
    initialize_coordinator(sim);
    coordinator->write_particles(sim);
    
    WRITE_LOG << "Snapshot " << m_count << " written at t=" << sim->time;
    ++m_count;
}

template<int Dim>
void Output<Dim>::output_energy(std::shared_ptr<Simulation<Dim>> sim)
{
    initialize_coordinator(sim);
    coordinator->write_energy(sim);
}

template<int Dim>
void Output<Dim>::set_unit_system(UnitSystemType type)
{
    unit_system = UnitSystemFactory::create(type);
    if (coordinator) {
        coordinator->set_unit_system(unit_system);
    }
    WRITE_LOG << "Unit system changed to: " << unit_system->get_name();
}

template<int Dim>
void Output<Dim>::set_output_formats(const std::vector<OutputFormat>& formats)
{
    // If coordinator exists, we need to preserve its simulation params
    // Otherwise, this will be applied when coordinator is initialized
    pending_formats_ = formats;
    
    if (coordinator) {
        // Clear and rebuild writers
        const std::string dir_name = Logger::get_dir_name();
        
        // Note: We can't access params here, so we need to store the simulation
        // reference in Output. For now, just clear and rebuild on next write.
        coordinator.reset();
    }
}

}
