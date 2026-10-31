/* SPDX-License-Identifier: MIT */
#include "../../src/components/cui_spring_internal.h"
#include "../fixtures/slider_switch_legacy_kernel.inc"
#include "cui/cui_components.h"
#include <assert.h>
#include <float.h>
#include <stdio.h>
#include <string.h>
static void equal(double a, double b) {
    assert(memcmp(&a, &b, sizeof a) == 0);
}
int main(void) {
    const double clocks[] = {0, .001, .01, .1, .1, .05, .5, 1, 3, 100, 1e100};
    const float damping[] = {0, .1f, .5f, 1, 2, 100, FLT_MAX};
    const float response[] = {0, .001f, .18f, 1, FLT_MIN, FLT_MAX};
    unsigned count = 0;
    for (unsigned z = 0; z < 7; ++z)
        for (unsigned r = 0; r < 6; ++r)
            for (unsigned travel = 0; travel < 2; ++travel) {
                cui_switch_state legacy = {0};
                cui_spring_sample_internal current = {0};
                cui_spring_settings policy = {response[r], damping[z], .001f, travel != 0};
                for (unsigned t = 0; t < 11; ++t) {
                    double target = t % 3 == 0 ? 0 : 1;
                    bool reduced = t == 8;
                    cui_legacy_switch_spring_step(&legacy, target, policy, clocks[t], reduced);
                    cui_spring_step_internal(&current, target, policy, clocks[t], reduced);
                    equal(legacy.position, current.position);
                    equal(legacy.velocity, current.velocity);
                    equal(legacy.target, current.target);
                    equal(legacy.updated_at, current.updated_at);
                    equal(legacy.next_wake_seconds, current.next_wake_seconds);
                    assert(legacy.initialized == current.initialized);
                    ++count;
                }
            }
    printf("PASS %u independent frozen-old-source spring samples (all fields bitwise)\n", count);
    return 0;
}
