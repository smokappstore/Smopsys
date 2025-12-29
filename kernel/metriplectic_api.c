/*
 * Unified Metriplectic Driver API - Implementation
 */

#include "metriplectic_api.h"
#include "../drivers/bayesian_serial.h"

#define MAX_METRIPLECTIC_COMPONENTS 16

static MetriplecticComponent registry[MAX_METRIPLECTIC_COMPONENTS];
static int component_count = 0;

void metriplectic_api_init(void) {
    component_count = 0;
    bayesian_serial_write("[API] Metriplectic Driver API initialized\n");
}

int metriplectic_register(const MetriplecticComponent *comp) {
    if (component_count >= MAX_METRIPLECTIC_COMPONENTS) {
        bayesian_serial_write("[API] Error: Registry full\n");
        return -1;
    }
    
    registry[component_count] = *comp;
    component_count++;
    
    bayesian_serial_write("[API] Registered component: ");
    bayesian_serial_write(comp->name);
    bayesian_serial_write("\n");
    
    return 0;
}

void metriplectic_get_global_competition(double *total_symp, double *total_metr) {
    *total_symp = 0.0;
    *total_metr = 0.0;
    
    for (int i = 0; i < component_count; i++) {
        double Ls = 0.0, Lm = 0.0;
        if (registry[i].compute_lagrangian) {
            registry[i].compute_lagrangian(registry[i].context, &Ls, &Lm);
            *total_symp += Ls;
            *total_metr += Lm;
        }
    }
}

void metriplectic_global_step(double dt) {
    for (int i = 0; i < component_count; i++) {
        if (registry[i].step) {
            registry[i].step(registry[i].context, dt);
        }
    }
}
