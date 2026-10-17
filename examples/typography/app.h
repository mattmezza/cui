/* SPDX-License-Identifier: MIT */
#ifndef TYPOGRAPHY_APP_H
#define TYPOGRAPHY_APP_H
#include "cui/cui_typography.h"
enum { TYPOGRAPHY_BODY_FAMILY = 11, TYPOGRAPHY_HEADING_FAMILY = 12, TYPOGRAPHY_CODE_FAMILY = 13 };
cui_typography_roles typography_example_roles(void);
cui_draw_list typography_example_frame(cui_context *, const cui_typography_roles *, float, float);
#ifdef CUI_TYPOGRAPHY_NATIVE
#include "cui/cui_text.h"
/* Caller supplies actual files. Returns a new owner; NULL on any failure.
 * Same file may be reused by the caller, but distinct files demonstrate families. */
cui_font *typography_example_font(const char *body, const char *heading,
                                   const char *heading_bold, const char *body_italic,
                                   const char *code);
#endif
#endif
