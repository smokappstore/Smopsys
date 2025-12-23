/*
 * Quantum Laser Simulation - Implementación
 * Smopsys Q-CORE
 * 
 * Sistema láser de 4 niveles con cavidad óptica.
 * 
 * Hamiltoniano del sistema:
 *   H = ℏω_c a†a + ℏω_a σ_22 + ℏg(a†σ_12 + aσ_21)
 * 
 * Operadores de salto (Lindblad):
 *   L_κ = √κ a                    (pérdida cavidad)
 *   L_p = √Γ_p σ_30               (bombeo 0 → 3)
 *   L_32 = √γ_32 σ_23             (decay 3 → 2)
 *   L_21 = √γ_21 σ_12             (decay 2 → 1)
 *   L_10 = √γ_10 σ_01             (decay 1 → 0)
 */

#include "quantum_laser.h"
#include "golden_operator.h"

/* ============================================================
 * PARÁMETROS POR DEFECTO
 * ============================================================ */

void laser_params_default(LaserParams *p) {
    p->dim_atom = 4;
    p->dim_cavity = 12;          /* N_max = 11 fotones */
    
    p->omega_atom = 1.0;
    p->omega_cavity = 1.0;       /* Resonancia */
    
    p->g = 0.1;                  /* Acoplamiento débil */
    
    p->kappa = 0.05;             /* Pérdida de cavidad */
    p->pump_rate = 0.2;          /* Tasa de bombeo */
    p->gamma_32 = 1.0;           /* Relajación rápida 3 → 2 */
    p->gamma_21 = 0.01;          /* Emisión espontánea lenta */
    p->gamma_10 = 1.0;           /* Relajación rápida 1 → 0 */
    
    p->t_start = 0.0;
    p->t_end = 50.0 / p->kappa;  /* 50 tiempos de vida de cavidad */
    p->dt = 0.01;
}

/* ============================================================
 * OPERADORES DEL SISTEMA
 * ============================================================ */

/* Producto de Kronecker para espacio átomo ⊗ cavidad */
static void kronecker(CMatrix *C, const CMatrix *A, const CMatrix *B) {
    uint32_t ra = A->rows, ca = A->cols;
    uint32_t rb = B->rows, cb = B->cols;
    
    C->rows = ra * rb;
    C->cols = ca * cb;
    
    for (uint32_t i = 0; i < ra; i++) {
        for (uint32_t j = 0; j < ca; j++) {
            for (uint32_t k = 0; k < rb; k++) {
                for (uint32_t l = 0; l < cb; l++) {
                    C->data[i * rb + k][j * cb + l] = 
                        complex_mul(A->data[i][j], B->data[k][l]);
                }
            }
        }
    }
}

/* Operador de destrucción de la cavidad: a|n⟩ = √n|n-1⟩ */
static void create_annihilation_cavity(CMatrix *a, uint32_t dim_cavity) {
    cmatrix_zero(a, dim_cavity, dim_cavity);
    for (uint32_t n = 1; n < dim_cavity; n++) {
        a->data[n-1][n] = complex_make(golden_sqrt((double)n), 0.0);
    }
}

/* Operador de creación: a†|n⟩ = √(n+1)|n+1⟩ */
static void create_creation_cavity(CMatrix *a_dag, uint32_t dim_cavity) {
    CMatrix a;
    create_annihilation_cavity(&a, dim_cavity);
    cmatrix_dagger(a_dag, &a);
}

/* Operador atómico σ_ij = |i⟩⟨j| */
static void create_sigma_atom(CMatrix *sigma, uint32_t i, uint32_t j, uint32_t dim_atom) {
    cmatrix_zero(sigma, dim_atom, dim_atom);
    if (i < dim_atom && j < dim_atom) {
        sigma->data[i][j] = complex_make(1.0, 0.0);
    }
}

/* Operadores en el espacio producto H_atom ⊗ H_cavity */

void laser_create_annihilation(CMatrix *a_total, uint32_t dim_atom, uint32_t dim_cavity) {
    CMatrix I_atom, a_cavity;
    cmatrix_identity(&I_atom, dim_atom);
    create_annihilation_cavity(&a_cavity, dim_cavity);
    kronecker(a_total, &I_atom, &a_cavity);
}

