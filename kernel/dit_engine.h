/*
 * DIT Engine - Smopsys Q-CORE
 * 
 * Distributed Information Theory (DIT) Engine.
 * Implements the Conscious/Subconscious duality via MPSC queues.
 */

#ifndef DIT_ENGINE_H
#define DIT_ENGINE_H

#include <stdint.h>
#include <stdbool.h>

/* DIT Packet Structure */
typedef struct {
    double content;     /* Signal content */
    uint32_t parity;    /* Even=Conscious, Odd=Subconscious */
    double response;    /* Processed upgrade from Subconscious */
    bool resolved;      /* Flag indicating resolution */
} DITPacket;

#define DIT_QUEUE_SIZE 32

/* MPSC Queue for Subconscious Engine */
typedef struct {
    DITPacket buffer[DIT_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} DITQueue;

/* Global Engine State */
typedef struct {
    DITQueue subconscious_queue;
    uint32_t updates_received;
} DITEngineState;

/* Engine API */
void dit_engine_init(DITEngineState *state);

/* Conscious: Inject packet or process immediately */
bool dit_conscious_process(DITEngineState *state, DITPacket packet, double *immediate_result);

/* Subconscious: Background processing of the queue */
void dit_subconscious_step(DITEngineState *state);

/* Check for upgrades */
bool dit_get_upgrade(DITEngineState *state, double *upgrade);

#endif /* DIT_ENGINE_H */
