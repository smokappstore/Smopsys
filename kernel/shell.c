/*
 * Smopsys Shell - Implementation
 */

#include "shell.h"
#include "../drivers/vga_holographic.h"
#include "../drivers/metriplectic_kbd.h"
#include "golden_operator.h"
#include "../drivers/metriplectic_heartbeat.h"
#include <stdint.h>

extern GoldenState current_golden_state;
extern GoldenObservables current_golden_obs;

/* Local string utilities */
static int strcmp(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

static uint32_t strlen(const char *s) {
    uint32_t len = 0;
    while (*s++) len++;
    return len;
}

#define MAX_CMD_LEN 64

static char cmd_buffer[MAX_CMD_LEN];
static int cmd_ptr = 0;

static void shell_prompt(void) {
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_holographic_write("ql-bias> ");
}

static void exec_command(const char *cmd) {
    if (strcmp(cmd, "help") == 0) {
        vga_holographic_write("Commands: status, ticks, memory, laser, clear, help\n");
    } else if (strcmp(cmd, "clear") == 0) {
        vga_holographic_clear();
    } else if (strcmp(cmd, "ticks") == 0) {
        vga_holographic_write("System Heartbeat (ms): ");
        vga_holographic_write_decimal(metriplectic_heartbeat_get_ticks());
        vga_holographic_write("\n");
    } else if (strcmp(cmd, "status") == 0) {
        vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        vga_holographic_write("\n--- METRIPLECTIC ENGINE STATE ---\n");
        vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_holographic_write("  Ticks: "); vga_holographic_write_decimal(metriplectic_heartbeat_get_ticks());
        vga_holographic_write("\n  O_n:   "); vga_holographic_write_float(current_golden_state.O_n, 6);
        vga_holographic_write("\n  Theta: "); vga_holographic_write_float(current_golden_state.theta, 6);
        vga_holographic_write("\n  Flow:  LAMINAR\n");
    } else if (strcmp(cmd, "laser") == 0) {
        vga_holographic_write("Laser: Active (Metriplectic feedback loop)\n");
    } else if (strlen(cmd) > 0) {
        vga_holographic_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_holographic_write("Unknown command: ");
        vga_holographic_write(cmd);
        vga_holographic_write_char('\n');
    }
}

void shell_init(void) {
    metriplectic_kbd_init();
}

void shell_start(void) {
    vga_holographic_clear();
    vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    vga_holographic_write("Smopsys Q-CORE Bias Interface v0.1\n");
    vga_holographic_write("Type 'help' for commands.\n\n");
    
    shell_prompt();
    
    while (1) {
        char c = metriplectic_kbd_getc();
        if (c == 0) continue;
        
        if (c == '\n') {
            vga_holographic_write_char('\n');
            cmd_buffer[cmd_ptr] = '\0';
            exec_command(cmd_buffer);
            cmd_ptr = 0;
            shell_prompt();
        } else if (c == '\b') {
            if (cmd_ptr > 0) {
                cmd_ptr--;
                /* Simple backspace visual (TODO: fix cursor) */
                vga_holographic_write_char('\b');
            }
        } else {
            if (cmd_ptr < MAX_CMD_LEN - 1) {
                cmd_buffer[cmd_ptr++] = c;
                vga_holographic_write_char(c);
            }
        }
    }
}
