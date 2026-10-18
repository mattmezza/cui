/* SPDX-License-Identifier: MIT */
#include "cui/cui_questionnaire.h"
#include "../base/cui_design_metrics.h"
#include "cui/cui_components.h"
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct {
    cui_questionnaire_slot_style styles[3];
    cui_questionnaire_labels labels;
} cui_questionnaire_options_snapshot;
static bool cui_questionnaire_length(float value, bool dimension) {
    return isfinite(value) && ((dimension && value == -1) || (value >= 0 && value <= 4096));
}
static bool cui_questionnaire_style_valid(cui_questionnaire_slot_style style) {
    uint32_t f = style.fields;
    cui_box b = style.box;
    return !(f & ~UINT32_C(511)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_WIDTH) || cui_questionnaire_length(b.width, true)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_HEIGHT) || cui_questionnaire_length(b.height, true)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_PADDING) || cui_questionnaire_length(b.padding, false)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_GAP) || cui_questionnaire_length(b.gap, false)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_RADIUS) || cui_questionnaire_length(b.radius, false)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_BORDER) ||
            cui_questionnaire_length(b.border_width, false)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_ALIGN) ||
            ((unsigned)b.align_x <= CUI_ALIGN_END && (unsigned)b.align_y <= CUI_ALIGN_END)) &&
           (!(f & CUI_QUESTIONNAIRE_SLOT_OPACITY) ||
            (isfinite(b.opacity) && b.opacity >= 0 && b.opacity <= 1));
}
static bool cui_questionnaire_snapshot(cui_questionnaire_options *o,
                                       cui_questionnaire_options_snapshot *snapshot) {
    if (!cui_design_valid(o->design_size) || !isfinite(o->density) || o->density < 0 ||
        o->density > 32)
        return false;
    const cui_questionnaire_slot_style *styles[] = {o->indicator_style, o->content_style,
                                                    o->shortcut_style};
    for (size_t i = 0; i < 3; ++i)
        if (styles[i]) {
            snapshot->styles[i] = *styles[i];
            if (!cui_questionnaire_style_valid(snapshot->styles[i]))
                return false;
        }
    if (o->labels) {
        snapshot->labels = *o->labels;
        cui_questionnaire_labels l = snapshot->labels;
        cui_str values[] = {l.previous, l.skip, l.next, l.submit, l.required, l.unfilled};
        if (l.fields & ~UINT32_C(63))
            return false;
        for (size_t i = 0; i < 6; ++i)
            if ((l.fields & (1u << i)) && !cui_edit_valid_utf8(values[i]))
                return false;
        o->labels = &snapshot->labels;
    }
    if (o->indicator_style)
        o->indicator_style = &snapshot->styles[0];
    if (o->content_style)
        o->content_style = &snapshot->styles[1];
    if (o->shortcut_style)
        o->shortcut_style = &snapshot->styles[2];
    return true;
}
static cui_box cui_questionnaire_refine(cui_box b, const cui_questionnaire_slot_style *style) {
    if (!style)
        return b;
    cui_box v = style->box;
    uint32_t f = style->fields;
    if (f & CUI_QUESTIONNAIRE_SLOT_WIDTH) {
        b.width = v.width;
        b.fixed_width = v.width == 0;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_HEIGHT) {
        b.height = v.height;
        b.fixed_height = v.height == 0;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_PADDING)
        b.padding = v.padding;
    if (f & CUI_QUESTIONNAIRE_SLOT_GAP)
        b.gap = v.gap;
    if (f & CUI_QUESTIONNAIRE_SLOT_RADIUS)
        b.radius = v.radius;
    if (f & CUI_QUESTIONNAIRE_SLOT_BACKGROUND)
        b.background = v.background;
    if (f & CUI_QUESTIONNAIRE_SLOT_BORDER) {
        b.border = v.border;
        b.border_width = v.border_width;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_ALIGN) {
        b.align_x = v.align_x;
        b.align_y = v.align_y;
    }
    if (f & CUI_QUESTIONNAIRE_SLOT_OPACITY) {
        b.custom_opacity = true;
        b.opacity = v.opacity;
    }
    return b;
}
typedef struct {
    float root_gap, gap, content_gap, px, py, height, indicator, mark, shortcut, font, line;
} cui_questionnaire_metrics;
static cui_questionnaire_metrics cui_questionnaire_measure(cui_questionnaire_options o) {
    if (o.design_size) {
        cui_design_metrics d;
        if (!cui_design_resolve(o.design_size, o.density > 0 ? o.density : 1, &d))
            return (cui_questionnaire_metrics){0};
        return (cui_questionnaire_metrics){2 * d.gap,
                                           d.gap,
                                           d.gap * .5f,
                                           d.padding_x,
                                           d.padding_y,
                                           d.control_height,
                                           d.indicator_size,
                                           d.indicator_size * .5f,
                                           d.indicator_size + 4,
                                           d.font_size,
                                           d.line_height};
    }
    switch (o.size) {
    case CUI_QUESTIONNAIRE_XSMALL:
        return (cui_questionnaire_metrics){8, 6, 2, 8, 4, 28, 12, 5, 16, 12, 16};
    case CUI_QUESTIONNAIRE_SMALL:
        return (cui_questionnaire_metrics){12, 8, 2, 10, 8, 36, 14, 6, 18, 14, 20};
    case CUI_QUESTIONNAIRE_LARGE:
        return (cui_questionnaire_metrics){24, 12, 4, 16, 12, 48, 18, 10, 24, 18, 28};
    default:
        return (cui_questionnaire_metrics){16, 10, 2, 12, 10, 44, 16, 8, 20, 16, 24};
    }
}
static cui_color cui_questionnaire_opacity(cui_color color, float opacity) {
    color.a = (uint8_t)(color.a * opacity + .5f);
    return color;
}
/* Match the source's bounded per-state scale publication without process globals. */
#define CUI_QUESTIONNAIRE_TRACKED_SIZES 128
typedef struct {
    const cui_questionnaire_state *state;
    cui_questionnaire_size size;
    cui_design_size design_size;
    float density;
} cui_questionnaire_size_entry;
typedef struct {
    size_t count;
    cui_questionnaire_size_entry entries[CUI_QUESTIONNAIRE_TRACKED_SIZES];
} cui_questionnaire_sizes;
static cui_questionnaire_sizes *cui_questionnaire_size_table(cui_context *ui) {
    return cui_resource_get(ui, UINT64_C(0x516e53697a657331), UINT64_C(0x516e53697a657332),
                            sizeof(cui_questionnaire_sizes), NULL, NULL);
}
static bool cui_questionnaire_publish_size(cui_context *ui, const cui_questionnaire_state *s,
                                           cui_questionnaire_options o) {
    cui_questionnaire_sizes *sizes = cui_questionnaire_size_table(ui);
    if (!sizes)
        return false;
    size_t i;
    for (i = 0; i < sizes->count; i++)
        if (sizes->entries[i].state == s)
            break;
    if (i == sizes->count) {
        if (sizes->count == CUI_QUESTIONNAIRE_TRACKED_SIZES)
            sizes->count = 0;
        i = sizes->count++;
    }
    sizes->entries[i] = (cui_questionnaire_size_entry){s, o.size, o.design_size, o.density};
    return true;
}
static void cui_questionnaire_resolve_shortcuts(cui_context *, const cui_questionnaire_state *,
                                                cui_questionnaire_options *);
