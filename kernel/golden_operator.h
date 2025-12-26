/*
 * Golden Operator - Smopsys Q-CORE
 * 
 * Operador Cuasiperiódico de Proyección Dimensional
 * 
 * Definición Matemática:
 *   Ô_n = cos(πn) · cos(πφn)
 * 
 * donde φ = (√5 - 1) / 2 ≈ 0.6180339887 (razón áurea conjugada)
 * 
 * Propiedades:
 * - Cuasiperiodicidad: nunca se repite exactamente
 * - Bounded: |Ô_n| ≤ 1 para todo n
 * - Proyección dimensional: mapea n → {-1, +1} en el límite
 * 
 * Mandato Metripléctico:
 * - L_symp: Rotación de fase πφn (conserva energía)
 * - L_metr: Amortiguamiento hacia el atractor (O_n → 0)
 */

#ifndef GOLDEN_OPERATOR_H
#define GOLDEN_OPERATOR_H

#include <stdint.h>
#include "../include/dit_physics.h"
#include "../include/dit_math_fixed.h"

/* ============================================================
 * CONSTANTES FUNDAMENTALES (ahora en dit_physics.h)
 * ============================================================ */

/* Parámetros del scheduling cuasiperiódico */
#define GOLDEN_SCHEDULING_PHI  0.18       /* Valor usado en el scheduler */
#define REYNOLDS_THRESHOLD     2300.0     /* Umbral laminar/turbulento */
#define CHAOS_THRESHOLD        0.5        /* Umbral OTOC para caos */

/* ============================================================
 * ESTRUCTURAS DE ESTADO
 * ============================================================ */

/* Estado del sistema metripléctico (punto fijo) */
typedef struct {
    fixed_t theta;          /* Ángulo de Bloch [0, 2π] */
    fixed_t O_n;            /* Valor actual del operador */
    fixed_t L_symp;         /* Componente Hamiltoniana (conservativa) */
    fixed_t L_metr;         /* Componente disipativa (Lindblad) */
    fixed_t entropy;        /* Entropía actual S */
    fixed_t viscosity;      /* Viscosidad η del baño */
    uint32_t n;             /* Paso temporal discreto */
} GoldenState;

/* Observables del sistema (punto fijo) */
typedef struct {
    fixed_t phase_accumulator; /* Fase acumulada Σ πφn */
    fixed_t ipr;               /* Inverse Participation Ratio */
    fixed_t reynolds_info;     /* Número de Reynolds informacional */
    fixed_t centroid_z;        /* Centroide z-finch */
} GoldenObservables;

/* ============================================================
 * API PÚBLICA - OPERADOR ÁUREO
 * ============================================================ */

/* Inicialización del estado */
void golden_operator_init(GoldenState *state);

/* Evolución metriplética de un paso */
void golden_operator_step(GoldenState *state);

/* Computar Lagrangianos separados (MANDATO METRIPLÉCTICO) */
void golden_operator_compute_lagrangian(
    const GoldenState *state,
    fixed_t *L_symp,     /* OUT: Parte hamiltoniana */
    fixed_t *L_metr      /* OUT: Parte disipativa */
);

/* ============================================================
 * API PÚBLICA - OBSERVABLES
 * ============================================================ */

/* Calcular observables completos */
void golden_operator_compute_observables(
    const GoldenState *state,
    GoldenObservables *obs
);

#endif /* GOLDEN_OPERATOR_H */
