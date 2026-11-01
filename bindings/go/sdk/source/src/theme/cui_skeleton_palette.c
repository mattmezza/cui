/* SPDX-License-Identifier: MIT */
#include "cui/cui_skeleton_palette.h"
#include "cui_skeleton_palette_presets.inc"
bool cui_skeleton_palette_from_theme(const cui_theme *theme, cui_skeleton_palette *out) {
    if (!theme || !out)
        return false;
    cui_skeleton_palette value = {theme->secondary};
    *out = value;
    return true;
}
bool cui_skeleton_palette_preset_get(size_t index, cui_skeleton_palette *out) {
    if (!out ||
        index >= sizeof(cui_skeleton_palette_records) / sizeof *cui_skeleton_palette_records)
        return false;
    *out = cui_skeleton_palette_records[index];
    return true;
}
