/*
 * Lindblad Master Equation - Implementación
 * Smopsys Q-CORE
 * 
 * Simulación de sistemas cuánticos abiertos sin STL (bare-metal).
 */

#include "lindblad.h"
#include "golden_operator.h"  /* Para golden_sqrt, golden_fabs */

/* ============================================================
 * OPERACIONES CON MATRICES
 * ============================================================ */

void cmatrix_zero(CMatrix *m, uint32_t rows, uint32_t cols) {
    m->rows = rows;
    m->cols = cols;
    for (uint32_t i = 0; i < rows; i++) {
        for (uint32_t j = 0; j < cols; j++) {
            m->data[i][j] = complex_make(0.0, 0.0);
        }
    }
}

void cmatrix_identity(CMatrix *m, uint32_t dim) {
    cmatrix_zero(m, dim, dim);
    for (uint32_t i = 0; i < dim; i++) {
        m->data[i][i] = complex_make(1.0, 0.0);
    }
}

void cmatrix_copy(CMatrix *dst, const CMatrix *src) {
    dst->rows = src->rows;
    dst->cols = src->cols;
    for (uint32_t i = 0; i < src->rows; i++) {
        for (uint32_t j = 0; j < src->cols; j++) {
            dst->data[i][j] = src->data[i][j];
        }
    }
}

void cmatrix_dagger(CMatrix *dst, const CMatrix *src) {
    dst->rows = src->cols;
    dst->cols = src->rows;
    for (uint32_t i = 0; i < src->rows; i++) {
        for (uint32_t j = 0; j < src->cols; j++) {
            dst->data[j][i] = complex_conj(src->data[i][j]);
        }
    }
}

void cmatrix_mul(CMatrix *C, const CMatrix *A, const CMatrix *B) {
    CMatrix temp;
    cmatrix_zero(&temp, A->rows, B->cols);
    
    for (uint32_t i = 0; i < A->rows; i++) {
        for (uint32_t j = 0; j < B->cols; j++) {
            Complex sum = complex_make(0.0, 0.0);
            for (uint32_t k = 0; k < A->cols; k++) {
                sum = complex_add(sum, complex_mul(A->data[i][k], B->data[k][j]));
            }
            temp.data[i][j] = sum;
        }
    }
    
    cmatrix_copy(C, &temp);
}

void cmatrix_commutator(CMatrix *C, const CMatrix *A, const CMatrix *B) {
    CMatrix AB, BA;
    cmatrix_mul(&AB, A, B);
    cmatrix_mul(&BA, B, A);
    
    C->rows = A->rows;
    C->cols = A->cols;
    for (uint32_t i = 0; i < A->rows; i++) {
        for (uint32_t j = 0; j < A->cols; j++) {
            C->data[i][j] = complex_sub(AB.data[i][j], BA.data[i][j]);
        }
    }
}

void cmatrix_anticommutator(CMatrix *C, const CMatrix *A, const CMatrix *B) {
    CMatrix AB, BA;
    cmatrix_mul(&AB, A, B);
    cmatrix_mul(&BA, B, A);
    
    C->rows = A->rows;
    C->cols = A->cols;
    for (uint32_t i = 0; i < A->rows; i++) {
        for (uint32_t j = 0; j < A->cols; j++) {
            C->data[i][j] = complex_add(AB.data[i][j], BA.data[i][j]);
        }
    }
}

void cmatrix_add(CMatrix *C, const CMatrix *A, const CMatrix *B) {
    C->rows = A->rows;
    C->cols = A->cols;
    for (uint32_t i = 0; i < A->rows; i++) {
        for (uint32_t j = 0; j < A->cols; j++) {
            C->data[i][j] = complex_add(A->data[i][j], B->data[i][j]);
        }
    }
}

void cmatrix_add_scaled(CMatrix *C, const CMatrix *A, const CMatrix *B, Complex scale) {
    C->rows = A->rows;
    C->cols = A->cols;
    for (uint32_t i = 0; i < A->rows; i++) {
        for (uint32_t j = 0; j < A->cols; j++) {
            C->data[i][j] = complex_add(A->data[i][j], complex_mul(scale, B->data[i][j]));
        }
    }
}

void cmatrix_scale(CMatrix *A, Complex s) {
    for (uint32_t i = 0; i < A->rows; i++) {
        for (uint32_t j = 0; j < A->cols; j++) {
            A->data[i][j] = complex_mul(s, A->data[i][j]);
        }
    }
}

Complex cmatrix_trace(const CMatrix *A) {
    Complex tr = complex_make(0.0, 0.0);
    uint32_t n = (A->rows < A->cols) ? A->rows : A->cols;
    for (uint32_t i = 0; i < n; i++) {
        tr = complex_add(tr, A->data[i][i]);
    }
    return tr;
}

/* ============================================================
 * SISTEMA DE LINDBLAD
 * ============================================================ */

void lindblad_init(LindbladSystem *sys, uint32_t dim) {
    sys->dim = dim;
    sys->num_ops = 0;
    cmatrix_zero(&sys->H, dim, dim);
}

void lindblad_set_hamiltonian(LindbladSystem *sys, const CMatrix *H) {
    cmatrix_copy(&sys->H, H);
}

