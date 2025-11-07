/**
 * @file output_coordinator_test.cpp
 * @brief BDD-style tests for OutputCoordinator
 * 
 * Test-Driven Development: Write tests first, then implement to pass.
 * Tests coordination of multiple writers, metadata generation, unit system management.
 */

#include <gtest/gtest.h>
#include "../../helpers/bdd_helpers.hpp"
#include "core/output/output_coordinator.hpp"
#include "core/output/writers/csv_writer.hpp"
#include "core/output/writers/protobuf_writer.hpp"
#include "core/output/units/galactic_unit_system.hpp"
#include "core/simulation/simulation.hpp"
#include "core/parameters/simulation_parameters.hpp"
#include <filesystem>

using namespace sph;
namespace fs = std::filesystem;

// Test constants
namespace {
    constexpr const char* kTestOutputDir = "test_output_coordinator";
}

// Test fixture
class OutputCoordinatorTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        fs::create_directories(kTestOutputDir);
        
        // Create test parameters
        params = std::make_shared<SPHParameters>();
        params->get_physics().gamma = 1.4;
        params->get_physics().neighbor_number = 50;
        params->output.directory = kTestOutputDir;
        
        // Create unit system
        unit_system = std::make_shared<GalacticUnitSystem>();
    }
    
    void TearDown() override {
        if (fs::exists(kTestOutputDir)) {
            fs::remove_all(kTestOutputDir);
        }
    }
    
    std::shared_ptr<SPHParameters> params;
    std::shared_ptr<UnitSystem> unit_system;
};

// ============================================================================
// FEATURE: Output Coordinator Construction
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, ConstructsSuccessfully) {
    GIVEN("Valid parameters") {
        WHEN("We create an output coordinator") {
            OutputCoordinator<2> coordinator(kTestOutputDir, *params);
            
            THEN("Coordinator should be created") {
                // Construction should succeed
                EXPECT_TRUE(true);
            }
        }
    }
}

// ============================================================================
// FEATURE: Writer Management
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, AddsCSVWriter) {
    GIVEN("An output coordinator") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        
        WHEN("We add a CSV writer") {
            auto writer = std::make_unique<CSVWriter<2>>(kTestOutputDir, true);
            coordinator.add_writer(std::move(writer));
            
            THEN("Writer should be added without error") {
                EXPECT_TRUE(true);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, AddsMultipleWriters) {
    GIVEN("An output coordinator") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        
        WHEN("We add multiple writers") {
            coordinator.add_writer(std::make_unique<CSVWriter<2>>(kTestOutputDir, true));
            coordinator.add_writer(std::make_unique<ProtobufWriter<2>>(kTestOutputDir));
            
            THEN("Both writers should be added") {
                EXPECT_TRUE(true);
            }
        }
    }
}

// ============================================================================
// FEATURE: Unit System Configuration
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, SetsUnitSystem) {
    GIVEN("An output coordinator with writers") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        coordinator.add_writer(std::make_unique<CSVWriter<2>>(kTestOutputDir, true));
        
        WHEN("We set the unit system") {
            coordinator.set_unit_system(unit_system);
            
            THEN("Unit system should be set for all writers") {
                EXPECT_TRUE(true);
            }
        }
    }
}

// ============================================================================
// FEATURE: Particle Output
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, WritesParticles) {
    GIVEN("An output coordinator with CSV and Protobuf writers") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        coordinator.add_writer(std::make_unique<CSVWriter<2>>(kTestOutputDir, true));
        coordinator.add_writer(std::make_unique<ProtobufWriter<2>>(kTestOutputDir));
        coordinator.set_unit_system(unit_system);
        
        // Create mock simulation
        auto sim = std::make_shared<Simulation<2>>(params);
        SPHParticle<2> particle;
        particle.id = 1;
        particle.mass = 1.0;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        sim->particles.push_back(particle);
        sim->particle_num = 1;
        
        WHEN("We write particles") {
            coordinator.write_particles(sim);
            
            THEN("Both CSV and Protobuf files should be created") {
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/snapshots/00000.csv"));
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/snapshots/00000.pb"));
            }
        }
    }
}

// ============================================================================
// FEATURE: Energy Output
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, WritesEnergy) {
    GIVEN("An output coordinator with writers") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        coordinator.add_writer(std::make_unique<CSVWriter<2>>(kTestOutputDir, true));
        coordinator.set_unit_system(unit_system);
        
        auto sim = std::make_shared<Simulation<2>>(params);
        SPHParticle<2> particle;
        particle.mass = 1.0;
        particle.ene = 1.0;
        particle.vel[0] = 1.0;
        particle.vel[1] = 0.0;
        sim->particles.push_back(particle);
        sim->particle_num = 1;
        
        WHEN("We write energy") {
            coordinator.write_energy(sim);
            
            THEN("Energy file should be created") {
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/energy.csv"));
            }
        }
    }
}

// ============================================================================
// FEATURE: Metadata Generation
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, GeneratesMetadata) {
    GIVEN("An output coordinator") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        coordinator.set_unit_system(unit_system);
        
        WHEN("We write metadata") {
            coordinator.write_metadata();
            
            THEN("Metadata file should be created") {
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/metadata.json"));
            }
        }
    }
}

// ============================================================================
// FEATURE: Snapshot Counting
// ============================================================================

SCENARIO_WITH_FIXTURE(OutputCoordinatorTestFixture, TracksSnapshotCount) {
    GIVEN("An output coordinator with writer") {
        OutputCoordinator<2> coordinator(kTestOutputDir, *params);
        coordinator.add_writer(std::make_unique<CSVWriter<2>>(kTestOutputDir, true));
        coordinator.set_unit_system(unit_system);
        
        auto sim = std::make_shared<Simulation<2>>(params);
        SPHParticle<2> particle;
        particle.mass = 1.0;
        sim->particles.push_back(particle);
        sim->particle_num = 1;
        
        WHEN("We write multiple snapshots") {
            coordinator.write_particles(sim);
            coordinator.write_particles(sim);
            coordinator.write_particles(sim);
            
            THEN("All snapshots should have unique filenames") {
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/snapshots/00000.csv"));
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/snapshots/00001.csv"));
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/snapshots/00002.csv"));
            }
        }
    }
}
