/*
 * Keyboard Driver - Smopsys Q-CORE
 * 
 * Controlador de teclado PS/2 (Polling)
 */

#ifndef METRIPLECTIC_KBD_H
#define METRIPLECTIC_KBD_H

#include <stdint.h>

/* Puertos PS/2 */
#define KBD_DATA_PORT 0x60
#define KBD_STATUS_PORT 0x64

/* Inicializar teclado */
void metriplectic_kbd_init(void);

/* Leer un carácter (Bloqueante por polling) */
char metriplectic_kbd_getc(void);

/* Verificar si hay tecla disponible */
int metriplectic_kbd_has_key(void);

#endif /* METRIPLECTIC_KBD_H */
