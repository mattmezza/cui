/* SPDX-License-Identifier: MIT */
/* Standalone TEST ONLY: no bridge/Core API or implementation is included. */
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
/* Proposed PRIVATE production policy; this file remains test-only. */
#if defined(CUI_BRIDGE_SLIDER_NO_TLS)
#define PROBE_OWNER_SUPPORTED 0
#elif defined(__wasm__)
#if defined(CUI_BRIDGE_SLIDER_WASM_UNSHARED) && CUI_BRIDGE_SLIDER_WASM_UNSHARED == 1 &&            \
    !defined(__wasm_atomics__) && !defined(_REENTRANT)
#define PROBE_OWNER_SUPPORTED 1
#else
#define PROBE_OWNER_SUPPORTED 0
#endif
#else
#define PROBE_OWNER_SUPPORTED 1
#endif
#if defined(PROBE_EXPECT_SUPPORT)
_Static_assert(PROBE_OWNER_SUPPORTED == PROBE_EXPECT_SUPPORT, "owner policy mismatch");
#endif
uint32_t probe_supported(void) {
    return PROBE_OWNER_SUPPORTED;
}
static _Thread_local unsigned char owner_tag;
static atomic_flag epoch_gate = ATOMIC_FLAG_INIT;
static uint64_t next_epoch = 1;
static bool epoch_exhausted;
static void gate_lock(void) {
    while (atomic_flag_test_and_set_explicit(&epoch_gate, memory_order_acquire)) {
    }
}
static void gate_unlock(void) {
    atomic_flag_clear_explicit(&epoch_gate, memory_order_release);
}
uintptr_t probe_owner(void) {
    return (uintptr_t)&owner_tag;
}
uint32_t probe_matches(uintptr_t owner) {
    return owner == probe_owner();
}
static bool issue(uint64_t *out) {
    gate_lock();
    if (epoch_exhausted) {
        gate_unlock();
        return false;
    }
    const uint64_t value = next_epoch;
    if (value == UINT64_MAX)
        epoch_exhausted = true;
    else
        next_epoch = value + 1;
    gate_unlock();
    *out = value;
    return true;
}
uint64_t probe_epoch(void) {
    uint64_t value = 0;
    (void)issue(&value);
    return value;
}
uint64_t probe_feature_epoch(void) {
#if PROBE_OWNER_SUPPORTED
    return probe_epoch();
#else
    return 0; /* Unsupported feature never allocates/uses owner/epoch primitives. */
#endif
}
/* Test-only epoch seeding, never an exported production bridge feature. */
void probe_seed_near_end(void) {
    gate_lock();
    next_epoch = UINT64_MAX - 1;
    epoch_exhausted = false;
    gate_unlock();
}
#if defined(PROBE_NATIVE)
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
enum { WORKERS = 4, ISSUES = 1000 };
typedef struct {
    uintptr_t tag;
    uint64_t values[ISSUES];
} worker_result;
static worker_result results[WORKERS];
static atomic_uint arrived, released;
static int worker(void *opaque) {
    worker_result *r = opaque;
    r->tag = probe_owner();
    assert(probe_matches(r->tag));
    for (unsigned i = 0; i < ISSUES; ++i) {
        assert(issue(&r->values[i]));
        assert(probe_owner() == r->tag);
    }
    atomic_fetch_add_explicit(&arrived, 1, memory_order_release);
    while (!atomic_load_explicit(&released, memory_order_acquire))
        thrd_yield();
    assert(probe_owner() == r->tag);
    return 0;
}
int main(void) {
    assert(probe_supported() == 1);
    const uintptr_t main_tag = probe_owner();
    thrd_t threads[WORKERS];
    bool seen[WORKERS * ISSUES + 1] = {false};
    for (unsigned i = 0; i < WORKERS; ++i)
        assert(thrd_create(&threads[i], worker, &results[i]) == thrd_success);
    while (atomic_load_explicit(&arrived, memory_order_acquire) != WORKERS)
        thrd_yield();
    for (unsigned i = 0; i < WORKERS; ++i) {
        assert(results[i].tag != main_tag && !probe_matches(results[i].tag));
        for (unsigned j = 0; j < i; ++j)
            assert(results[i].tag != results[j].tag);
        for (unsigned j = 0; j < ISSUES; ++j) {
            uint64_t value = results[i].values[j];
            assert(value > 0 && value <= WORKERS * ISSUES && !seen[value]);
            seen[value] = true;
        }
    }
    atomic_store_explicit(&released, 1, memory_order_release);
    for (unsigned i = 0; i < WORKERS; ++i) {
        int code;
        assert(thrd_join(threads[i], &code) == thrd_success && !code);
    }
    assert(probe_matches(main_tag));
    probe_seed_near_end();
    uint64_t out = 0;
    assert(issue(&out) && out == UINT64_MAX - 1);
    assert(issue(&out) && out == UINT64_MAX);
    out = 173;
    assert(!issue(&out) && out == 173);
    puts("PASS: 4 simultaneously live distinct TLS owners; 4000 unique epochs; saturation atomic");
    return EXIT_SUCCESS;
}
#endif
