target_sources(sph_tests
    PRIVATE
        particle_test.cpp
        particle_initialization_test.cpp
        plugin_loader_test.cpp
        solver_plugin_test.cpp
        parameter_builder_test.cpp
        parameter_categories_test.cpp
        dimension_system_test_simple.cpp
        kernel_function_test.cpp
        # bhtree_test.cpp  # TODO: Fix SCENARIO macro usage
        # sph_2_5d_test.cpp  # TODO: Fix compilation errors
        # ghost_particle_manager_test.cpp  # TODO: Fix member names (r,v,rho vs pos,vel,dens)
        # ghost_particle_integration_test.cpp  # TODO: Fix constructor and TEST macro errors
        # dimension_system_test.cpp  # TODO: Fix syntax errors
)
