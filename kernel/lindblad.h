/*
 * Lindblad Master Equation - Smopsys Q-CORE
 * 
 * Implementación de la ecuación maestra de Lindblad para
 * sistemas cuánticos abiertos en bare-metal.
 * 
 * Ecuación: dρ/dt = -i[H, ρ] + Σ_k (L_k ρ L_k† - ½{L_k† L_k, ρ})
 *                    ↑            ↑
 *               Unitario      Disipativo
 * 
 * Mandato Metripléctico:
 * - L_symp: Evolución unitaria -i[H, ρ]
 * - L_metr: Términos de Lindblad (disipación)
 */

#ifndef LINDBLAD_H
#define LINDBLAD_H

#include <stdint.h>

/* ============================================================
 * CONFIGURACIÓN DEL SISTEMA
 * ============================================================ */

/* Dimensiones máximas (para bare-metal sin malloc) */
#define LINDBLAD_MAX_DIM      64    /* Dimensión máxima del espacio de Hilbert */
#define LINDBLAD_MAX_OPS      8     /* Número máximo de operadores de salto */

/* ============================================================
 * ESTRUCTURAS DE DATOS
 * ============================================================ */

/* Número complejo (sin STL) */
typedef struct {
    double re;  /* Parte real */
    double im;  /* Parte imaginaria */
} Complex;

/* Matriz compleja (tamaño fijo para bare-metal) */
typedef struct {
    Complex data[LINDBLAD_MAX_DIM][LINDBLAD_MAX_DIM];
    uint32_t rows;
    uint32_t cols;
} CMatrix;

/* Vector complejo */
typedef struct {
    Complex data[LINDBLAD_MAX_DIM * LINDBLAD_MAX_DIM];
    uint32_t size;
} CVector;

/* Sistema de Lindblad completo */
typedef struct {
    CMatrix H;                          /* Hamiltoniano */
    CMatrix L_ops[LINDBLAD_MAX_OPS];    /* Operadores de salto L_k */
    CMatrix L_dag[LINDBLAD_MAX_OPS];    /* L_k† (adjuntos) */
    CMatrix L_dag_L[LINDBLAD_MAX_OPS];  /* L_k† L_k (precalculado) */
    uint32_t num_ops;                   /* Número de operadores activos */
    uint32_t dim;                       /* Dimensión del sistema */
} LindbladSystem;

/* Estado del sistema (matriz densidad) */
typedef struct {
    CMatrix rho;        /* Matriz densidad */
    double trace;       /* Tr(ρ) - debería ser 1 */
    double purity;      /* Tr(ρ²) */
    double entropy;     /* -Tr(ρ log ρ) aproximada */
} LindbladState;

/* ============================================================
 * OPERACIONES CON COMPLEJOS
 * ============================================================ */

static inline Complex complex_make(double re, double im) {
    Complex c = {re, im};
    return c;
}

static inline Complex complex_add(Complex a, Complex b) {
    return complex_make(a.re + b.re, a.im + b.im);
}

static inline Complex complex_sub(Complex a, Complex b) {
    return complex_make(a.re - b.re, a.im - b.im);
}

static inline Complex complex_mul(Complex a, Complex b) {
    return complex_make(a.re * b.re - a.im * b.im,
                        a.re * b.im + a.im * b.re);
}

static inline Complex complex_conj(Complex a) {
    return complex_make(a.re, -a.im);
}

static inline Complex complex_scale(Complex a, double s) {
    return complex_make(a.re * s, a.im * s);
}

static inline double complex_abs2(Complex a) {
    return a.re * a.re + a.im * a.im;
}

/* i * z */
static inline Complex complex_mul_i(Complex a) {
    return complex_make(-a.im, a.re);
}

/* ============================================================
 * API PÚBLICA - MATRICES
 * ============================================================ */

/* Inicializar matriz a cero */
void cmatrix_zero(CMatrix *m, uint32_t rows, uint32_t cols);

/* Matriz identidad */
void cmatrix_identity(CMatrix *m, uint32_t dim);

/* Copiar matriz */
void cmatrix_copy(CMatrix *dst, const CMatrix *src);

/* Transpuesta conjugada (dagger) */
void cmatrix_dagger(CMatrix *dst, const CMatrix *src);

/* Multiplicación de matrices: C = A * B */
void cmatrix_mul(CMatrix *C, const CMatrix *A, const CMatrix *B);

/* Conmutador: [A, B] = AB - BA */
void cmatrix_commutator(CMatrix *C, const CMatrix *A, const CMatrix *B);

/* Anticonmutador: {A, B} = AB + BA */
void cmatrix_anticommutator(CMatrix *C, const CMatrix *A, const CMatrix *B);

/* Suma: C = A + B */
void cmatrix_add(CMatrix *C, const CMatrix *A, const CMatrix *B);

/* C = A + scale * B */
void cmatrix_add_scaled(CMatrix *C, const CMatrix *A, const CMatrix *B, Complex scale);

/* Escalar: A = s * A */
void cmatrix_scale(CMatrix *A, Complex s);

/* Traza */
Complex cmatrix_trace(const CMatrix *A);

/* ============================================================
 * API PÚBLICA - LINDBLAD
 * ============================================================ */

/* Inicializar sistema de Lindblad */
void lindblad_init(LindbladSystem *sys, uint32_t dim);

/* Establecer Hamiltoniano */
void lindblad_set_hamiltonian(LindbladSystem *sys, const CMatrix *H);

/* Agregar operador de salto con tasa gamma */
void lindblad_add_jump_operator(LindbladSystem *sys, const CMatrix *L, double gamma);

/* Calcular dρ/dt dado ρ actual */
void lindblad_rhs(const LindbladSystem *sys, const CMatrix *rho, CMatrix *drho_dt);

/* Calcular términos separados (Mandato Metripléctico) */
void lindblad_compute_terms(
    const LindbladSystem *sys,
    const CMatrix *rho,
    CMatrix *unitary_term,      /* -i[H, ρ] */
    CMatrix *dissipative_term   /* Σ L_k ρ L_k† - ½{L_k† L_k, ρ} */
);

/* ============================================================
 * API PÚBLICA - INTEGRACIÓN TEMPORAL
 * ============================================================ */

/* Paso RK4 */
void lindblad_step_rk4(LindbladSystem *sys, CMatrix *rho, double dt);

/* Evolucionar por tiempo total */
void lindblad_evolve(LindbladSystem *sys, CMatrix *rho, double t_total, double dt);

/* ============================================================
 * API PÚBLICA - OBSERVABLES
 * ============================================================ */

/* Valor esperado <O> = Tr(ρ O) */
Complex lindblad_expect(const CMatrix *rho, const CMatrix *O);

/* Calcular estado (traza, pureza, entropía) */
void lindblad_compute_state(LindbladState *state, const CMatrix *rho);

#endif /* LINDBLAD_H */
