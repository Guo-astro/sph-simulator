# System Tests

End-to-end workflow tests that validate complete simulation scenarios.

**Execution Time:** < 5 minutes total  
**Purpose:** Validate complete workflows (setup → simulation → output → validation)

## Test Coverage

- Full shock tube simulations (1D, 2D, 3D)
- Multi-algorithm comparisons
- Plugin loading and execution
- Output format validation

## Adding System Tests

System tests should:
1. Use realistic problem configurations
2. Validate against known solutions or benchmarks
3. Check output file integrity
4. Verify conservation laws
