target_sources(sph_tests
    PRIVATE
        particle_test.cpp
        plugin_loader_test.cpp
        solver_plugin_test.cpp
        parameter_builder_test.cpp
        parameter_categories_test.cpp
        dimension_system_test_simple.cpp
        # dimension_system_test.cpp  # TODO: Fix syntax errors
)
