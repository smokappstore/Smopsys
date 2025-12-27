#ifndef IDT_H
#define IDT_H

#include <stdint.h>

/* Atributos de compuertas (Interrupt/Trap gates) */
#define IDT_GATE_INTERRUPT 0x8E  /* 32-bit Interrupt Gate, Present, Ring 0 */
#define IDT_GATE_TRAP      0x8F  /* 32-bit Trap Gate, Present, Ring 0 */

/* Estructura de una entrada en la IDT (8 bytes) */
struct idt_entry {
    uint16_t base_low;      /* Bits 0-15 de la dirección del handler */
    uint16_t selector;      /* Selector de segmento de código (GDT) */
    uint8_t  always0;       /* Reservado, siempre 0 */
    uint8_t  flags;         /* Atributos (P, DPL, Type) */
    uint16_t base_high;     /* Bits 16-31 de la dirección del handler */
} __attribute__((packed));

/* Estructura del puntero IDT (para instrucción lidt) */
struct idt_ptr {
    uint16_t limit;         /* Tamaño de la IDT - 1 */
    uint32_t base;          /* Dirección base de la IDT */
} __attribute__((packed));

/* Funciones públicas */
void idt_init(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);

#endif /* IDT_H */
