/*
 * Quantum Laser Simulation - Smopsys Q-CORE
 * 
 * Simulación de un láser de 4 niveles acoplado a una cavidad óptica.
 * Implementa la dinámica de Lindblad para el sistema átomo-cavidad.
 * 
 * Niveles atómicos:
 *   |3⟩ → Nivel de bombeo
 *   |2⟩ → Nivel láser superior
 *   |1⟩ → Nivel láser inferior
 *   |0⟩ → Estado fundamental
 * 
 * Transiciones:
 *   Γ_p:  |0⟩ → |3⟩  (bombeo incoherente)
 *   γ_32: |3⟩ → |2⟩  (relajación rápida)
 *   γ_21: |2⟩ → |1⟩  (emisión estimulada/espontánea)
 *   γ_10: |1⟩ → |0⟩  (relajación rápida)
 *   κ:    Pérdidas de la cavidad
 *   g:    Acoplamiento Jaynes-Cummings
 */

#ifndef QUANTUM_LASER_H
#define QUANTUM_LASER_H

#include "lindblad.h"

/* ============================================================
 * PARÁMETROS DEL LÁSER
 * ============================================================ */

typedef struct {
    /* Dimensiones */
    uint32_t dim_atom;      /* Niveles atómicos (4) */
    uint32_t dim_cavity;    /* Número de Fock máximo + 1 */
    
    /* Frecuencias */
    double omega_atom;      /* Frecuencia de transición 2 → 1 */
    double omega_cavity;    /* Frecuencia de la cavidad */
    
    /* Acoplamiento */
    double g;               /* Constante de acoplamiento Jaynes-Cummings */
    
    /* Tasas de disipación */
    double kappa;           /* Pérdida de la cavidad */
    double pump_rate;       /* Tasa de bombeo Γ_p (0 → 3) */
    double gamma_32;        /* Decaimiento 3 → 2 */
    double gamma_21;        /* Decaimiento 2 → 1 (emisión espontánea) */
    double gamma_10;        /* Decaimiento 1 → 0 */
    
    /* Integración temporal */
    double t_start;
    double t_end;
    double dt;
} LaserParams;

/* Estado del láser */
typedef struct {
    double n_photons;       /* Número medio de fotones <a†a> */
    double population[4];   /* Poblaciones P_0, P_1, P_2, P_3 */
    double inversion;       /* Inversión de población (P_2 - P_1) */
    double coherence;       /* |⟨σ_21⟩| (coherencia láser) */
    double purity;          /* Tr(ρ²) */
    double entropy;         /* Entropía del sistema */
    double threshold_param; /* Parámetro de umbral (pump_rate / pump_threshold) */
} LaserState;

/* Observables para evolución temporal */
typedef struct {
    double time;
    double n_photons;
    double inversion;
    double g2;              /* Función de correlación g²(0) */
} LaserObservable;

/* ============================================================
 * API PÚBLICA
 * ============================================================ */

/* Inicializar parámetros por defecto */
void laser_params_default(LaserParams *p);

/* Construir el sistema de Lindblad para el láser */
void laser_build_system(
    const LaserParams *p,
    LindbladSystem *sys,
    CMatrix *rho0           /* Estado inicial (salida) */
);

/* Calcular observables del estado actual */
void laser_compute_observables(
    const LaserParams *p,
    const CMatrix *rho,
    LaserState *state
);

/* Evolucionar y obtener observables */
void laser_evolve(
    const LaserParams *p,
    LindbladSystem *sys,
    CMatrix *rho,
    LaserObservable *obs,   /* Array de observables (salida) */
    uint32_t num_samples    /* Número de muestras temporales */
);

/* Calcular umbral de láser teórico */
double laser_threshold(const LaserParams *p);

/* ============================================================
 * OPERADORES AUXILIARES
 * ============================================================ */

/* Crear operador de destrucción a (cavidad) en espacio producto */
void laser_create_annihilation(CMatrix *a, uint32_t dim_atom, uint32_t dim_cavity);

/* Crear operador de creación a† */
void laser_create_creation(CMatrix *a_dag, uint32_t dim_atom, uint32_t dim_cavity);

/* Crear operador σ_ij = |i⟩⟨j| (átomo) en espacio producto */
void laser_create_sigma(CMatrix *sigma, uint32_t i, uint32_t j, 
                        uint32_t dim_atom, uint32_t dim_cavity);

/* Crear operador número de fotones N = a†a */
void laser_create_number(CMatrix *N, uint32_t dim_atom, uint32_t dim_cavity);

#endif /* QUANTUM_LASER_H */
