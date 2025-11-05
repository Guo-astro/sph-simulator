#include <cassert>
#include <iostream>
#include <chrono>

#include "solver.hpp"
#include "parameters.hpp"
#include "core/sph_particle.hpp"
#include "logger.hpp"
#include "exception.hpp"
#include "output.hpp"
#include "core/simulation.hpp"
#include "core/periodic.hpp"
#include "core/bhtree.hpp"
#include "core/plugin_loader.hpp"
#include "core/simulation_plugin.hpp"
#include "core/sph_algorithm_registry.hpp"

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
    
    // Set output directory
    m_output_dir = "output/" + m_plugin->get_name();
    
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

    WRITE_LOG << "parameters";
    WRITE_LOG << "output directory     = " << m_output_dir;

    WRITE_LOG << "time";
    WRITE_LOG << "* start time         = " << m_param->time.start;
    WRITE_LOG << "* end time           = " << m_param->time.end;
    WRITE_LOG << "* output time        = " << m_param->time.output;
    WRITE_LOG << "* energy output time = " << m_param->time.energy;

    switch(m_param->type) {
    case SPHType::SSPH:
        WRITE_LOG << "SPH type: Standard SPH";
        break;
    case SPHType::DISPH:
        WRITE_LOG << "SPH type: Density Independent SPH";
        break;
    case SPHType::GSPH:
        if(m_param->gsph.is_2nd_order) {
            WRITE_LOG << "SPH type: Godunov SPH (2nd order)";
        } else {
            WRITE_LOG << "SPH type: Godunov SPH (1st order)";
        }
        break;
    }

    WRITE_LOG << "CFL condition";
    WRITE_LOG << "* sound speed = " << m_param->cfl.sound;
    WRITE_LOG << "* force       = " << m_param->cfl.force;

    WRITE_LOG << "Artificial Viscosity";
    WRITE_LOG << "* alpha = " << m_param->av.alpha;
    if(m_param->av.use_balsara_switch) {
        WRITE_LOG << "* use Balsara switch";
    }
    if(m_param->av.use_time_dependent_av) {
        WRITE_LOG << "* use time dependent AV";
        WRITE_LOG << "  * alpha max = " << m_param->av.alpha_max;
        WRITE_LOG << "  * alpha min = " << m_param->av.alpha_min;
        WRITE_LOG << "  * epsilon   = " << m_param->av.epsilon;
    }

    if(m_param->ac.is_valid) {
        WRITE_LOG << "Artificial Conductivity";
        WRITE_LOG << "* alpha = " << m_param->ac.alpha;
    }

    WRITE_LOG << "Tree";
    WRITE_LOG << "* max tree level       = " << m_param->tree.max_level;
    WRITE_LOG << "* leaf particle number = " << m_param->tree.leaf_particle_num;

    WRITE_LOG << "Physics";
    WRITE_LOG << "* Neighbor number = " << m_param->physics.neighbor_number;
    WRITE_LOG << "* gamma           = " << m_param->physics.gamma;

    WRITE_LOG << "Kernel";
    switch(m_param->kernel) {
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

    if(m_param->iterative_sml) {
        WRITE_LOG << "Iterative calculation for smoothing length is valid.";
    }

    if(m_param->periodic.is_valid) {
        WRITE_LOG << "Periodic boundary condition is valid.";
    }

    if(m_param->gravity.is_valid) {
        WRITE_LOG << "Gravity is valid.";
        WRITE_LOG << "* G     = " << m_param->gravity.constant;
        WRITE_LOG << "* theta = " << m_param->gravity.theta;
    }

    // Display plugin information
    WRITE_LOG << "Simulation: " << m_plugin->get_name();
    WRITE_LOG << "Description: " << m_plugin->get_description();
    WRITE_LOG << "Version: " << m_plugin->get_version();

    m_output = std::make_shared<Output<Dim>>();
}

template<int Dim>
void Solver<Dim>::run()
{
    initialize();
    assert(m_sim->particles.size() == m_sim->particle_num);

    const real t_end = m_param->time.end;
    real t_out = m_param->time.output;
    real t_ene = m_param->time.energy;

    m_output->output_particle(m_sim);
    m_output->output_energy(m_sim);

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
            WRITE_LOG << "loop: " << loop << ", time: " << t << ", dt: " << dt << ", num: " << num;
            t_cout_i = std::chrono::system_clock::now();
        } else {
            WRITE_LOG_ONLY << "loop: " << loop << ", time: " << t << ", dt: " << dt << ", num: " << num;
        }

        if(t > t_out) {
            m_output->output_particle(m_sim);
            t_out += m_param->time.output;
        }

        if(t > t_ene) {
            m_output->output_energy(m_sim);
            t_ene += m_param->time.energy;
        }
    }
    const auto end = std::chrono::system_clock::now();
    const real calctime = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    WRITE_LOG << "\ncalculation is finished";
    WRITE_LOG << "calculation time: " << calctime << " ms";
}

