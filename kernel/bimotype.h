#ifndef BIMOTYPE_H
#define BIMOTYPE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    float phase;
    int is_dash;
} BiMOPulse;

void bimotype_init(void);
void bimotype_pulse_message(const char* message);
void bimotype_update(uint32_t delta_ms);

#ifdef __cplusplus
}
#endif

#endif
