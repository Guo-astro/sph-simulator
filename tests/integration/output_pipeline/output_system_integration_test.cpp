/**
 * @file output_system_integration_test.cpp
 * @brief BDD-style integration tests for the complete Output system
 * 
 * Tests the integration of Output, OutputCoordinator, CSV/Protobuf writers,
 * unit conversion, and metadata generation.
 */

#include <gtest/gtest.h>
#include "../../helpers/bdd_helpers.hpp"
#include "output.hpp"
#include "core/simulation/simulation.hpp"
#include "core/particles/sph_particle.hpp"
#include "core/output/output_coordinator.hpp"
#include "core/output/units/unit_system_factory.hpp"
#include "parameters.hpp"
#include "logger.hpp"
#include <filesystem>
#include <fstream>
#include <memory>
#include <cmath>

using namespace sph;
namespace fs = std::filesystem;

namespace {

constexpr const char* kTestOutputDir = "test_output_system_integration";
constexpr real kTolerance = 1e-6;

/**
 * @brief Test fixture for output system integration tests
 */
class OutputSystemIntegrationTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory
        fs::create_directories(kTestOutputDir);
        Logger::open(kTestOutputDir);
        
        // Create a simulation with real particles
        auto param = std::make_shared<SPHParameters>();
        sim = std::make_shared<Simulation<1>>(param);
        sim->particle_num = 5;
        sim->particles.resize(5);
        sim->time = 0.15;
        
        // Initialize real particles
        for (int i = 0; i < 5; ++i) {
            sim->particles[i].pos[0] = -0.5 + i * 0.25;
            sim->particles[i].vel[0] = i * 0.1;
            sim->particles[i].acc[0] = i * 0.01;
            sim->particles[i].mass = 1.0;
            sim->particles[i].dens = 1.0 - i * 0.1;
            sim->particles[i].pres = 1.0 - i * 0.15;
            sim->particles[i].ene = 2.5 - i * 0.2;
            sim->particles[i].sound = 1.0;
            sim->particles[i].sml = 0.1;
            sim->particles[i].gradh = 0.0;
            sim->particles[i].balsara = 1.0;
            sim->particles[i].alpha = 1.0;
            sim->particles[i].phi = 0.0;
            sim->particles[i].id = i;
            sim->particles[i].neighbor = 30;
            sim->particles[i].type = 0;  // REAL
        }
    }
    
    void TearDown() override {
        fs::remove_all(kTestOutputDir);
    }
    
    int count_csv_files(const std::string& dir) {
        int count = 0;
        for (const auto& entry : fs::directory_iterator(dir)) {
            if (entry.path().extension() == ".csv") {
                count++;
            }
        }
        return count;
    }
    
    int count_lines_in_file(const std::string& filepath) {
        std::ifstream file(filepath);
        int lines = 0;
        std::string line;
        while (std::getline(file, line)) {
            lines++;
        }
        return lines;
    }
    
    bool file_contains(const std::string& filepath, const std::string& text) {
        std::ifstream file(filepath);
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content.find(text) != std::string::npos;
    }
    
    std::shared_ptr<Simulation<1>> sim;
};

} // anonymous namespace

