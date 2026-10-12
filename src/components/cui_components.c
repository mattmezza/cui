/* SPDX-License-Identifier: MIT */
#include "cui/cui_components.h"
#include <math.h>
static cui_box cui_components_control(const cui_theme *t, bool disabled, bool selected) {
    return (cui_box){.height = t->control_height,
                     .padding = t->padding,
                     .gap = t->gap,
                     .radius = t->radius,
                     .row = true,
                     .background = selected && !disabled ? t->primary : t->surface,
                     .border = disabled ? t->muted : t->border,
                     .border_width = 1};
}
static void cui_components_focus(cui_context *ui, cui_id id, const cui_theme *t, cui_box *b) {
    if (cui_focused(ui) == id) {
        b->border = t->focus;
        b->border_width = 2;
    }
}
static void cui_components_mark(cui_context *ui, const cui_theme *t, const char *text,
                                bool selected, bool disabled) {
    cui_text(ui, cui_string(text), t->font_size,
             disabled   ? t->muted
             : selected ? t->on_primary
                        : t->text);
}
bool cui_button_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                   cui_button_options options) {
    if (!ui || !t)
        return false;
    bool disabled = options.disabled || options.loading;
    cui_interaction state = cui_base_button(ui, id, name, disabled);
    cui_box b = cui_components_control(t, disabled, false);
    float scale = options.size == CUI_CONTROL_SMALL   ? .8f
                  : options.size == CUI_CONTROL_LARGE ? 1.2f
                                                      : 1;
    b.height *= scale;
    b.padding *= scale;
    b.width = isfinite(options.width) ? options.width : 0;
    b.align_x = CUI_ALIGN_CENTER;
    b.align_y = CUI_ALIGN_CENTER;
    cui_color foreground = t->text;
    switch (options.variant) {
    case CUI_BUTTON_SECONDARY:
        b.background = t->border;
        break;
    case CUI_BUTTON_OUTLINE:
        b.background.a = 0;
        break;
    case CUI_BUTTON_GHOST:
    case CUI_BUTTON_LINK:
        b.background.a = 0;
        b.border_width = 0;
        if (options.variant == CUI_BUTTON_LINK)
            foreground = t->primary;
        break;
    case CUI_BUTTON_DANGER:
        b.background = t->danger;
        foreground = t->background;
        break;
    default:
        b.background = t->primary;
        foreground = t->on_primary;
        break;
    }
    if (options.selected && options.variant != CUI_BUTTON_DANGER) {
        b.background = t->primary;
        foreground = t->on_primary;
    }
    if (!disabled && (state.hovered || state.pressed)) {
        if (b.background.a == 0)
            b.background = t->surface;
        float factor = state.pressed ? .75f : .9f;
        b.background.r = (uint8_t)(b.background.r * factor);
        b.background.g = (uint8_t)(b.background.g * factor);
        b.background.b = (uint8_t)(b.background.b * factor);
    }
    if (disabled && b.background.a)
        b.background = t->surface;
    cui_components_focus(ui, id, t, &b);
    cui_update_semantic(ui, id, options.selected, options.selected);
    if (options.loading)
        cui_set_semantic_details(
            ui, id, (cui_semantic_details){.description = cui_string("Loading"), .busy = true});
    cui_begin_box(ui, id, b);
    foreground = disabled ? t->muted : foreground;
    if (options.loading)
        cui_text(ui, cui_string("..."), t->font_size * scale, foreground);
    else if (options.leading.length)
        cui_text(ui, options.leading, t->font_size * scale, foreground);
    if (options.content)
        options.content(ui, options.content_user);
    else
        cui_text(ui, name, t->font_size * scale, foreground);
    if (options.trailing.length)
        cui_text(ui, options.trailing, t->font_size * scale, foreground);
    cui_end_box(ui);
    return state.activated;
}
bool cui_button(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, bool disabled) {
    return cui_button_ex(ui, id, t, name, (cui_button_options){.disabled = disabled});
}
static bool cui_components_binary_control(cui_context *ui, cui_id id, const cui_theme *t,
                                          cui_str name, bool *value, bool disabled, int kind) {
    if (!value)
        return false;
    bool changed = kind == 0   ? cui_base_checkbox(ui, id, name, value, disabled)
                   : kind == 1 ? cui_base_switch(ui, id, name, value, disabled)
                               : cui_base_toggle(ui, id, name, value, disabled);
    cui_box b = cui_components_control(t, disabled, kind == 2 && *value);
    cui_components_focus(ui, id, t, &b);
    cui_begin_box(ui, id, b);
    const char *indicator = kind == 0   ? (*value ? "[x]" : "[ ]")
                            : kind == 1 ? (*value ? "ON" : "OFF")
                                        : (*value ? "+" : "-");
    cui_components_mark(ui, t, indicator, kind == 2 && *value, disabled);
    cui_text(ui, name, t->font_size,
             disabled              ? t->muted
             : kind == 2 && *value ? t->on_primary
                                   : t->text);
    cui_end_box(ui);
    return changed;
}
bool cui_checkbox(cui_context *u, cui_id i, const cui_theme *t, cui_str n, bool *v, bool d) {
    return cui_components_binary_control(u, i, t, n, v, d, 0);
}
bool cui_switch(cui_context *u, cui_id i, const cui_theme *t, cui_str n, bool *v, bool d) {
    return cui_components_binary_control(u, i, t, n, v, d, 1);
}
bool cui_toggle(cui_context *u, cui_id i, const cui_theme *t, cui_str n, bool *v, bool d) {
    return cui_components_binary_control(u, i, t, n, v, d, 2);
}
bool cui_radio(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, int *value, int option,
               bool disabled) {
    if (!value)
        return false;
    bool changed = cui_base_radio(ui, id, name, value, option, disabled);
    cui_box b = cui_components_control(t, disabled, false);
    cui_components_focus(ui, id, t, &b);
    cui_begin_box(ui, id, b);
    cui_components_mark(ui, t, *value == option ? "(*)" : "( )", false, disabled);
    cui_text(ui, name, t->font_size, disabled ? t->muted : t->text);
    cui_end_box(ui);
    return changed;
}
void cui_label(cui_context *ui, const cui_theme *t, cui_str label) {
    cui_text(ui, label, t->font_size, t->text);
}
void cui_badge(cui_context *ui, cui_id id, const cui_theme *t, cui_str label) {
    cui_begin_box(ui, id, (cui_box){.padding = 4, .radius = t->radius, .background = t->primary});
    cui_text(ui, label, t->font_size * .8f, t->on_primary);
    cui_end_box(ui);
}
void cui_progress(cui_context *ui, cui_id id, const cui_theme *t, float fraction, float width) {
    if (!isfinite(fraction))
        fraction = 0;
    fraction = fraction < 0 ? 0 : fraction > 1 ? 1 : fraction;
    if (width <= 0 || !isfinite(width))
        width = 160;
    cui_begin_box(ui, id,
                  (cui_box){.width = width,
                            .height = 8,
                            .radius = t->radius,
                            .background = t->border,
                            .row = true});
    if (fraction > 0) {
        cui_id fill = id ^ UINT64_C(0x7129d4cd19342f11);
        cui_begin_box(ui, fill,
                      (cui_box){.width = width * fraction,
                                .height = 8,
                                .radius = t->radius,
                                .background = t->primary});
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
bool cui_slider(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, float *value,
                float min, float max, float step, bool disabled) {
    cui_box b = cui_components_control(t, disabled, false);
    b.width = 260;
    b.row = false;
    b.height = 0;
    cui_components_focus(ui, id, t, &b);
    cui_begin_box(ui, id, b);
    bool changed = cui_base_slider(ui, id, name, value, min, max, step, disabled);
    cui_text(ui, name, t->font_size, disabled ? t->muted : t->text);
    float ratio = value && isfinite(*value) && max > min ? (*value - min) / (max - min) : 0;
    cui_progress(ui, id ^ UINT64_C(0x39c1a2170384edaa), t, ratio, 240);
    cui_end_box(ui);
    return changed;
}
void cui_separator(cui_context *ui, cui_id id, const cui_theme *t, float width) {
    cui_begin_box(
        ui, id, (cui_box){.width = width != 0 ? width : 160, .height = 1, .background = t->border});
    cui_end_box(ui);
}

static void cui_components_input_span(cui_context *ui, cui_str text, const cui_theme *t,
                                      bool selected, bool disabled) {
    if (!text.length)
        return;
    cui_size extent = cui_measure_text(ui, text, t->font_size);
    cui_begin_box(ui, 0,
                  (cui_box){.width = extent.width,
                            .height = extent.height,
                            .background = selected ? t->primary : (cui_color){0}});
    cui_text(ui, text, t->font_size, disabled ? t->muted : selected ? t->on_primary : t->text);
    cui_end_box(ui);
}
static void cui_components_input_caret(cui_context *ui, const cui_theme *t, float height) {
    cui_begin_box(ui, 0, (cui_box){.width = 1, .height = height, .background = t->text});
    cui_end_box(ui);
}
cui_id cui_input_viewport_id(cui_id id) {
    return id ^ UINT64_C(0xb1642b97583fbd63);
}
void cui_input_paint(cui_context *ui, cui_id id, const cui_theme *t, cui_str name, cui_edit *edit,
                     cui_input_state *state, cui_input_options options) {
    if (!ui || !id || !t || !edit || !state)
        return;
    cui_id viewport_id = cui_input_viewport_id(id);
    cui_input_hit_geometry hit = {ui, cui_bounds(ui, viewport_id), t->font_size, state->scroll_x};
    bool multiline = cui_edit_get_state(edit).multiline;
    cui_box box = cui_components_control(t, options.disabled, false);
    box.width = options.width ? options.width : 280;
    box.height = 0;
    box.row = false;
    box.clip = true;
    if (options.invalid)
        box.border = t->danger;
    else
        cui_components_focus(ui, id, t, &box);
    cui_begin_box(ui, id, box);
    cui_text(ui, name, t->font_size * .8f, t->muted);
    cui_str text = cui_edit_get_text(edit);
    cui_edit_state selection = cui_edit_get_state(edit);
    bool focused = cui_focused(ui) == id && !options.disabled;
    if (multiline) {
        cui_text(ui, text.length ? text : cui_string(" "), t->font_size,
                 options.disabled ? t->muted : t->text);
    } else {
        float height = cui_measure_text(ui, cui_string("M"), t->font_size).height;
        float viewport_width = hit.viewport.w;
        if (box.width > 0)
            viewport_width = fmaxf(1, box.width - 2 * box.padding);
        float caret =
            cui_measure_text(ui, (cui_str){text.data, selection.cursor}, t->font_size).width;
        float total = cui_measure_text(ui, text, t->font_size).width + (focused ? 1 : 0);
        if (!isfinite(state->scroll_x) || state->scroll_x < 0)
            state->scroll_x = 0;
        if (focused && viewport_width > 0) {
            if (caret < state->scroll_x)
                state->scroll_x = caret;
            if (caret + 1 > state->scroll_x + viewport_width)
                state->scroll_x = caret + 1 - viewport_width;
        }
        state->scroll_x = fminf(state->scroll_x, fmaxf(0, total - viewport_width));
        cui_begin_box(ui, viewport_id,
                      (cui_box){.width = -1,
                                .height = height,
                                .row = true,
                                .clip = true,
                                .offset_x = -state->scroll_x});
        if (!text.length && options.placeholder.length) {
            if (focused)
                cui_components_input_caret(ui, t, height);
            cui_text(ui, options.placeholder, t->font_size, t->muted);
        } else {
            size_t first =
                selection.anchor < selection.cursor ? selection.anchor : selection.cursor;
            size_t last = selection.anchor > selection.cursor ? selection.anchor : selection.cursor;
            cui_components_input_span(ui, (cui_str){text.data, first}, t, false, options.disabled);
            if (focused && selection.cursor == first)
                cui_components_input_caret(ui, t, height);
            cui_components_input_span(ui, (cui_str){text.data + first, last - first}, t, focused,
                                      options.disabled);
            if (focused && selection.cursor != first)
                cui_components_input_caret(ui, t, height);
            cui_components_input_span(ui, (cui_str){text.data + last, text.length - last}, t, false,
                                      options.disabled);
        }
        cui_end_box(ui);
    }
    cui_end_box(ui);
}
cui_input_result cui_input_ex(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                              cui_edit *edit, cui_input_state *state, cui_input_options options) {
    if (!ui || !t || !edit || !state)
        return (cui_input_result){.error = CUI_EDIT_INVALID_ARGUMENT};
    cui_id viewport_id = cui_input_viewport_id(id);
    cui_input_hit_geometry hit = {ui, cui_bounds(ui, viewport_id), t->font_size, state->scroll_x};
    bool multiline = cui_edit_get_state(edit).multiline;
    cui_input_result result = cui_base_input_ex(
        ui, id, name, edit, &state->behavior,
        (cui_base_input_options){.disabled = options.disabled,
                                 .readonly = options.readonly,
                                 .required = options.required,
                                 .invalid = options.invalid,
                                 .clipboard = options.clipboard,
                                 .hit_test = multiline ? NULL : cui_base_input_hit_test,
                                 .hit_test_user = &hit});
    options.invalid = options.invalid || result.error != CUI_EDIT_OK;
    cui_input_paint(ui, id, t, name, edit, state, options);
    return result;
}

cui_input_result cui_input(cui_context *ui, cui_id id, const cui_theme *t, cui_str name,
                           cui_edit *edit, bool disabled) {
    cui_input_state state = {0};
    return cui_input_ex(ui, id, t, name, edit, &state, (cui_input_options){.disabled = disabled});
}
