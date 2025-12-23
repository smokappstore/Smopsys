/*
 * VGA Holographic Driver - Implementación
 * Smopsys Q-CORE
 * 
 * Dinámica Metriplética del Buffer:
 * - L_symp: Escritura circular en buffer (conserva información)
 * - L_metr: Scroll disipativo (pierde información superior)
 */

#include "vga_holographic.h"

/* Estado global del driver */
static uint16_t *vga_buffer = (uint16_t *)VGA_MEMORY;
static uint8_t vga_row = 0;
static uint8_t vga_col = 0;
static uint8_t vga_color = 0x0F; /* Blanco sobre negro por defecto */

/* Tabla de caracteres hexadecimales */
static const char hex_table[] = "0123456789ABCDEF";

/* ============================================================
 * INICIALIZACIÓN Y LIMPIEZA
 * ============================================================ */

void vga_holographic_init(void) {
    vga_buffer = (uint16_t *)VGA_MEMORY;
    vga_row = 0;
    vga_col = 0;
    vga_color = vga_make_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_holographic_clear();
}

void vga_holographic_clear(void) {
    uint16_t blank = vga_make_entry(' ', vga_color);
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        vga_buffer[i] = blank;
    }
    vga_row = 0;
    vga_col = 0;
}

/* ============================================================
 * CONTROL DE ESTADO
 * ============================================================ */

void vga_holographic_set_color(uint8_t fg, uint8_t bg) {
    vga_color = vga_make_color(fg, bg);
}

void vga_holographic_set_cursor(uint8_t row, uint8_t col) {
    if (row < VGA_HEIGHT) vga_row = row;
    if (col < VGA_WIDTH) vga_col = col;
}

/* ============================================================
 * SCROLL (Componente Disipativo L_metr)
 * 
 * La información en la línea superior se "evapora" hacia el
 * polo sur (θ → 2π), perdiendo entropía del sistema visual.
 * ============================================================ */

void vga_holographic_scroll(void) {
    /* Mover todas las líneas hacia arriba */
    for (int row = 0; row < VGA_HEIGHT - 1; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            int src = (row + 1) * VGA_WIDTH + col;
            int dst = row * VGA_WIDTH + col;
            vga_buffer[dst] = vga_buffer[src];
        }
    }
    
    /* Limpiar última línea (nueva entropía cero) */
    uint16_t blank = vga_make_entry(' ', vga_color);
    for (int col = 0; col < VGA_WIDTH; col++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + col] = blank;
    }
    
    vga_row = VGA_HEIGHT - 1;
}

/* ============================================================
 * ESCRITURA DE CARACTERES
 * ============================================================ */

void vga_holographic_write_char(char c) {
    if (c == '\n') {
        vga_col = 0;
        vga_row++;
    } else if (c == '\r') {
        vga_col = 0;
    } else if (c == '\t') {
        vga_col = (vga_col + 4) & ~3; /* Alinear a 4 columnas */
    } else {
        int index = vga_row * VGA_WIDTH + vga_col;
        vga_buffer[index] = vga_make_entry(c, vga_color);
        vga_col++;
    }
    
    /* Wrap de columna */
    if (vga_col >= VGA_WIDTH) {
        vga_col = 0;
        vga_row++;
    }
    
    /* Scroll si llegamos al final */
    if (vga_row >= VGA_HEIGHT) {
        vga_holographic_scroll();
    }
}

void vga_holographic_write(const char *str) {
    while (*str) {
        vga_holographic_write_char(*str++);
    }
}

void vga_holographic_write_at(const char *str, uint8_t row, uint8_t col) {
    vga_holographic_set_cursor(row, col);
    vga_holographic_write(str);
}

/* ============================================================
 * ESCRITURA DE NÚMEROS HEXADECIMALES
 * ============================================================ */

void vga_holographic_write_hex(uint32_t val) {
    vga_holographic_write("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0x0F;
        vga_holographic_write_char(hex_table[nibble]);
    }
}

/* ============================================================
 * ESCRITURA DE NÚMEROS DECIMALES
 * ============================================================ */

void vga_holographic_write_decimal(uint32_t num) {
    if (num == 0) {
        vga_holographic_write_char('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    /* Imprimir en orden inverso */
    while (i > 0) {
        vga_holographic_write_char(buffer[--i]);
    }
}

void vga_holographic_write_signed(int32_t num) {
    if (num < 0) {
        vga_holographic_write_char('-');
        num = -num;
    }
    vga_holographic_write_decimal((uint32_t)num);
}

/* ============================================================
 * ESCRITURA DE PUNTO FLOTANTE
 * 
 * Implementación sin libmath para bare-metal.
 * Precisión limitada pero suficiente para diagnósticos.
 * ============================================================ */

void vga_holographic_write_float(double val, uint8_t precision) {
    /* Manejar negativos */
    if (val < 0) {
        vga_holographic_write_char('-');
        val = -val;
    }
    
    /* Parte entera */
    uint32_t int_part = (uint32_t)val;
    vga_holographic_write_decimal(int_part);
    
    /* Punto decimal */
    vga_holographic_write_char('.');
    
    /* Parte fraccionaria */
    double frac = val - (double)int_part;
    for (uint8_t i = 0; i < precision; i++) {
        frac *= 10.0;
        uint8_t digit = (uint8_t)frac;
        vga_holographic_write_char('0' + digit);
        frac -= digit;
    }
}

/* ============================================================
 * ESCRITURA ETIQUETADA (Para diagnósticos metriplécticos)
 * 
 * Formato: "label: value"
 * Usa colores según el valor para indicar estado cuántico.
 * ============================================================ */

void vga_holographic_write_labeled(const char *label, double value) {
    /* Guardar color actual */
    uint8_t saved_color = vga_color;
    
    /* Label en blanco */
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
    vga_holographic_write(label);
    vga_holographic_write(": ");
    
    /* Valor con color según magnitud (mapeo θ → color) */
    if (value < 1.0) {
        vga_holographic_set_color(COLOR_COHERENT, VGA_COLOR_BLACK);
    } else if (value < 3.14159) {
        vga_holographic_set_color(COLOR_TRANSITION, VGA_COLOR_BLACK);
    } else {
        vga_holographic_set_color(COLOR_DISSIPATIVE, VGA_COLOR_BLACK);
    }
    
    vga_holographic_write_float(value, 4);
    
    /* Restaurar color */
    vga_color = saved_color;
    vga_holographic_write_char('\n');
}
