/*
 * SmopsysQL Bridge - Smopsys Q-CORE
 * 
 * Interface entre el código compilado de SmopsysQL y el kernel.
 */
#ifndef QL_BRIDGE_H
#define QL_BRIDGE_H

#include <stdint.h>

/* Emitir un pulso láser configurado */
void laser_pulse_emit(const char* wavelength, const char* duration, char polarization);

/* Delay preciso en nanosegundos (emulación ciclos) */
void busy_wait_ns(uint32_t ns);

/* Medir un qubit y reportar por serial */
void measure_qubit(const char* qubit_id);

/* Enviar cadena por puerto serial bayesiano */
void serial_putstr(const char* str);

/* Chequear entropía de una página de memoria metriplética */
void check_thermal_page(uint32_t address, double threshold);

/* Sincronizar fase metriplética global */
void sync_metriplectc_phase(double phase);

/* Función principal generada por el compilador */
void quantum_program(void);

#endif /* QL_BRIDGE_H */
