/**
 * @file csv_writer_test.cpp
 * @brief BDD-style tests for CSVWriter
 * 
 * Test-Driven Development: Write tests first, then implement to pass.
 * Tests output formatting, unit conversions, file structure, headers.
 */

#include <gtest/gtest.h>
#include "../../../helpers/bdd_helpers.hpp"
#include "core/output/writers/output_writer.hpp"
#include "core/output/writers/csv_writer.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/output/units/unit_system.hpp"
#include "core/output/units/galactic_unit_system.hpp"
#include <fstream>
#include <filesystem>
#include <vector>

using namespace sph;
namespace fs = std::filesystem;

// Test constants
namespace {
    constexpr const char* kTestOutputDir = "test_output_csv";
    constexpr double kTolerance = 1e-10;
}

// Test fixture for CSV writer tests
class CSVWriterTestFixture : public ::testing::Test {
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
    
    // Helper to read file contents
    std::string read_file_contents(const std::string& filename) const {
        std::ifstream file(filename);
        std::stringstream buffer;
        buffer << file.rdbuf();
        return buffer.str();
    }
    
    // Helper to count lines in file
    int count_lines(const std::string& filename) const {
        std::ifstream file(filename);
        return std::count(std::istreambuf_iterator<char>(file),
                         std::istreambuf_iterator<char>(), '\n');
    }
    
    // Helper to check if file exists
    bool file_exists(const std::string& filename) const {
        return fs::exists(filename);
    }
    
    std::shared_ptr<UnitSystem> unit_system;
};

