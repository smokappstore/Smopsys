/*
 * Keyboard Driver - Implementation
 * Smopsys Q-CORE
 */

#include "metriplectic_kbd.h"

/* Funciones de puerto E/S */
static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ __volatile__("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/* Tabla de scancodes (US QWERTY) - Simplificada */
static const char scancode_map[] = {
    0,  27, '1', '2', '3', '4', '5', '6', '7', '8',	/* 9 */
  '9', '0', '-', '=', '\b',	/* Backspace */
  '\t',			/* Tab */
  'q', 'w', 'e', 'r',	/* 19 */
  't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\n',	/* Enter key */
    0,			/* 29   - Control */
  'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',	/* 39 */
 '\'', '`',   0,		/* Left shift */
 '\\', 'z', 'x', 'c', 'v', 'b', 'n',			/* 49 */
  'm', ',', '.', '/',   0,				/* Right shift */
  '*',
    0,	/* Alt */
  ' ',	/* Space bar */
    0,	/* Caps lock */
};

void metriplectic_kbd_init(void) {
    /* Por ahora no hace falta nada para polling básico */
}

int metriplectic_kbd_has_key(void) {
    return (inb(KBD_STATUS_PORT) & 1);
}

char metriplectic_kbd_getc(void) {
    while (!metriplectic_kbd_has_key()) {
        __asm__ __volatile__("pause");
    }
    
    uint8_t scancode = inb(KBD_DATA_PORT);
    
    /* Filtrar "key up" (bit 7 set) y códigos extendidos */
    if (scancode & 0x80) return 0;
    if (scancode >= sizeof(scancode_map)) return 0;
    
    return scancode_map[scancode];
}
