#!/bin/bash
# Comprehensive test script for all workflows
# Tests that each workflow can build successfully

set -e  # Exit on first error

echo "======================================"
echo "Testing All Workflows Build"
echo "======================================"
echo ""

WORKFLOWS=(
    "evrard_workflow/01_simulation"
    "gresho_chan_vortex_workflow/01_simulation"
    "hydrostatic_workflow/01_simulation"
    "khi_workflow/01_simulation"
    "pairing_instability_workflow/01_simulation"
)

FAILED=()
PASSED=()

for workflow in "${WORKFLOWS[@]}"; do
    echo "======================================"
    echo "Testing: $workflow"
    echo "======================================"
    
    cd "/Users/guo/sph-simulation/workflows/$workflow"
    
    if make clean > /dev/null 2>&1 && make build > build_test.log 2>&1; then
        echo "✓ $workflow: BUILD SUCCESSFUL"
        PASSED+=("$workflow")
    else
        echo "✗ $workflow: BUILD FAILED"
        echo "  See build_test.log for details"
        FAILED+=("$workflow")
    fi
    
    echo ""
done

echo "======================================"
echo "Summary"
echo "======================================"
echo "Passed: ${#PASSED[@]}"
echo "Failed: ${#FAILED[@]}"
echo ""

if [ ${#PASSED[@]} -gt 0 ]; then
    echo "✓ Successful builds:"
    for workflow in "${PASSED[@]}"; do
        echo "  - $workflow"
    done
    echo ""
fi

if [ ${#FAILED[@]} -gt 0 ]; then
    echo "✗ Failed builds:"
    for workflow in "${FAILED[@]}"; do
        echo "  - $workflow"
    done
    echo ""
    exit 1
fi

echo "======================================"
echo "All workflows built successfully!"
echo "======================================"
