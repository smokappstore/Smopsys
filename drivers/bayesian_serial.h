/*
 * Bayesian Serial Driver - Smopsys Q-CORE
 * 
 * Driver de comunicación serial con inferencia bayesiana implícita:
 * - Prior: Estado esperado del buffer de transmisión
 * - Likelihood: Datos observados en el puerto
 * - Posterior: Decisión de timeout/retransmisión
 * 
 * Puerto: COM1 (0x3F8)
 * Baudrate: 38400
 */

#ifndef BAYESIAN_SERIAL_H
#define BAYESIAN_SERIAL_H

#include <bayesian_serial.h>

/* Registros del puerto serial COM1 */
#define SERIAL_COM1_BASE     0x3F8

#define SERIAL_DATA          (SERIAL_COM1_BASE + 0)  /* Data register (R/W) */
#define SERIAL_INT_ENABLE    (SERIAL_COM1_BASE + 1)  /* Interrupt Enable */
#define SERIAL_FIFO_CTRL     (SERIAL_COM1_BASE + 2)  /* FIFO Control */
#define SERIAL_LINE_CTRL     (SERIAL_COM1_BASE + 3)  /* Line Control */
#define SERIAL_MODEM_CTRL    (SERIAL_COM1_BASE + 4)  /* Modem Control */
#define SERIAL_LINE_STATUS   (SERIAL_COM1_BASE + 5)  /* Line Status */
#define SERIAL_MODEM_STATUS  (SERIAL_COM1_BASE + 6)  /* Modem Status */
#define SERIAL_SCRATCH       (SERIAL_COM1_BASE + 7)  /* Scratch Register */

/* Divisor del baud rate (acceso con DLAB=1) */
#define SERIAL_DIVISOR_LOW   (SERIAL_COM1_BASE + 0)
#define SERIAL_DIVISOR_HIGH  (SERIAL_COM1_BASE + 1)

/* Baudrates comunes (divisor = 115200 / baud) */
#define BAUD_115200  1
#define BAUD_57600   2
#define BAUD_38400   3
#define BAUD_19200   6
#define BAUD_9600    12

/* Bits de Line Status Register */
#define LSR_DATA_READY       0x01
#define LSR_OVERRUN_ERROR    0x02
#define LSR_PARITY_ERROR     0x04
#define LSR_FRAMING_ERROR    0x08
#define LSR_BREAK_INDICATOR  0x10
#define LSR_TX_HOLDING_EMPTY 0x20
#define LSR_TX_EMPTY         0x40
#define LSR_FIFO_ERROR       0x80

/* ============================================================
 * FUNCIONES INLINE DE I/O (x86)
 * 
 * Estas son las operaciones atómicas de bajo nivel
 * para comunicación con puertos de hardware.
 * ============================================================ */

static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ __volatile__("inb %1, %0" : "=a"(val) : "dN"(port));
    return val;
}

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ __volatile__("outb %0, %1" : : "a"(val), "dN"(port));
}

static inline void io_wait(void) {
    /* Pequeño delay usando puerto 0x80 (POST code) */
    outb(0x80, 0);
}

/* ============================================================
 * API PÚBLICA
 * ============================================================ */

/* Inicialización del puerto serial */
void bayesian_serial_init(void);

/* Escritura */
void bayesian_serial_write_char(char c);
void bayesian_serial_write(const char *str);
void bayesian_serial_write_hex(uint32_t val);
void bayesian_serial_write_decimal(uint32_t num);
void bayesian_serial_write_float(double val, uint8_t precision);

/* Lectura (no bloqueante) */
int bayesian_serial_read_char(void);       /* Retorna -1 si no hay datos */
int bayesian_serial_available(void);       /* Retorna 1 si hay datos */

/* Diagnósticos */
void bayesian_serial_write_labeled(const char *label, double value);

#endif /* BAYESIAN_SERIAL_H */
