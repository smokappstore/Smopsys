/*
 * Panic System - Smopsys Q-CORE
 * 
 * "Singularidad de Entropía Máxima" (Regla 1.3 - El Mandato Metriplético)
 */

#ifndef PANIC_H
#define PANIC_H

#include <stdint.h>

/**
 * Detiene el sistema inmediatamente debido a un error crítico o excepción.
 * Apaga el flujo conservativo para preservar la integridad del sistema.
 */
void panic(const char *message);

/**
 * Macro para aserciones en el kernel.
 */
#define panic_assert(condition, message) \
    if (!(condition)) { \
        panic("Assertion failed: " message); \
    }

#endif /* PANIC_H */
