/* SPDX-License-Identifier: MIT */
/* Include the private adapter to exercise real write/progress callbacks without
 * ever invoking a network transfer. No production test hooks are exported. */
#include "curl_transport.c"
#include <assert.h>
static bool flag(void *user) {
    return *(bool *)user;
}
int main(void) {
    af_result r = {0};
    bool cancelled = false;
    transfer t = {&r, flag, &cancelled};
    unsigned char bytes[AF_BODY_MAX];
    memset(bytes, 0x81, sizeof bytes);
    bytes[1] = 0;
    assert(receive((char *)bytes, 1, sizeof bytes, &t) == sizeof bytes);
    assert(r.size == AF_BODY_MAX && !memcmp(r.body, bytes, sizeof bytes));
    assert(receive("x", 1, 1, &t) == 0 && r.status == AF_TOO_LARGE && r.size == AF_BODY_MAX);
    r = (af_result){0};
    assert(receive("x", SIZE_MAX, 2, &t) == 0 && r.status == AF_TOO_LARGE && r.size == 0);
    assert(progress(&t, 0, 0, 0, 0) == 0);
    cancelled = true;
    assert(progress(&t, 0, 0, 0, 0) == 1);
    if (!af_curl_initialize())
        return 77;
    r = (af_result){0};
    /* Cancellation is checked before easy_perform, DNS or any socket access. */
    af_curl_transport(NULL, "https://never-contact.invalid/", flag, &cancelled, &r);
    assert(r.status == AF_CANCELLED && r.size == 0);
    af_curl_shutdown();
    puts("PASS libcurl adapter: binary body bounds, overflow, progress and pre-transfer "
         "cancellation (offline)");
    return 0;
}
