#include "idt.h"
#include "panic.h"
#include "../drivers/bayesian_serial.h"

extern void metriplectic_heartbeat_handler(void);

/* Tabla IDT */
static struct idt_entry idt[256];
static struct idt_ptr   idtp;

/* Puertos del PIC */
#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1

/* Comandos ICW */
#define ICW1_INIT    0x11
#define ICW4_8086    0x01

/* EOI (End of Interrupt) */
#define PIC_EOI      0x20

/* Declaración de los stubs de ensamblador */
/* EOI (End of Interrupt) */
#define PIC_EOI      0x20

/* Declaración de los stubs de ensamblador */
extern uint32_t isr_stub_table[256];

/* Remapeo del PIC para evitar conflictos con excepciones de CPU */
static void pic_remap(uint8_t offset1, uint8_t offset2) {
    uint8_t a1, a2;
    
    a1 = inb(PIC1_DATA);
    a2 = inb(PIC2_DATA);
    
    outb(PIC1_COMMAND, ICW1_INIT);
    outb(PIC2_COMMAND, ICW1_INIT);
    
    outb(PIC1_DATA, offset1);
    outb(PIC2_DATA, offset2);
    
    outb(PIC1_DATA, 4);
    outb(PIC2_DATA, 2);
    
    outb(PIC1_DATA, ICW4_8086);
    outb(PIC2_DATA, ICW4_8086);
    
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
}

void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = (base & 0xFFFF);
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].selector  = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;
    
    /* Inicializar todas las entradas con ceros */
    for (int i = 0; i < 256; i++) {
        idt_set_gate(i, 0, 0, 0);
    }
    
    /* Remapear PIC: Master -> 0x20, Slave -> 0x28 */
    pic_remap(0x20, 0x28);
    
    /* Configurar los stubs (se asume que isr_stub_table está definida en assembly) */
    for (uint16_t i = 0; i < 256; i++) {
        idt_set_gate(i, (uint32_t)isr_stub_table[i], 0x08, IDT_GATE_INTERRUPT);
    }
    
    /* Cargar IDT */
    __asm__ __volatile__ ("lidt %0" : : "m" (idtp));
    
    bayesian_serial_write("[INIT] IDT and PIC remapped successfully\n");
}

/* Handler genérico llamado desde los stubs */
void isr_handler(uint32_t int_no) {
    /* Excepciones de CPU (0-31) */
    if (int_no < 32) {
        char msg[64] = "CPU Exception: ";
        /* En un kernel más avanzado mapearíamos int_no a nombres de excepción */
        panic("Unhandled CPU Exception");
    }

    /* IRQ0: Metriplectic Heartbeat */
    if (int_no == 32) {
        metriplectic_heartbeat_handler();
    }
    
    /* Por ahora solo enviamos EOI si es una interrupción física (IRQ) */

    if (int_no >= 32 && int_no <= 47) {
        if (int_no >= 40) {
            outb(PIC2_COMMAND, PIC_EOI);
        }
        outb(PIC1_COMMAND, PIC_EOI);
    }
}
