/**
 * @file metadata_writer_test.cpp
 * @brief BDD-style tests for MetadataWriter
 * 
 * Test-Driven Development: Write tests first, then implement to pass.
 * Tests JSON metadata generation, schema versioning, timestamp formatting.
 */

#include <gtest/gtest.h>
#include "../bdd_helpers.hpp"
#include "core/output/writers/metadata_writer.hpp"
#include "core/output/units/unit_system.hpp"
#include "core/output/units/galactic_unit_system.hpp"
#include "core/output/units/si_unit_system.hpp"
#include "core/parameters/simulation_parameters.hpp"
#include "core/parameters/physics_parameters.hpp"
#include "core/parameters/computational_parameters.hpp"
#include "core/parameters/output_parameters.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <filesystem>

using namespace sph;
using json = nlohmann::json;
namespace fs = std::filesystem;

// Test constants
namespace {
    constexpr const char* kTestOutputDir = "test_output_metadata";
}

// Test fixture for metadata writer tests
class MetadataWriterTestFixture : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test output directory
        fs::create_directories(kTestOutputDir);
        
        // Create test parameters
        params = std::make_shared<SPHParameters>();
        params->get_physics().gamma = 1.4;
        params->get_physics().neighbor_number = 50;
        params->computational.tree.max_particles_per_leaf = 20;
        params->computational.tree.opening_angle = 0.5;
        params->output.directory = kTestOutputDir;
        params->get_time().end = 1.0;
        params->get_time().dt_out = 0.1;
        
        // Create unit systems
        galactic_units = std::make_shared<GalacticUnitSystem>();
        si_units = std::make_shared<SIUnitSystem>();
    }
    
    void TearDown() override {
        // Clean up test output directory
        if (fs::exists(kTestOutputDir)) {
            fs::remove_all(kTestOutputDir);
        }
    }
    
    // Helper to read JSON file
    json read_json_file(const std::string& filename) const {
        std::ifstream file(filename);
        json j;
        file >> j;
        return j;
    }
    
    std::shared_ptr<SPHParameters> params;
    std::shared_ptr<UnitSystem> galactic_units;
    std::shared_ptr<UnitSystem> si_units;
};

// ============================================================================
// FEATURE: Metadata File Generation
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, CreatesMetadataFile) {
    GIVEN("A metadata writer and simulation parameters") {
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            
            THEN("Metadata file should be created") {
                EXPECT_TRUE(fs::exists(std::string(kTestOutputDir) + "/metadata.json"));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, GeneratesValidJSON) {
    GIVEN("A metadata writer and parameters") {
        WHEN("We write and read back metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("JSON should be valid and parseable") {
                EXPECT_FALSE(metadata.empty());
            }
            
            AND_THEN("Required top-level fields should exist") {
                EXPECT_TRUE(metadata.contains("schema_version"));
                EXPECT_TRUE(metadata.contains("timestamp"));
                EXPECT_TRUE(metadata.contains("unit_system"));
                EXPECT_TRUE(metadata.contains("physics"));
                EXPECT_TRUE(metadata.contains("computational"));
            }
        }
    }
}

// ============================================================================
// FEATURE: Schema Versioning
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, IncludesSchemaVersion) {
    GIVEN("Metadata output") {
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Schema version should be present") {
                EXPECT_TRUE(metadata.contains("schema_version"));
                EXPECT_EQ(metadata["schema_version"], "1.0.0");
            }
        }
    }
}

// ============================================================================
// FEATURE: Unit System Information
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, RecordsGalacticUnits) {
    GIVEN("Galactic unit system") {
        WHEN("We write metadata with galactic units") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Unit system should be galactic") {
                EXPECT_EQ(metadata["unit_system"]["name"], "galactic");
            }
            
            AND_THEN("Fundamental units should be documented") {
                auto& units = metadata["unit_system"];
                EXPECT_TRUE(units.contains("length_unit"));
                EXPECT_TRUE(units.contains("mass_unit"));
                EXPECT_TRUE(units.contains("time_unit"));
            }
        }
    }
}

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, RecordsSIUnits) {
    GIVEN("SI unit system") {
        WHEN("We write metadata with SI units") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, si_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Unit system should be SI") {
                EXPECT_EQ(metadata["unit_system"]["name"], "SI");
            }
        }
    }
}

// ============================================================================
// FEATURE: Physics Parameters
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, RecordsPhysicsParameters) {
    GIVEN("Physics parameters") {
        params->get_physics().gamma = 1.4;
        params->get_physics().neighbor_number = 50;
        
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Physics parameters should be recorded") {
                EXPECT_EQ(metadata["physics"]["gamma"], 1.4);
                EXPECT_EQ(metadata["physics"]["neighbor_number"], 50);
            }
        }
    }
}

// ============================================================================
// FEATURE: Computational Parameters
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, RecordsComputationalParameters) {
    GIVEN("Computational parameters") {
        params->computational.tree.max_particles_per_leaf = 20;
        params->computational.tree.opening_angle = 0.5;
        
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Computational parameters should be recorded") {
                EXPECT_TRUE(metadata["computational"].contains("tree"));
            }
        }
    }
}

// ============================================================================
// FEATURE: Timestamp Formatting
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, IncludesISO8601Timestamp) {
    GIVEN("Metadata generation") {
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Timestamp should be in ISO 8601 format") {
                EXPECT_TRUE(metadata.contains("timestamp"));
                std::string timestamp = metadata["timestamp"];
                
                // ISO 8601 format contains 'T' separator and colons
                EXPECT_TRUE(timestamp.find('T') != std::string::npos);
                EXPECT_TRUE(timestamp.find(':') != std::string::npos);
                
                // Should be at least 19 chars: YYYY-MM-DDTHH:MM:SS
                EXPECT_GE(timestamp.length(), 19);
            }
        }
    }
}

// ============================================================================
// FEATURE: Output Configuration
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, RecordsOutputConfiguration) {
    GIVEN("Output parameters") {
        params->output.directory = "output";
        params->get_time().dt_out = 0.1;
        params->get_time().dt_energy = 0.01;
        
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("Output configuration should be recorded") {
                EXPECT_TRUE(metadata.contains("output"));
                // Check for presence of output-related fields
            }
        }
    }
}

// ============================================================================
// FEATURE: SPH Algorithm Type
// ============================================================================

SCENARIO_WITH_FIXTURE(MetadataWriterTestFixture, RecordsSPHAlgorithmType) {
    GIVEN("SPH parameters with algorithm type") {
        params->get_type() = SPHType::GSPH;
        
        WHEN("We write metadata") {
            MetadataWriter::write_metadata(kTestOutputDir, *params, galactic_units);
            json metadata = read_json_file(std::string(kTestOutputDir) + "/metadata.json");
            
            THEN("SPH algorithm type should be recorded") {
                EXPECT_TRUE(metadata.contains("sph_type") || metadata.contains("algorithm"));
            }
        }
    }
}
