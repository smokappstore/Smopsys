/*
 * Test Surgical Scheduler - Smopsys Q-CORE
 */

#include <stdio.h>
#include <assert.h>
#include "kernel/surgical_scheduler.h"

/* Mocks */
void bayesian_serial_write(const char *str) {
    printf("%s", str);
}

void bayesian_serial_write_char(char c) {
    printf("%c", c);
}

void laser_params_default(LaserParams *p) {
    p->pump_rate = 0.0;
}

/* Mock de dit_math_fixed para el test host */
#include "include/dit_physics.h"
#define FP_SHIFT 16
#define FP_ONE (1 << FP_SHIFT)

int main() {
    printf("--- [TEST] Surgical Scheduler ---\n");
    
    SurgicalState s_state;
    GoldenState g_state;
    LaserParams l_params;
    
    surgical_scheduler_init(&s_state);
    
    /* Test Case 1: Positive O_n (L-Chirality) */
    g_state.O_n = (int32_t)(0.5 * FP_ONE);
    surgical_scheduler_step(&s_state, &g_state, &l_params);
    assert(s_state.current_chirality == CHIRALITY_L);
    assert(l_params.pump_rate > 0.1);
    
    /* Test Case 2: Negative O_n (D-Chirality) */
    g_state.O_n = (int32_t)(-0.2 * FP_ONE);
    g_state.entropy = (int32_t)(0.1 * FP_ONE);
    g_state.viscosity = (int32_t)(0.1 * FP_ONE);
    
    surgical_scheduler_step(&s_state, &g_state, &l_params);
    assert(s_state.current_chirality == CHIRALITY_D);
    assert(l_params.pump_rate == 0.0);
    
    /* Verify Entropy Evaporation */
    assert(s_state.total_evaporated_entropy > 0);
    
    printf("\n[SUCCESS] Surgical Scheduler logic verified.\n");
    return 0;
}
