target_sources(sph_tests
    PRIVATE
        particle_test.cpp
        particle_initialization_test.cpp
        plugin_loader_test.cpp
        solver_plugin_test.cpp
        parameter_builder_test.cpp
        parameter_categories_test.cpp
        algorithm_parameters_builder_test.cpp
        dimension_system_test_simple.cpp
        kernel_function_test.cpp
        ghost_particle_boundary_edge_cases_test.cpp
        ghost_particle_debug_test.cpp
        ghost_particle_regression_test.cpp
        ghost_particle_coordinator_test.cpp
        spatial_tree_coordinator_test.cpp
        neighbor_search_test.cpp  # New declarative neighbor search API tests
        boundary_spacing_validation_test.cpp  # BDD tests for per-boundary spacing
        unit_system_test.cpp  # BDD tests for unit system framework (TDD)
        unit_system_factory_test.cpp  # BDD tests for unit system factory (TDD)
        csv_writer_test.cpp  # BDD tests for CSV output writer (TDD)
        unit_conversion_mode_test.cpp  # BDD tests for type-safe unit conversion control
        output_system_integration_test.cpp  # BDD integration tests for complete output system
        type_safe_neighbor_accessor_test.cpp  # BDD unit tests for type-safe neighbor access (TDD)
        # protobuf_writer_test.cpp  # BDD tests for Protobuf output writer (TDD) - TODO: fix protobuf types
        # metadata_writer_test.cpp  # BDD tests for metadata writer (TDD) - TODO: fix parameter types
        # output_coordinator_test.cpp  # BDD tests for output coordinator (TDD) - TODO: fix compilation errors
        # ghost_particle_integration_test.cpp  # TODO: Vector initialization syntax issues - needs manual cleanup
        # sph_2_5d_test.cpp  # TODO: 2.5D implementation headers have compilation errors - needs major refactoring
        # ghost_particle_manager_test.cpp  # TODO: Too many member name issues, covered by new tests
        # dimension_system_test.cpp  # TODO: Fix syntax errors
)
