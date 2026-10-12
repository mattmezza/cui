/* SPDX-License-Identifier: MIT */
#include "cui/cui_icon.h"
#include <math.h>
#include <string.h>
cui_icon_error cui_icon(cui_context *ui, cui_id id, const cui_theme *theme, cui_icon_state *state,
                        cui_icon_options options) {
    if (!ui || !id || !theme || !state || options.size < CUI_ICON_INHERITED ||
        options.size > CUI_ICON_CUSTOM)
        return CUI_ICON_INVALID;
    float size = options.size == CUI_ICON_XSMALL   ? 12
                 : options.size == CUI_ICON_SMALL  ? 14
                 : options.size == CUI_ICON_MEDIUM ? 16
                 : options.size == CUI_ICON_LARGE  ? 24
                 : options.size == CUI_ICON_CUSTOM ? options.custom_size
                 : options.inherited_text_size > 0 ? options.inherited_text_size
                                                   : theme->font_size;
    if (options.size >= CUI_ICON_XSMALL && options.size <= CUI_ICON_LARGE &&
        isfinite(theme->font_size) && theme->font_size > 0)
        size *= theme->font_size / 16;
    bool styled = options.custom_width || options.custom_height;
    float width = options.size != CUI_ICON_INHERITED || !styled ? size
                  : options.custom_width                        ? options.width
                                                                : 0;
    float height = options.size != CUI_ICON_INHERITED || !styled ? size
                   : options.custom_height                       ? options.height
                                                                 : 0;
    if (!isfinite(width) || !isfinite(height) || width < 0 || height < 0 || width > 4096 ||
        height > 4096)
        return CUI_ICON_INVALID;
    cui_color color = options.custom_color             ? options.color
                      : options.custom_inherited_color ? options.inherited_color
                                                       : theme->text;
    cui_icon_error result = cui_icon_prepare(
        state, width, height, options.custom_transform ? options.transform : cui_icon_identity());
    cui_begin_box(
        ui, id,
        (cui_box){.width = width, .height = height, .fixed_width = true, .fixed_height = true});
    cui_bitmap bitmap = cui_icon_state_bitmap(state);
    if (result == CUI_ICON_OK && cui_bitmap_valid(bitmap)) {
        cui_rect paint = cui_icon_state_paint_bounds(state);
        cui_begin_absolute_box(ui, id ^ UINT64_C(0xbed8b36147394a2c), (cui_box){0}, paint);
        float opacity = options.custom_opacity ? options.opacity : 1;
        if (!isfinite(opacity))
            opacity = 0;
        opacity = fminf(1, fmaxf(0, opacity));
        cui_paint_bitmap(
            ui, id ^ UINT64_C(0x5c136a8492ef704d),
            (cui_bitmap_paint){
                .bitmap = bitmap, .opacity = opacity, .custom_tint = true, .tint = color},
            (cui_box){
                .width = paint.w, .height = paint.h, .fixed_width = true, .fixed_height = true});
        cui_end_box(ui);
    }
    cui_end_box(ui);
    return result;
}

typedef struct {
    cui_icon_state state;
    size_t catalog_index;
    double started;
    bool initialized;
} cui_icon_resource;
static void cui_icon_resource_collect(void *payload) {
    cui_icon_state_collect(&((cui_icon_resource *)payload)->state);
}
static void cui_icon_resource_destroy(void *payload) {
    cui_icon_state_destroy(&((cui_icon_resource *)payload)->state);
}
static cui_icon_resource *cui_icon_resource_get(cui_context *ui, cui_id id, cui_id namespace_id) {
    cui_icon_resource *resource =
        cui_resource_get(ui, id, namespace_id, sizeof(*resource), cui_icon_resource_collect,
                         cui_icon_resource_destroy);
    if (resource && !resource->initialized) {
        cui_icon_state_init(&resource->state);
        if (!cui_icon_set_limits(&resource->state, (cui_icon_limits){.max_cache_bytes = 1024 * 1024,
                                                                     .max_versions = 4}))
            return NULL;
        resource->catalog_index = SIZE_MAX;
        resource->started = cui_frame_time(ui);
        resource->initialized = true;
    }
    return resource;
}
static bool cui_icon_name_equal(cui_str first, cui_str second) {
    return first.length == second.length && first.data && second.data &&
           !memcmp(first.data, second.data, first.length);
}
cui_icon_error cui_icon_named(cui_context *ui, cui_id id, const cui_theme *theme, cui_str name,
                              cui_icon_options options) {
    if (!ui || !id || !theme || !name.data || !name.length)
        return CUI_ICON_INVALID;
    size_t index = SIZE_MAX;
    for (size_t i = 0; i < cui_icon_catalog_count(); i++)
        if (cui_icon_name_equal(name, cui_icon_catalog_name(i)) ||
            cui_icon_name_equal(name, cui_icon_catalog_path(i))) {
            index = i;
            break;
        }
    if (index == SIZE_MAX)
        return CUI_ICON_NOT_FOUND;
    cui_icon_resource *resource = cui_icon_resource_get(ui, id, UINT64_C(0xe8e2ea151d9d14b4));
    if (!resource)
        return CUI_ICON_ALLOCATION;
    if (resource->catalog_index != index) {
        if (!cui_icon_set_name(&resource->state, cui_icon_catalog_name(index), NULL, NULL))
            return resource->state.error;
        resource->catalog_index = index;
    }
    return cui_icon(ui, id, theme, &resource->state, options);
}
cui_icon_error cui_icon_spinner(cui_context *ui, cui_id id, const cui_theme *theme,
                                cui_icon_options options) {
    if (!ui || !id || !theme)
        return CUI_ICON_INVALID;
    cui_icon_resource *resource = cui_icon_resource_get(ui, id, UINT64_C(0x81f69ad695af247d));
    if (!resource)
        return CUI_ICON_ALLOCATION;
    if (resource->catalog_index == SIZE_MAX) {
        if (!cui_icon_set_name(&resource->state, cui_string("loader"), NULL, NULL))
            return resource->state.error;
        resource->catalog_index = 0;
    }
    float phase =
        cui_reduced_motion(ui) ? 0 : (float)fmod((cui_frame_time(ui) - resource->started) / .8, 1);
    float eased = phase < .5f ? 2 * phase * phase : 1 - (-2 * phase + 2) * (-2 * phase + 2) / 2;
    options.custom_transform = true;
    options.transform = cui_icon_rotation(eased * 6.2831853071795864769f);
    if (options.size == CUI_ICON_INHERITED)
        options.size = CUI_ICON_MEDIUM;
    cui_icon_error error = cui_icon(ui, id, theme, &resource->state, options);
    if (error == CUI_ICON_OK && !cui_reduced_motion(ui))
        cui_invalidate(ui);
    return error;
}
