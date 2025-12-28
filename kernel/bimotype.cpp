#include "bimotype.h"

extern "C" {
#include "../drivers/vga_holographic.h"
#include "../drivers/bayesian_serial.h"
}


static int is_pulsing = 0;
static uint32_t pulse_timer = 0;
static int current_pulse_idx = 0;
static int pulse_state = 0; // 0: OFF, 1: ON

// Morse timing (ms)
#define DOT_TIME 100
#define DASH_TIME 300
#define GAP_TIME 100

void bimotype_init(void) {
    is_pulsing = 0;
    pulse_timer = 0;
}

void bimotype_pulse_message(const char* message) {
    bayesian_serial_write("[BiMO] Starting pulse sequence for: ");
    bayesian_serial_write(message);
    bayesian_serial_write("\n");
    
    is_pulsing = 1;
    current_pulse_idx = 0;
    pulse_timer = 0;
    pulse_state = 0;
}

void bimotype_update(uint32_t delta_ms) {
    if (!is_pulsing) return;

    pulse_timer += delta_ms;

    // Lógica simplificada de parpadeo (flicker)
    // En una implementación real, esto usaría la secuencia generada por el compilador
    // Por ahora, simulamos el efecto visual en el VGA
    
    if (pulse_timer > 200) {
        pulse_timer = 0;
        pulse_state = !pulse_state;
        
        if (pulse_state) {
            vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
            bayesian_serial_write(".");
        } else {
            vga_holographic_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
            bayesian_serial_write(" ");
        }

        
        // El "parpadeo" se nota en el cursor o en el siguiente texto impreso
    }
}
