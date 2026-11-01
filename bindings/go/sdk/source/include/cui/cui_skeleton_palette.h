/* SPDX-License-Identifier: MIT */
#ifndef CUI_SKELETON_PALETTE_H
#define CUI_SKELETON_PALETTE_H
#include "cui_theme.h"
#ifdef __cplusplus
extern "C" {
#endif
/* Owned encoded RGBA8 paint value. Every channel combination is valid; alpha0
 * preserves hidden RGB and is never an unset/fallback sentinel. No allocation. */
typedef struct {
    cui_color background;
} cui_skeleton_palette;
/* Copies theme.secondary exactly, including custom alpha0; no preset inference.
 * Pure functions: null/invalid index fails with output untouched, no diagnostic.
 * All four builtins use this same explicit theme fallback adaptation. */
bool cui_skeleton_palette_from_theme(const cui_theme *, cui_skeleton_palette *out);
/* Existing cui_theme_preset_count/name owns the index order. Exact pinned
 * skeleton.background or source secondary.background fallback, encoded bytes;
 * not source shader/Hsla compositing identity. Output owns its entire value. */
bool cui_skeleton_palette_preset_get(size_t index, cui_skeleton_palette *out);
#ifdef __cplusplus
}
#endif
#endif
