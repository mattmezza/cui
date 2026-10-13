/* SPDX-License-Identifier: MIT */
#include "host.h"
#include <stdio.h>
typedef struct {
    int count, sequence;
} observations;
static void changed(void *user, int count) {
    observations *seen = user;
    ++seen->count;
    seen->sequence = seen->sequence * 10 + count;
}
int main(void) {
    for (int i = 0; i < 2; ++i) {
        observations seen = {0};
        int result = cui_example_run(NULL, 0, changed, &seen);
        if (result || seen.count != 2 || seen.sequence != 12) {
            fprintf(stderr, "host test: status=%d callbacks=%d sequence=%d\n", result, seen.count,
                    seen.sequence);
            return 1;
        }
    }
    /* A callback is optional; each run owns independent CUI state. */
    return cui_example_run(NULL, 0, NULL, NULL);
}
