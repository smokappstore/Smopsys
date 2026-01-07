#include <stdio.h>
#include <assert.h>
#include "kernel/dit_engine.h"

/* Mocking golden_operator definitions if needed, but dit_engine.c uses constants here */

void test_dit_basic() {
    DITEngineState state;
    dit_engine_init(&state);

    printf("[TEST] Testing Conscious Process (Even Parity)...\n");
    DITPacket p1 = { .content = 10.0, .parity = 2 };
    double res1;
    assert(dit_conscious_process(&state, p1, &res1) == true);
    assert(res1 == 10.0);

    printf("[TEST] Testing Subconscious Queuing (Odd Parity)...\n");
    DITPacket p2 = { .content = 5.0, .parity = 1 };
    double res2;
    assert(dit_conscious_process(&state, p2, &res2) == false);
    assert(state.subconscious_queue.head == 1);

    printf("[TEST] Testing Subconscious Step (Pattern Resolution)...\n");
    dit_subconscious_step(&state);
    assert(state.updates_received == 1);
    assert(state.subconscious_queue.tail == 1);

    printf("[TEST] Testing Intuition Upgrade...\n");
    double upgrade;
    assert(dit_get_upgrade(&state, &upgrade) == true);
    printf("[TEST] Upgrade received: %f\n", upgrade);

    printf("[SUCCESS] DIT Engine logic verified.\n");
}

int main() {
    test_dit_basic();
    return 0;
}
