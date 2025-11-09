#include <cassert>
#include <iostream>
#include <chrono>
#include <cmath>

#include "solver.hpp"
#include "parameters.hpp"
#include "core/particles/sph_particle.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "output.hpp"
#include "core/simulation/simulation.hpp"
#include "core/boundaries/periodic.hpp"
#include "core/spatial/bhtree.hpp"
#include "core/plugins/plugin_loader.hpp"
#include "core/plugins/simulation_plugin_v3.hpp"
#include "core/algorithms/sph_algorithm_registry.hpp"

// SPH modules
#include "timestep.hpp"
#include "pre_interaction.hpp"
#include "fluid_force.hpp"
#include "gravity_force.hpp"
#include "disph/d_pre_interaction.hpp"
#include "disph/d_fluid_force.hpp"
#include "gsph/g_pre_interaction.hpp"
#include "gsph/g_fluid_force.hpp"

#ifdef _OPENMP
#include <omp.h>
#endif

namespace sph
{

template<int Dim>
Solver<Dim>::Solver(int argc, char * argv[])
{
    std::cout << "--------------SPH simulation-------------\n\n";
    
    // Validate arguments
    if(argc == 1) {
        std::cerr << "Usage: sph <plugin.dylib|.so|.dll> [num_threads]\n" << std::endl;
        std::cerr << "Arguments:" << std::endl;
        std::cerr << "  plugin     - Path to simulation plugin (.dylib/.so/.dll)" << std::endl;
        std::cerr << "  num_threads - (optional) Number of OpenMP threads to use" << std::endl;
        std::cerr << "\nExamples:" << std::endl;
        std::cerr << "  sph shock_tube.dylib           # Use default thread count" << std::endl;
        std::cerr << "  sph shock_tube.dylib 4         # Use 4 threads" << std::endl;
        std::cerr << "\nNote: All configuration is now in the plugin C++ file." << std::endl;
        std::cerr << "JSON configuration files are no longer supported." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    // First argument must be a plugin
    std::string first_arg = argv[1];
    bool is_plugin = (first_arg.find(".dylib") != std::string::npos ||
                      first_arg.find(".so") != std::string::npos ||
                      first_arg.find(".dll") != std::string::npos);
    
    if (!is_plugin) {
        std::cerr << "Error: First argument must be a plugin file (.dylib/.so/.dll)" << std::endl;
        std::cerr << "All configuration is now in the plugin source file." << std::endl;
        std::cerr << "JSON configuration files are no longer supported." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    // Load plugin
    m_plugin_path = first_arg;
    load_plugin();
    
    // Initialize parameters (will be configured by plugin)
    m_param = std::make_shared<SPHParameters>();
    
    // Set output directory (relative to executable, goes up to workflow root)
    m_output_dir = "../output/" + m_plugin->get_name();
    
    Logger::open(m_output_dir);

#ifdef _OPENMP
    WRITE_LOG << "Open MP is valid.";
    int num_threads;
    if(argc >= 3) {  // plugin threads
        // Validate that argv[2] is a valid number
        char* end_ptr = nullptr;
        long parsed_threads = std::strtol(argv[2], &end_ptr, 10);
        
        // Check if parsing was successful and the value is valid
        if (end_ptr == argv[2] || *end_ptr != '\0') {
            std::cerr << "Error: Second argument must be a number (thread count), got: " 
                      << argv[2] << std::endl;
            std::cerr << "Usage: sph <plugin> [num_threads]" << std::endl;
            std::exit(EXIT_FAILURE);
        }
        
        if (parsed_threads <= 0) {
            std::cerr << "Error: Thread count must be positive, got: " 
                      << parsed_threads << std::endl;
            std::exit(EXIT_FAILURE);
        }
        
        num_threads = static_cast<int>(parsed_threads);
        omp_set_num_threads(num_threads);
    } else {
        num_threads = omp_get_max_threads();
    }
    
    // Additional safety check
    if (num_threads <= 0) {
        std::cerr << "Error: Invalid OpenMP thread count: " << num_threads << std::endl;
        std::cerr << "OpenMP environment may not be properly configured." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    WRITE_LOG << "the number of threads = " << num_threads << "\n";
#else
    WRITE_LOG << "OpenMP is invalid.\n";
#endif

    // NOTE: Parameter logging moved to log_parameters() method
    // which is called after plugin configures parameters in make_initial_condition()

    // Display plugin information
    WRITE_LOG << "Simulation: " << m_plugin->get_name();
    WRITE_LOG << "Description: " << m_plugin->get_description();
    WRITE_LOG << "Version: " << m_plugin->get_version();

    m_output = std::make_shared<Output<Dim>>();
}

template<int Dim>
Solver<Dim>::~Solver()
{
    // Explicitly destroy all members that might contain references to plugin code
    // BEFORE m_plugin_loader's destructor calls dlclose().
    // This prevents accessing unloaded shared library code during member destruction.
    
    // Destroy plugin first (while library is still loaded)
    m_plugin.reset();
    
    // Destroy SPH modules (they might reference plugin-allocated data)
    m_gforce.reset();
    m_fforce.reset();
    m_pre.reset();
    m_timestep.reset();
    
    // Destroy simulation state (might contain plugin-allocated particles or callbacks)
    m_sim.reset();
    
    // Destroy output manager (might have references to simulation data)
    m_output.reset();
    
    // Destroy parameters (created by plugin configuration)
    m_param.reset();
    
    // Now m_plugin_loader can safely be destroyed (calls dlclose())
    // All objects that might reference plugin code have been cleaned up
}

template<int Dim>
void Solver<Dim>::run()
{
    initialize();

    const real t_end = m_param->get_time().end;
    real t_out = m_param->get_time().output;
    real t_ene = m_param->get_time().energy;

    m_output->output_particle(m_sim);
    m_output->output_energy(m_sim);

    WRITE_LOG_ALWAYS << "Starting simulation: t=0.0 -> t=" << t_end;

    const auto start = std::chrono::system_clock::now();
    auto t_cout_i = start;
    int loop = 0;

    real t = m_sim->time;
    while(t < t_end) {
        integrate();
        const real dt = m_sim->dt;
        const int num = m_sim->particle_num;
        ++loop;

        m_sim->update_time();
        t = m_sim->time;
        
        // Print status every second
        const auto t_cout_f = std::chrono::system_clock::now();
        const real t_cout_s = std::chrono::duration_cast<std::chrono::seconds>(t_cout_f - t_cout_i).count();
        if(t_cout_s >= 1.0) {
            WRITE_LOG_ALWAYS << "Progress: loop=" << loop << ", t=" << t << ", dt=" << dt;
            t_cout_i = std::chrono::system_clock::now();
        } else {
            WRITE_LOG_ONLY << "loop: " << loop << ", time: " << t << ", dt: " << dt << ", num: " << num;
        }

        if(t > t_out) {
            m_output->output_particle(m_sim);
            t_out += m_param->get_time().output;
        }

        if(t > t_ene) {
            m_output->output_energy(m_sim);
            t_ene += m_param->get_time().energy;
        }
    }
    const auto end = std::chrono::system_clock::now();
    const real calctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    WRITE_LOG_ALWAYS << "\nSimulation completed successfully";
    WRITE_LOG_ALWAYS << "Total calculation time: " << calctime / 1000.0 << " seconds";
}

template<int Dim>
void Solver<Dim>::initialize()
{
    // Step 1: Create initial condition from plugin (configures m_param)
    if (!m_plugin) {
        THROW_ERROR("No plugin loaded. Plugin is required for simulation.");
    }
    
    WRITE_LOG_ALWAYS << "Initializing simulation from plugin: " << m_plugin->get_name();
    
    // ===== V3 INTERFACE: Plugin returns InitialCondition data =====
    auto initial_condition = m_plugin->create_initial_condition();
    
    // Validate initial condition
    if (!initial_condition.is_valid()) {
        THROW_ERROR("Plugin returned invalid initial condition (no particles or parameters)");
    }
    
    WRITE_LOG_ALWAYS << "Plugin returned " << initial_condition.particle_count() << " particles";
    
    // Step 2: Apply SPH parameters BEFORE creating Simulation
    // Use std::move to transfer ownership and avoid copy issues with complex members
    if (initial_condition.parameters) {
        m_param = std::move(initial_condition.parameters);
        WRITE_LOG << "Parameters applied from plugin";
    } else {
        THROW_ERROR("Plugin did not provide parameters");
    }
    
    // Step 3: NOW create Simulation with properly configured parameters
    m_sim = std::make_shared<Simulation<Dim>>(m_param);
    
    // Step 4: Transfer particles to simulation
    m_sim->particles = std::move(initial_condition.particles);
    m_sim->particle_num = static_cast<int>(m_sim->particles.size());
    
    // Step 5: Configure boundaries
    if (initial_condition.boundary_config) {
        m_sim->ghost_manager->initialize(*initial_condition.boundary_config);
        WRITE_LOG << "Boundary configuration applied from plugin";
    }
    
    WRITE_LOG << "Plugin initialization complete";
    
    // ✅ CRITICAL: Re-initialize tree with gravity parameters set by plugin
    // The tree was initially created with default NoGravity in Simulation constructor
    // After plugin modifies parameters, we must re-initialize to get correct G and theta
    m_sim->tree->initialize(m_param);
    
#ifndef NDEBUG
    WRITE_LOG << ">>> Tree re-initialized with plugin parameters";
#endif
    
    // Log parameters AFTER plugin has configured them
    log_parameters();

    m_timestep = std::make_shared<TimeStep<Dim>>();
    
    if(m_param->get_type() == SPHType::SSPH) {
        m_pre = std::make_shared<PreInteraction<Dim>>();
        m_fforce = std::make_shared<FluidForce<Dim>>();
    } else if(m_param->get_type() == SPHType::DISPH) {
        m_pre = std::make_shared<disph::PreInteraction<Dim>>();
        m_fforce = std::make_shared<disph::FluidForce<Dim>>();
    } else if(m_param->get_type() == SPHType::GSPH) {
        m_pre = std::make_shared<gsph::PreInteraction<Dim>>();
        m_fforce = std::make_shared<gsph::FluidForce<Dim>>();
    }
    
    m_gforce = std::make_shared<GravityForce<Dim>>();

    // GSPH - only add gradient arrays if 2nd order is enabled
    if(m_param->get_type() == SPHType::GSPH && m_param->get_gsph().is_2nd_order) {
        std::vector<std::string> names;
        names.push_back("grad_density");
        names.push_back("grad_pressure");
        names.push_back("grad_velocity_0");
        
        // Add gradient components based on dimension
        if constexpr (Dim >= 2) {
            names.push_back("grad_velocity_1");
        }
        if constexpr (Dim >= 3) {
            names.push_back("grad_velocity_2");
        }
        
        m_sim->add_vector_array(names);
    }

    m_timestep->initialize(m_param);
    m_pre->initialize(m_param);
    m_fforce->initialize(m_param);
    m_gforce->initialize(m_param);

    // Ghost particle system is initialized by plugins or can be configured here
    // Plugins handle their own boundary conditions - no additional initialization needed
    // The ghost_manager is already created in Simulation constructor

    auto & p = m_sim->particles;
    const int num = m_sim->particle_num;
    
    const real gamma = m_param->get_physics().gamma;
    const real c_sound = gamma * (gamma - 1.0);

    assert(p.size() == num);
    const real alpha = m_param->get_av().alpha;
#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        p[i].alpha = alpha;
        p[i].balsara = 1.0;
        p[i].sound = std::sqrt(c_sound * p[i].ene);
    }

    // Initialize particle cache for neighbor search
    m_sim->sync_particle_cache();
    
    // Build initial spatial tree
    auto tree = m_sim->tree;
    tree->resize(num);
    tree->make(m_sim->cached_search_particles, num);

    // Calculate initial densities and smoothing lengths
    m_pre->calculation(m_sim);
    
    // CRITICAL: Sync cache after pre_interaction updates densities
    // This ensures fluid_force reads correct neighbor densities
    m_sim->sync_particle_cache();
    
    // Calculate initial forces
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
    
    // Generate ghost particles AFTER all force calculations are complete
    // This ensures densities, pressures, and accelerations are all computed
    // before creating ghost particles
    if (m_sim->ghost_manager && m_sim->ghost_manager->get_config().is_valid) {
        // Find maximum smoothing length among real particles
        real max_sml = 0.0;
        for (int i = 0; i < num; ++i) {
            if (p[i].sml > max_sml) {
                max_sml = p[i].sml;
            }
        }
        
        // Set kernel support radius based on actual maximum smoothing length
        // For cubic spline kernel, support is 2h
        const real kernel_support = 2.0 * max_sml;
        m_sim->ghost_manager->set_kernel_support_radius(kernel_support);
        
        // Generate ghost particles
        m_sim->ghost_manager->generate_ghosts(p);
        
        WRITE_LOG << "Ghost particle system initialized:";
        WRITE_LOG << "* Max smoothing length = " << max_sml;
        WRITE_LOG << "* Kernel support radius = " << kernel_support;
        WRITE_LOG << "* Generated " << m_sim->ghost_manager->get_ghost_count() 
                  << " ghost particles";
        
        // Extend cache with ghost particles (declarative API)
        m_sim->extend_cache_with_ghosts();

        // Rebuild spatial tree with combined particles
        m_sim->make_tree();
    }
    
    // CRITICAL: Calculate initial timestep after forces are computed
    // This ensures accelerations are valid before computing dt
    m_timestep->calculation(m_sim);
}

template<int Dim>
void Solver<Dim>::integrate()
{
    m_timestep->calculation(m_sim);

    predict();
    
    // Regenerate ghost particles at new positions
    if (m_sim->ghost_manager) {
        m_sim->ghost_manager->regenerate_ghosts(m_sim->particles);
        m_sim->extend_cache_with_ghosts();
    }
    
    m_sim->make_tree();
    m_pre->calculation(m_sim);
    m_sim->sync_particle_cache();
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
    correct();
}

template<int Dim>
void Solver<Dim>::predict()
{
    auto & p = m_sim->particles;
    const int num = m_sim->particle_num;
    auto * periodic = m_sim->periodic.get();
    const real dt = m_sim->dt;
    const real gamma = m_param->get_physics().gamma;
    const real c_sound = gamma * (gamma - 1.0);

    assert(p.size() >= static_cast<size_t>(num));

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        // k -> k+1/2
        p[i].vel_p = p[i].vel + p[i].acc * (0.5 * dt);
        p[i].ene_p = p[i].ene + p[i].dene * (0.5 * dt);

        // k -> k+1
        p[i].pos += p[i].vel_p * dt;
        p[i].vel += p[i].acc * dt;
        p[i].ene += p[i].dene * dt;
        p[i].sound = std::sqrt(c_sound * p[i].ene);

        periodic->apply_periodic_condition(p[i].pos);
    }
    
    // Apply periodic wrapping
    if (m_sim->ghost_manager) {
        m_sim->ghost_manager->apply_periodic_wrapping(p);
    }
}

template<int Dim>
void Solver<Dim>::correct()
{
    auto & p = m_sim->particles;
    const int num = m_sim->particle_num;
    const real dt = m_sim->dt;
    const real gamma = m_param->get_physics().gamma;
    const real c_sound = gamma * (gamma - 1.0);

    assert(p.size() >= static_cast<size_t>(num));

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        p[i].vel = p[i].vel_p + p[i].acc * (0.5 * dt);
        p[i].ene = p[i].ene_p + p[i].dene * (0.5 * dt);
        p[i].sound = std::sqrt(c_sound * p[i].ene);
    }
}



template<int Dim>
void Solver<Dim>::log_parameters()
{
    WRITE_LOG << "parameters";
    WRITE_LOG << "output directory     = " << m_output_dir;

    WRITE_LOG << "time";
    WRITE_LOG << "* start time         = " << m_param->get_time().start;
    WRITE_LOG << "* end time           = " << m_param->get_time().end;
    WRITE_LOG << "* output time        = " << m_param->get_time().output;
    WRITE_LOG << "* energy output time = " << m_param->get_time().energy;

    switch(m_param->get_type()) {
    case SPHType::SSPH:
        WRITE_LOG << "SPH type: Standard SPH";
        break;
    case SPHType::DISPH:
        WRITE_LOG << "SPH type: Density Independent SPH";
        break;
    case SPHType::GSPH:
        if(m_param->get_gsph().is_2nd_order) {
            WRITE_LOG << "SPH type: Godunov SPH (2nd order)";
        } else {
            WRITE_LOG << "SPH type: Godunov SPH (1st order)";
        }
        break;
    }

    WRITE_LOG << "CFL condition";
    WRITE_LOG << "* sound speed = " << m_param->get_cfl().sound;
    WRITE_LOG << "* force       = " << m_param->get_cfl().force;

    WRITE_LOG << "Artificial Viscosity";
    WRITE_LOG << "* alpha = " << m_param->get_av().alpha;
    if(m_param->get_av().use_balsara_switch) {
        WRITE_LOG << "* use Balsara switch";
    }
    if(m_param->get_av().use_time_dependent_av) {
        WRITE_LOG << "* use time dependent AV";
        WRITE_LOG << "  * alpha max = " << m_param->get_av().alpha_max;
        WRITE_LOG << "  * alpha min = " << m_param->get_av().alpha_min;
        WRITE_LOG << "  * epsilon   = " << m_param->get_av().epsilon;
    }

    if(m_param->get_ac().is_valid) {
        WRITE_LOG << "Artificial Conductivity";
        WRITE_LOG << "* alpha = " << m_param->get_ac().alpha;
    }

    WRITE_LOG << "Tree";
    WRITE_LOG << "* max tree level       = " << m_param->get_tree().max_level;
    WRITE_LOG << "* leaf particle number = " << m_param->get_tree().leaf_particle_num;

    WRITE_LOG << "Physics";
    WRITE_LOG << "* Neighbor number = " << m_param->get_physics().neighbor_number;
    WRITE_LOG << "* gamma           = " << m_param->get_physics().gamma;

    WRITE_LOG << "Kernel";
    switch(m_param->get_kernel()) {
    case KernelType::CUBIC_SPLINE:
        WRITE_LOG << "* Cubic Spline";
        break;
    case KernelType::WENDLAND:
        WRITE_LOG << "* Wendland";
        break;
    case KernelType::UNKNOWN:
        WRITE_LOG << "* Unknown";
        break;
    }

    if(m_param->get_iterative_sml()) {
        WRITE_LOG << "Iterative calculation for smoothing length is valid.";
    }

    if(m_param->get_periodic().is_valid) {
        WRITE_LOG << "Periodic boundary condition is valid.";
    }

    if(m_param->has_gravity()) {
        WRITE_LOG << "Gravity is valid.";
        WRITE_LOG << "* G     = " << m_param->get_newtonian_gravity().constant;
        WRITE_LOG << "* theta = " << m_param->get_newtonian_gravity().theta;
    }
}

template<int Dim>
void Solver<Dim>::load_plugin()
{
    WRITE_LOG << "Loading plugin: " << m_plugin_path;
    
    try {
        m_plugin_loader = std::make_unique<PluginLoader<Dim>>(m_plugin_path);
        
        if (!m_plugin_loader->is_loaded()) {
            THROW_ERROR("Failed to load plugin: " + m_plugin_loader->get_error());
        }
        
        m_plugin = m_plugin_loader->create_plugin();
        
        if (!m_plugin) {
            THROW_ERROR("Failed to create plugin instance");
        }
        
        WRITE_LOG << "Plugin loaded successfully";
        WRITE_LOG << "* Name:        " << m_plugin->get_name();
        WRITE_LOG << "* Description: " << m_plugin->get_description();
        WRITE_LOG << "* Version:     " << m_plugin->get_version();
        
    } catch (const PluginLoadError& e) {
        THROW_ERROR(std::string("Plugin error: ") + e.what());
    }
}

// Explicit template instantiations for all supported dimensions
template class Solver<1>;
template class Solver<2>;
template class Solver<3>;

}
