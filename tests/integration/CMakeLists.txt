# Integration tests - component interaction tests

target_sources(sph_tests
    PRIVATE
        neighbor_accessor_sph_integration_test.cpp  # BDD integration tests for type-safe neighbor access in SPH methods
)

# Full initialization sequence test
# add_executable(full_initialization_test
#     full_initialization_test.cpp
#     ${CMAKE_SOURCE_DIR}/tests/test_main.cpp
# )
# target_link_libraries(full_initialization_test
#     PRIVATE
#         sph_lib
#         GTest::gtest
#         GTest::gtest_main
# )
# add_test(NAME FullInitializationTest COMMAND full_initialization_test)

# Uncomment tests as they are implemented