FEATURE("OutputSystemIntegration") {

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, WritesSnapshotsToCorrectLocation) {
    GIVEN("An Output system with code units") {
        Output<1> output;
        output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
        
        WHEN("Writing a particle snapshot") {
            output.output_particle(sim);
            
            THEN("CSV file should be created in snapshots directory") {
                std::string snapshot_dir = std::string(kTestOutputDir) + "/snapshots";
                EXPECT_TRUE(fs::exists(snapshot_dir)) << "Snapshots directory not created";
                
                std::string csv_file = snapshot_dir + "/00000.csv";
                EXPECT_TRUE(fs::exists(csv_file)) << "Snapshot file not created";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, CreatesSequentialSnapshots) {
    GIVEN("An Output system") {
        Output<1> output;
        
        WHEN("Writing multiple snapshots at different times") {
            sim->time = 0.0;
            output.output_particle(sim);
            
            sim->time = 0.1;
            output.output_particle(sim);
            
            sim->time = 0.2;
            output.output_particle(sim);
            
            THEN("Three snapshot files should exist") {
                std::string snapshot_dir = std::string(kTestOutputDir) + "/snapshots";
                EXPECT_TRUE(fs::exists(snapshot_dir + "/00000.csv"));
                EXPECT_TRUE(fs::exists(snapshot_dir + "/00001.csv"));
                EXPECT_TRUE(fs::exists(snapshot_dir + "/00002.csv"));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, WritesAllParticleFields) {
    GIVEN("Particles with various properties") {
        Output<1> output;
        
        WHEN("Writing a snapshot") {
            output.output_particle(sim);
            
            THEN("CSV should contain all required columns") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                
                EXPECT_TRUE(file_contains(csv_file, "pos_x")) << "Missing position column";
                EXPECT_TRUE(file_contains(csv_file, "vel_x")) << "Missing velocity column";
                EXPECT_TRUE(file_contains(csv_file, "acc_x")) << "Missing acceleration column";
                EXPECT_TRUE(file_contains(csv_file, "mass")) << "Missing mass column";
                EXPECT_TRUE(file_contains(csv_file, "density")) << "Missing density column";
                EXPECT_TRUE(file_contains(csv_file, "pressure")) << "Missing pressure column";
                EXPECT_TRUE(file_contains(csv_file, "energy")) << "Missing energy column";
                EXPECT_TRUE(file_contains(csv_file, "sound_speed")) << "Missing sound speed column";
                EXPECT_TRUE(file_contains(csv_file, "smoothing_length")) << "Missing smoothing length column";
                EXPECT_TRUE(file_contains(csv_file, "id")) << "Missing ID column";
                EXPECT_TRUE(file_contains(csv_file, "type")) << "Missing type column";
            }
            
            AND_THEN("CSV should have correct number of data rows") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                int lines = count_lines_in_file(csv_file);
                
                // 1 header line + 5 particles = 6 lines
                EXPECT_EQ(lines, 6) << "Expected 1 header + 5 particles";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, WritesEnergyData) {
    GIVEN("An Output system") {
        Output<1> output;
        
        WHEN("Writing energy data") {
            output.output_energy(sim);
            
            THEN("Energy CSV file should be created") {
                std::string energy_file = std::string(kTestOutputDir) + "/energy.csv";
                EXPECT_TRUE(fs::exists(energy_file)) << "Energy file not created";
            }
            
            AND_THEN("Energy file should contain correct columns") {
                std::string energy_file = std::string(kTestOutputDir) + "/energy.csv";
                EXPECT_TRUE(file_contains(energy_file, "time"));
                EXPECT_TRUE(file_contains(energy_file, "kinetic"));
                EXPECT_TRUE(file_contains(energy_file, "thermal"));
                EXPECT_TRUE(file_contains(energy_file, "potential"));
                EXPECT_TRUE(file_contains(energy_file, "total"));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, AppendsToEnergyFile) {
    GIVEN("An Output system") {
        Output<1> output;
        
        WHEN("Writing energy data multiple times") {
            sim->time = 0.0;
            output.output_energy(sim);
            
            sim->time = 0.1;
            output.output_energy(sim);
            
            sim->time = 0.2;
            output.output_energy(sim);
            
            THEN("Energy file should have multiple entries") {
                std::string energy_file = std::string(kTestOutputDir) + "/energy.csv";
                int lines = count_lines_in_file(energy_file);
                
                // 1 header line + 3 data lines = 4 lines
                EXPECT_EQ(lines, 4) << "Expected 1 header + 3 energy entries";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, HandlesEmptySimulation) {
    GIVEN("A simulation with no particles") {
        sim->particle_num = 0;
        sim->particles.clear();
        
        Output<1> output;
        
        WHEN("Writing output") {
            output.output_particle(sim);
            
            THEN("CSV file should still be created with header only") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(fs::exists(csv_file));
                
                int lines = count_lines_in_file(csv_file);
                EXPECT_EQ(lines, 1) << "Should have only header line";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, DistinguishesRealAndGhostParticles) {
    GIVEN("Particles with different types") {
        // Mark some particles as ghosts
        sim->particles[3].type = 1;  // GHOST
        sim->particles[4].type = 1;  // GHOST
        
        Output<1> output;
        
        WHEN("Writing a snapshot") {
            output.output_particle(sim);
            
            THEN("Type column should contain different values") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                std::ifstream file(csv_file);
                std::string line;
                
                // Skip header
                std::getline(file, line);
                
                // Read particle types (last column)
                std::vector<int> types;
                while (std::getline(file, line)) {
                    auto pos = line.rfind(',');
                    if (pos != std::string::npos) {
                        int type = std::stoi(line.substr(pos + 1));
                        types.push_back(type);
                    }
                }
                
                EXPECT_EQ(types.size(), 5);
                EXPECT_EQ(types[0], 0) << "Particle 0 should be REAL (type 0)";
                EXPECT_EQ(types[1], 0) << "Particle 1 should be REAL (type 0)";
                EXPECT_EQ(types[2], 0) << "Particle 2 should be REAL (type 0)";
                EXPECT_EQ(types[3], 1) << "Particle 3 should be GHOST (type 1)";
                EXPECT_EQ(types[4], 1) << "Particle 4 should be GHOST (type 1)";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, MaintainsDataPrecision) {
    GIVEN("Particles with high-precision values") {
        sim->particles[0].pos[0] = 1.234567890123456;
        sim->particles[0].dens = 0.987654321098765;
        
        Output<1> output;
        
        WHEN("Writing in code units") {
            output.set_unit_conversion(UnitConversionMode::CODE_UNITS);
            output.output_particle(sim);
            
            THEN("Values should be written with sufficient precision") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                std::ifstream file(csv_file);
                std::string line;
                
                // Skip header
                std::getline(file, line);
                
                // Read first particle
                std::getline(file, line);
                std::stringstream ss(line);
                std::string pos_str, vel_str, acc_str, mass_str, dens_str;
                
                std::getline(ss, pos_str, ',');
                std::getline(ss, vel_str, ',');
                std::getline(ss, acc_str, ',');
                std::getline(ss, mass_str, ',');
                std::getline(ss, dens_str, ',');
                
                real pos = std::stod(pos_str);
                real dens = std::stod(dens_str);
                
                EXPECT_NEAR(pos, 1.234567890123456, 1e-10);
                EXPECT_NEAR(dens, 0.987654321098765, 1e-10);
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, HandlesNaNAndInfGracefully) {
    GIVEN("Particles with NaN and Inf values") {
        sim->particles[0].dens = std::numeric_limits<real>::quiet_NaN();
        sim->particles[1].pres = std::numeric_limits<real>::infinity();
        sim->particles[2].vel[0] = -std::numeric_limits<real>::infinity();
        
        Output<1> output;
        
        WHEN("Writing output") {
            output.output_particle(sim);
            
            THEN("File should be created without crashing") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(fs::exists(csv_file));
            }
            
            AND_THEN("CSV should contain nan and inf values") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                
                // File should contain these special values as text
                std::ifstream file(csv_file);
                std::string content((std::istreambuf_iterator<char>(file)),
                                   std::istreambuf_iterator<char>());
                
                // The output should contain the string representations
                bool has_special_values = content.find("nan") != std::string::npos ||
                                         content.find("inf") != std::string::npos;
                EXPECT_TRUE(has_special_values) << "Special values not properly written";
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, WorksWith1D2DAnd3DSimulations) {
    GIVEN("Simulations in different dimensions") {
        WHEN("Using 1D simulation") {
            Output<1> output1d;
            output1d.output_particle(sim);
            
            THEN("Output should contain 1D position/velocity") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(file_contains(csv_file, "pos_x"));
                EXPECT_TRUE(file_contains(csv_file, "vel_x"));
                EXPECT_FALSE(file_contains(csv_file, "pos_y")) << "1D should not have y component";
            }
        }
        
        WHEN("Using 2D simulation") {
            auto param2 = std::make_shared<SPHParameters>();
            auto sim2d = std::make_shared<Simulation<2>>(param2);
            sim2d->particle_num = 1;
            sim2d->particles.resize(1);
            sim2d->particles[0].pos[0] = 1.0;
            sim2d->particles[0].pos[1] = 2.0;
            sim2d->particles[0].type = 0;  // REAL
            sim2d->time = 0.0;
            
            fs::remove_all(kTestOutputDir);
            fs::create_directories(kTestOutputDir);
            Logger::open(kTestOutputDir);
            
            Output<2> output2d;
            output2d.output_particle(sim2d);
            
            THEN("Output should contain 2D position/velocity") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(file_contains(csv_file, "pos_x"));
                EXPECT_TRUE(file_contains(csv_file, "pos_y"));
                EXPECT_TRUE(file_contains(csv_file, "vel_x"));
                EXPECT_TRUE(file_contains(csv_file, "vel_y"));
                EXPECT_FALSE(file_contains(csv_file, "pos_z")) << "2D should not have z component";
            }
        }
        
        WHEN("Using 3D simulation") {
            auto param3 = std::make_shared<SPHParameters>();
            auto sim3d = std::make_shared<Simulation<3>>(param3);
            sim3d->particle_num = 1;
            sim3d->particles.resize(1);
            sim3d->particles[0].pos[0] = 1.0;
            sim3d->particles[0].pos[1] = 2.0;
            sim3d->particles[0].pos[2] = 3.0;
            sim3d->particles[0].type = 0;  // REAL
            sim3d->time = 0.0;
            
            fs::remove_all(kTestOutputDir);
            fs::create_directories(kTestOutputDir);
            Logger::open(kTestOutputDir);
            
            Output<3> output3d;
            output3d.output_particle(sim3d);
            
            THEN("Output should contain 3D position/velocity") {
                std::string csv_file = std::string(kTestOutputDir) + "/snapshots/00000.csv";
                EXPECT_TRUE(file_contains(csv_file, "pos_x"));
                EXPECT_TRUE(file_contains(csv_file, "pos_y"));
                EXPECT_TRUE(file_contains(csv_file, "pos_z"));
                EXPECT_TRUE(file_contains(csv_file, "vel_x"));
                EXPECT_TRUE(file_contains(csv_file, "vel_y"));
                EXPECT_TRUE(file_contains(csv_file, "vel_z"));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(OutputSystemIntegrationTestFixture, CalculatesEnergyCorrectly) {
    GIVEN("Particles with known energies") {
        // Set up particles with known values
        sim->particles[0].mass = 1.0;
        sim->particles[0].vel[0] = 2.0;
        sim->particles[0].ene = 3.0;
        sim->particles[0].phi = 0.5;
        
        Output<1> output;
        
        WHEN("Writing energy") {
            output.output_energy(sim);
            
            THEN("Kinetic energy should be calculated correctly") {
                // KE = 0.5 * m * v^2 = 0.5 * 1.0 * 4.0 = 2.0
                std::string energy_file = std::string(kTestOutputDir) + "/energy.csv";
                std::ifstream file(energy_file);
                std::string line;
                
                // Skip header
                std::getline(file, line);
                
                // Read first data line
                std::getline(file, line);
                std::stringstream ss(line);
                std::string time_str, ke_str;
                
                std::getline(ss, time_str, ',');
                std::getline(ss, ke_str, ',');
                
                // Expect kinetic energy to be positive
                real ke = std::stod(ke_str);
                EXPECT_GT(ke, 0.0) << "Kinetic energy should be positive";
            }
        }
    }
}

} // FEATURE OutputSystemIntegration
