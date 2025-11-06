#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>
#include "../../parameters.hpp"

namespace sph
{

/**
 * @brief Registry for SPH algorithm types, enabling extensibility without core code changes.
 * 
 * This class implements the Registry pattern to map string names to SPH algorithm types.
 * New algorithms can be registered at runtime, eliminating hardcoded if-else chains.
 * 
 * Usage:
 *   // Query existing algorithm
 *   SPHType type = SPHAlgorithmRegistry::get_type("gsph");
 *   
 *   // Register custom algorithm (future extensibility)
 *   SPHAlgorithmRegistry::register_algorithm("my_sph", SPHType::CUSTOM);
 */
class SPHAlgorithmRegistry {
private:
    static std::unordered_map<std::string, SPHType> registry;
    static bool initialized;
    
    static void initialize();
    
public:
    /**
     * @brief Get SPH type by algorithm name.
     * @param name Algorithm name (e.g., "ssph", "disph", "gsph")
     * @return SPHType enum value
     * @throws std::runtime_error if algorithm name is not registered
     */
    static SPHType get_type(const std::string& name);
    
    /**
     * @brief Register a new SPH algorithm type.
     * @param name Algorithm name (lowercase recommended)
     * @param type SPH type enum value
     * @throws std::runtime_error if name already registered
     */
    static void register_algorithm(const std::string& name, SPHType type);
    
    /**
     * @brief Check if algorithm is registered.
     * @param name Algorithm name
     * @return true if registered, false otherwise
     */
    static bool is_registered(const std::string& name);
    
    /**
     * @brief Get list of all registered algorithm names.
     * @return Vector of algorithm names
     */
    static std::vector<std::string> list_algorithms();
    
    /**
     * @brief Get human-readable name for SPH type.
     * @param type SPH type enum
     * @return Descriptive name
     */
    static std::string get_name(SPHType type);
};

}