static cui_questionnaire_options cui_questionnaire_resolve_size(cui_context *ui,
                                                                const cui_questionnaire_state *s,
                                                                cui_questionnaire_options o) {
    cui_questionnaire_resolve_shortcuts(ui, s, &o);

    cui_questionnaire_sizes *sizes = cui_questionnaire_size_table(ui);
    if (sizes)
        for (size_t i = 0; i < sizes->count; i++)
            if (sizes->entries[i].state == s) {
                if (!o.custom_size && o.size == CUI_QUESTIONNAIRE_MEDIUM)
                    o.size = sizes->entries[i].size;
                if (!o.custom_size && !o.design_size)
                    o.design_size = sizes->entries[i].design_size;
                if (!o.density)
                    o.density = sizes->entries[i].density;
                break;
            }
    return o;
}
/* Context resources keep the previous submitted controls available to root capture.
 * Rebuild the list each frame so omitted controls cannot leave dangling bindings. */
typedef struct cui_questionnaire_binding cui_questionnaire_binding;
typedef struct cui_questionnaire_route cui_questionnaire_route;
struct cui_questionnaire_binding {
    cui_questionnaire_binding *next;
    cui_id id;
    size_t item, choice;
    unsigned kind; /* 0 item, 1 choice, 2 input */
    bool disabled;
};
struct cui_questionnaire_route {
    cui_questionnaire_route *parent;
    cui_questionnaire_state *state;
    cui_questionnaire_binding *previous, *current;
    cui_id scope;
    size_t opened_item;
    bool focus_pending, invalid_pending, numeric;
};
typedef struct {
    cui_questionnaire_route *active;
} cui_questionnaire_routes;
static cui_questionnaire_routes *cui_questionnaire_route_stack(cui_context *ui) {
    return cui_resource_get(ui, UINT64_C(0x516e526f75746531), UINT64_C(0x516e526f75746532),
                            sizeof(cui_questionnaire_routes), NULL, NULL);
}
static void cui_questionnaire_resolve_shortcuts(cui_context *ui, const cui_questionnaire_state *s,
                                                cui_questionnaire_options *o) {
    cui_questionnaire_routes *stack = cui_questionnaire_route_stack(ui);
    if (stack && stack->active && stack->active->state == s)
        o->numeric_shortcuts = stack->active->numeric;
}
static void cui_questionnaire_bind(cui_context *ui, cui_id id, const cui_questionnaire_state *s,
                                   size_t item, size_t choice, unsigned kind, bool disabled) {
    cui_questionnaire_routes *stack = cui_questionnaire_route_stack(ui);
    if (!stack || !stack->active || stack->active->state != s)
        return;
    cui_questionnaire_binding *binding = cui_resource_get(
        ui, id, UINT64_C(0x516e42696e64696e), sizeof(cui_questionnaire_binding), NULL, NULL);
    if (!binding)
        return;
    for (cui_questionnaire_binding *b = stack->active->current; b; b = b->next)
        if (b == binding)
            return;
    *binding =
        (cui_questionnaire_binding){stack->active->current, id, item, choice, kind, disabled};
    stack->active->current = binding;
}
static cui_questionnaire_binding *
cui_questionnaire_bound(cui_questionnaire_route *route, size_t item, unsigned kind, size_t choice) {
    for (cui_questionnaire_binding *b = route->previous; b; b = b->next)
        if (!b->disabled && b->item == item && b->kind == kind &&
            (kind != 1 || b->choice == choice))
            return b;
    return NULL;
}
static bool cui_questionnaire_root_key(cui_context *ui, cui_questionnaire_route *r,
                                       const cui_event *e) {
    cui_questionnaire_state *s = r->state;
    if (s->current >= s->count || s->storage.items[s->current].disabled)
        return false;
    size_t item = s->current, count = s->items[item].choice_count;
    const cui_questionnaire_item *d = &s->items[item];
    cui_id focused = cui_focused(ui);
    cui_questionnaire_binding *input = cui_questionnaire_bound(r, item, 2, 0);
    bool input_focused = input && input->id == focused;
    size_t current = SIZE_MAX;
    for (size_t c = 0; c < count; ++c) {
        cui_questionnaire_binding *b = cui_questionnaire_bound(r, item, 1, c);
        if (b && b->id == focused &&
            !s->storage.choices[s->storage.items[item].choice_offset + c].disabled)
            current = c;
    }
    if (e->key == CUI_KEY_ENTER) {
        bool modified = e->modifiers == CUI_MOD_CTRL || e->modifiers == CUI_MOD_META;
        bool filled = input_focused ? cui_questionnaire_freeform(s, item).length != 0
                                    : current != SIZE_MAX &&
                                          cui_questionnaire_choice_selected(s, item, current);
        if (modified || (!e->modifiers && filled)) {
            bool confirmed = cui_questionnaire_confirm(s);
            if (!confirmed && s->current < s->count &&
                cui_questionnaire_item_error(s, s->current).kind != CUI_QUESTIONNAIRE_VALID) {
                r->invalid_pending = true;
                cui_invalidate(ui);
            }
            return confirmed || modified;
        }
        return false;
    }
    if (e->modifiers)
        return false;
    if (e->key == CUI_KEY_UP || e->key == CUI_KEY_DOWN) {
        if (input_focused && cui_edit_get_text(d->input).length)
            return false;
        bool previous = e->key == CUI_KEY_UP;
        size_t slots = count + 1;
        if (input_focused)
            current = count;
        if (current == SIZE_MAX) {
            for (size_t step = 0; step < slots; ++step) {
                size_t next = previous ? slots - 1 - step : step;
                cui_questionnaire_binding *b =
                    cui_questionnaire_bound(r, item, next == count ? 2 : 1, next);
                bool filled = next == count
                                  ? input && cui_questionnaire_freeform(s, item).length != 0
                                  : cui_questionnaire_choice_selected(s, item, next);
                if (b && filled) {
                    cui_focus(ui, b->id);
                    return true;
                }
            }
        }
        size_t start = current == SIZE_MAX ? (previous ? 0 : slots - 1) : current;
        for (size_t step = 1; step <= slots; ++step) {
            size_t next = previous ? (start + slots - step) % slots : (start + step) % slots;
            cui_questionnaire_binding *b =
                cui_questionnaire_bound(r, item, next == count ? 2 : 1, next);
            if (!b || (next < count &&
                       s->storage.choices[s->storage.items[item].choice_offset + next].disabled))
                continue;
            if (next < count && !d->multiple)
                (void)cui_questionnaire_activate(s, item, next);
            cui_focus(ui, b->id);
            return true;
        }
        return false;
    }
    if (input_focused)
        return false;
    if (e->key == CUI_KEY_LEFT || e->key == CUI_KEY_RIGHT) {
        if (!d->multiple && current != SIZE_MAX) {
            for (size_t step = 1; step <= count; ++step) {
                size_t next = e->key == CUI_KEY_LEFT ? (current + count - step) % count
                                                     : (current + step) % count;
                cui_questionnaire_binding *b = cui_questionnaire_bound(r, item, 1, next);
                if (!b || s->storage.choices[s->storage.items[item].choice_offset + next].disabled)
                    continue;
                (void)cui_questionnaire_activate(s, item, next);
                cui_focus(ui, b->id);
                return true;
            }
            return false;
        }
        if (e->key == CUI_KEY_LEFT)
            return cui_questionnaire_previous(s);
        if (cui_questionnaire_item_status(s, item) != CUI_QUESTIONNAIRE_UNANSWERED) {
            bool next = cui_questionnaire_next(s);
            if (!next && cui_questionnaire_item_error(s, item).kind != CUI_QUESTIONNAIRE_VALID) {
                r->invalid_pending = true;
                cui_invalidate(ui);
            }
            return next;
        }
        return false;
    }
    char key = cui_key_character(e->key);
    if (!key)
        return false;
    for (size_t c = 0; c < count; ++c) {
        cui_questionnaire_binding *b = cui_questionnaire_bound(r, item, 1, c);
        if (b && cui_questionnaire_shortcut(s, item, c, r->numeric) == key &&
            cui_questionnaire_choose(s, item, c, cui_frame_time(ui), cui_reduced_motion(ui)) ==
                CUI_QUESTIONNAIRE_OK) {
            if (s->current == item)
                cui_focus(ui, b->id);
            return true;
        }
    }
    return false;
}
static void cui_questionnaire_root_capture(cui_context *ui, cui_questionnaire_route *r,
                                           bool disabled) {
    if (disabled || !cui_focus_within(ui, r->scope))
        return;
    size_t count;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t i = 0; i < count; ++i) {
        /* The input owner must interleave edits and composite keys. A root
         * prepass cannot confirm against a later text event's old answer. */
        cui_questionnaire_binding *input = cui_questionnaire_bound(r, r->state->current, 2, 0);
        if (input && input->id == cui_focused(ui))
            break;
        const cui_event *e = &events[i];
        if (cui_event_consumed(ui, i) || e->type != CUI_EVENT_KEY_DOWN || e->repeat ||
            e->ime_composing || e->prefer_character_input)
            continue;
        size_t before = r->state->current;
        if (cui_questionnaire_root_key(ui, r, e)) {
            (void)cui_consume_event(ui, i);
            if (e->key == CUI_KEY_ENTER)
                (void)cui_cancel_event_activation(ui, i);
            cui_invalidate(ui);
        }
        if (r->state->current != before)
            r->focus_pending = true;
    }
}
bool cui_questionnaire_begin(cui_context *ui, cui_id id, const cui_theme *theme,
                             cui_questionnaire_state *s, cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return false;
    if (!ui || !id || !theme || !s || !s->initialized)
        return false;
    if (o.design_size && !o.density)
        o.density = theme->font_size / 16;
    cui_design_metrics valid_metrics;
    if (o.design_size && !cui_design_resolve(o.design_size, o.density, &valid_metrics))
        return false;
    if (!cui_questionnaire_publish_size(ui, s, o))
        return false;
    cui_questionnaire_routes *stack = cui_questionnaire_route_stack(ui);
    cui_questionnaire_route *route = cui_resource_get(ui, id, UINT64_C(0x516e526f6f744b65),
                                                      sizeof(cui_questionnaire_route), NULL, NULL);
    if (!stack || !route)
        return false;
    if (route->state != s) {
        memset(route, 0, sizeof *route);
        route->state = s;
    }
    route->scope = id;
    route->numeric = o.numeric_shortcuts;
    route->opened_item = s->current;
    cui_questionnaire_root_capture(ui, route, o.disabled);
    cui_questionnaire_metrics m = cui_questionnaire_measure(o);
    size_t before_tick = s->current;
    (void)cui_questionnaire_tick(s, cui_frame_time(ui));
    if (s->current != before_tick)
        route->focus_pending = true;
    if (s->pending_confirm)
        cui_invalidate(ui);
    if (!cui_begin_focus_scope(ui, id, false))
        return false;
    route->parent = stack->active;
    route->current = NULL;
    stack->active = route;
    cui_interact(ui, id, CUI_ROLE_GROUP, (cui_str){0}, o.disabled, false, 0);
    cui_set_semantic_details(ui, id, (cui_semantic_details){.focusable = true});
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id, (cui_box){.width = o.width > 0 ? o.width : -1, .gap = m.root_gap});
    return true;
}
void cui_questionnaire_end(cui_context *ui) {
    cui_questionnaire_routes *stack = cui_questionnaire_route_stack(ui);
    if (stack && stack->active) {
        cui_questionnaire_route *r = stack->active;
        r->previous = r->current;
        size_t current = r->state->current;
        if (current != r->opened_item && current < r->state->count &&
            cui_questionnaire_item_error(r->state, current).kind != CUI_QUESTIONNAIRE_VALID)
            r->invalid_pending = true;
        if (r->focus_pending || r->invalid_pending || current != r->opened_item) {
            cui_questionnaire_binding *target = NULL;
            if (r->invalid_pending && current < r->state->count) {
                if (cui_questionnaire_freeform(r->state, current).length)
                    target = cui_questionnaire_bound(r, current, 2, 0);
                for (size_t c = 0; !target && c < r->state->items[current].choice_count; ++c)
                    if (cui_questionnaire_choice_selected(r->state, current, c))
                        target = cui_questionnaire_bound(r, current, 1, c);
                for (size_t c = 0; !target && c < r->state->items[current].choice_count; ++c)
                    target = cui_questionnaire_bound(r, current, 1, c);
                if (!target)
                    target = cui_questionnaire_bound(r, current, 2, 0);
            }
            if (!target)
                target = cui_questionnaire_bound(r, current, 0, 0);
            cui_focus(ui, target ? target->id : r->scope);
            if (target)
                r->focus_pending = r->invalid_pending = false;
            else
                r->focus_pending = true;
        }
        stack->active = r->parent;
    }
    cui_end_box(ui);
    cui_end_focus_scope(ui);
}
static bool cui_questionnaire_visible_item(const cui_questionnaire_state *s, size_t item) {
    return s && s->initialized && item < s->count && item == s->current &&
           !s->storage.items[item].disabled;
}
static float cui_questionnaire_choices_gap(cui_questionnaire_size size) {
    switch (size) {
    case CUI_QUESTIONNAIRE_XSMALL:
        return 4;
    case CUI_QUESTIONNAIRE_SMALL:
        return 6;
    case CUI_QUESTIONNAIRE_LARGE:
        return 10;
    default:
        return 8;
    }
}
bool cui_questionnaire_item_begin(cui_context *ui, cui_id id, const cui_questionnaire_state *s,
                                  size_t item, cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return false;
    if (!ui || !id || !cui_questionnaire_visible_item(s, item))
        return false;
    cui_questionnaire_bind(ui, id, s, item, 0, 0, o.disabled);
    o = cui_questionnaire_resolve_size(ui, s, o);
    const cui_questionnaire_item *definition = &s->items[item];
    cui_interact(ui, id, CUI_ROLE_GROUP,
                 definition->label.length ? definition->label : definition->name, o.disabled, false,
                 0);
    cui_set_semantic_details(
        ui, id, (cui_semantic_details){.description = definition->description, .focusable = true});
    cui_set_tab_stop(ui, id, false);
    cui_begin_box(ui, id, (cui_box){.width = -1, .gap = cui_questionnaire_measure(o).root_gap});
    return true;
}
bool cui_questionnaire_choices_begin(cui_context *ui, cui_id id, const cui_questionnaire_state *s,
                                     size_t item, cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return false;
    if (!ui || !id || !cui_questionnaire_visible_item(s, item))
        return false;
    o = cui_questionnaire_resolve_size(ui, s, o);
    cui_interact(ui, id, s->items[item].multiple ? CUI_ROLE_GROUP : CUI_ROLE_RADIOGROUP,
                 (cui_str){0}, o.disabled, false, 0);
    cui_begin_box(ui, id,
                  (cui_box){.width = -1,
                            .gap = (o.design_size ? cui_questionnaire_measure(o).gap
                                                  : cui_questionnaire_choices_gap(o.size))});
    return true;
}
bool cui_questionnaire_actions_begin(cui_context *ui, cui_id id, const cui_questionnaire_state *s,
                                     cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return false;
    if (!ui || !id || !s || !s->initialized)
        return false;
    o = cui_questionnaire_resolve_size(ui, s, o);
    cui_begin_box(ui, id,
                  (cui_box){.width = -1,
                            .row = true,
                            .align_y = CUI_ALIGN_CENTER,
                            .gap = (o.design_size ? cui_questionnaire_measure(o).gap
                                                  : cui_questionnaire_choices_gap(o.size))});
    return true;
}
void cui_questionnaire_part_end(cui_context *ui) {
    cui_end_box(ui);
}
void cui_questionnaire_progress(cui_context *ui, cui_id id, const cui_theme *t,
                                const cui_questionnaire_state *s, cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return;
    if (!ui || !id || !t || !s || !s->initialized)
        return;
    o = cui_questionnaire_resolve_size(ui, s, o);
    size_t position = cui_questionnaire_position(s), total = cui_questionnaire_total(s);
    size_t current = position == SIZE_MAX ? 0 : position + 1;
    char label[80];
    int n = snprintf(label, sizeof label, "Question %zu of %zu", current, total);
    if (n < 0 || (size_t)n >= sizeof label)
        return;
    cui_str text = {label, (size_t)n};
    cui_interact(ui, id, CUI_ROLE_PROGRESSBAR, text, false, false, (float)current);
    cui_set_semantic_details(ui, id, (cui_semantic_details){.minimum = 0, .maximum = (float)total});
    cui_begin_box(ui, id, (cui_box){.width = -1});
    cui_text_ex(ui, text,
                o.design_size                       ? cui_questionnaire_measure(o).font * .75f
                : o.size == CUI_QUESTIONNAIRE_LARGE ? 14
                                                    : 12,
                t->muted, 0, (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM});
    cui_end_box(ui);
}
bool cui_questionnaire_choice_control(cui_context *ui, cui_id id, const cui_theme *t,
                                      cui_questionnaire_state *s, size_t item, size_t choice,
                                      cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return false;
    if (!ui || !id || !t || !s || !s->initialized || item != s->current || item >= s->count ||
        choice >= s->items[item].choice_count)
        return false;
    o = cui_questionnaire_resolve_size(ui, s, o);
    const cui_questionnaire_item *d = &s->items[item];
    const cui_questionnaire_choice *c = &d->choices[choice];
    bool disabled = o.disabled || s->storage.items[item].disabled ||
                    s->storage.choices[s->storage.items[item].choice_offset + choice].disabled;
    cui_questionnaire_bind(ui, id, s, item, choice, 1, disabled);
    bool selected = cui_questionnaire_choice_selected(s, item, choice);
    cui_str label = c->label.length ? c->label : c->value;
    cui_interaction interaction = cui_interact(
        ui, id, d->multiple ? CUI_ROLE_CHECKBOX : CUI_ROLE_RADIO, label, disabled, selected, 0);
    bool changed = interaction.activated && !disabled;
    if (changed) {
        (void)cui_questionnaire_choose(s, item, choice, cui_frame_time(ui), cui_reduced_motion(ui));
        selected = cui_questionnaire_choice_selected(s, item, choice);
        cui_update_semantic(ui, id, selected, selected ? 1 : 0);
        cui_invalidate(ui);
    }
    cui_questionnaire_metrics m = cui_questionnaire_measure(o);
    bool invalid = cui_questionnaire_item_error(s, item).kind != CUI_QUESTIONNAIRE_VALID;
    size_t position = 0, total = 0;
    (void)cui_questionnaire_choice_position(s, item, choice, &position, &total);
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.invalid = invalid,
                               .required = d->required,
                               .description = c->description,
                               .pos_in_set = position <= UINT_MAX ? (unsigned)position : 0,
                               .set_size = total <= UINT_MAX ? (unsigned)total : 0,
                               .pointer_activatable = true});
    char key = cui_questionnaire_shortcut(s, item, choice, o.numeric_shortcuts);
    cui_questionnaire_choice_view view = {.item = item,
                                          .choice = choice,
                                          .size = o.size,
                                          .design_size = o.design_size,
                                          .multiple = d->multiple,
                                          .selected = selected,
                                          .disabled = disabled,
                                          .invalid = invalid,
                                          .focused = cui_focused(ui) == id,
                                          .shortcut = key};
    float card_radius = o.size == CUI_QUESTIONNAIRE_XSMALL  ? t->radius
                        : o.size == CUI_QUESTIONNAIRE_LARGE ? t->radius * 2
                                                            : t->radius_lg;
    cui_color card_background =
        selected ? t->muted_background
                 : cui_questionnaire_opacity(t->dark ? t->input : t->background, t->dark ? .2f : 0);
    if (interaction.hovered && !disabled)
        card_background = cui_questionnaire_opacity(t->muted_background, .5f);
    cui_begin_box(ui, id,
                  (cui_box){.width = -1,
                            .padding = m.py,
                            .row = true,
                            .gap = 0,
                            .radius = card_radius,
                            .border_width = 1,
                            .border = view.focused ? t->focus
                                      : invalid    ? t->danger
                                      : selected   ? cui_questionnaire_opacity(t->primary, .4f)
                                                   : t->input,
                            .background = card_background,
                            .custom_opacity = disabled,
                            .opacity = .5f});
    if (view.focused && !t->hide_focus_ring) {
        cui_color ring = t->focus;
        ring.a = 127;
        (void)cui_set_outline(
            ui, 3, ring, (cui_corner_radii){card_radius, card_radius, card_radius, card_radius});
    }
    /* Side spacers supply independent horizontal padding and the card's minimum
     * height without centering the indicator against multi-line descriptions. */
    cui_begin_box(
        ui, 0, (cui_box){.width = m.px - m.py, .fixed_width = true, .height = m.height - 2 * m.py});
    cui_end_box(ui);
    cui_begin_box(ui, 0, (cui_box){.width = -1, .row = true, .gap = m.gap});
    if (o.render_indicator) {
        cui_begin_box(ui, 0, (cui_box){0});
        cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.indicator) / 2)});
        cui_end_box(ui);
        o.render_indicator(ui, id ^ UINT64_C(0x751ec681a5920310), &view, o.render_user);
        cui_end_box(ui);
    } else {
        cui_begin_box(ui, 0,
                      (cui_box){.height = m.line,
                                .align_y = o.indicator_style ? CUI_ALIGN_START : CUI_ALIGN_CENTER});
        if (o.indicator_style) {
            cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.indicator) / 2)});
            cui_end_box(ui);
        }
        cui_begin_box(ui, 0,
                      cui_questionnaire_refine(
                          (cui_box){.width = m.indicator,
                                    .height = m.indicator,
                                    .radius = d->multiple ? t->radius / 2 : m.indicator / 2,
                                    .border_width = 1,
                                    .border = selected ? t->primary : t->input,
                                    .background = selected ? t->primary : t->input_background,
                                    .align_x = CUI_ALIGN_CENTER,
                                    .align_y = CUI_ALIGN_CENTER},
                          o.indicator_style));
        if (selected) {
            if (d->multiple)
                cui_checkmark(ui, m.indicator - 2, t->on_primary);
            else {
                cui_begin_box(ui, 0,
                              (cui_box){.width = m.mark,
                                        .height = m.mark,
                                        .radius = m.mark / 2,
                                        .background = t->on_primary});
                cui_end_box(ui);
            }
        }
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_begin_box(
        ui, 0,
        cui_questionnaire_refine((cui_box){.width = -1, .gap = m.content_gap}, o.content_style));
    if (o.render_content)
        o.render_content(ui, id ^ UINT64_C(0x1f0e69b892345708), &view, o.render_user);
    else {
        cui_text_with_line_height(ui, label, m.font, t->text, m.line);
        if (c->description.length)
            cui_text_with_line_height(ui, c->description, m.font > 14 ? m.font - 2 : 12, t->muted,
                                      m.line);
    }
    cui_end_box(ui);
    if (o.render_shortcut) {
        cui_begin_box(ui, 0, (cui_box){0});
        cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.shortcut) / 2)});
        cui_end_box(ui);
        o.render_shortcut(ui, id ^ UINT64_C(0x931ac84625c0a917), &view, o.render_user);
        cui_end_box(ui);
    } else if (key) {
        cui_begin_box(ui, 0,
                      (cui_box){.width = o.shortcut_style ? 0 : m.shortcut,
                                .height = m.line,
                                .align_y = o.shortcut_style ? CUI_ALIGN_START : CUI_ALIGN_CENTER});
        if (o.shortcut_style) {
            cui_begin_box(ui, 0, (cui_box){.height = fmaxf(0, (m.line - m.shortcut) / 2)});
            cui_end_box(ui);
        }
        cui_begin_box(ui, 0,
                      cui_questionnaire_refine(
                          (cui_box){.width = m.shortcut,
                                    .height = m.shortcut,
                                    .background = t->background,
                                    .border_width = 1,
                                    .border = t->input,
                                    .radius = o.size == CUI_QUESTIONNAIRE_XSMALL  ? t->radius / 2
                                              : o.size == CUI_QUESTIONNAIRE_LARGE ? t->radius_lg
                                                                                  : t->radius,
                                    .align_x = CUI_ALIGN_CENTER,
                                    .align_y = CUI_ALIGN_CENTER},
                          o.shortcut_style));
        float shortcut_font = o.design_size                        ? m.font * .625f
                              : o.size == CUI_QUESTIONNAIRE_XSMALL ? 8
                              : o.size == CUI_QUESTIONNAIRE_SMALL  ? 9
                              : o.size == CUI_QUESTIONNAIRE_LARGE  ? 12
                                                                   : 10;
        cui_text_ex(ui, (cui_str){&key, 1}, shortcut_font, t->muted, 0,
                    (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM, .monospace = true});
        cui_end_box(ui);
        cui_end_box(ui);
    }
    cui_end_box(ui); /* answer row */
    cui_begin_box(ui, 0, (cui_box){.width = m.px - m.py, .fixed_width = true});
    cui_end_box(ui);
    cui_end_box(ui); /* card */
    return changed;
}
void cui_questionnaire_heading(cui_context *ui, cui_id id, const cui_theme *t,
                               const cui_questionnaire_state *s, size_t item,
                               cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return;
    if (!ui || !id || !t || !s || !s->initialized || item >= s->count || item != s->current)
        return;
    o = cui_questionnaire_resolve_size(ui, s, o);
    const cui_questionnaire_item *d = &s->items[item];
    cui_questionnaire_metrics m = cui_questionnaire_measure(o);
    cui_begin_box(ui, id, (cui_box){.width = -1, .gap = m.content_gap});
    cui_text_ex(ui, d->label.length ? d->label : d->name, m.font + 2, t->text, m.line + 4,
                (cui_text_style){.weight = CUI_TEXT_WEIGHT_MEDIUM});
    if (d->description.length)
        cui_text_with_line_height(ui, d->description, m.font > 14 ? m.font - 2 : 12, t->muted,
                                  m.line);
    cui_end_box(ui);
}
void cui_questionnaire_error_control(cui_context *ui, cui_id id, const cui_theme *t,
                                     const cui_questionnaire_state *s, size_t item,
                                     cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return;
    if (!ui || !id || !t || !s || !s->initialized || item >= s->count || item != s->current)
        return;
    o = cui_questionnaire_resolve_size(ui, s, o);
    cui_questionnaire_error e = cui_questionnaire_item_error(s, item);
    if (e.kind == CUI_QUESTIONNAIRE_VALID)
        return;
    cui_str message = e.kind == CUI_QUESTIONNAIRE_MESSAGE ? e.message
                      : e.kind == CUI_QUESTIONNAIRE_REQUIRED
                          ? cui_string("This question is required")
                          : cui_string("Answer or skip this question");
    if (o.labels && e.kind == CUI_QUESTIONNAIRE_REQUIRED &&
        (o.labels->fields & CUI_QUESTIONNAIRE_LABEL_REQUIRED))
        message = o.labels->required;
    if (o.labels && e.kind == CUI_QUESTIONNAIRE_UNFILLED &&
        (o.labels->fields & CUI_QUESTIONNAIRE_LABEL_UNFILLED))
        message = o.labels->unfilled;
    cui_interact(ui, id, CUI_ROLE_ALERT, message, false, false, 0);
    cui_begin_box(ui, id, (cui_box){.width = -1});
    cui_text(ui, message,
             o.design_size                       ? cui_questionnaire_measure(o).font
             : o.size == CUI_QUESTIONNAIRE_LARGE ? 16
                                                 : 12,
             t->danger);
    cui_end_box(ui);
}
bool cui_questionnaire_action_control(cui_context *ui, cui_id id, const cui_theme *t,
                                      cui_questionnaire_state *s, cui_questionnaire_action action,
                                      cui_str label, cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return false;
    if (!ui || !id || !t || !s || !s->initialized || s->current >= s->count)
        return false;
    o = cui_questionnaire_resolve_size(ui, s, o);
    size_t position = cui_questionnaire_position(s), total = cui_questionnaire_total(s);
    bool visible = action == CUI_QUESTIONNAIRE_PREVIOUS ? position > 0
                   : action == CUI_QUESTIONNAIRE_SKIP   ? !s->items[s->current].required
                   : action == CUI_QUESTIONNAIRE_NEXT
                       ? position + 1 < total
                       : action == CUI_QUESTIONNAIRE_FINISH && position + 1 == total;
    if (!visible)
        return false;
    if (!cui_edit_valid_utf8(label))
        return false;
    bool explicit_label = label.length != 0;
    if (!explicit_label)
        label = cui_string(action == CUI_QUESTIONNAIRE_PREVIOUS ? "Previous"
                           : action == CUI_QUESTIONNAIRE_SKIP   ? "Skip"
                           : action == CUI_QUESTIONNAIRE_NEXT   ? "Next"
                                                                : "Submit");
    if (!explicit_label && o.labels && (o.labels->fields & (1u << (unsigned)action))) {
        cui_str values[] = {o.labels->previous, o.labels->skip, o.labels->next, o.labels->submit};
        label = values[(unsigned)action];
    }
    cui_theme action_theme = *t;
    if (o.design_size && o.density > 0)
        action_theme.font_size = 16 * o.density;
    bool activated = cui_button_ex(
        ui, id, &action_theme, label,
        (cui_button_options){.size = (cui_control_size)o.size,
                             .design_size = o.design_size,
                             .disabled = o.disabled,
                             .variant = action == CUI_QUESTIONNAIRE_PREVIOUS ? CUI_BUTTON_OUTLINE
                                        : action == CUI_QUESTIONNAIRE_SKIP   ? CUI_BUTTON_GHOST
                                                                             : CUI_BUTTON_PRIMARY});
    if (!activated)
        return false;
    bool result = action == CUI_QUESTIONNAIRE_PREVIOUS ? cui_questionnaire_previous(s)
                  : action == CUI_QUESTIONNAIRE_SKIP   ? cui_questionnaire_skip(s)
                  : action == CUI_QUESTIONNAIRE_NEXT   ? cui_questionnaire_next(s)
                                                       : cui_questionnaire_submit(s);
    cui_invalidate(ui);
    return result;
}
cui_questionnaire_result cui_questionnaire_input_control(cui_context *ui, cui_id id,
                                                         const cui_theme *t,
                                                         cui_questionnaire_state *s, size_t item,
                                                         cui_questionnaire_input_state *view,
                                                         const cui_platform *clipboard,
                                                         cui_questionnaire_options o) {
    cui_questionnaire_options_snapshot snapshot;
    if (!cui_questionnaire_snapshot(&o, &snapshot))
        return CUI_QUESTIONNAIRE_INVALID;
    if (!ui || !id || !t || !s || !s->initialized || !view)
        return CUI_QUESTIONNAIRE_INVALID;
    if (item >= s->count)
        return CUI_QUESTIONNAIRE_UNKNOWN_ITEM;
    o = cui_questionnaire_resolve_size(ui, s, o);
    const cui_questionnaire_item *d = &s->items[item];
    if (!d->input)
        return CUI_QUESTIONNAIRE_ANSWER_MISMATCH;
    if (item != s->current)
        return CUI_QUESTIONNAIRE_OK;
    cui_questionnaire_bind(ui, id, s, item, 0, 2,
                           o.disabled || d->input_disabled || s->storage.items[item].disabled);

    cui_questionnaire_metrics m = cui_questionnaire_measure(o);
    bool disabled = o.disabled || s->storage.items[item].disabled || d->input_disabled;
    cui_str label = d->input_label.length ? d->input_label : d->label;
    cui_input_state input = {.behavior = {.dragging = view->dragging,
                                          .composition = view->composition,
                                          .composition_owned = view->composition_owned},
                             .scroll_x = view->scroll_x};
    input.behavior.composition =
        cui_base_composition(ui, id, input.behavior.composition, &input.behavior.composition_owned);
    if (input.behavior.composition && input.behavior.composition_owned)
        input.behavior.composition->grow_view = true;
    if (input.behavior.composition && (disabled || cui_focused(ui) != id))
        (void)cui_edit_composition_event(d->input, input.behavior.composition,
                                         (cui_event){.type = CUI_EVENT_COMPOSITION_CANCEL}, false);
    cui_input_options paint = {.width = -1,
                               .size = (cui_input_size)o.size,
                               .design_size = o.design_size,
                               .density = o.density,
                               .disabled = disabled,
                               .required = d->required,
                               .invalid = cui_questionnaire_item_error(s, item).kind !=
                                          CUI_QUESTIONNAIRE_VALID,
                               .clipboard = clipboard,
                               .custom_padding = true,
                               .padding_x = m.px};
    cui_input_metrics metrics;
    if (!cui_input_measure(paint, t->font_size, &metrics))
        return CUI_QUESTIONNAIRE_INVALID;
    cui_input_hit_geometry hit = {ui, cui_bounds(ui, cui_input_viewport_id(id)), metrics.font_size,
                                  input.scroll_x};
    cui_base_input_options base = {
        .disabled = disabled,
        .required = d->required,
        .invalid = paint.invalid,
        .clipboard = clipboard,
        .hit_test = cui_edit_get_state(d->input).multiline ? NULL : cui_base_input_hit_test,
        .hit_test_user = &hit};
    (void)cui_interact(ui, id, CUI_ROLE_TEXTBOX, label, disabled, false,
                       (float)cui_edit_get_state(d->input).length);
    if (disabled)
        input.behavior.dragging = false;
    cui_questionnaire_routes *stack = cui_questionnaire_route_stack(ui);
    cui_questionnaire_route *route =
        stack && stack->active && stack->active->state == s ? stack->active : NULL;
    cui_edit_result error = CUI_EDIT_OK;
    cui_questionnaire_result sync = CUI_QUESTIONNAIRE_OK;
    size_t count = 0;
    const cui_event *events = cui_frame_events(ui, &count);
    for (size_t n = 0; n < count && s->current == item; ++n) {
        const cui_event *event = &events[n];
        bool eligible = !disabled && !cui_event_consumed(ui, n) && cui_event_target(ui, n) == id &&
                        cui_focused(ui) == id;
        bool key = eligible && event->type == CUI_EVENT_KEY_DOWN && !event->repeat &&
                   !event->ime_composing && !event->prefer_character_input;
        bool handled = false;
        if (key && route)
            handled = cui_questionnaire_root_key(ui, route, event);
        else if (key && event->key == CUI_KEY_ENTER && !event->modifiers &&
                 cui_questionnaire_freeform(s, item).length)
            handled = cui_questionnaire_confirm(s);
        if (handled) {
            cui_consume_event(ui, n);
            if (event->key == CUI_KEY_ENTER)
                cui_cancel_event_activation(ui, n);
            cui_invalidate(ui);
        }
        if (s->current != item || (handled && s->complete)) {
            input.behavior.dragging = false;
            view->dragging = false;
            view->scroll_x = input.scroll_x;
            if (route && s->current != item)
                route->focus_pending = true;
            return error != CUI_EDIT_OK ? CUI_QUESTIONNAIRE_INVALID : sync;
        }
        /* Focus can move to a choice without moving the questionnaire item. */
        if (!handled && (cui_focused(ui) == id ||
                         (event->type != CUI_EVENT_TEXT && event->type != CUI_EVENT_KEY_DOWN &&
                          event->type != CUI_EVENT_KEY_UP))) {
            cui_input_result result =
                cui_base_input_reduce_event(ui, id, d->input, &input.behavior, base, n);
            if (error == CUI_EDIT_OK && result.error != CUI_EDIT_OK)
                error = result.error;
            if (result.changed) {
                sync = cui_questionnaire_sync_input(s, item, true);
                cui_invalidate(ui);
                if (sync != CUI_QUESTIONNAIRE_OK)
                    break;
            }
        }
    }
    view->composition = input.behavior.composition;
    view->composition_owned = input.behavior.composition_owned;
    view->dragging = input.behavior.dragging;
    cui_update_semantic(ui, id, false, (float)cui_edit_get_state(d->input).length);
    paint.invalid |= error != CUI_EDIT_OK ||
                     cui_questionnaire_item_error(s, item).kind != CUI_QUESTIONNAIRE_VALID;
    cui_set_semantic_details(
        ui, id,
        (cui_semantic_details){.required = d->required,
                               .invalid = paint.invalid,
                               .multiline = cui_edit_get_state(d->input).multiline});
    cui_input_paint(ui, id, t, label, d->input, &input, paint);
    view->scroll_x = input.scroll_x;
    return error != CUI_EDIT_OK ? CUI_QUESTIONNAIRE_INVALID : sync;
}

