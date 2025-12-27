/*
 * Panic System Implementation - Smopsys Q-CORE
 */

#include "panic.h"
#include "../drivers/vga_holographic.h"
#include "../drivers/bayesian_serial.h"

void panic(const char *message) {
    /* Deshabilitar interrupciones para evitar reentrada */
    __asm__ __volatile__ ("cli");

    /* Configurar visual disipativo (Rojo) */
    vga_holographic_set_color(COLOR_DISSIPATIVE, VGA_COLOR_BLACK);
    vga_holographic_clear();

    /* Mensaje de pánico */
    vga_holographic_write_at("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  ", 5, 10);
    vga_holographic_write_at("  !!             KERNEL PANIC: SINGULARITY          !!  ", 6, 10);
    vga_holographic_write_at("  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!  ", 7, 10);

    vga_holographic_write_at("Error:", 10, 10);
    vga_holographic_write_at(message, 11, 12);

    vga_holographic_write_at("Status: Maximum Entropy reached. Conservative flow stopped.", 14, 10);
    vga_holographic_write_at("        System halted to prevent thermal death.", 15, 10);

    /* Log serial bayesiano */
    bayesian_serial_write("\n[CRITICAL] KERNEL PANIC: ");
    bayesian_serial_write(message);
    bayesian_serial_write("\n[STATUS] Dissipative limit reached. Halted.\n");

    /* Bucle infinito de detención */
    while (1) {
        __asm__ __volatile__ ("hlt");
    }
}
