/*
 * Surgical Scheduler - Implementación
 * Smopsys Q-CORE
 */

#include "surgical_scheduler.h"
#include "../drivers/bayesian_serial.h"

void surgical_scheduler_init(SurgicalState *s_state) {
    s_state->current_chirality = CHIRALITY_L;
    s_state->switch_count = 0;
    s_state->total_evaporated_entropy = 0;
    s_state->dit_state = 0;
    
    bayesian_serial_write("[SURGICAL] Scheduler initialized.\n");
}

void surgical_scheduler_step(SurgicalState *s_state, GoldenState *g_state, LaserParams *l_params) {
    /* O_n de punto fijo a float para comparación */
    double on_val = (double)g_state->O_n / FP_ONE;
    
    /* CHECK_CHIRALITY logic */
    SurgicalChirality next_chirality = (on_val >= 0) ? CHIRALITY_L : CHIRALITY_D;
    
    if (next_chirality != s_state->current_chirality) {
        s_state->current_chirality = next_chirality;
        s_state->switch_count++;
        
        bayesian_serial_write("[SURGICAL] Chirality switch: ");
        bayesian_serial_write(next_chirality == CHIRALITY_L ? "L (BOSONIC)" : "D (FERMIONIC)");
        bayesian_serial_write("\n");
    }
    
    if (s_state->current_chirality == CHIRALITY_L) {
        /* FORWARD_L: Pulso de Bombeo Laminar */
        l_params->pump_rate = 0.2; /* Tasa nominal */
        // bayesian_serial_write("[SURGICAL] FORWARD_L: Pumping...\n");
    } else {
        /* FEEDBACK_D: Evaporación de Hawking */
        l_params->pump_rate = 0.0; /* Detener bombeo */
        evaporate_entropy(s_state, g_state);

        /* ENGINE 2: EL SUBCONSCIENTE (Procesamiento en sector disipativo) */
        if (s_state->dit_state != 0) {
            dit_subconscious_step(s_state->dit_state);
        }
    }
}

void evaporate_entropy(SurgicalState *s_state, GoldenState *g_state) {
    /* 
     * Regla 1.2 (Componente Métrica): Relaxación hacia el atractor.
     * En el sector D, forzamos la disipación.
     */
    double current_entropy = (double)g_state->entropy / FP_ONE;
    
    if (current_entropy > 0.01) {
        /* Aumentar viscosidad temporalmente para "evaporar" */
        g_state->viscosity += (FP_ONE / 100); 
        s_state->total_evaporated_entropy += 0.01;
        
        // bayesian_serial_write("[SURGICAL] FEEDBACK_D: Evaporating entropy...\n");
    }
}
