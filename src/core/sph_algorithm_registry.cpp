#include "core/sph_algorithm_registry.hpp"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace sph
{

std::unordered_map<std::string, SPHType> SPHAlgorithmRegistry::registry;
bool SPHAlgorithmRegistry::initialized = false;

void SPHAlgorithmRegistry::initialize() {
    if (initialized) return;
    
    // Register standard SPH algorithms
    registry["ssph"] = SPHType::SSPH;
    registry["disph"] = SPHType::DISPH;
    registry["gsph"] = SPHType::GSPH;
    
    initialized = true;
}

SPHType SPHAlgorithmRegistry::get_type(const std::string& name) {
    initialize();
    
    auto it = registry.find(name);
    if (it == registry.end()) {
        throw std::runtime_error(
            "Unknown SPH algorithm: '" + name + "'. " +
            "Available algorithms: " + 
            [&]() {
                std::string list;
                for (const auto& pair : registry) {
                    if (!list.empty()) list += ", ";
                    list += pair.first;
                }
                return list;
            }()
        );
    }
    
    return it->second;
}

void SPHAlgorithmRegistry::register_algorithm(const std::string& name, SPHType type) {
    initialize();
    
    if (registry.find(name) != registry.end()) {
        throw std::runtime_error(
            "SPH algorithm '" + name + "' is already registered"
        );
    }
    
    registry[name] = type;
}

bool SPHAlgorithmRegistry::is_registered(const std::string& name) {
    initialize();
    return registry.find(name) != registry.end();
}

std::vector<std::string> SPHAlgorithmRegistry::list_algorithms() {
    initialize();
    
    std::vector<std::string> algorithms;
    algorithms.reserve(registry.size());
    
    for (const auto& pair : registry) {
        algorithms.push_back(pair.first);
    }
    
    std::sort(algorithms.begin(), algorithms.end());
    return algorithms;
}

std::string SPHAlgorithmRegistry::get_name(SPHType type) {
    switch (type) {
        case SPHType::SSPH:  return "Standard SPH";
        case SPHType::DISPH: return "Density Independent SPH";
        case SPHType::GSPH:  return "Godunov SPH";
        default:             return "Unknown";
    }
}

} // namespace sph
