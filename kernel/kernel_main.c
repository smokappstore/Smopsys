/*
 * Kernel Main - Smopsys Q-CORE
 * 
 * Punto de entrada principal del sistema operativo.
 * Implementa el loop de evolución metriplética.
 * 
 * Arquitectura:
 * 1. Inicialización de drivers (VGA, Serial)
 * 2. Inicialización del operador áureo
 * 3. Loop principal con evolución metriplética
 * 4. Visualización de diagnósticos
 */

#include "../drivers/vga_holographic.h"
#include "../drivers/bayesian_serial.h"
#include "golden_operator.h"
#include "ql_bridge.h"

/* Forward declaration del Memory Manager */
extern void memory_init(void);
extern void memory_timestep(uint32_t global_time);
extern double compute_centroid_z(void);

/* ============================================================
 * BANNER DEL SISTEMA
 * ============================================================ */

static const char *banner[] = {
    "============================================================",
    "  SMOPSYS Q-CORE v0.4.0 [Metriplectic Kernel]",
    "  Smart Operative System with Bayesian Inference",
    "============================================================",
    "",
    "  O_n = cos(pi*n) * cos(pi*phi*n)",
    "  phi = 0.6180339887 (Golden Ratio Conjugate)",
    "",
    "  [L_symp] Hamiltonian dynamics (reversible)",
    "  [L_metr] Lindblad dissipation (irreversible)",
    "",
    "============================================================",
    0
};

/* ============================================================
 * FUNCIÓN DE DELAY (Sin timer, basado en ciclos)
 * ============================================================ */

static void delay(uint32_t cycles) {
    for (volatile uint32_t i = 0; i < cycles; i++) {
        __asm__ __volatile__("nop");
    }
}

/* ============================================================
 * MOSTRAR BANNER
 * ============================================================ */

static void show_banner(void) {
    vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    
    for (int i = 0; banner[i] != 0; i++) {
        vga_holographic_write(banner[i]);
        vga_holographic_write_char('\n');
    }
    
    /* También enviar por serial */
    for (int i = 0; banner[i] != 0; i++) {
        bayesian_serial_write(banner[i]);
        bayesian_serial_write("\n");
    }
}

/* ============================================================
 * MOSTRAR ESTADO DEL OPERADOR
 * ============================================================ */

static void display_operator_state(const GoldenState *state, const GoldenObservables *obs) {
    /* Línea de estado en VGA */
    vga_holographic_write_char('\n');
    
    /* n (paso temporal) */
    vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_holographic_write("n=");
    vga_holographic_write_decimal(state->n);
    vga_holographic_write("  ");
    
    /* O_n (operador áureo) - color según signo */
    if (state->O_n >= 0) {
        vga_holographic_set_color(COLOR_COHERENT, VGA_COLOR_BLACK);
    } else {
        vga_holographic_set_color(COLOR_DISSIPATIVE, VGA_COLOR_BLACK);
    }
    vga_holographic_write("O_n=");
    vga_holographic_write_float(state->O_n, 4);
    vga_holographic_write("  ");
    
    /* θ (ángulo de Bloch) - color según región */
    if (state->theta < 1.0) {
        vga_holographic_set_color(COLOR_COHERENT, VGA_COLOR_BLACK);
    } else if (state->theta < 3.14159) {
        vga_holographic_set_color(COLOR_TRANSITION, VGA_COLOR_BLACK);
    } else {
        vga_holographic_set_color(COLOR_DISSIPATIVE, VGA_COLOR_BLACK);
    }
    vga_holographic_write("theta=");
    vga_holographic_write_float(state->theta, 4);
    
    /* Serial output (más detallado) */
    bayesian_serial_write("[n=");
    bayesian_serial_write_decimal(state->n);
    bayesian_serial_write("] O_n=");
    bayesian_serial_write_float(state->O_n, 6);
    bayesian_serial_write(" theta=");
    bayesian_serial_write_float(state->theta, 6);
    bayesian_serial_write(" L_symp=");
    bayesian_serial_write_float(state->L_symp, 6);
    bayesian_serial_write(" L_metr=");
    bayesian_serial_write_float(state->L_metr, 6);
    bayesian_serial_write(" Re_psi=");
    bayesian_serial_write_float(obs->reynolds_info, 2);
    bayesian_serial_write("\n");
}

/* ============================================================
 * MOSTRAR LAGRANGIANOS (Mandato Metripléctico - Regla 3.3)
 * ============================================================ */