void laser_create_creation(CMatrix *a_dag_total, uint32_t dim_atom, uint32_t dim_cavity) {
    CMatrix a_total;
    laser_create_annihilation(&a_total, dim_atom, dim_cavity);
    cmatrix_dagger(a_dag_total, &a_total);
}

void laser_create_sigma(CMatrix *sigma_total, uint32_t i, uint32_t j,
                        uint32_t dim_atom, uint32_t dim_cavity) {
    CMatrix sigma_atom, I_cavity;
    create_sigma_atom(&sigma_atom, i, j, dim_atom);
    cmatrix_identity(&I_cavity, dim_cavity);
    kronecker(sigma_total, &sigma_atom, &I_cavity);
}

void laser_create_number(CMatrix *N, uint32_t dim_atom, uint32_t dim_cavity) {
    CMatrix a, a_dag;
    laser_create_annihilation(&a, dim_atom, dim_cavity);
    laser_create_creation(&a_dag, dim_atom, dim_cavity);
    cmatrix_mul(N, &a_dag, &a);
}

/* ============================================================
 * CONSTRUCCIÓN DEL SISTEMA
 * ============================================================ */

void laser_build_system(
    const LaserParams *p,
    LindbladSystem *sys,
    CMatrix *rho0
) {
    uint32_t dim_a = p->dim_atom;
    uint32_t dim_c = p->dim_cavity;
    uint32_t dim = dim_a * dim_c;
    
    lindblad_init(sys, dim);
    
    /* ========================================
     * HAMILTONIANO
     * H = ω_c a†a + ω_a |2⟩⟨2| + g(a†σ_12 + a σ_21)
     * ======================================== */
    
    CMatrix H, a, a_dag, N, sigma_22, sigma_12, sigma_21;
    CMatrix term1, term2, term3, temp;
    
    /* a†a (número de fotones) */
    laser_create_number(&N, dim_a, dim_c);
    cmatrix_copy(&term1, &N);
    cmatrix_scale(&term1, complex_make(p->omega_cavity, 0.0));
    
    /* |2⟩⟨2| (energía del nivel láser superior) */
    laser_create_sigma(&sigma_22, 2, 2, dim_a, dim_c);
    cmatrix_copy(&term2, &sigma_22);
    cmatrix_scale(&term2, complex_make(p->omega_atom, 0.0));
    
    /* Interacción Jaynes-Cummings: g(a†σ_12 + a σ_21) */
    laser_create_annihilation(&a, dim_a, dim_c);
    laser_create_creation(&a_dag, dim_a, dim_c);
    laser_create_sigma(&sigma_12, 1, 2, dim_a, dim_c);
    laser_create_sigma(&sigma_21, 2, 1, dim_a, dim_c);
    
    /* a† σ_12 */
    cmatrix_mul(&temp, &a_dag, &sigma_12);
    cmatrix_copy(&term3, &temp);
    
    /* a σ_21 */
    cmatrix_mul(&temp, &a, &sigma_21);
    cmatrix_add(&term3, &term3, &temp);
    cmatrix_scale(&term3, complex_make(p->g, 0.0));
    
    /* H total */
    cmatrix_zero(&H, dim, dim);
    cmatrix_add(&H, &term1, &term2);
    cmatrix_add(&H, &H, &term3);
    
    lindblad_set_hamiltonian(sys, &H);
    
    /* ========================================
     * OPERADORES DE SALTO (LINDBLAD)
     * ======================================== */
    
    /* L_κ = a (pérdida de cavidad) */
    lindblad_add_jump_operator(sys, &a, p->kappa);
    
    /* L_p = σ_30 (bombeo 0 → 3) */
    CMatrix sigma_30;
    laser_create_sigma(&sigma_30, 3, 0, dim_a, dim_c);
    lindblad_add_jump_operator(sys, &sigma_30, p->pump_rate);
    
    /* L_32 = σ_23 (decay 3 → 2) */
    CMatrix sigma_23;
    laser_create_sigma(&sigma_23, 2, 3, dim_a, dim_c);
    lindblad_add_jump_operator(sys, &sigma_23, p->gamma_32);
    
    /* L_21 = σ_12 (decay 2 → 1) */
    lindblad_add_jump_operator(sys, &sigma_12, p->gamma_21);
    
    /* L_10 = σ_01 (decay 1 → 0) */
    CMatrix sigma_01;
    laser_create_sigma(&sigma_01, 0, 1, dim_a, dim_c);
    lindblad_add_jump_operator(sys, &sigma_01, p->gamma_10);
    
    /* ========================================
     * ESTADO INICIAL: |0⟩_atom ⊗ |0⟩_cavity
     * ======================================== */
    
    cmatrix_zero(rho0, dim, dim);
    rho0->data[0][0] = complex_make(1.0, 0.0);  /* |0,0⟩⟨0,0| */
}

