/* SPDX-License-Identifier: MIT */
/* TEST ONLY exact-seam adapter; installed Core/Base sources remain untouched. */
#include "scrollbar_track_probe.h"
#include <assert.h>
#include <math.h>
#include <string.h>
typedef struct {
    prototype_track_record record;
    cui_rect track, thumb;
    uint32_t clamp_index, flags;
} prototype_track_entry;
static bool prototype_track_stage(cui_context *, void *);
static bool prototype_track_validate(cui_context *, void *);
static void prototype_track_publish(cui_context *, void *);
#include "scrollbar_track_core.inc"

_Static_assert(sizeof(prototype_track_record) == 48, "test record ABI");
_Static_assert(sizeof(prototype_track_entry) == 88, "test entry budget");
_Static_assert(sizeof(cui_core_sl_state) <= 16384, "test opt-in resource cap");
static cui_semantic *track_semantic(cui_context *ui, cui_id id) {
    cui_semantic *found = NULL;
    for (size_t i = 0; i < ui->semantic_count; ++i)
        if (ui->semantics[i].id == id) {
            if (found)
                return NULL;
            found = &ui->semantics[i];
        }
    return found;
}
static bool track_topology(cui_context *ui, const prototype_track_record *r, bool staged) {
    size_t v = cui_core_sl_node(ui, r->viewport), b = cui_core_sl_node(ui, r->bar),
           t = cui_core_sl_node(ui, r->thumb);
    cui_semantic *owner = track_semantic(ui, r->viewport), *bar = track_semantic(ui, r->bar);
    if (v == SIZE_MAX || b == SIZE_MAX || t == SIZE_MAX || v == b || v == t || b == t ||
        !cui_core_sl_closed(ui, v) || !cui_core_sl_closed(ui, b) || !cui_core_sl_closed(ui, t) ||
        ui->nodes[b].parent != v || ui->nodes[t].parent != b || track_semantic(ui, r->thumb))
        return false;
    bool inactive = ui->nodes[v].inert || ui->nodes[v].layout_only;
    if ((!inactive && (!owner || !bar)) ||
        (bar && (bar->role != CUI_ROLE_SCROLLBAR || bar->details.parent != r->viewport ||
                 bar->details.controls != r->viewport ||
                 bar->details.orientation != CUI_ORIENTATION_VERTICAL)))
        return false;
    size_t nodes[] = {b, t};
    for (size_t i = 0; i < 2; ++i) {
        Clay_LayoutElement *e = cui_core_sl_element_node(ui, nodes[i]);
        if (!e || e->config.floating.attachTo != CLAY_ATTACH_TO_PARENT ||
            e->config.floating.clipTo != CLAY_CLIP_TO_ATTACHED_PARENT ||
            e->config.floating.parentId != ui->nodes[ui->nodes[nodes[i]].parent].clay_id ||
            e->config.layout.sizing.width.type != CLAY__SIZING_TYPE_FIXED ||
            e->config.layout.sizing.width.size.minMax.min != r->thickness ||
            !cui_core_sl_no_padding(e) || e->config.aspectRatio.aspectRatio ||
            e->config.layout.sizing.height.type != CLAY__SIZING_TYPE_FIXED ||
            !isfinite(e->config.layout.sizing.height.size.minMax.min) ||
            e->config.layout.sizing.height.size.minMax.min < 0 ||
            (!staged && e->config.layout.sizing.height.size.minMax.min == 0) ||
            e->children.length || e->floatingChildrenCount != (i == 0 ? 1 : 0))
            return false;
    }
    return true;
}
bool prototype_scrollbar_track(cui_context *ui, const prototype_track_record *r) {
    if (!cui_core_sl_begin(ui))
        return false;
    if (!r || r->struct_size != 48 || r->version != 1 || r->reserved || r->flags > 3 ||
        r->flags == PROTOTYPE_TRACK_INTERACTIVE || !isfinite(r->thickness) || r->thickness <= 0 ||
        !isfinite(r->minimum_thumb) || r->minimum_thumb <= 0 || !ui->scroll_layout ||
        !track_topology(ui, r, false))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    cui_core_sl_state *state = cui_core_sl_state_get(ui);
    if (!state)
        return false;
    size_t clamp = SIZE_MAX;
    for (size_t i = 0; i < state->header.clamps; ++i) {
        if (state->clamps[i].record.viewport == r->viewport)
            clamp = i;
        if (state->clamps[i].record.viewport == r->bar ||
            state->clamps[i].record.content == r->bar ||
            state->clamps[i].record.viewport == r->thumb ||
            state->clamps[i].record.content == r->thumb)
            return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    }
    if (clamp == SIZE_MAX)
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    for (size_t i = 0; i < state->track_count; ++i) {
        prototype_track_record old = state->tracks[i].record;
        if (old.viewport == r->viewport || old.bar == r->bar || old.bar == r->thumb ||
            old.thumb == r->bar || old.thumb == r->thumb)
            return cui_core_sl_fail(ui, CUI_ERROR_DUPLICATE_ID);
    }
    if (state->track_count == 64)
        return cui_core_sl_fail(ui, CUI_ERROR_CAPACITY);
    state->tracks[state->track_count++] =
        (prototype_track_entry){.record = *r, .clamp_index = (uint32_t)clamp};
    return true;
}
/* Frozen old Base formula moved here for the prototype Core side. Comparator
 * additionally calls the still-unmodified real Base implementation. */
