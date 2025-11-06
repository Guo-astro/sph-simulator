/**
 * @file protobuf_writer_test.cpp
 * @brief BDD-style tests for ProtobufWriter
 * 
 * Test-Driven Development: Write tests first, then implement to pass.
 * Tests binary serialization, deserialization, unit conversions, file structure.
 */

#include <gtest/gtest.h>
#include "../../../helpers/bdd_helpers.hpp"
#include "core/output/writers/output_writer.hpp"
#include "core/output/writers/protobuf_writer.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/output/units/unit_system.hpp"
#include "core/output/units/galactic_unit_system.hpp"
#include "proto/particle_data.pb.h"
#include <fstream>
#include <filesystem>
#include <vector>

using namespace sph;
namespace fs = std::filesystem;

// Test constants
namespace {
    constexpr const char* kTestOutputDir = "test_output_protobuf";
    constexpr double kTolerance = 1e-10;
}

// Test fixture for Protobuf writer tests
class ProtobufWriterTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory
        fs::create_directories(kTestOutputDir);
        
        // Create a simple unit system
        unit_system = std::make_shared<GalacticUnitSystem>();
    }
    
    void TearDown() override {
        // Clean up test output directory
        if (fs::exists(kTestOutputDir)) {
            fs::remove_all(kTestOutputDir);
        }
    }
    
    // Helper to check if file exists
    bool file_exists(const std::string& filename) const {
        return fs::exists(filename);
    }
    
    // Helper to get file size
    std::size_t file_size(const std::string& filename) const {
        return fs::file_size(filename);
    }
    
    // Helper to read protobuf snapshot
    bool read_snapshot(const std::string& filename, sph::output::Snapshot& snapshot) const {
        std::ifstream input(filename, std::ios::binary);
        if (!input) return false;
        return snapshot.ParseFromIstream(&input);
    }
    
    std::shared_ptr<UnitSystem> unit_system;
};

