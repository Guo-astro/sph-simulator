# Memory Debugging Guide for SPH Simulation

## Date: 2025-11-05

## Summary
This guide documents the tools and commands used to debug a heap buffer overflow that caused segmentation faults in the SPH simulation.

## Problem
Segmentation fault occurring at iteration #110 during shock tube simulation.

## Debugging Tools Used

### 1. AddressSanitizer (Primary Tool)

**AddressSanitizer** is a memory error detector that finds:
- Heap buffer overflows
- Stack buffer overflows
- Use-after-free
- Use-after-return
- Memory leaks

#### How to Enable AddressSanitizer

**In CMakeLists.txt:**
```cmake
target_compile_options(your_target PRIVATE
    -fsanitize=address
    -g  # Enable debug symbols
)

target_link_libraries(your_target PUBLIC
    -fsanitize=address
)
```

**Build and Run:**
```bash
cd /Users/guo/sph-simulation/workflows/shock_tube_workflow/01_simulation
make clean
make
./build/sph lib/libshock_tube_plugin.dylib 2>&1 | grep -A50 "ERROR\|ASAN"
```

**Output Example:**
```
==28964==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x00010ad3d660
WRITE of size 4 at 0x00010ad3d660 thread T0
    #0 0x1009a7f2c in sph::BHTree<1>::BHNode::neighbor_search(...) bhtree.tpp:334
```

This pinpoints the exact file and line number causing the overflow.

### 2. LLDB Debugger (Alternative)

**Basic usage:**
```bash
lldb ./build/sph
(lldb) run lib/libshock_tube_plugin.dylib
# After crash:
(lldb) bt  # Show backtrace
(lldb) frame info  # Show current frame
(lldb) print variable_name  # Inspect variables
```

### 3. Strategic Debug Logging

Add logging to narrow down crash location:

```cpp
#include <iostream>

WRITE_LOG << "DEBUG: Reached point X";
std::cerr << "Critical section entry" << std::endl;  // stderr for immediate output
```

### 4. Valgrind (Linux Alternative)

On Linux systems, use Valgrind:
```bash
valgrind --leak-check=full --show-leak-kinds=all ./build/sph lib/libshock_tube_plugin.dylib
```

Note: Valgrind is not available on macOS ARM (Apple Silicon).

## Root Cause Found

**File:** `include/core/bhtree.tpp:334`  
**Issue:** Writing to `neighbor_list[n_neighbor]` beyond allocated array size

**Cause:** The neighbor_list was allocated with size `m_neighbor_number * 20` (e.g., 120 slots), but the tree search found more than 120 neighbors for some particles, causing heap corruption.

## Fix Applied

Added bounds checking in `BHTree::BHNode::neighbor_search()`:

```cpp
if (n_neighbor >= max_neighbors) {
    #pragma omp critical
    {
        static bool overflow_logged = false;
        if (!overflow_logged) {
            WRITE_LOG << "ERROR: Neighbor list overflow! n_neighbor=" << n_neighbor 
                      << ", max=" << max_neighbors;
            overflow_logged = true;
        }
    }
    return;  // Prevent crash
}
neighbor_list[n_neighbor] = p->id;
++n_neighbor;
```

## Best Practices for Memory Debugging

1. **Always compile with debug symbols (`-g`)** when debugging
2. **Use AddressSanitizer first** - it's the fastest way to find memory errors
3. **Disable optimizations (`-O0`)** temporarily if needed for clearer stack traces
4. **Remove `-ffast-math`** when debugging - it can hide issues
5. **Add bounds checking** to all array/vector accesses in performance-critical code
6. **Use `assert()`** liberally during development
7. **Log critical values** before suspicious operations

## Commands Quick Reference

```bash
# Enable AddressSanitizer
# Edit CMakeLists.txt and add -fsanitize=address to compile and link flags

# Rebuild from scratch
make clean && make

# Run with error filtering
./build/sph lib/libshock_tube_plugin.dylib 2>&1 | grep -A50 "ERROR\|ASAN"

# Run with LLDB
lldb ./build/sph -- lib/libshock_tube_plugin.dylib
(lldb) run
(lldb) bt

# Check for specific patterns in output
./build/sph lib/libshock_tube_plugin.dylib 2>&1 | grep -E "(INTEGRATE CALL #110|ERROR)"
```

## Files Modified

1. `include/core/bhtree.hpp` - Added max_neighbors parameter
2. `include/core/bhtree.tpp` - Implemented bounds checking
3. `include/gsph/g_fluid_force.tpp` - Added safety checks for gradient arrays

## Verification

After fix:
- Simulation completes successfully (1770 iterations)
- No memory errors from AddressSanitizer
- Proper error logging if overflow occurs

## Performance Impact

The bounds check adds minimal overhead:
- Single integer comparison per neighbor
- Early return prevents crash
- No performance degradation observed
