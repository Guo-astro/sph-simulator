#!/bin/bash

# Script to replace direct parameter member access with getter calls
# Applies to include/, src/, and workflows/ directories

for dir in include src workflows; do
    find "$dir" -name "*.cpp" -o -name "*.tpp" -o -name "*.hpp" | while read -r file; do
        # Skip parameters.hpp itself
        if [[ "$file" == *"parameters.hpp"* ]]; then
            continue
        fi
        
        echo "Processing: $file"
        
        # Replace params->type with params->get_type()
        sed -i '' 's/\([^_]\)params\?->type\([^_]\)/\1params->get_type()\2/g' "$file"
        
        # Replace param->av. with param->get_av().
        sed -i '' 's/param\(s\)\?->av\./param->get_av()./g' "$file"
        
        # Replace param->ac. with param->get_ac().
        sed -i '' 's/param\(s\)\?->ac\./param->get_ac()./g' "$file"
        
        # Replace param->time. with param->get_time().
        sed -i '' 's/param\(s\)\?->time\./param->get_time()./g' "$file"
        
        # Replace param->cfl. with param->get_cfl().
        sed -i '' 's/param\(s\)\?->cfl\./param->get_cfl()./g' "$file"
        
        # Replace param->physics. with param->get_physics().
        sed -i '' 's/param\(s\)\?->physics\./param->get_physics()./g' "$file"
        
        # Replace param->kernel with param->get_kernel()
        sed -i '' 's/param\(s\)\?->kernel\([^_]\)/param->get_kernel()\2/g' "$file"
        
        # Replace param->gsph. with param->get_gsph().
        sed -i '' 's/param\(s\)\?->gsph\./param->get_gsph()./g' "$file"
        
        # Replace param->periodic. with param->get_periodic().
        sed -i '' 's/param\(s\)\?->periodic\./param->get_periodic()./g' "$file"
        
        # Replace param->tree. with param->get_tree().
        sed -i '' 's/param\(s\)\?->tree\./param->get_tree()./g' "$file"
        
        # Replace param->boundary. with param->get_boundary().
        sed -i '' 's/param\(s\)\?->boundary\./param->get_boundary()./g' "$file"
        
        # Replace param->iterative_sml with param->get_iterative_sml()
        sed -i '' 's/param\(s\)\?->iterative_sml/param->get_iterative_sml()/g' "$file"
        
        # Replace param->dimension with param->get_dimension()
        sed -i '' 's/param\(s\)\?->dimension/param->get_dimension()/g' "$file"
    done
done

echo "All source files updated with getter access patterns"