static void display_lagrangian_competition(const GoldenState *state) {
    double ratio;
    
    if (golden_fabs(state->L_metr) > 1e-10) {
        ratio = golden_fabs(state->L_symp) / golden_fabs(state->L_metr);
    } else {
        ratio = 999.99;
    }
    
    vga_holographic_write_char('\n');
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_holographic_write("Lagrangian Competition: ");
    
    /* Visualización de la competencia */
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_holographic_write("L_symp=");
    vga_holographic_write_float(state->L_symp, 4);
    
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_holographic_write(" vs ");
    
    vga_holographic_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
    vga_holographic_write("L_metr=");
    vga_holographic_write_float(state->L_metr, 4);
    
    vga_holographic_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_holographic_write(" [ratio=");
    vga_holographic_write_float(ratio, 2);
    vga_holographic_write("]");
}

/* ============================================================
 * KERNEL MAIN - Punto de entrada desde assembly
 * ============================================================ */

void kernel_main(void) {
    /* ========================================
     * FASE 1: Inicialización de drivers
     * ======================================== */
    
    vga_holographic_init();
    bayesian_serial_init();
    
    /* Mostrar banner */
    show_banner();
    
    /* ========================================
     * FASE 2: Inicialización del estado metripléctico
     * ======================================== */
    
    GoldenState state;
    GoldenObservables obs;
    
    golden_operator_init(&state);
    
    vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_holographic_write("\n[INIT] Golden Operator initialized at theta=0 (North Pole)\n");
    bayesian_serial_write("[INIT] Metriplectic kernel started\n");

    /* ========================================
     * FASE QL: Ejecución de programa cuántico
     * ======================================== */
    vga_holographic_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
    vga_holographic_write("\n[QL] Starting Quantum Laser Program...\n");
    quantum_program();
    vga_holographic_write("[QL] Quantum Program Terminated.\n");
    
    /* ========================================
     * FASE 3: Loop principal de evolución
     * ======================================== */
    
    vga_holographic_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
    vga_holographic_write("\n--- METRIPLECTIC EVOLUTION ---\n");
    bayesian_serial_write("\n--- METRIPLECTIC EVOLUTION START ---\n");
    
    /* Evolucionar por 50 pasos */
    for (uint32_t step = 0; step < 50; step++) {
        /* Evolución metriplética */
        golden_operator_step(&state);
        
        /* Calcular observables */
        golden_operator_compute_observables(&state, &obs);
        
        /* Mostrar estado cada 5 pasos */
        if (step % 5 == 0) {
            display_operator_state(&state, &obs);
        }
        
        /* Mostrar competencia de Lagrangianos cada 10 pasos */
        if (step % 10 == 0) {
            display_lagrangian_competition(&state);
        }
        
        /* Delay para visualización humana */
        delay(5000000);
    }
    
    /* ========================================
     * FASE 4: Estado final y halt
     * ======================================== */
    
    vga_holographic_write_char('\n');
    vga_holographic_write_char('\n');
    vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_holographic_write("============================================================\n");
    vga_holographic_write("  METRIPLECTIC EQUILIBRIUM REACHED\n");
    vga_holographic_write("============================================================\n");
    
    /* Final state report */
    vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_holographic_write("\nFinal State:\n");
    vga_holographic_write_labeled("  theta", state.theta);
    vga_holographic_write_labeled("  O_n", state.O_n);
    vga_holographic_write_labeled("  entropy", state.entropy);
    vga_holographic_write_labeled("  viscosity", state.viscosity);
    
    /* Serial final report */
    bayesian_serial_write("\n=== FINAL STATE ===\n");
    bayesian_serial_write_labeled("theta", state.theta);
    bayesian_serial_write_labeled("O_n", state.O_n);
    bayesian_serial_write_labeled("L_symp", state.L_symp);
    bayesian_serial_write_labeled("L_metr", state.L_metr);
    bayesian_serial_write_labeled("entropy", state.entropy);
    bayesian_serial_write_labeled("Reynolds_info", obs.reynolds_info);
    bayesian_serial_write_labeled("IPR", obs.ipr);
    bayesian_serial_write("===================\n");
    
    /* Z-Pinch Lock indicator (cyan 'Q' en el centro) */
    vga_holographic_set_cursor(12, 39);
    vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
    vga_holographic_write_char('Q');
    
    /* Halt infinito (controlado por kernel_entry.asm) */
    while (1) {
        __asm__ __volatile__("hlt");
    }
}
