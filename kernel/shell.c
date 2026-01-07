/*
 * Smopsys Shell - Implementation
 */

#include "shell.h"
#include "../drivers/vga_holographic.h"
#include "../drivers/metriplectic_kbd.h"
#include "golden_operator.h"
#include "../drivers/metriplectic_heartbeat.h"
#include <stdint.h>
#include "dit_engine.h"

extern GoldenState current_golden_state;
extern GoldenObservables current_golden_obs;
extern DITEngineState global_dit_state;

/* Memory Manager Bridge */
uint32_t memory_get_used_pages(void);
uint32_t memory_get_total_pages(void);
double memory_get_centroid_z(void);
double memory_get_total_entropy(void);

int memory_get_page_stats(uint32_t idx, uint32_t *addr, double *theta, int *state);
void memory_get_lagrangian(uint32_t page_idx, double *L_symp, double *L_metr);
void panic(const char *message);



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

static int strncmp(const char *s1, const char *s2, uint32_t n) {
    while (n-- > 0 && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    if (n == (uint32_t)-1) return 0; // All n characters matched
    return *(unsigned char *)s1 - *(unsigned char *)s2;
}

#define MAX_CMD_LEN 64

static char cmd_buffer[MAX_CMD_LEN];
static int cmd_ptr = 0;

extern void bimotype_pulse_message(const char* message);

static void shell_prompt(void) {
    vga_holographic_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
    vga_holographic_write("ql-bias> ");
}

static void exec_command(const char *cmd) {
    bayesian_serial_write("[SHELL] Executing: ");
    bayesian_serial_write(cmd);
    bayesian_serial_write("\n");

    if (strcmp(cmd, "help") == 0) {

        vga_holographic_write("Commands: status, ticks, memory, pages, competition, laser, panic, clear, bimotype, help\n");


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
    } else if (strcmp(cmd, "memory") == 0) {
        uint32_t used = memory_get_used_pages();
        uint32_t total = memory_get_total_pages();
        double z_finch = memory_get_centroid_z();
        double entropy = memory_get_total_entropy();

        vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        vga_holographic_write("\n--- METRIPLECTIC MEMORY ---\n");
        vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        
        vga_holographic_write("  Pages:    "); 
        vga_holographic_write_decimal(used);
        vga_holographic_write("/");
        vga_holographic_write_decimal(total);
        
        vga_holographic_write("\n  Centroid: ");
        if (z_finch < 0.5) vga_holographic_set_color(COLOR_COHERENT, VGA_COLOR_BLACK);
        else vga_holographic_set_color(COLOR_DISSIPATIVE, VGA_COLOR_BLACK);
        vga_holographic_write_float(z_finch, 4);
        vga_holographic_write(" (Z-Finch)");

        vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_holographic_write("\n  Entropy:  ");
        vga_holographic_write_float(entropy, 4);
        vga_holographic_write("\n");
    } else if (strcmp(cmd, "pages") == 0) {
        vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        vga_holographic_write("\n--- METRIPLECTIC PAGES (First 15) ---\n");
        vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        vga_holographic_write(" IDX  ADDRESS     THETA   STATE\n");
        
        for (uint32_t i = 0; i < 15; i++) {
            uint32_t addr;
            double theta;
            int state;
            if (memory_get_page_stats(i, &addr, &theta, &state)) {
                vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_holographic_write_decimal(i);
                vga_holographic_write("   ");
                vga_holographic_write_hex(addr);
                vga_holographic_write("  ");
                
                /* Color coding based on theta region */
                if (theta < 1.0) vga_holographic_set_color(COLOR_COHERENT, VGA_COLOR_BLACK);
                else if (theta < 3.14159) vga_holographic_set_color(COLOR_TRANSITION, VGA_COLOR_BLACK);
                else vga_holographic_set_color(COLOR_DISSIPATIVE, VGA_COLOR_BLACK);
                
                vga_holographic_write_float(theta, 3);
                vga_holographic_write("   ");
                
                if (state == 0) vga_holographic_write("EMPTY");
                else if (state == 1) vga_holographic_write("ALLOC");
                else if (state == 2) vga_holographic_write("THERMAL");
                else vga_holographic_write("EVAP");
                
                vga_holographic_write_char('\n');
            }
        }
    } else if (strcmp(cmd, "panic") == 0) {
        panic("Manual singularity triggered via shell.");
    } else if (strcmp(cmd, "competition") == 0) {
        double L_symp, L_metr;
        memory_get_lagrangian(0, &L_symp, &L_metr); // Check first page
        
        vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
        vga_holographic_write("\n--- LAGRANGIAN COMPETITION (Page 0) ---\n");
        
        vga_holographic_set_color(VGA_COLOR_LIGHT_GREEN, VGA_COLOR_BLACK);
        vga_holographic_write("  L_symp (Ham): ");
        vga_holographic_write_float(L_symp, 6);
        
        vga_holographic_set_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK);
        vga_holographic_write("\n  L_metr (Diss): ");
        vga_holographic_write_float(L_metr, 6);
        
        double ratio = (L_metr != 0) ? (L_symp / L_metr) : 0;
        vga_holographic_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
        vga_holographic_write("\n  Ratio: ");
        vga_holographic_write_float(ratio, 4);
        vga_holographic_write_char('\n');
    } else if (strcmp(cmd, "bimotype") == 0 || strncmp(cmd, "bimotype ", 9) == 0) {
        const char *msg = (strlen(cmd) > 9) ? cmd + 9 : "BIMO";
        bimotype_pulse_message(msg);
    } else if (strcmp(cmd, "dit") == 0) {
        vga_holographic_set_color(VGA_COLOR_LIGHT_MAGENTA, VGA_COLOR_BLACK);
        vga_holographic_write("\n[CON] Conscious Engine Active. Starting simulation...\n");

        for (uint32_t n = 0; n < 10; n++) {
            DITPacket packet = { .content = (double)n, .parity = n, .resolved = false };
            double immediate;

            if (dit_conscious_process(&global_dit_state, packet, &immediate)) {
                vga_holographic_set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
                vga_holographic_write("[CON] Node Par: Proyectando realidad ");
                vga_holographic_write_decimal(n);
                vga_holographic_write_char('\n');
            } else {
                vga_holographic_set_color(VGA_COLOR_CYAN, VGA_COLOR_BLACK);
                vga_holographic_write("[CON] Node Impar: Subconsciente validando n=");
                vga_holographic_write_decimal(n);
                vga_holographic_write_char('\n');
            }

            /* Check for "Eureka" moments from background */
            double upgrade;
            if (dit_get_upgrade(&global_dit_state, &upgrade)) {
                vga_holographic_set_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK);
                vga_holographic_write("[CON] !INTUICION! Patron resuelto: ");
                vga_holographic_write_float(upgrade, 4);
                vga_holographic_write_char('\n');
            }
        }
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
    vga_holographic_write("  status  - Show engine state\n");
    vga_holographic_write("  ticks   - Show system heartbeat ticks\n");
    vga_holographic_write("  memory  - Show memory manager stats\n");
    vga_holographic_write("  pages   - List first 15 memory pages\n");
    vga_holographic_write("  laser   - Show laser status\n");
    vga_holographic_write("  panic   - Trigger a manual system panic\n");
    vga_holographic_write("  clear   - Clear the screen\n");
    vga_holographic_write("  competition - Show Lagrangian competition\n");
    vga_holographic_write("  bimotype <msg> - Pulse message via BiMOtype\n");
    vga_holographic_write("  help    - Show this help\n");
    vga_holographic_write("\n");
    
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