template<int Dim>
void Solver<Dim>::initialize()
{
    m_sim = std::make_shared<Simulation<Dim>>(m_param);

    make_initial_condition();

    m_timestep = std::make_shared<TimeStep<Dim>>();
    
    if(m_param->type == SPHType::SSPH) {
        m_pre = std::make_shared<PreInteraction<Dim>>();
        m_fforce = std::make_shared<FluidForce<Dim>>();
    } else if(m_param->type == SPHType::DISPH) {
        m_pre = std::make_shared<disph::PreInteraction<Dim>>();
        m_fforce = std::make_shared<disph::FluidForce<Dim>>();
    } else if(m_param->type == SPHType::GSPH) {
        m_pre = std::make_shared<gsph::PreInteraction<Dim>>();
        m_fforce = std::make_shared<gsph::FluidForce<Dim>>();
    }
    
    m_gforce = std::make_shared<GravityForce<Dim>>();

    // GSPH - only add gradient arrays if 2nd order is enabled
    if(m_param->type == SPHType::GSPH && m_param->gsph.is_2nd_order) {
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
    
    const real gamma = m_param->physics.gamma;
    const real c_sound = gamma * (gamma - 1.0);

    assert(p.size() == num);
    const real alpha = m_param->av.alpha;
#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        p[i].alpha = alpha;
        p[i].balsara = 1.0;
        p[i].sound = std::sqrt(c_sound * p[i].ene);
    }

#ifndef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
    auto tree = m_sim->tree;
    tree->resize(num);
    
    // Populate cached_search_particles for neighbor search
    // Initially contains only real particles (ghost particles not yet generated)
    m_sim->cached_search_particles.clear();
    m_sim->cached_search_particles.resize(num);
    for (int i = 0; i < num; ++i) {
        m_sim->cached_search_particles[i] = p[i];
    }
    
    tree->make(p, num);
#endif

    m_pre->calculation(m_sim);
    
    // Generate ghost particles AFTER smoothing lengths are calculated
    // This ensures we use actual sml values for proper kernel support radius
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
        
        // Update cached_search_particles to include both real and ghost particles
        // This is CRITICAL for neighbor search in subsequent calculations
        // CRITICAL FIX: Use assign() instead of operator= to avoid reallocation
        // that would invalidate tree's internal particle.next pointers
        const auto all_particles_combined = m_sim->get_all_particles_for_search();
        m_sim->cached_search_particles.assign(all_particles_combined.begin(), all_particles_combined.end());

        // Rebuild spatial tree now that cached_search_particles and IDs are consistent
        m_sim->make_tree();
    }
    
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
}

template<int Dim>
void Solver<Dim>::integrate()
{
    // Calculate timestep based on current state
    m_timestep->calculation(m_sim);

    // Move particles to new positions (predict step)
    predict();
    
    // Regenerate ghost particles based on NEW particle positions
    // This ensures Morris 1997 formula is applied to current positions:
    //   x_ghost = 2*x_wall - x_real
    // where x_real is the UPDATED position after predict()
    if (m_sim->ghost_manager) {
        m_sim->ghost_manager->regenerate_ghosts(m_sim->particles);
        
        WRITE_LOG << "Ghost particles regenerated: " 
                  << m_sim->ghost_manager->get_ghost_count() << " ghosts";
    }
    
    // Populate cached search particles for neighbor search (real + ghost)
    // CRITICAL: Must not reallocate to avoid invalidating tree pointers
    const auto all_particles = m_sim->get_all_particles_for_search();
    const size_t new_size = all_particles.size();
    const size_t old_capacity = m_sim->cached_search_particles.capacity();
    
    // CRITICAL FIX: Reserve BEFORE any size changes to ensure no reallocation
    // Reserve extra space to avoid future reallocations
    if (old_capacity < new_size) {
        const size_t new_capacity = new_size + 100;  // Extra buffer for safety
        WRITE_LOG << "WARNING: Resizing cached_search_particles capacity: " 
                  << old_capacity << " -> " << new_capacity;
        m_sim->cached_search_particles.reserve(new_capacity);
    }
    
    // Now resize is safe - capacity is guaranteed >= new_size, so no reallocation
    m_sim->cached_search_particles.resize(new_size);
    std::copy(all_particles.begin(), all_particles.end(), m_sim->cached_search_particles.begin());
    
    // CRITICAL: Clear particle.next pointers after copy to avoid stale linked-list pointers
    // The tree builder (BHNode::assign) modifies particle.next to build linked lists,
    // and these must not contain addresses from previous vector iterations
    for (auto& p : m_sim->cached_search_particles) {
        p.next = nullptr;
    }
    
#ifndef EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
    m_sim->make_tree();
#endif
    
    m_pre->calculation(m_sim);
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
    const real gamma = m_param->physics.gamma;
    const real c_sound = gamma * (gamma - 1.0);

    // Note: p.size() may be > num if ghosts are appended
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

        // Apply legacy periodic boundary condition (for backward compatibility)
        periodic->apply_periodic_condition(p[i].pos);
    }
    
    // Apply periodic wrapping using new ghost particle system if available
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
    const real gamma = m_param->physics.gamma;
    const real c_sound = gamma * (gamma - 1.0);

    // Note: p.size() may be > num if ghosts are appended
    assert(p.size() >= static_cast<size_t>(num));

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        p[i].vel = p[i].vel_p + p[i].acc * (0.5 * dt);
        p[i].ene = p[i].ene_p + p[i].dene * (0.5 * dt);
        p[i].sound = std::sqrt(c_sound * p[i].ene);
    }
}

template<int Dim>
void Solver<Dim>::make_initial_condition()
{
    if (!m_plugin) {
        THROW_ERROR("No plugin loaded. Plugin is required for simulation.");
    }
    
    WRITE_LOG << "Initializing simulation from plugin: " << m_plugin->get_name();
    
    // Let plugin configure the simulation
    m_plugin->initialize(m_sim, m_param);
    
    WRITE_LOG << "Plugin initialization complete";
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