void lindblad_add_jump_operator(LindbladSystem *sys, const CMatrix *L, double gamma) {
    if (sys->num_ops >= LINDBLAD_MAX_OPS) return;
    
    uint32_t idx = sys->num_ops;
    
    /* L_k = sqrt(gamma) * L */
    double sqrt_gamma = golden_sqrt(gamma);
    cmatrix_copy(&sys->L_ops[idx], L);
    cmatrix_scale(&sys->L_ops[idx], complex_make(sqrt_gamma, 0.0));
    
    /* L_k† */
    cmatrix_dagger(&sys->L_dag[idx], &sys->L_ops[idx]);
    
    /* L_k† L_k */
    cmatrix_mul(&sys->L_dag_L[idx], &sys->L_dag[idx], &sys->L_ops[idx]);
    
    sys->num_ops++;
}

/* ============================================================
 * CÁLCULO DE dρ/dt (LINDBLAD RHS)
 * 
 * dρ/dt = -i[H, ρ] + Σ_k (L_k ρ L_k† - ½{L_k† L_k, ρ})
 * ============================================================ */

void lindblad_compute_terms(
    const LindbladSystem *sys,
    const CMatrix *rho,
    CMatrix *unitary_term,
    CMatrix *dissipative_term
) {
    uint32_t dim = sys->dim;
    
    /* Término unitario: -i[H, ρ] */
    cmatrix_commutator(unitary_term, &sys->H, rho);
    cmatrix_scale(unitary_term, complex_make(0.0, -1.0));  /* -i */
    
    /* Término disipativo */
    cmatrix_zero(dissipative_term, dim, dim);
    
    for (uint32_t k = 0; k < sys->num_ops; k++) {
        CMatrix LrhoLd, anticomm, term;
        
        /* L_k ρ L_k† */
        CMatrix Lrho;
        cmatrix_mul(&Lrho, &sys->L_ops[k], rho);
        cmatrix_mul(&LrhoLd, &Lrho, &sys->L_dag[k]);
        
        /* {L_k† L_k, ρ} */
        cmatrix_anticommutator(&anticomm, &sys->L_dag_L[k], rho);
        
        /* L_k ρ L_k† - 0.5 * {L_k† L_k, ρ} */
        cmatrix_add_scaled(&term, &LrhoLd, &anticomm, complex_make(-0.5, 0.0));
        
        /* Acumular */
        cmatrix_add(dissipative_term, dissipative_term, &term);
    }
}

void lindblad_rhs(const LindbladSystem *sys, const CMatrix *rho, CMatrix *drho_dt) {
    CMatrix unitary, dissipative;
    
    lindblad_compute_terms(sys, rho, &unitary, &dissipative);
    
    /* dρ/dt = unitary + dissipative */
    cmatrix_add(drho_dt, &unitary, &dissipative);
}

/* ============================================================
 * INTEGRACIÓN RK4
 * ============================================================ */

void lindblad_step_rk4(LindbladSystem *sys, CMatrix *rho, double dt) {
    uint32_t dim = sys->dim;
    CMatrix k1, k2, k3, k4, temp, result;
    Complex half_dt = complex_make(dt * 0.5, 0.0);
    Complex sixth_dt = complex_make(dt / 6.0, 0.0);
    Complex dt_c = complex_make(dt, 0.0);
    
    /* k1 = f(rho) */
    lindblad_rhs(sys, rho, &k1);
    
    /* k2 = f(rho + dt/2 * k1) */
    cmatrix_add_scaled(&temp, rho, &k1, half_dt);
    lindblad_rhs(sys, &temp, &k2);
    
    /* k3 = f(rho + dt/2 * k2) */
    cmatrix_add_scaled(&temp, rho, &k2, half_dt);
    lindblad_rhs(sys, &temp, &k3);
    
    /* k4 = f(rho + dt * k3) */
    cmatrix_add_scaled(&temp, rho, &k3, dt_c);
    lindblad_rhs(sys, &temp, &k4);
    
    /* rho_new = rho + dt/6 * (k1 + 2*k2 + 2*k3 + k4) */
    cmatrix_copy(&result, rho);
    
    for (uint32_t i = 0; i < dim; i++) {
        for (uint32_t j = 0; j < dim; j++) {
            Complex weighted_sum = complex_add(
                complex_add(k1.data[i][j], complex_scale(k2.data[i][j], 2.0)),
                complex_add(complex_scale(k3.data[i][j], 2.0), k4.data[i][j])
            );
            result.data[i][j] = complex_add(
                result.data[i][j],
                complex_mul(sixth_dt, weighted_sum)
            );
        }
    }
    
    cmatrix_copy(rho, &result);
}

void lindblad_evolve(LindbladSystem *sys, CMatrix *rho, double t_total, double dt) {
    double t = 0.0;
    while (t < t_total) {
        lindblad_step_rk4(sys, rho, dt);
        t += dt;
    }
}

/* ============================================================
 * OBSERVABLES
 * ============================================================ */

Complex lindblad_expect(const CMatrix *rho, const CMatrix *O) {
    /* <O> = Tr(ρ O) */
    CMatrix rhoO;
    cmatrix_mul(&rhoO, rho, O);
    return cmatrix_trace(&rhoO);
}

void lindblad_compute_state(LindbladState *state, const CMatrix *rho) {
    cmatrix_copy(&state->rho, rho);
    
    /* Traza */
    Complex tr = cmatrix_trace(rho);
    state->trace = tr.re;
    
    /* Pureza = Tr(ρ²) */
    CMatrix rho2;
    cmatrix_mul(&rho2, rho, rho);
    Complex purity = cmatrix_trace(&rho2);
    state->purity = purity.re;
    
    /* Entropía aproximada: S ≈ 1 - Tr(ρ²) para estados mixtos */
    state->entropy = 1.0 - state->purity;
}
