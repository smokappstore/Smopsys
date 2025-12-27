#ifndef METRIPLECTIC_HEARTBEAT_H
#define METRIPLECTIC_HEARTBEAT_H

#include <stdint.h>

/* Puertos del PIT (Intel 8253/8254) */
#define PIT_CHANNEL0_DATA 0x40
#define PIT_COMMAND       0x43

/* Frecuencia base del PIT (1.193182 MHz) */
#define PIT_BASE_FREQUENCY 1193182

/* Frecuencia deseada para el latido metriplético (1000 Hz = 1ms) */
#define HEARTBEAT_HZ 1000

/* Estructura de estadísticas del latido */
typedef struct {
    uint32_t total_ticks;
    uint32_t global_seconds;
    double   drift_correction;  /* Corrección bayesiana de latencia */
} HeartbeatStats;

/* Funciones públicas */
void metriplectic_heartbeat_init(void);
uint32_t metriplectic_heartbeat_get_ticks(void);
void metriplectic_heartbeat_wait(uint32_t ticks);

#endif /* METRIPLECTIC_HEARTBEAT_H */
