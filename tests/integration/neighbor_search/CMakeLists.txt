target_sources(sph_tests
    PRIVATE
        neighbor_accessor_sph_integration_test.cpp
        neighbor_search_test.cpp
        # bhtree_ghost_integration_test.cpp  # TODO: Fix outdated API (r→pos, m→mass, etc.)
)
