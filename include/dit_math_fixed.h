#ifndef DIT_MATH_FIXED_H
#define DIT_MATH_FIXED_H

/**
 * dit_math_fixed.h
 * Provides fixed-point trigonometric and pulse synthesis functions for Smopsys.
 * Standardizes PI and PHI for the Metriplectic Kernel.
 */

#include <stdint.h>
#include "dit_physics.h"

// Fixed-point configuration (Q16.16)
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)

typedef int32_t fixed_t;

/**
 * dit_cos_fixed
 * 4th order Taylor approximation for cosine.
 * Optimized for normalized phase within [0, 2*PI].
 */
static inline fixed_t dit_cos_fixed(fixed_t x) {
    // Normalize x to [-PI_FP, PI_FP] for better Taylor accuracy
    while (x > PI_FP) x -= 2 * PI_FP;
    while (x < -PI_FP) x += 2 * PI_FP;

    fixed_t sign = FP_ONE;
    // Map to [-PI/2, PI/2] using cos(x) = -cos(pi-x) etc.
    if (x > (PI_FP / 2)) {
        x = PI_FP - x;
        sign = -FP_ONE;
    } else if (x < -(PI_FP / 2)) {
        x = -PI_FP - x;
        sign = -FP_ONE;
    }

    int64_t x_long = x;
    int64_t x2 = (x_long * x_long) >> FP_SHIFT;
    int64_t x4 = (x2 * x2) >> FP_SHIFT;

    fixed_t term2 = (fixed_t)(x2 >> 1);
    fixed_t term4 = (fixed_t)(x4 / 24);

    fixed_t cos_val = FP_ONE - term2 + term4;

    return (fixed_t)(((int64_t)sign * cos_val) >> FP_SHIFT);
}

/**
 * get_golden_operator_fixed
 * Calculates O_n = cos(pi * n) * cos(pi * phi * n + delta)
 */
static inline fixed_t get_golden_operator_fixed(int n, fixed_t delta) {
    // Parity: cos(pi * n) is just (-1)^n
    fixed_t parity = (n % 2 == 0) ? FP_ONE : -FP_ONE;

    // Quasiperiodic phase: pi * phi * n + delta
    // Note: Use 64-bit for intermediate phase to avoid overflow
    int64_t phase = (((int64_t)PI_FP * PHI_CONJUGATE_FP) >> FP_SHIFT) * n;
    phase += delta;

    // Apply cosine to the quasiperiodic part
    fixed_t qp_cos = dit_cos_fixed((fixed_t)phase);

    return (fixed_t)(((int64_t)parity * qp_cos) >> FP_SHIFT);
}

#endif
