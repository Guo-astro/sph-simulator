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
#include "core/sph_parameters_builder.hpp"

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

Solver::Solver(int argc, char * argv[])
{
    std::cout << "--------------SPH simulation-------------\n\n";
    
    // Validate arguments
    if(argc == 1) {
        std::cerr << "Usage: sph <plugin.dylib|.so|.dll> [config.json]\n" << std::endl;
        std::cerr << "Examples:" << std::endl;
        std::cerr << "  sph shock_tube.dylib                 # Plugin with defaults" << std::endl;
        std::cerr << "  sph shock_tube.dylib gsph.json       # Plugin with GSPH config" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    // First argument must be a plugin
    std::string first_arg = argv[1];
    bool is_plugin = (first_arg.find(".dylib") != std::string::npos ||
                      first_arg.find(".so") != std::string::npos ||
                      first_arg.find(".dll") != std::string::npos);
    
    if (!is_plugin) {
        std::cerr << "Error: First argument must be a plugin file (.dylib/.so/.dll)" << std::endl;
        std::cerr << "Legacy JSON-only mode is no longer supported." << std::endl;
        std::cerr << "Please convert your simulation to a plugin." << std::endl;
        std::exit(EXIT_FAILURE);
    }
    
    // Load plugin
    m_plugin_path = first_arg;
    load_plugin();
    
    // Initialize parameters (will be configured by plugin or JSON)
    m_param = std::make_shared<SPHParameters>();
    
    // Read configuration file if provided
    if (argc >= 3) {
        std::string second_arg = argv[2];
        if (second_arg.find(".json") != std::string::npos) {
            read_parameterfile(argv[2]);
        }
    }
    
    // Set output directory (from JSON or default)
    if (m_output_dir.empty()) {
        m_output_dir = "output/" + m_plugin->get_name();
    }
    
    Logger::open(m_output_dir);

#ifdef _OPENMP
    WRITE_LOG << "Open MP is valid.";
    int num_threads;
    if(argc == 4) {  // plugin config threads
        num_threads = std::atoi(argv[3]);
        omp_set_num_threads(num_threads);
    } else {
        num_threads = omp_get_max_threads();
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

    m_output = std::make_shared<Output<DIM>>();
}

void Solver::read_parameterfile(const char * filename)
{
    namespace pt = boost::property_tree;

    if (!m_param) {
        m_param = std::make_shared<SPHParameters>();
    }

    pt::ptree input;
    pt::read_json(filename, input);

    m_output_dir = input.get<std::string>("outputDirectory", "");

    // time
    m_param->time.start = input.get<real>("startTime", real(0));
    m_param->time.end   = input.get<real>("endTime", real(0));
    if(m_param->time.end < m_param->time.start && m_param->time.end != 0) {
        THROW_ERROR("endTime < startTime");
    }
    m_param->time.output = input.get<real>("outputTime", (m_param->time.end - m_param->time.start) / 100);
    m_param->time.energy = input.get<real>("energyTime", m_param->time.output);

    // SPH type - use registry instead of hardcoded if-else
    std::string sph_type = input.get<std::string>("SPHType", "ssph");
    try {
        m_param->type = SPHAlgorithmRegistry::get_type(sph_type);
    } catch (const std::exception& e) {
        THROW_ERROR(std::string("Failed to set SPH type: ") + e.what());
    }

    // CFL
    m_param->cfl.sound = input.get<real>("cflSound", 0.3);
    m_param->cfl.force = input.get<real>("cflForce", 0.125);

    // Artificial Viscosity
    m_param->av.alpha = input.get<real>("avAlpha", 1.0);
    m_param->av.use_balsara_switch = input.get<bool>("useBalsaraSwitch", true);
    m_param->av.use_time_dependent_av = input.get<bool>("useTimeDependentAV", false);
    if(m_param->av.use_time_dependent_av) {
        m_param->av.alpha_max = input.get<real>("alphaMax", 2.0);
        m_param->av.alpha_min = input.get<real>("alphaMin", 0.1);
        if(m_param->av.alpha_max < m_param->av.alpha_min) {
            THROW_ERROR("alphaMax < alphaMin");
        }
        m_param->av.epsilon = input.get<real>("epsilonAV", 0.2);
    }

    // Artificial Conductivity
    m_param->ac.is_valid = input.get<bool>("useArtificialConductivity", false);
    if(m_param->ac.is_valid) {
        m_param->ac.alpha = input.get<real>("alphaAC", 1.0);
    }

    // Tree
    m_param->tree.max_level = input.get<int>("maxTreeLevel", 20);
    m_param->tree.leaf_particle_num = input.get<int>("leafParticleNumber", 1);

    // Physics
    m_param->physics.neighbor_number = input.get<int>("neighborNumber", 32);
    m_param->physics.gamma = input.get<real>("gamma", 1.4);

    // Kernel
    std::string kernel_name = input.get<std::string>("kernel", "cubic_spline");
    if(kernel_name == "cubic_spline") {
        m_param->kernel = KernelType::CUBIC_SPLINE;
    } else if(kernel_name == "wendland") {
        m_param->kernel = KernelType::WENDLAND;
    } else {
        THROW_ERROR("kernel is unknown.");
    }

    // smoothing length
    m_param->iterative_sml = input.get<bool>("iterativeSmoothingLength", true);

    // periodic
    m_param->periodic.is_valid = input.get<bool>("periodic", false);
    if(m_param->periodic.is_valid) {
        {
            auto & range_max = input.get_child("rangeMax");
            if(range_max.size() != DIM) {
                THROW_ERROR("rangeMax != DIM");
            }
            int i = 0;
            for(auto & v : range_max) {
                m_param->periodic.range_max[i] = std::stod(v.second.data());
                ++i;
            }
        }

        {
            auto & range_min = input.get_child("rangeMin");
            if(range_min.size() != DIM) {
                THROW_ERROR("rangeMax != DIM");
            }
            int i = 0;
            for(auto & v : range_min) {
                m_param->periodic.range_min[i] = std::stod(v.second.data());
                ++i;
            }
        }
    }

    // gravity
    m_param->gravity.is_valid = input.get<bool>("useGravity", false);
    if(m_param->gravity.is_valid) {
        m_param->gravity.constant = input.get<real>("G", 1.0);
        m_param->gravity.theta = input.get<real>("theta", 0.5);
    }

    // GSPH
    if(m_param->type == SPHType::GSPH) {
        m_param->gsph.is_2nd_order = input.get<bool>("use2ndOrderGSPH", true);
    }
}

void Solver::run()
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

void Solver::initialize()
{
    m_sim = std::make_shared<Simulation<DIM>>(m_param);

    make_initial_condition();

    m_timestep = std::make_shared<TimeStep<DIM>>();
    if(m_param->type == SPHType::SSPH) {
        m_pre = std::make_shared<PreInteraction<DIM>>();
        m_fforce = std::make_shared<FluidForce<DIM>>();
    } else if(m_param->type == SPHType::DISPH) {
        m_pre = std::make_shared<disph::PreInteraction<DIM>>();
        m_fforce = std::make_shared<disph::FluidForce<DIM>>();
    } else if(m_param->type == SPHType::GSPH) {
        m_pre = std::make_shared<gsph::PreInteraction<DIM>>();
        m_fforce = std::make_shared<gsph::FluidForce<DIM>>();
    }
    m_gforce = std::make_shared<GravityForce<DIM>>();

    // GSPH
    if(m_param->type == SPHType::GSPH) {
        std::vector<std::string> names;
        names.push_back("grad_density");
        names.push_back("grad_pressure");
        names.push_back("grad_velocity_0");
        
        // Add gradient components based on dimension
        if(DIM >= 2) {
            names.push_back("grad_velocity_1");
        }
        if(DIM >= 3) {
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

#ifndef EXHAUSTIVE_SEARCH
    auto tree = m_sim->tree;
    tree->resize(num);
    tree->make(p, num);
#endif

    m_pre->calculation(m_sim);
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
}

void Solver::integrate()
{
    // Update ghost particle properties before neighbor search
    if (m_sim->ghost_manager) {
        m_sim->ghost_manager->update_ghosts(m_sim->particles);
    }

    m_timestep->calculation(m_sim);

    predict();
#ifndef EXHAUSTIVE_SEARCH
    m_sim->make_tree();
#endif
    m_pre->calculation(m_sim);
    m_fforce->calculation(m_sim);
    m_gforce->calculation(m_sim);
    correct();
}

void Solver::predict()
{
    auto & p = m_sim->particles;
    const int num = m_sim->particle_num;
    auto * periodic = m_sim->periodic.get();
    const real dt = m_sim->dt;
    const real gamma = m_param->physics.gamma;
    const real c_sound = gamma * (gamma - 1.0);

    assert(p.size() == num);

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

void Solver::correct()
{
    auto & p = m_sim->particles;
    const int num = m_sim->particle_num;
    const real dt = m_sim->dt;
    const real gamma = m_param->physics.gamma;
    const real c_sound = gamma * (gamma - 1.0);

    assert(p.size() == num);

#pragma omp parallel for
    for(int i = 0; i < num; ++i) {
        p[i].vel = p[i].vel_p + p[i].acc * (0.5 * dt);
        p[i].ene = p[i].ene_p + p[i].dene * (0.5 * dt);
        p[i].sound = std::sqrt(c_sound * p[i].ene);
    }
}

void Solver::make_initial_condition()
{
    if (!m_plugin) {
        THROW_ERROR("No plugin loaded. Plugin is required for simulation.");
    }
    
    WRITE_LOG << "Initializing simulation from plugin: " << m_plugin->get_name();
    
    // Let plugin configure the simulation
    m_plugin->initialize(m_sim, m_param);
    
    WRITE_LOG << "Plugin initialization complete";
}

void Solver::load_plugin()
{
    WRITE_LOG << "Loading plugin: " << m_plugin_path;
    
    try {
        m_plugin_loader = std::make_unique<PluginLoader>(m_plugin_path);
        
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

}
