#ifndef QUANTUM_BRIDGE_H
#define QUANTUM_BRIDGE_H

#include <stdint.h>

// Constants
#define VM_MAX_PAGES 1024
#define MU_PHI_LAMINAR 0.1
#define REI_WARNING_THRESHOLD 2000.0
#define REI_CRITICAL_THRESHOLD 2300.0
#define Q5_STATE_COUNT 32
#define D1024_BIT_WIDTH 1024
#define SUBSTRATE_VT_NOMINAL 0.8 // Nominal voltage in Volts

// Side Management Constants
#define QC_SIDE_LIGHTNING  0x01 // Quantum Discharge Side
#define QC_SIDE_LIGHTHOUSE 0x02 // Classical Structure Side

// Feedback Commands
#define FEEDBACK_NONE      0x00
#define FEEDBACK_STABILIZE 0x01 // Order Golden Operator stabilization
#define FEEDBACK_BOOST_VT  0x02 // Increase transistor voltage
#define FEEDBACK_QUENCH    0x03 // Force decoherence to prevent runaway


// VM States
#define VM_STATE_INIT       0
#define VM_STATE_LAMINAR    1
#define VM_STATE_TRANSITION 2
#define VM_STATE_TURBULENT  3
#define VM_STATE_SUPERFLUID 4
#define VM_STATE_HALTED     99

// Structs
typedef struct {
    double rho_I;          // Normalized information density
    double v_delta_E;      // Information velocity
    double xi_coh;         // Coherence length
    double mu_phi;         // Phase viscosity
    double ReI;            // Informational Reynolds Number
} QuantumFlowState;

typedef struct {
    uint32_t vortex_count;
    double total_winding;
    double otoc_lyapunov;
    uint32_t instability_ticks;
} QuantumChaosState;

typedef struct {
    uint32_t n;
    double theta;
    double phi;
    double O_n;
    double delta;
} QuantumGoldenState;

typedef struct {
    double q_amplitudes[Q5_STATE_COUNT]; // 5 Qubits -> 32 amplitudes
    uint32_t output_buffer[32];          // 32 * 32 bits = 1024 bits
    double transistor_voltage;           // Simulated substrate voltage
    uint32_t measurement_count;
    uint8_t active_side;                 // Current dominant side (Lightning/Lighthouse)
} QuantumTransceiver;

typedef struct {
    uint8_t command;
    double intensity;
    uint32_t timestamp;
} QuantumFeedback;


typedef struct {
    uint32_t state;
    
    // Guest CPU State (Simple)
    uint32_t guest_pc;
    uint32_t guest_sp;
    uint32_t guest_regs[8];
    
    // Memory
    uint32_t *page_table;
    uint32_t page_count;
    uint8_t *framebuffer;
    
    // Sub-systems
    QuantumFlowState flow;
    QuantumChaosState chaos;
    QuantumGoldenState golden_state;
    QuantumTransceiver transceiver;
    QuantumFeedback last_feedback;
    
    // I/O
    char serial_buffer[4096];
} QuantumVM;

// API
void quantum_bridge_init(QuantumVM *vm);
void quantum_bridge_run(QuantumVM *vm);
int quantum_bridge_load_kernel(QuantumVM *vm, const uint8_t *kernel_image, uint32_t size);
void quantum_bridge_halt(QuantumVM *vm);
void quantum_bridge_print_diagnostics(QuantumVM *vm);

// Q5-D1024 Transceiver API
void quantum_bridge_transceive(QuantumVM *vm);
void quantum_bridge_measure_q5(QuantumVM *vm, double *amplitudes);

// Side Management API
void quantum_bridge_inject_feedback(QuantumVM *vm, uint8_t command, double intensity);
void quantum_bridge_sync_sides(QuantumVM *vm);

#endif