static float thumb_positive(float value, float fallback) {
    return isfinite(value) && value > 0 ? value : fallback;
}
static float thumb_limit(float value, float maximum) {
    return isfinite(value) ? fminf(fmaxf(value, 0), maximum) : 0;
}
cui_rect prototype_thumb_geometry(cui_rect track, float scroll, float viewport, float content,
                                  bool vertical, float minimum) {
    float length = vertical ? track.h : track.w;
    if (!isfinite(length) || length <= 0)
        return (cui_rect){0};
    viewport = thumb_positive(viewport, length);
    content = thumb_positive(content, viewport);
    float maximum = fmaxf(0, content - viewport);
    minimum = thumb_positive(minimum, 12);
    float thumb =
        fminf(length, fmaxf(fminf(minimum, length), length * fminf(1, viewport / content)));
    float position = maximum > 0 ? thumb_limit(scroll, maximum) / maximum * (length - thumb) : 0;
    if (vertical) {
        track.y += position;
        track.h = thumb;
    } else {
        track.x += position;
        track.w = thumb;
    }
    return track;
}
static bool prototype_track_stage(cui_context *ui, void *raw) {
    cui_core_sl_state *state = raw;
    for (size_t i = 0; i < state->track_count; ++i) {
        prototype_track_entry *p = &state->tracks[i];
        if (!track_topology(ui, &p->record, false))
            return false;
        Clay_LayoutElement *viewport = cui_core_sl_element(ui, p->record.viewport);
        float width = viewport->dimensions.width, height = viewport->dimensions.height;
        for (size_t j = 0; j < state->header.bodies; ++j) {
            cui_core_sl_body *body = &state->bodies[j];
            if (body->record.viewport == p->record.viewport)
                height = body->body_h;
            if (body->record.outer_viewport == p->record.viewport)
                height = body->inner_h;
        }
        cui_core_sl_clamp *clamp = &state->clamps[p->clamp_index];
        if (!isfinite(width) || !isfinite(height) || width < p->record.thickness || height < 0 ||
            !isfinite(height + clamp->maximum))
            return false;
        p->flags = (p->record.flags & PROTOTYPE_TRACK_VISIBLE) && height > 0 && clamp->maximum > 0;
        if (p->flags && (p->record.flags & PROTOTYPE_TRACK_INTERACTIVE)) {
            bool eligible = true;
            cui_semantic *bar_semantic = track_semantic(ui, p->record.bar);
            if (bar_semantic && bar_semantic->details.readonly)
                eligible = false;
            size_t node = cui_core_sl_node(ui, p->record.viewport), visits = 0;
            for (; node != SIZE_MAX && visits < ui->node_count; ++visits) {
                if (node >= ui->node_count)
                    return false;
                cui_semantic *ancestor = track_semantic(ui, ui->nodes[node].id);
                if (ui->nodes[node].inert || ui->nodes[node].layout_only ||
                    (ancestor && (ancestor->disabled || ancestor->details.readonly)))
                    eligible = false;
                node = ui->nodes[node].parent;
            }
            if (node != SIZE_MAX)
                return false;
            if (eligible)
                p->flags |= PROTOTYPE_TRACK_INTERACTIVE;
        }
        p->track =
            (cui_rect){width - p->record.thickness, 0, p->record.thickness, p->flags ? height : 0};
        p->thumb = p->flags
                       ? prototype_thumb_geometry((cui_rect){0, 0, p->record.thickness, height},
                                                  clamp->staged, height, height + clamp->maximum,
                                                  true, p->record.minimum_thumb)
                       : (cui_rect){0, 0, p->record.thickness, 0};
        Clay_LayoutElement *bar = cui_core_sl_element(ui, p->record.bar);
        Clay_LayoutElement *thumb = cui_core_sl_element(ui, p->record.thumb);
        cui_core_sl_fixed_height(bar, p->track.h);
        cui_core_sl_fixed_height(thumb, p->thumb.h);
        bar->config.floating.offset = (Clay_Vector2){p->track.x, p->track.y};
        thumb->config.floating.offset = (Clay_Vector2){p->thumb.x, p->thumb.y};
    }
    return true;
}
static bool track_rect_equal(cui_rect a, cui_rect b) {
    return cui_core_sl_near(a.x, b.x) && cui_core_sl_near(a.y, b.y) && cui_core_sl_near(a.w, b.w) &&
           cui_core_sl_near(a.h, b.h);
}
static cui_rect track_bounds(cui_context *ui, cui_id id) {
    (void)ui;
    Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = cui_core_folded(id)});
    return (cui_rect){d.boundingBox.x, d.boundingBox.y, d.boundingBox.width, d.boundingBox.height};
}
static bool prototype_track_validate(cui_context *ui, void *raw) {
    cui_core_sl_state *state = raw;
    for (size_t i = 0; i < state->track_count; ++i) {
        prototype_track_entry *p = &state->tracks[i];
        if (!track_topology(ui, &p->record, true))
            return false;
        cui_rect viewport = track_bounds(ui, p->record.viewport);
        cui_rect expected = p->track;
        expected.x += viewport.x;
        expected.y += viewport.y;
        if (!track_rect_equal(track_bounds(ui, p->record.bar), expected))
            return false;
        expected = p->thumb;
        expected.x += viewport.x + p->track.x;
        expected.y += viewport.y + p->track.y;
        if (!track_rect_equal(track_bounds(ui, p->record.thumb), expected))
            return false;
    }
    return true;
}
static void prototype_track_publish(cui_context *ui, void *raw) {
    cui_core_sl_state *state = raw;
    for (size_t i = 0; i < state->track_count; ++i) {
        prototype_track_entry *p = &state->tracks[i];
        cui_core_sl_clamp *clamp = &state->clamps[p->clamp_index];
        cui_semantic *bar = track_semantic(ui, p->record.bar);
        if (bar) {
            bar->disabled = !(p->flags & PROTOTYPE_TRACK_INTERACTIVE);
            bar->value = clamp->staged;
            bar->details.minimum = 0;
            bar->details.maximum = clamp->maximum;
        }
        if (!p->flags) {
            ui->nodes[cui_core_sl_node(ui, p->record.bar)].opacity = 0;
            ui->nodes[cui_core_sl_node(ui, p->record.thumb)].opacity = 0;
            /* Mark omitted before current prepaint selection AND final filtering.
             * TEST ONLY private marker reuses inert exclusion for these leaf nodes. */
            ui->nodes[cui_core_sl_node(ui, p->record.bar)].inert = true;
            ui->nodes[cui_core_sl_node(ui, p->record.thumb)].inert = true;
        }
    }
}
size_t prototype_track_resource_bytes(void) {
    return sizeof(cui_core_sl_state);
}
