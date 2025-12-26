#ifndef DIT_PHYSICS_H
#define DIT_PHYSICS_H

/**
 * @file dit_physics.h
 * @brief Centralized Physical Constants for Smopsys (QSOS).
 * * Standardizes universal constants for both Floating-Point (Simulation)
 * and Fixed-Point (Bare Metal) environments.
 */

// --- Floating Point Constants (Simulation/Tests) ---
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef PHI
#define PHI 1.618033988749895
#endif

#ifndef PHI_CONJUGATE
#define PHI_CONJUGATE 0.6180339887498948
#endif

// --- Fixed-Point Constants (Q16.16) ---
// Scaling: value * 2^16
#define PI_FP  205887        // 3.14159... * 65536
#define PHI_FP 106044       // 1.61803... * 65536
#define PHI_CONJUGATE_FP 40502       // 0.61803... * 65536

// --- Phase Offsets ---
#define DIT_DELTA_DEFAULT 0.18

#endif // DIT_PHYSICS_H
