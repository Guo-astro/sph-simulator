#!/bin/bash

# Fix test includes for reorganized structure
find tests -name "*.cpp" -type f -exec sed -i.bak \
  -e 's|../../include/core/vector\.hpp|../../include/core/utilities/vector.hpp|g' \
  -e 's|../../include/core/cubic_spline\.hpp|../../include/core/kernels/cubic_spline.hpp|g' \
  -e 's|../../include/core/wendland_kernel\.hpp|../../include/core/kernels/wendland_kernel.hpp|g' \
  -e 's|../../include/core/sph_particle\.hpp|../../include/core/particles/sph_particle.hpp|g' \
  -e 's|../../include/core/sph_particle_2_5d\.hpp|../../include/core/particles/sph_particle_2_5d.hpp|g' \
  -e 's|../../include/core/bhtree\.hpp|../../include/core/spatial/bhtree.hpp|g' \
  -e 's|../../include/core/bhtree_2_5d\.hpp|../../include/core/spatial/bhtree_2_5d.hpp|g' \
  -e 's|../../include/core/cubic_spline_2_5d\.hpp|../../include/core/kernels/cubic_spline_2_5d.hpp|g' \
  -e 's|../../include/core/simulation_2_5d\.hpp|../../include/core/simulation/simulation_2_5d.hpp|g' \
  -e 's|../../include/core/periodic\.hpp|../../include/core/boundaries/periodic.hpp|g' \
  {} \;

# Remove backup files
find tests -name "*.bak" -type f -delete

echo "Test includes fixed"
