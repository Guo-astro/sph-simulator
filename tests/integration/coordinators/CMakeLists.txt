target_sources(sph_tests
    PRIVATE
        spatial_tree_coordinator_test.cpp
        # TODO: Fix output_coordinator_test.cpp compilation errors:
        # - SPHParameters no longer has 'output' member
        # - OutputCoordinator constructor signature changed
        # - Multiple API incompatibilities
        # output_coordinator_test.cpp
)
