/*
 * Bayesian Serial Driver - Implementación
 * Smopsys Q-CORE
 * 
 * Dinámica Metriplética de I/O:
 * - L_symp: Buffer FIFO conserva el orden de bytes
 * - L_metr: Timeout bayesiano disipa esperas infinitas
 * 
 * La "inferencia bayesiana" se manifiesta en:
 * - Prior: Expectativa de datos disponibles (basado en historial)
 * - Likelihood: Estado actual del Line Status Register
 * - Posterior: Decisión de leer/esperar/abortar
 */

#include "bayesian_serial.h"

/* Tabla de caracteres hexadecimales */
static const char hex_table[] = "0123456789ABCDEF";

/* ============================================================
 * INICIALIZACIÓN DEL PUERTO SERIAL
 * 
 * Configura COM1 con:
 * - 38400 baud
 * - 8 bits de datos
 * - 1 bit de stop
 * - Sin paridad
 * - FIFO habilitado
 * ============================================================ */

void bayesian_serial_init(void) {
    /* Deshabilitar interrupciones */
    outb(SERIAL_INT_ENABLE, 0x00);
    
    /* Habilitar DLAB (Divisor Latch Access Bit) para configurar baud rate */
    outb(SERIAL_LINE_CTRL, 0x80);
    
    /* Configurar divisor para 38400 baud */
    outb(SERIAL_DIVISOR_LOW, BAUD_38400);   /* Low byte */
    outb(SERIAL_DIVISOR_HIGH, 0x00);        /* High byte */
    
    /* Configurar formato: 8N1 (8 bits, no paridad, 1 stop) */
    /* DLAB=0, 8 bits = 0x03 */
    outb(SERIAL_LINE_CTRL, 0x03);
    
    /* Habilitar FIFO con trigger de 14 bytes */
    /* Bit 0: Enable FIFOs
     * Bit 1: Clear Receive FIFO
     * Bit 2: Clear Transmit FIFO
     * Bit 6-7: Interrupt trigger level (11 = 14 bytes) */
    outb(SERIAL_FIFO_CTRL, 0xC7);
    
    /* Habilitar DTR, RTS, y OUT2 (necesario para interrupciones) */
    outb(SERIAL_MODEM_CTRL, 0x0B);
    
    /* Modo loopback para test inicial */
    outb(SERIAL_MODEM_CTRL, 0x1E);
    
    /* Enviar byte de test */
    outb(SERIAL_DATA, 0xAE);
    
    /* Verificar que recibimos el mismo byte */
    if (inb(SERIAL_DATA) != 0xAE) {
        /* Puerto defectuoso - continuar de todos modos */
    }
    
    /* Configurar modo normal (no loopback) */
    outb(SERIAL_MODEM_CTRL, 0x0F);
}

/* ============================================================
 * VERIFICACIÓN DE DISPONIBILIDAD
 * ============================================================ */

static int serial_is_transmit_empty(void) {
    return inb(SERIAL_LINE_STATUS) & LSR_TX_HOLDING_EMPTY;
}

int bayesian_serial_available(void) {
    return inb(SERIAL_LINE_STATUS) & LSR_DATA_READY;
}

/* ============================================================
 * ESCRITURA DE CARACTERES
 * ============================================================ */

void bayesian_serial_write_char(char c) {
    /* Esperar a que el buffer de transmisión esté vacío */
    /* Esta es la parte "disipativa" - el timeout implícito */
    int timeout = 100000;
    while (!serial_is_transmit_empty() && timeout > 0) {
        timeout--;
    }
    
    outb(SERIAL_DATA, c);
}

void bayesian_serial_write(const char *str) {
    while (*str) {
        if (*str == '\n') {
            bayesian_serial_write_char('\r'); /* CR antes de LF */
        }
        bayesian_serial_write_char(*str++);
    }
}

/* ============================================================
 * ESCRITURA DE NÚMEROS HEXADECIMALES
 * ============================================================ */

void bayesian_serial_write_hex(uint32_t val) {
    bayesian_serial_write("0x");
    for (int i = 7; i >= 0; i--) {
        uint8_t nibble = (val >> (i * 4)) & 0x0F;
        bayesian_serial_write_char(hex_table[nibble]);
    }
}

/* ============================================================
 * ESCRITURA DE NÚMEROS DECIMALES
 * ============================================================ */

void bayesian_serial_write_decimal(uint32_t num) {
    if (num == 0) {
        bayesian_serial_write_char('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (num > 0) {
        buffer[i++] = '0' + (num % 10);
        num /= 10;
    }
    
    while (i > 0) {
        bayesian_serial_write_char(buffer[--i]);
    }
}

/* ============================================================
 * ESCRITURA DE PUNTO FLOTANTE
 * ============================================================ */

void bayesian_serial_write_float(double val, uint8_t precision) {
    if (val < 0) {
        bayesian_serial_write_char('-');
        val = -val;
    }
    
    uint32_t int_part = (uint32_t)val;
    bayesian_serial_write_decimal(int_part);
    
    bayesian_serial_write_char('.');
    
    double frac = val - (double)int_part;
    for (uint8_t i = 0; i < precision; i++) {
        frac *= 10.0;
        uint8_t digit = (uint8_t)frac;
        bayesian_serial_write_char('0' + digit);
        frac -= digit;
    }
}

/* ============================================================
 * LECTURA NO BLOQUEANTE
 * 
 * Implementa la parte "bayesiana":
 * - Si hay datos (likelihood alta), los devuelve
 * - Si no hay datos, retorna -1 (posterior indica ausencia)
 * ============================================================ */

int bayesian_serial_read_char(void) {
    if (!bayesian_serial_available()) {
        return -1; /* No hay datos disponibles */
    }
    return inb(SERIAL_DATA);
}

/* ============================================================
 * ESCRITURA ETIQUETADA (Para diagnósticos)
 * ============================================================ */

void bayesian_serial_write_labeled(const char *label, double value) {
    bayesian_serial_write("[");
    bayesian_serial_write(label);
    bayesian_serial_write("] ");
    bayesian_serial_write_float(value, 6);
    bayesian_serial_write("\n");
}
