#include "metriplectic_heartbeat.h"
#include "../kernel/golden_operator.h"
#include "../kernel/idt.h"
#include "../drivers/bayesian_serial.h"

static volatile uint32_t global_ticks = 0;
extern GoldenState current_golden_state;
extern GoldenObservables current_golden_obs;

/* Envía un comando al PIT */
static void pit_send_command(uint8_t cmd) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(cmd), "Nd"(PIT_COMMAND));
}

/* Envía datos al Canal 0 del PIT */
static void pit_send_data(uint8_t data) {
    __asm__ __volatile__ ("outb %0, %1" : : "a"(data), "Nd"(PIT_CHANNEL0_DATA));
}

/* Handler del latido (IRQ0) */
void metriplectic_heartbeat_handler(void) {
    global_ticks++;
    
    /* Avanzar el operador áureo en cada tick (1ms) */
    /* Nota: Esto desacopla la física de la velocidad de ejecución del shell */
    golden_operator_step(&current_golden_state);
    golden_operator_compute_observables(&current_golden_state, &current_golden_obs);
    
    /* Mostrar un log cada 1000 ticks (1 segundo) por serial */
    if (global_ticks % 1000 == 0) {
        bayesian_serial_write("[HEARTBEAT] 1 second elapsed. O_n: ");
        /* Aquí podríamos imprimir O_n si tuviéramos float to string, 
           pero por ahora solo marcamos el paso del tiempo */
        bayesian_serial_write("OK\n");
    }
}

void metriplectic_heartbeat_init(void) {
    /* Configurar PIT: Canal 0, modo Square Wave, Access lobyte/hibyte */
    /* Comando 0x36: 00 (Canal 0) | 11 (Lo/Hi Byte) | 011 (Modo 3) | 0 (Binary) */
    pit_send_command(0x36);
    
    /* Calcular el divisor */
    uint32_t divisor = PIT_BASE_FREQUENCY / HEARTBEAT_HZ;
    
    /* Enviar divisor (lobyte, luego hibyte) */
    pit_send_data((uint8_t)(divisor & 0xFF));
    pit_send_data((uint8_t)((divisor >> 8) & 0xFF));
    
    bayesian_serial_write("[INIT] Metriplectic Heartbeat configured at 1000Hz\n");
}

uint32_t metriplectic_heartbeat_get_ticks(void) {
    return global_ticks;
}

void metriplectic_heartbeat_wait(uint32_t ticks) {
    uint32_t start_ticks = global_ticks;
    while ((global_ticks - start_ticks) < ticks) {
        /* Busy wait pero basado en hardware ticks */
        __asm__ __volatile__ ("pause");
    }
}
