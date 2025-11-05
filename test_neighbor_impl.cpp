#include "core/neighbor_search_result.hpp"
#include "core/neighbor_collector.hpp"
#include "core/neighbor_search_config.hpp"
#include <iostream>
#include <cassert>

// Quick manual tests to verify implementation works
int main() {
    std::cout << "Testing NeighborSearchResult..." << std::endl;
    
    // Test 1: Valid result
    NeighborSearchResult result1{
        .neighbor_indices = {0, 5, 10, 15},
        .is_truncated = false,
        .total_candidates_found = 4
    };
    assert(result1.is_valid());
    assert(result1.size() == 4);
    assert(!result1.empty());
    std::cout << "  ✓ Valid result test passed" << std::endl;
    
    // Test 2: Invalid result (negative index)
    NeighborSearchResult result2{
        .neighbor_indices = {0, -1, 10},
        .is_truncated = false,
        .total_candidates_found = 3
    };
    assert(!result2.is_valid());
    std::cout << "  ✓ Invalid index detection test passed" << std::endl;
    
    // Test 3: Empty result
    NeighborSearchResult result3{
        .neighbor_indices = {},
        .is_truncated = false,
        .total_candidates_found = 0
    };
    assert(result3.empty());
    assert(result3.is_valid());
    std::cout << "  ✓ Empty result test passed" << std::endl;
    
    std::cout << "\nTesting NeighborCollector..." << std::endl;
    
    // Test 4: Add within capacity
    NeighborCollector collector1(5);
    assert(collector1.try_add(10));
    assert(collector1.try_add(20));
    assert(collector1.try_add(30));
    assert(!collector1.is_full());
    std::cout << "  ✓ Add within capacity test passed" << std::endl;
    
    // Test 5: Exceed capacity
    NeighborCollector collector2(3);
    collector2.try_add(1);
    collector2.try_add(2);
    collector2.try_add(3);
    assert(collector2.is_full());
    assert(!collector2.try_add(4));  // Should fail
    auto result4 = std::move(collector2).finalize();
    assert(result4.size() == 3);
    assert(result4.is_truncated);
    assert(result4.total_candidates_found == 4);
    std::cout << "  ✓ Exceed capacity test passed" << std::endl;
    
    // Test 6: Reject negative index
    NeighborCollector collector3(5);
    assert(!collector3.try_add(-1));
    auto result5 = std::move(collector3).finalize();
    assert(result5.size() == 0);
    assert(result5.total_candidates_found == 1);
    std::cout << "  ✓ Negative index rejection test passed" << std::endl;
    
    std::cout << "\nTesting NeighborSearchConfig..." << std::endl;
    
    // Test 7: Valid config creation
    auto config1 = NeighborSearchConfig::create(6, false);
    assert(config1.is_valid());
    assert(config1.max_neighbors == 120);  // 6 * 20
    assert(!config1.use_max_kernel);
    std::cout << "  ✓ Valid config creation test passed" << std::endl;
    
    // Test 8: Invalid config (negative neighbor_number)
    bool threw_exception = false;
    try {
        auto bad_config = NeighborSearchConfig::create(-5, false);
    } catch (const std::invalid_argument&) {
        threw_exception = true;
    }
    assert(threw_exception);
    std::cout << "  ✓ Invalid config rejection test passed" << std::endl;
    
    // Test 9: Sanity check upper bound
    NeighborSearchConfig config2{
        .max_neighbors = 1000000,
        .use_max_kernel = false
    };
    assert(!config2.is_valid());  // Too large
    std::cout << "  ✓ Upper bound sanity check test passed" << std::endl;
    
    std::cout << "\n✅ ALL TESTS PASSED!" << std::endl;
    return 0;
}