bool cui_questionnaire_radio_keys(cui_context *ui, cui_questionnaire_state *s, const cui_id *ids,
                                  size_t count) {
    if (!ui || !s || !s->initialized || s->current >= s->count || !ids)
        return false;
    size_t item = s->current;
    const cui_questionnaire_item *d = &s->items[item];
    if (d->multiple || count != d->choice_count || s->storage.items[item].disabled)
        return false;
    bool changed = false;
    size_t event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    for (size_t e = 0; e < event_count && s->current == item; e++) {
        if (cui_event_consumed(ui, e) || events[e].type != CUI_EVENT_KEY_DOWN ||
            events[e].ime_composing || events[e].prefer_character_input || events[e].repeat ||
            events[e].modifiers ||
            (events[e].key != CUI_KEY_LEFT && events[e].key != CUI_KEY_RIGHT))
            continue;
        size_t current = SIZE_MAX;
        for (size_t c = 0; c < count; c++)
            if (ids[c] && cui_focused(ui) == ids[c] &&
                !s->storage.choices[s->storage.items[item].choice_offset + c].disabled)
                current = c;
        if (current == SIZE_MAX)
            continue;
        for (size_t step = 1; step <= count; step++) {
            size_t next = events[e].key == CUI_KEY_LEFT ? (current + count - step) % count
                                                        : (current + step) % count;
            if (!ids[next] ||
                s->storage.choices[s->storage.items[item].choice_offset + next].disabled)
                continue;
            (void)cui_questionnaire_activate(s, item, next);
            cui_focus(ui, ids[next]);
            cui_consume_event(ui, e);
            cui_invalidate(ui);
            changed = true;
            break;
        }
    }
    return changed;
}
bool cui_questionnaire_answer_keys(cui_context *ui, cui_id scope, cui_questionnaire_state *s,
                                   const cui_id *ids, size_t count, cui_id input_id) {
    if (!ui || !scope || !s || !s->initialized || s->current >= s->count || (count && !ids) ||
        count == SIZE_MAX || !cui_focus_within(ui, scope))
        return false;
    size_t item = s->current;
    const cui_questionnaire_item *d = &s->items[item];
    if (count != d->choice_count || s->storage.items[item].disabled)
        return false;
    bool input = d->input && !d->input_disabled && input_id;
    size_t event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    bool handled = false;
    for (size_t e = 0; e < event_count && s->current == item; e++) {
        if (cui_event_consumed(ui, e) || events[e].type != CUI_EVENT_KEY_DOWN ||
            events[e].ime_composing || events[e].prefer_character_input || events[e].repeat ||
            events[e].modifiers || (events[e].key != CUI_KEY_UP && events[e].key != CUI_KEY_DOWN))
            continue;
        cui_id focused = cui_focused(ui);
        if (input && focused == input_id && cui_edit_get_text(d->input).length)
            continue;
        bool previous = events[e].key == CUI_KEY_UP;
        size_t slots = count + 1, current = SIZE_MAX;
        for (size_t c = 0; c < count; c++)
            if (ids[c] && ids[c] == focused &&
                !s->storage.choices[s->storage.items[item].choice_offset + c].disabled)
                current = c;
        if (input && focused == input_id)
            current = count;
        /* Entering the answers from an item/action focus restores a filled
         * answer first, without selecting an unrelated radio. */
        if (current == SIZE_MAX) {
            bool restored = false;
            for (size_t step = 0; step < slots; step++) {
                size_t next = previous ? slots - 1 - step : step;
                bool filled = next == count
                                  ? input && cui_questionnaire_freeform(s, item).length != 0
                                  : ids[next] && cui_questionnaire_choice_selected(s, item, next);
                if (!filled)
                    continue;
                cui_focus(ui, next == count ? input_id : ids[next]);
                cui_consume_event(ui, e);
                cui_invalidate(ui);
                handled = restored = true;
                break;
            }
            if (restored)
                continue;
        }
        size_t start = current == SIZE_MAX ? (previous ? 0 : slots - 1) : current;
        for (size_t step = 1; step <= slots; step++) {
            size_t next = previous ? (start + slots - step) % slots : (start + step) % slots;
            if (next == count
                    ? !input
                    : (!ids[next] ||
                       s->storage.choices[s->storage.items[item].choice_offset + next].disabled))
                continue;
            if (next < count && !d->multiple)
                (void)cui_questionnaire_activate(s, item, next);
            cui_focus(ui, next == count ? input_id : ids[next]);
            cui_consume_event(ui, e);
            cui_invalidate(ui);
            handled = true;
            break;
        }
    }
    return handled;
}
bool cui_questionnaire_shortcut_keys(cui_context *ui, cui_id scope, cui_questionnaire_state *s,
                                     const cui_id *ids, size_t count, cui_id input_id,
                                     bool numbers) {
    if (!ui || !scope || !s || !s->initialized || s->current >= s->count || (count && !ids) ||
        count != s->items[s->current].choice_count || s->storage.items[s->current].disabled ||
        !cui_focus_within(ui, scope))
        return false;
    size_t semantic_count;
    const cui_semantic *semantics = cui_semantics(ui, &semantic_count);
    bool enabled_scope = false;
    for (size_t n = 0; n < semantic_count; n++)
        if (semantics[n].id == scope)
            enabled_scope = !semantics[n].disabled;
    if (!enabled_scope)
        return false;
    size_t item = s->current, event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    bool handled = false;
    for (size_t e = 0; e < event_count && s->current == item; e++) {
        if (cui_event_consumed(ui, e) || events[e].type != CUI_EVENT_KEY_DOWN || events[e].repeat ||
            events[e].modifiers || events[e].ime_composing || events[e].prefer_character_input ||
            (input_id && cui_focused(ui) == input_id))
            continue;
        char key = cui_key_character(events[e].key);
        if (!key)
            continue;
        for (size_t c = 0; c < count; c++) {
            if (!ids[c] || cui_questionnaire_shortcut(s, item, c, numbers) != key)
                continue;
            if (cui_questionnaire_choose(s, item, c, cui_frame_time(ui), cui_reduced_motion(ui)) ==
                CUI_QUESTIONNAIRE_OK) {
                if (s->current == item)
                    cui_focus(ui, ids[c]);
                cui_consume_event(ui, e);
                cui_invalidate(ui);
                handled = true;
            }
            break;
        }
    }
    return handled;
}
bool cui_questionnaire_confirm_keys(cui_context *ui, cui_id scope, cui_questionnaire_state *s,
                                    const cui_id *ids, size_t count, cui_id input_id,
                                    unsigned submit_modifier) {
    if (!ui || !scope || !s || !s->initialized || s->current >= s->count || (count && !ids) ||
        count != s->items[s->current].choice_count || !cui_focus_within(ui, scope) ||
        s->storage.items[s->current].disabled ||
        (submit_modifier && submit_modifier != CUI_MOD_CTRL && submit_modifier != CUI_MOD_META))
        return false;
    size_t item = s->current, event_count;
    const cui_event *events = cui_frame_events(ui, &event_count);
    bool handled = false;
    for (size_t e = 0; e < event_count && s->current == item; e++) {
        if (cui_event_consumed(ui, e) || events[e].type != CUI_EVENT_KEY_DOWN ||
            events[e].key != CUI_KEY_ENTER || events[e].repeat || events[e].ime_composing ||
            events[e].prefer_character_input)
            continue;
        bool filled = false;
        cui_id focus = cui_focused(ui);
        cui_questionnaire_routes *stack = cui_questionnaire_route_stack(ui);
        if (input_id && focus == input_id && stack && stack->active && stack->active->state == s)
            continue; /* Input control confirms after preceding edit events. */
        if (input_id && focus == input_id)
            filled = cui_questionnaire_freeform(s, item).length != 0;
        for (size_t c = 0; c < count; c++)
            if (ids[c] && focus == ids[c])
                filled = cui_questionnaire_choice_selected(s, item, c);
        bool modified = submit_modifier && events[e].modifiers == submit_modifier;
        if (modified || (!events[e].modifiers && filled)) {
            bool confirmed = cui_questionnaire_confirm(s);
            /* Modified Enter belongs to the form even when validation fails;
             * it must not fall through to selecting the focused radio. */
            if (confirmed || modified) {
                cui_consume_event(ui, e);
                cui_cancel_event_activation(ui, e);
                handled = true;
            }
            cui_invalidate(ui);
        }
    }
    return handled;
}
