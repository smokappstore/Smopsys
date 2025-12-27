/* AUTO-GENERATED SMOPSYSQL CODE */
#include "ql_bridge.h"

void quantum_program(void) {
    laser_pulse_emit("1550nm", "100ns", 'H');
    busy_wait_ns(50);
    measure_qubit("q0");
    serial_putstr("QUANTUM_HELLO\n");
    check_thermal_page(0x10000, 0.7);
    sync_metriplectc_phase(3.14159);
    laser_pulse_emit("405nm", "200ns", 'V');
    busy_wait_ns(100);
}
