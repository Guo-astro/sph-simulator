target_sources(sph_lib
    PRIVATE
        logger.cpp
        core/algorithms/sph_algorithm_registry.cpp
        core/parameters/sph_parameters_builder_base.cpp
        core/parameters/ssph_parameters_builder.cpp
        core/parameters/disph_parameters_builder.cpp
        core/parameters/gsph_parameters_builder.cpp
        core/parameters/physics_parameters.cpp
        core/parameters/computational_parameters.cpp
        core/parameters/output_parameters.cpp
        core/parameters/simulation_parameters.cpp
        core/spatial/simulation_templates.cpp
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