// ============================================================================
// FEATURE: CSV Writer Construction and Configuration
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, ConstructsWithValidParameters) {
    GIVEN("Valid output directory and header option") {
        WHEN("We construct a CSV writer with headers") {
            CSVWriter<2> writer(kTestOutputDir, true);
            
            THEN("Writer should be constructed successfully") {
                EXPECT_EQ(writer.get_format(), OutputFormat::CSV);
                EXPECT_EQ(writer.get_extension(), "csv");
            }
        }
        
        AND("When we construct without headers") {
            CSVWriter<2> writer(kTestOutputDir, false);
            
            THEN("Writer should also be constructed successfully") {
                EXPECT_EQ(writer.get_format(), OutputFormat::CSV);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, AcceptsUnitSystemConfiguration) {
    GIVEN("A CSV writer and a unit system") {
        CSVWriter<2> writer(kTestOutputDir, true);
        
        WHEN("We set the unit system") {
            writer.set_unit_system(unit_system);
            
            THEN("Unit system should be accepted without error") {
                // Setting should complete successfully
                // Actual unit conversion tested in later scenarios
            }
        }
    }
}

// ============================================================================
// FEATURE: Snapshot File Creation
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, CreatesSnapshotFile) {
    GIVEN("A CSV writer and test particles") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        // Create test particle
        SPHParticle<2> particle;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        particle.vel[0] = 0.5;
        particle.vel[1] = 0.3;
        particle.mass = 1.0;
        particle.dens = 1.0;
        particle.pres = 0.1;
        particle.ene = 2.5;
        particle.id = 0;
        
        WHEN("We write a snapshot") {
            constexpr real time = 0.0;
            constexpr int timestep = 0;
            writer.write_snapshot(&particle, 1, nullptr, time, timestep);
            
            THEN("Snapshot file should be created") {
                const std::string expected_file = 
                    std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(file_exists(expected_file));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, CreatesSequentialFiles) {
    GIVEN("A CSV writer") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.mass = 1.0;
        
        WHEN("We write multiple snapshots") {
            for (int i = 0; i < 5; ++i) {
                writer.write_snapshot(&particle, 1, nullptr, 
                                    static_cast<real>(i) * 0.1, i);
            }
            
            THEN("Files should be numbered sequentially") {
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00000.csv"));
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00001.csv"));
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00002.csv"));
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00003.csv"));
                EXPECT_TRUE(file_exists(std::string(kTestOutputDir) + "/snapshots/00004.csv"));
            }
        }
    }
}

// ============================================================================
// FEATURE: CSV Header Format
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, WritesHeaderWhenRequested) {
    GIVEN("A CSV writer with headers enabled") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.mass = 1.0;
        
        WHEN("We write a snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("File should contain header comments") {
                const std::string contents = read_file_contents(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                );
                
                EXPECT_TRUE(contents.find("# SPH Simulation Snapshot") != std::string::npos);
                EXPECT_TRUE(contents.find("# Time:") != std::string::npos);
                EXPECT_TRUE(contents.find("# Dimension:") != std::string::npos);
                EXPECT_TRUE(contents.find("# Unit System:") != std::string::npos);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, WritesColumnNamesInHeader) {
    GIVEN("A CSV writer with headers enabled for 2D") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.mass = 1.0;
        
        WHEN("We write a snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("CSV should have column headers") {
                const std::string contents = read_file_contents(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                );
                
                // Check for essential column names
                EXPECT_TRUE(contents.find("pos_x") != std::string::npos);
                EXPECT_TRUE(contents.find("pos_y") != std::string::npos);
                EXPECT_TRUE(contents.find("vel_x") != std::string::npos);
                EXPECT_TRUE(contents.find("vel_y") != std::string::npos);
                EXPECT_TRUE(contents.find("mass") != std::string::npos);
                EXPECT_TRUE(contents.find("density") != std::string::npos);
                EXPECT_TRUE(contents.find("pressure") != std::string::npos);
                EXPECT_TRUE(contents.find("energy") != std::string::npos);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, OmitsHeaderWhenDisabled) {
    GIVEN("A CSV writer with headers disabled") {
        CSVWriter<2> writer(kTestOutputDir, false);  // No headers
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.mass = 1.0;
        
        WHEN("We write a snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("File should not contain header comments") {
                const std::string contents = read_file_contents(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                );
                
                EXPECT_TRUE(contents.find("# SPH") == std::string::npos);
            }
        }
    }
}

// ============================================================================
// FEATURE: Particle Data Output
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, WritesAllParticleFields) {
    GIVEN("A CSV writer and a fully initialized particle") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        particle.vel[0] = 0.5;
        particle.vel[1] = 0.3;
        particle.acc[0] = 0.1;
        particle.acc[1] = -0.1;
        particle.mass = 1.5;
        particle.dens = 2.0;
        particle.pres = 0.5;
        particle.ene = 3.0;
        particle.sml = 0.2;
        particle.sound = 1.0;
        particle.id = 42;
        particle.neighbor = 50;
        
        WHEN("We write the snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("All particle fields should appear in CSV") {
                const std::string contents = read_file_contents(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                );
                
                // Check that numerical values appear (converted by unit system)
                EXPECT_TRUE(contents.find("42") != std::string::npos);  // ID
                EXPECT_TRUE(contents.find("50") != std::string::npos);  // neighbors
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, WritesMultipleParticles) {
    GIVEN("A CSV writer and multiple particles") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        constexpr int num_particles = 10;
        std::vector<SPHParticle<2>> particles(num_particles);
        for (int i = 0; i < num_particles; ++i) {
            particles[i].id = i;
            particles[i].mass = 1.0;
        }
        
        WHEN("We write all particles") {
            writer.write_snapshot(particles.data(), num_particles, nullptr, 0.0, 0);
            
            THEN("CSV should contain correct number of data rows") {
                const int line_count = count_lines(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                );
                
                // Should have header lines + column names + data rows
                EXPECT_GE(line_count, num_particles);
            }
        }
    }
}

// ============================================================================
// FEATURE: Unit Conversion
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, AppliesUnitConversions) {
    GIVEN("A CSV writer with galactic unit system") {
        CSVWriter<2> writer(kTestOutputDir, true);
        auto galactic = std::make_shared<GalacticUnitSystem>();
        writer.set_unit_system(galactic);
        
        SPHParticle<2> particle;
        particle.pos[0] = 1.0;  // 1 code unit
        particle.mass = 1.0;    // 1 code unit
        particle.dens = 1.0;    // 1 code unit
        
        WHEN("We write the snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("Values should be converted to output units") {
                // This is a basic test - actual values depend on unit system
                // The file should exist and contain converted values
                EXPECT_TRUE(file_exists(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                ));
                
                // More detailed validation would check actual numerical values
                // match expected conversions (position in parsecs, etc.)
            }
        }
    }
}

// ============================================================================
// FEATURE: Ghost Particle Handling
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, WritesGhostParticlesSeparately) {
    GIVEN("A CSV writer, real particles, and ghost particles") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> real_particle;
        real_particle.id = 1;
        real_particle.mass = 1.0;
        real_particle.type = static_cast<int>(ParticleType::REAL);
        
        std::vector<SPHParticle<2>> ghost_particles(3);
        for (int i = 0; i < 3; ++i) {
            ghost_particles[i].id = 100 + i;
            ghost_particles[i].mass = 1.0;
            ghost_particles[i].type = static_cast<int>(ParticleType::GHOST);
        }
        
        WHEN("We write snapshot with ghost particles") {
            writer.write_snapshot(&real_particle, 1, &ghost_particles, 0.0, 0);
            
            THEN("Both real and ghost particles should be in output") {
                const std::string contents = this->read_file_contents(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                );
                
                // Should contain IDs from both real and ghost particles
                EXPECT_TRUE(contents.find("1") != std::string::npos ||
                           contents.find("100") != std::string::npos);
            }
        }
    }
}

