target_sources(sph_lib
    PRIVATE
        logger.cpp
        core/sph_algorithm_registry.cpp
        core/sph_parameters_builder_base.cpp
        core/ssph_parameters_builder.cpp
        core/disph_parameters_builder.cpp
        core/gsph_parameters_builder.cpp
        core/physics_parameters.cpp
        core/computational_parameters.cpp
        core/output_parameters.cpp
        core/simulation_parameters.cpp
        core/simulation_templates.cpp
        algorithms/riemann/hll_solver.cpp
    )

target_sources(sph
    PRIVATE
        solver.cpp
    )

target_sources(sph2d
    PRIVATE
        solver.cpp
    )

target_sources(sph3d
    PRIVATE
        solver.cpp
    )

add_subdirectory(disph)
add_subdirectory(gsph)