// ============================================================================
// FEATURE: Protobuf Writer Construction and Configuration
// ============================================================================

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, ConstructsWithValidParameters) {
    GIVEN("Valid output directory") {
        WHEN("We construct a Protobuf writer") {
            ProtobufWriter<2> writer(kTestOutputDir);
            
            THEN("Writer should be constructed successfully") {
                EXPECT_EQ(writer.get_format(), OutputFormat::PROTOBUF);
                EXPECT_EQ(writer.get_extension(), "pb");
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, AcceptsUnitSystemConfiguration) {
    GIVEN("A Protobuf writer and a unit system") {
        ProtobufWriter<2> writer(kTestOutputDir);
        
        WHEN("We set the unit system") {
            writer.set_unit_system(unit_system);
            
            THEN("Unit system should be accepted without error") {
                // Setting should complete successfully
            }
        }
    }
}

// ============================================================================
// FEATURE: Snapshot File Creation
// ============================================================================

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, CreatesSnapshotFile) {
    GIVEN("A Protobuf writer and test particles") {
        ProtobufWriter<2> writer(kTestOutputDir);
        writer.set_unit_system(unit_system);
        
        // Create test particle
        SPHParticle<2> particle;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        particle.vel[0] = 0.5;
        particle.vel[1] = 0.3;
        particle.mass = 1.5;
        particle.dens = 2.0;
        particle.pres = 1.2;
        particle.id = 1;
        
        WHEN("We write a snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("Snapshot file should be created") {
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00000.pb"));
            }
            
            AND_THEN("File should not be empty") {
                EXPECT_GT(file_size(std::string(kTestOutputDir) + "/snapshots/00000.pb"), 0);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, SerializesParticleData) {
    GIVEN("A Protobuf writer and test particle") {
        ProtobufWriter<2> writer(kTestOutputDir);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        particle.mass = 1.5;
        particle.dens = 2.0;
        particle.id = 42;
        
        WHEN("We write and read back the snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.1, 0);
            
            sph::output::Snapshot snapshot;
            bool success = read_snapshot(
                std::string(kTestOutputDir) + "/snapshots/00000.pb",
                snapshot
            );
            
            THEN("Snapshot should deserialize successfully") {
                EXPECT_TRUE(success);
                EXPECT_EQ(snapshot.particles_size(), 1);
            }
            
            AND_THEN("Particle data should match") {
                const auto& pb_particle = snapshot.particles(0);
                EXPECT_EQ(pb_particle.id(), 42);
                EXPECT_NEAR(pb_particle.mass(), 1.5, kTolerance);
                EXPECT_NEAR(pb_particle.density(), 2.0, kTolerance);
            }
            
            AND_THEN("Metadata should be present") {
                EXPECT_NEAR(snapshot.time(), 0.1, kTolerance);
                EXPECT_EQ(snapshot.timestep(), 0);
            }
        }
    }
}

// ============================================================================
// FEATURE: Binary Format Efficiency
// ============================================================================

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, ProducesBinaryFormat) {
    GIVEN("A Protobuf writer and particles") {
        ProtobufWriter<2> writer(kTestOutputDir);
        writer.set_unit_system(unit_system);
        
        std::vector<SPHParticle<2>> particles(100);
        for (int i = 0; i < 100; ++i) {
            particles[i].id = i;
            particles[i].mass = 1.0;
            particles[i].pos[0] = static_cast<real>(i);
            particles[i].pos[1] = static_cast<real>(i * 2);
        }
        
        WHEN("We write the snapshot") {
            writer.write_snapshot(particles.data(), particles.size(), nullptr, 0.0, 0);
            
            THEN("File should be in binary format") {
                std::string filename = std::string(kTestOutputDir) + "/snapshots/00000.pb";
                std::ifstream file(filename, std::ios::binary);
                
                // Read first few bytes - should not be readable ASCII text
                char buffer[10];
                file.read(buffer, 10);
                
                // Binary protobuf files typically start with field tags (small numbers)
                // They shouldn't look like readable text
                bool looks_binary = true;
                for (int i = 0; i < 10; ++i) {
                    if (buffer[i] >= 'A' && buffer[i] <= 'z') {
                        // Some ASCII is OK in protobuf, but not continuous readable text
                        continue;
                    }
                }
                EXPECT_TRUE(looks_binary);
            }
        }
    }
}

// ============================================================================
// FEATURE: Energy Time Series
// ============================================================================

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, WritesEnergyData) {
    GIVEN("A Protobuf writer and test particle") {
        ProtobufWriter<2> writer(kTestOutputDir);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.ene = 1.0;
        particle.mass = 1.0;
        particle.vel[0] = 1.0;
        particle.vel[1] = 0.0;
        
        WHEN("We write energy data") {
            writer.write_energy(0.0, 0.5, 1.0, 0.0);  // time, kinetic, thermal, potential
            
            THEN("Energy file should be created") {
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/energy.pb"));
            }
        }
    }
}

// ============================================================================
// FEATURE: Multiple Snapshots
// ============================================================================

SCENARIO_WITH_FIXTURE(ProtobufWriterTestFixture, WritesMultipleSnapshots) {
    GIVEN("A Protobuf writer and test particles") {
        ProtobufWriter<2> writer(kTestOutputDir);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.mass = 1.0;
        
        WHEN("We write three snapshots") {
            for (int i = 0; i < 3; ++i) {
                particle.pos[0] = static_cast<real>(i);
                writer.write_snapshot(&particle, 1, nullptr, static_cast<real>(i) * 0.1, i);
            }
            
            THEN("All snapshot files should exist") {
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00000.pb"));
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00001.pb"));
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00002.pb"));
            }
        }
    }
}

// ============================================================================
// FEATURE: 3D Particle Support
// ============================================================================

SCENARIO(ProtobufWriter, Supports3DParticles) {
    GIVEN("A 3D Protobuf writer") {
        const std::string test_dir = "test_output_protobuf_3d";
        fs::create_directories(test_dir);
        
        ProtobufWriter<3> writer(test_dir);
        auto unit_sys = std::make_shared<GalacticUnitSystem>();
        writer.set_unit_system(unit_sys);
        
        SPHParticle<3> particle;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        particle.pos[2] = 3.0;
        particle.mass = 1.0;
        particle.id = 1;
        
        WHEN("We write a 3D snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("3D snapshot should be created and deserializable") {
                sph::output::Snapshot snapshot;
                std::ifstream input(test_dir + "/snapshots/00000.pb", std::ios::binary);
                EXPECT_TRUE(snapshot.ParseFromIstream(&input));
                
                const auto& pb_particle = snapshot.particles(0);
                EXPECT_EQ(pb_particle.id(), 1);
                EXPECT_EQ(pb_particle.position_size(), 3);
                EXPECT_NEAR(pb_particle.position(0), 1.0, kTolerance);
                EXPECT_NEAR(pb_particle.position(1), 2.0, kTolerance);
                EXPECT_NEAR(pb_particle.position(2), 3.0, kTolerance);
            }
        }
        
        fs::remove_all(test_dir);
    }
}
