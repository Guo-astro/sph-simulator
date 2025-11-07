#!/bin/bash

# Script to replace direct parameter member access with getter calls in test files
# This enforces the private member pattern with type-safe getters

find tests -name "*.cpp" -type f | while read -r file; do
    echo "Processing: $file"
    
    # Replace params->type with params->get_type()
    sed -i '' 's/params->type\([^_]\)/params->get_type()\1/g' "$file"
    
    # Replace params->av. with params->get_av().
    sed -i '' 's/params->av\./params->get_av()./g' "$file"
    
    # Replace params->ac. with params->get_ac().
    sed -i '' 's/params->ac\./params->get_ac()./g' "$file"
    
    # Replace params->time. with params->get_time().
    sed -i '' 's/params->time\./params->get_time()./g' "$file"
    
    # Replace params->cfl. with params->get_cfl().
    sed -i '' 's/params->cfl\./params->get_cfl()./g' "$file"
    
    # Replace params->physics. with params->get_physics().
    sed -i '' 's/params->physics\./params->get_physics()./g' "$file"
    
    # Replace params->kernel with params->get_kernel()
    sed -i '' 's/params->kernel\([^_]\)/params->get_kernel()\1/g' "$file"
    
    # Replace params->gravity.is_valid with params->has_gravity()
    sed -i '' 's/params->gravity\.is_valid/params->has_gravity()/g' "$file"
    
    # Replace params->gravity.constant with params->get_newtonian_gravity().constant
    sed -i '' 's/params->gravity\.constant/params->get_newtonian_gravity().constant/g' "$file"
    
    # Replace params->gravity.theta with params->get_newtonian_gravity().theta
    sed -i '' 's/params->gravity\.theta/params->get_newtonian_gravity().theta/g' "$file"
    
    # Replace params->gravity. with params->get_gravity().
    sed -i '' 's/params->gravity\./params->get_gravity()./g' "$file"
    
    # Replace params->gsph. with params->get_gsph().
    sed -i '' 's/params->gsph\./params->get_gsph()./g' "$file"
    
    # Replace params->periodic. with params->get_periodic().
    sed -i '' 's/params->periodic\./params->get_periodic()./g' "$file"
    
    # Replace params->tree. with params->get_tree().
    sed -i '' 's/params->tree\./params->get_tree()./g' "$file"
    
    # Replace params->boundary. with params->get_boundary().
    sed -i '' 's/params->boundary\./params->get_boundary()./g' "$file"
    
    # Replace params->iterative_sml with params->get_iterative_sml()
    sed -i '' 's/params->iterative_sml/params->get_iterative_sml()/g' "$file"
    
    # Replace params->dimension with params->get_dimension()
    sed -i '' 's/params->dimension/params->get_dimension()/g' "$file"
done

echo "Test files updated with getter access patterns"
