/*
 * Golden Operator - Implementación
 * Smopsys Q-CORE
 * 
 * Implementación del Operador Cuasiperiódico con:
 * - Funciones trigonométricas propias (sin libmath)
 * - Separación explícita L_symp + L_metr (Mandato Metripléctico)
 * - Cálculo de observables cuánticos (IPR, Reynolds, etc.)
 */

#include "golden_operator.h"

/* ============================================================
 * NOTA: FUNCIONES MATEMÁTICAS REFACTORIZADAS
 * 
 * Las funciones trigonométricas ahora usan `fixed_t` y se
 * encuentran en `include/dit_math_fixed.h`.
 * ============================================================ */

/* ============================================================
 * OPERADOR ÁUREO PRINCIPAL
 * 
 * Ô_n = cos(πn) · cos(πφn)
 * 
 * Este es el corazón del sistema cuasiperiódico.
 * ============================================================ */

/* ============================================================
 * INICIALIZACIÓN DEL ESTADO
 * ============================================================ */

void golden_operator_init(GoldenState *state) {
    state->theta = 0;
    state->O_n = FP_ONE;
    state->L_symp = 0;
    state->L_metr = 0;
    state->entropy = 0;
    state->viscosity = (fixed_t)(0.1 * FP_ONE);
    state->n = 0;
}

/* ============================================================
 * CÓMPUTO DE LAGRANGIANOS (MANDATO METRIPLÉCTICO - REGLA 3.1)
 * 
 * Este método DEBE retornar L_symp y L_metr separados.
 * ============================================================ */

void golden_operator_compute_lagrangian(
    const GoldenState *state,
    fixed_t *L_symp,
    fixed_t *L_metr
) {
    // L_symp = ½ θ̇² + V(θ) donde V(θ) = -cos(θ)
    // Aproximamos θ̇ ≈ O_n
    fixed_t theta_dot = state->O_n;
    fixed_t potential = -dit_cos_fixed(state->theta);
    *L_symp = ((theta_dot * theta_dot) >> (FP_SHIFT + 1)) + potential;

    // L_metr = ½ η (θ - θ_eq)² donde θ_eq = π
    fixed_t deviation = state->theta - PI_FP;
    fixed_t deviation_sq = (deviation * deviation) >> FP_SHIFT;
    *L_metr = (state->viscosity * deviation_sq) >> (FP_SHIFT + 1);
}

/* ============================================================
 * EVOLUCIÓN METRIPLÉTICA DE UN PASO
 * 
 * dθ/dt = {θ, H} + [θ, S]
 *          ↑          ↑
 *       Poisson    Lindblad
 * ============================================================ */

void golden_operator_step(GoldenState *state) {
    state->n++;

    // Calcular O_n usando el nuevo operador de punto fijo
    fixed_t delta = (fixed_t)(DIT_DELTA_DEFAULT * FP_ONE);
    state->O_n = get_golden_operator_fixed(state->n, delta);

    golden_operator_compute_lagrangian(state, &state->L_symp, &state->L_metr);

    // Parte Hamiltoniana: dθ_H/dt ∝ O_n
    fixed_t dtheta_hamiltonian = (state->O_n) >> 4; // Reducir la magnitud

    // Parte Disipativa: dθ_D/dt ∝ η · (θ_eq - θ)
    fixed_t theta_equilibrium = PI_FP;
    fixed_t relaxation_factor = FP_ONE / 50;
    fixed_t dtheta_dissipative = (state->viscosity * (theta_equilibrium - state->theta)) >> FP_SHIFT;
    dtheta_dissipative = (dtheta_dissipative * relaxation_factor) >> FP_SHIFT;
    
    state->theta += dtheta_hamiltonian + dtheta_dissipative;
    
    // Mantener θ en [0, 2π]
    fixed_t TWO_PI_FP = 2 * PI_FP;
    while (state->theta < 0) state->theta += TWO_PI_FP;
    while (state->theta > TWO_PI_FP) state->theta -= TWO_PI_FP;
    
    // La viscosidad y la entropía se simplifican en el modelo de punto fijo por ahora
    state->viscosity = (fixed_t)(0.1 * FP_ONE);
    state->entropy = (FP_ONE - dit_cos_fixed(state->theta)) >> 2;
}

/* ============================================================
 * CÁLCULO DE OBSERVABLES COMPLETOS
 * ============================================================ */

void golden_operator_compute_observables(
    const GoldenState *state,
    GoldenObservables *obs
) {
    // Fase acumulada: π * φ * n
    obs->phase_accumulator = (((int64_t)PI_FP * PHI_CONJUGATE_FP) >> FP_SHIFT) * state->n;

    // IPR: 1 - |cos(θ/2)|²
    fixed_t cos_half = dit_cos_fixed(state->theta >> 1);
    obs->ipr = FP_ONE - ((cos_half * cos_half) >> FP_SHIFT);

    // Reynolds informacional: Re ∝ |O_n| / η
    if (state->viscosity != 0) {
        obs->reynolds_info = (state->O_n << FP_SHIFT) / state->viscosity;
    } else {
        obs->reynolds_info = 0;
    }

    // Centroide z: cos(θ)
    obs->centroid_z = dit_cos_fixed(state->theta);
}
