/*
 * Unified Metriplectic Driver API - Smopsys Q-CORE
 * 
 * Este módulo permite la integración de componentes clásicos y cuánticos
 * bajo el marco de "El Mandato Metriplético".
 */

#ifndef METRIPLECTIC_API_H
#define METRIPLECTIC_API_H

#include <stdint.h>
#include "golden_operator.h"

/* 
 * Clasificación de Reversibilidad (Rule 1.1 & 1.2)
 */
typedef enum {
    REV_SYMPLECTIC,  /* Reversible (Hamiltoniano dominante) */
    REV_METRIC,      /* Disipativo (Lindblad/Entropía dominante) */
    REV_HYBRID       /* Mixto (Competencia de Lagrangianos) */
} ReversibilityType;

/* 
 * Definición de un componente metripléctico
 */
typedef struct {
    const char *name;
    ReversibilityType type;
    
    /* Función para computar Lagrangianos separados (Regla 3.1) */
    void (*compute_lagrangian)(void *context, double *L_symp, double *L_metr);
    
    /* Función de evolución (opcional, llamada por el core) */
    void (*step)(void *context, double dt);
    
    void *context;
} MetriplecticComponent;

/* 
 * API de Registro y Control
 */

/* Inicializar el subsistema API */
void metriplectic_api_init(void);

/* Registrar un nuevo driver o circuito */
int metriplectic_register(const MetriplecticComponent *comp);

/* Obtener la competencia de Lagrangianos global (Regla 3.3) */
void metriplectic_get_global_competition(double *total_symp, double *total_metr);

/* Avanzar todos los componentes registrados */
void metriplectic_global_step(double dt);

#endif /* METRIPLECTIC_API_H */