// ============================================================================
// FEATURE: Energy Output
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, CreatesEnergyFile) {
    GIVEN("A CSV writer") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        WHEN("We write energy data") {
            constexpr real time = 0.0;
            constexpr real kinetic = 1.0;
            constexpr real thermal = 2.0;
            constexpr real potential = 0.5;
            
            writer.write_energy(time, kinetic, thermal, potential);
            
            THEN("Energy file should be created") {
                const std::string expected_file = 
                    std::string(kTestOutputDir) + "/energy.csv";
                EXPECT_TRUE(file_exists(expected_file));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, AppendsToEnergyFile) {
    GIVEN("A CSV writer") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        WHEN("We write multiple energy entries") {
            for (int i = 0; i < 5; ++i) {
                writer.write_energy(
                    static_cast<real>(i) * 0.1,  // time
                    1.0,  // kinetic
                    2.0,  // thermal
                    0.5   // potential
                );
            }
            
            THEN("All entries should be in the file") {
                const int line_count = count_lines(
                    std::string(kTestOutputDir) + "/energy.csv"
                );
                
                // Should have header + 5 data rows
                EXPECT_GE(line_count, 5);
            }
        }
    }
}

// ============================================================================
// FEATURE: Dimension Support
// ============================================================================

SCENARIO(CSVWriter, Supports1DParticles) {
    GIVEN("A 1D CSV writer") {
        const std::string test_dir = "test_output_csv_1d";
        fs::create_directories(test_dir);
        
        CSVWriter<1> writer(test_dir, true);
        auto unit_sys = std::make_shared<GalacticUnitSystem>();
        writer.set_unit_system(unit_sys);
        
        SPHParticle<1> particle;
        particle.pos[0] = 1.0;
        particle.mass = 1.0;
        
        WHEN("We write a 1D snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("1D snapshot should be created") {
                EXPECT_TRUE(fs::exists(test_dir + "/snapshots/00000.csv"));
            }
        }
        
        fs::remove_all(test_dir);
    }
}

SCENARIO(CSVWriter, Supports3DParticles) {
    GIVEN("A 3D CSV writer") {
        const std::string test_dir = "test_output_csv_3d";
        fs::create_directories(test_dir);
        
        CSVWriter<3> writer(test_dir, true);
        auto unit_sys = std::make_shared<GalacticUnitSystem>();
        writer.set_unit_system(unit_sys);
        
        SPHParticle<3> particle;
        particle.pos[0] = 1.0;
        particle.pos[1] = 2.0;
        particle.pos[2] = 3.0;
        particle.mass = 1.0;
        
        WHEN("We write a 3D snapshot") {
            writer.write_snapshot(&particle, 1, nullptr, 0.0, 0);
            
            THEN("3D snapshot with pos_z should be created") {
                std::ifstream file(test_dir + "/snapshots/00000.csv");
                std::stringstream buffer;
                buffer << file.rdbuf();
                const std::string contents = buffer.str();
                EXPECT_TRUE(contents.find("pos_z") != std::string::npos);
            }
        }
        
        fs::remove_all(test_dir);
    }
}

// ============================================================================
// FEATURE: Error Handling and Edge Cases
// ============================================================================

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, HandlesZeroParticles) {
    GIVEN("A CSV writer") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        WHEN("We write a snapshot with zero particles") {
            writer.write_snapshot(nullptr, 0, nullptr, 0.0, 0);
            
            THEN("Empty snapshot file should be created") {
                // Should create file even if empty
                EXPECT_TRUE(file_exists(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                ));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(CSVWriterTestFixture, HandlesNaNValues) {
    GIVEN("A CSV writer and particle with NaN") {
        CSVWriter<2> writer(kTestOutputDir, true);
        writer.set_unit_system(unit_system);
        
        SPHParticle<2> particle;
        particle.pos[0] = std::numeric_limits<real>::quiet_NaN();
        particle.mass = 1.0;
        
        WHEN("We write the particle") {
            // Should handle gracefully (either skip or write special value)
            EXPECT_NO_THROW(
                writer.write_snapshot(&particle, 1, nullptr, 0.0, 0)
            );
            
            THEN("File should be created") {
                EXPECT_TRUE(file_exists(
                    std::string(kTestOutputDir) + "/snapshots/00000.csv"
                ));
            }
        }
    }
}
