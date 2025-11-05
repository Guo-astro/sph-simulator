#pragma once

#include <cmath>

/**
 * @file defines.hpp
 * @brief Global definitions and compile-time constants
 * 
 * DIM macro has been completely removed from the codebase.
 * All dimension-dependent code now uses template parameter Dim or constexpr.
 */

typedef double real;

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795028841971693993751
#endif

// Helper functions for power operations
inline real sqr(real x) { return x * x; }
inline real pow3(real x) { return x * x * x; }
inline real pow4(real x) { return x * x * x * x; }
inline real pow5(real x) { return x * x * x * x * x; }
inline real pow6(real x) { return x * x * x * x * x * x; }

// Template helper for h^Dim used in kernel normalization
template<int Dim>
inline constexpr real powh(real h) {
    if constexpr(Dim == 1) return h;
    else if constexpr(Dim == 2) return h * h;
    else if constexpr(Dim == 3) return h * h * h;
}

constexpr int neighbor_list_size = 20;

// for debug
//#define EXHAUSTIVE_SEARCH_ONLY_FOR_DEBUG
