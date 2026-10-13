/* SPDX-License-Identifier: MIT */
#ifndef CUI_EXAMPLE_HOST_H
#define CUI_EXAMPLE_HOST_H
/* Example ABI: callback is synchronous on the calling thread, never retained. */
typedef void (*cui_example_changed)(void *user, int count);
/* font == NULL: deterministic headless interaction test. Otherwise SDL3 window.
 * frames == 0: run until close; positive values bound native smoke tests. */
int cui_example_run(const char *font, unsigned frames, cui_example_changed callback, void *user);
#endif
