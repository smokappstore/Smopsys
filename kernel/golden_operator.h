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

/* ============================================================
 * CONSTANTES FUNDAMENTALES
 * ============================================================ */

/* Razón Áurea y derivadas */
#define PHI           1.6180339887498948  /* φ = (1 + √5) / 2 */
#define PHI_CONJUGATE 0.6180339887498948  /* φ' = φ - 1 = 1/φ */
#define PHI_SQUARED   2.6180339887498948  /* φ² = φ + 1 */

/* Constantes matemáticas */
#define PI            3.14159265358979323846
#define TWO_PI        6.28318530717958647692
#define HALF_PI       1.57079632679489661923

/* Parámetros del scheduling cuasiperiódico */
#define GOLDEN_SCHEDULING_PHI  0.18       /* Valor usado en el scheduler */
#define REYNOLDS_THRESHOLD     2300.0     /* Umbral laminar/turbulento */
#define CHAOS_THRESHOLD        0.5        /* Umbral OTOC para caos */

/* ============================================================
 * ESTRUCTURAS DE ESTADO
 * ============================================================ */

/* Estado del sistema metripléctico */
typedef struct {
    double theta;           /* Ángulo de Bloch [0, 2π] */
    double O_n;             /* Valor actual del operador */
    double L_symp;          /* Componente Hamiltoniana (conservativa) */
    double L_metr;          /* Componente disipativa (Lindblad) */
    double entropy;         /* Entropía actual S */
    double viscosity;       /* Viscosidad η del baño */
    uint32_t n;             /* Paso temporal discreto */
} GoldenState;

/* Observables del sistema */
typedef struct {
    double phase_accumulator;  /* Fase acumulada Σ πφn */
    double ipr;                /* Inverse Participation Ratio */
    double reynolds_info;      /* Número de Reynolds informacional */
    double centroid_z;         /* Centroide z-finch */
} GoldenObservables;

/* ============================================================
 * API PÚBLICA - OPERADOR ÁUREO
 * ============================================================ */

/* Inicialización del estado */
void golden_operator_init(GoldenState *state);

/* Computar Ô_n para un paso n dado */
double golden_operator_compute(uint32_t n);

/* Computar con parámetro φ personalizado */
double golden_operator_compute_phi(uint32_t n, double phi);

/* Evolución metriplética de un paso */
void golden_operator_step(GoldenState *state);

/* Computar Lagrangianos separados (MANDATO METRIPLÉCTICO) */
void golden_operator_compute_lagrangian(
    const GoldenState *state,
    double *L_symp,     /* OUT: Parte hamiltoniana */
    double *L_metr      /* OUT: Parte disipativa */
);

/* ============================================================
 * API PÚBLICA - OBSERVABLES
 * ============================================================ */

/* Calcular IPR (localización de la función de onda) */
double golden_operator_compute_ipr(const double *amplitudes, uint32_t size);

/* Calcular observables completos */
void golden_operator_compute_observables(
    const GoldenState *state,
    GoldenObservables *obs
);

/* ============================================================
 * FUNCIONES TRIGONOMÉTRICAS (Sin libmath)
 * 
 * Implementaciones Taylor para bare-metal.
 * ============================================================ */

/* Coseno usando serie de Taylor (precisión ~10^-6) */
double golden_cos(double x);

/* Seno usando serie de Taylor */
double golden_sin(double x);

/* Valor absoluto */
double golden_fabs(double x);

/* Raíz cuadrada (Newton-Raphson) */
double golden_sqrt(double x);

/* Exponencial (serie de Taylor) */
double golden_exp(double x);

#endif /* GOLDEN_OPERATOR_H */
