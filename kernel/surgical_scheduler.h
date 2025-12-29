/*
 * Surgical Scheduler - Smopsys Q-CORE
 * 
 * Gestiona el switching entre sectores bosónicos (L) y fermiónicos (D)
 * basado en la quiralidad del Operador Áureo O_n.
 */

#ifndef SURGICAL_SCHEDULER_H
#define SURGICAL_SCHEDULER_H

#include "golden_operator.h"
#include "quantum_laser.h"

typedef enum {
    CHIRALITY_L = 1,    /* Sector Bosónico (Raya -) */
    CHIRALITY_D = 0     /* Sector Fermiónico (Punto .) */
} SurgicalChirality;

typedef struct {
    SurgicalChirality current_chirality;
    uint32_t switch_count;
    double total_evaporated_entropy;
} SurgicalState;

/* Inicialización del scheduler quirúrgico */
void surgical_scheduler_init(SurgicalState *s_state);

/* Paso del scheduler: Evalúa O_n y actúa sobre el láser */
void surgical_scheduler_step(SurgicalState *s_state, GoldenState *g_state, LaserParams *l_params);

/* Feedback disipativo: Evaporación de Hawking / Entropía */
void evaporate_entropy(SurgicalState *s_state, GoldenState *g_state);

#endif /* SURGICAL_SCHEDULER_H */
