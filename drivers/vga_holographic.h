/*
 * VGA Holographic Driver - Smopsys Q-CORE
 * 
 * Driver de salida visual con esquema de colores cuántico:
 * - Verde: Estados coherentes (θ ≈ 0)
 * - Amarillo: Estados en transición (θ ≈ π)
 * - Rojo: Estados disipativos (θ ≈ 2π)
 */

#ifndef VGA_HOLOGRAPHIC_H
#define VGA_HOLOGRAPHIC_H

#include <bayesian_serial.h>

/* Dimensiones de pantalla VGA texto */
#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

/* Colores VGA (16 colores estándar) */
typedef enum {
    VGA_COLOR_BLACK         = 0,
    VGA_COLOR_BLUE          = 1,
    VGA_COLOR_GREEN         = 2,
    VGA_COLOR_CYAN          = 3,
    VGA_COLOR_RED           = 4,
    VGA_COLOR_MAGENTA       = 5,
    VGA_COLOR_BROWN         = 6,
    VGA_COLOR_LIGHT_GREY    = 7,
    VGA_COLOR_DARK_GREY     = 8,
    VGA_COLOR_LIGHT_BLUE    = 9,
    VGA_COLOR_LIGHT_GREEN   = 10,
    VGA_COLOR_LIGHT_CYAN    = 11,
    VGA_COLOR_LIGHT_RED     = 12,
    VGA_COLOR_LIGHT_MAGENTA = 13,
    VGA_COLOR_YELLOW        = 14,
    VGA_COLOR_WHITE         = 15,
} VGAColor;

/* Colores Metriplécticos (mapeo físico → visual) */
#define COLOR_COHERENT    VGA_COLOR_LIGHT_GREEN   /* θ ≈ 0 (Polo Norte) */
#define COLOR_TRANSITION  VGA_COLOR_YELLOW        /* θ ≈ π (Ecuador) */
#define COLOR_DISSIPATIVE VGA_COLOR_LIGHT_RED     /* θ ≈ 2π (Polo Sur) */
#define COLOR_OPERATOR    VGA_COLOR_CYAN          /* Operador Ô_n */
#define COLOR_HEADER      VGA_COLOR_WHITE         /* Headers */

/* Funciones del driver VGA */
void vga_holographic_init(void);
void vga_holographic_clear(void);
void vga_holographic_set_color(uint8_t fg, uint8_t bg);
void vga_holographic_set_cursor(uint8_t row, uint8_t col);

/* Escritura de texto */
void vga_holographic_write_char(char c);
void vga_holographic_write(const char *str);
void vga_holographic_write_at(const char *str, uint8_t row, uint8_t col);

/* Escritura de números */
void vga_holographic_write_hex(uint32_t val);
void vga_holographic_write_decimal(uint32_t num);
void vga_holographic_write_signed(int32_t num);
void vga_holographic_write_float(double val, uint8_t precision);

/* Escritura con prefijo (para diagnósticos) */
void vga_holographic_write_labeled(const char *label, double value);

/* Scroll de pantalla */
void vga_holographic_scroll(void);

/* Crear atributo de color */
static inline uint8_t vga_make_color(VGAColor fg, VGAColor bg) {
    return (uint8_t)(fg | (bg << 4));
}

/* Crear entrada de video (caracter + atributo) */
static inline uint16_t vga_make_entry(char c, uint8_t color) {
    return (uint16_t)c | ((uint16_t)color << 8);
}

#endif /* VGA_HOLOGRAPHIC_H */