/* ============================================================
 * CÁLCULO DE OBSERVABLES
 * ============================================================ */

void laser_compute_observables(
    const LaserParams *p,
    const CMatrix *rho,
    LaserState *state
) {
    uint32_t dim_a = p->dim_atom;
    uint32_t dim_c = p->dim_cavity;
    
    /* Número medio de fotones <a†a> */
    CMatrix N;
    laser_create_number(&N, dim_a, dim_c);
    Complex n_exp = lindblad_expect(rho, &N);
    state->n_photons = n_exp.re;
    
    /* Poblaciones P_i = Tr(ρ |i⟩⟨i|) */
    for (uint32_t i = 0; i < 4; i++) {
        CMatrix sigma_ii;
        laser_create_sigma(&sigma_ii, i, i, dim_a, dim_c);
        Complex pop = lindblad_expect(rho, &sigma_ii);
        state->population[i] = pop.re;
    }
    
    /* Inversión de población */
    state->inversion = state->population[2] - state->population[1];
    
    /* Coherencia |⟨σ_21⟩| */
    CMatrix sigma_21;
    laser_create_sigma(&sigma_21, 2, 1, dim_a, dim_c);
    Complex coh = lindblad_expect(rho, &sigma_21);
    state->coherence = golden_sqrt(coh.re * coh.re + coh.im * coh.im);
    
    /* Estado del sistema */
    LindbladState lstate;
    lindblad_compute_state(&lstate, rho);
    state->purity = lstate.purity;
    state->entropy = lstate.entropy;
    
    /* Parámetro de umbral */
    double threshold = laser_threshold(p);
    state->threshold_param = (threshold > 0) ? p->pump_rate / threshold : 0.0;
}

/* ============================================================
 * UMBRAL DE LÁSER
 * ============================================================ */

double laser_threshold(const LaserParams *p) {
    /*
     * Umbral aproximado para láser de 4 niveles:
     * Γ_p^th ≈ κ · γ_21 / (4g²)
     * 
     * Esto viene de la condición de ganancia = pérdida
     */
    double g2 = p->g * p->g;
    if (g2 < 1e-10) return 1e10;  /* Sin acoplamiento → umbral infinito */
    
    return (p->kappa * p->gamma_21) / (4.0 * g2);
}

/* ============================================================
 * EVOLUCIÓN TEMPORAL
 * ============================================================ */

void laser_evolve(
    const LaserParams *p,
    LindbladSystem *sys,
    CMatrix *rho,
    LaserObservable *obs,
    uint32_t num_samples
) {
    double t = p->t_start;
    double t_total = p->t_end - p->t_start;
    double dt_sample = t_total / (num_samples - 1);
    
    uint32_t sample_idx = 0;
    double next_sample = t;
    
    while (t < p->t_end && sample_idx < num_samples) {
        /* Tomar muestra */
        if (t >= next_sample) {
            LaserState state;
            laser_compute_observables(p, rho, &state);
            
            obs[sample_idx].time = t;
            obs[sample_idx].n_photons = state.n_photons;
            obs[sample_idx].inversion = state.inversion;
            
            /* g²(0) ≈ 1 para luz coherente, 2 para luz térmica */
            /* Aproximación: g² = 1 + (1 - purity) */
            obs[sample_idx].g2 = 1.0 + (1.0 - state.purity);
            
            sample_idx++;
            next_sample += dt_sample;
        }
        
        /* Paso de integración */
        lindblad_step_rk4(sys, rho, p->dt);
        t += p->dt;
    }
}
