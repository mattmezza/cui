/* SPDX-License-Identifier: MIT */
/* Phase observations only; registrations below use the production public APIs. */
#include "scroll_layout_probe.h"
static void probe_before(cui_context *);
static void probe_after(cui_context *);
#include "scroll_layout_core.inc"
#include <assert.h>

typedef struct {
    cui_context *ui;
    prototype_stats stats;
    uint64_t input, records;
    size_t resources, styles;
} probe_observation;
static probe_observation observations[8];
static bool second_pass;
bool prototype_second_pass(void) {
    return second_pass;
}
static probe_observation *observation(cui_context *ui) {
    for (size_t i = 0; i < 8; ++i)
        if (observations[i].ui == ui || !observations[i].ui) {
            observations[i].ui = ui;
            return &observations[i];
        }
    assert(false);
    return NULL;
}
static uint64_t hash(uint64_t h, const void *bytes, size_t count) {
    const unsigned char *p = bytes;
    for (size_t i = 0; i < count; ++i)
        h = (h ^ p[i]) * UINT64_C(1099511628211);
    return h;
}
static uint64_t input_hash(cui_context *ui) {
    uint64_t h = 0;
    h = hash(h, ui->events, ui->event_count * sizeof *ui->events);
    h = hash(h, ui->event_targets, ui->event_count * sizeof *ui->event_targets);
    h = hash(h, ui->event_consumed, ui->event_count * sizeof *ui->event_consumed);
    h = hash(h, ui->event_activations, ui->event_count * sizeof *ui->event_activations);
    h = hash(h, ui->activations, ui->activation_count * sizeof *ui->activations);
    h = hash(h, &ui->focus, sizeof ui->focus);
    h = hash(h, &ui->capture, sizeof ui->capture);
    h = hash(h, &ui->frame_time, sizeof ui->frame_time);
    return h;
}
static cui_core_sl_state *find_state(cui_context *ui) {
    for (size_t i = 0; i < ui->config.max_resources; ++i)
        if (ui->resources[i].collect == cui_core_sl_collect)
            return ui->resources[i].payload;
    return NULL;
}
static void probe_before(cui_context *ui) {
    probe_observation *o = observation(ui);
    *o = (probe_observation){.ui = ui};
    o->stats.first = 1;
    second_pass = true;
    o->stats.wrapped_before = (size_t)ui->clay->wrappedTextLines.length;
    o->stats.strings_before = ui->string_used;
    o->stats.sources_before = ui->text_source_count;
    o->input = input_hash(ui);
    o->records = hash(0, ui->text_sources, ui->text_source_count * sizeof *ui->text_sources);
    o->resources = ui->resource_bytes;
    o->styles = ui->text_style_count;
}
static void probe_after(cui_context *ui) {
    probe_observation *o = observation(ui);
    o->stats.second = 1;
    second_pass = false;
    o->stats.wrapped_after = (size_t)ui->clay->wrappedTextLines.length;
    o->stats.strings_after = ui->string_used;
    o->stats.sources_after = ui->text_source_count;
    o->stats.input_unchanged = o->input == input_hash(ui);
    o->stats.resources_unchanged = o->resources == ui->resource_bytes;
    o->stats.text_unchanged =
        o->stats.strings_before == ui->string_used &&
        o->stats.sources_before == ui->text_source_count && o->styles == ui->text_style_count &&
        o->records == hash(0, ui->text_sources, ui->text_source_count * sizeof *ui->text_sources);
}
bool prototype_register(cui_context *ui, prototype_link link) {
    if (!ui->scroll_layout) {
        *observation(ui) = (probe_observation){.ui = ui};
        second_pass = false;
    }
    cui_scroll_body_layout_v1 r = {.struct_size = 80,
                                   .version = 1,
                                   .mode = link.automatic ? CUI_SCROLL_BODY_AUTO
                                                          : CUI_SCROLL_BODY_EXPLICIT,
                                   .reserved = link.reserved_scrollbar,
                                   .panel = link.panel,
                                   .outer_viewport = link.outer,
                                   .column = link.column,
                                   .body_frame = link.frame,
                                   .viewport = link.viewport,
                                   .content = link.content,
                                   .requested_height = link.automatic ? 0 : link.requested_height,
                                   .max_height = link.max_height};
    return cui_layout_scroll_body_v1(ui, &r, link.outer_y, link.body_y);
}
prototype_stats prototype_get_stats(cui_context *ui) {
    probe_observation *o = observation(ui);
    prototype_stats stats = o->stats;
    stats.commits = 0;
    cui_core_sl_state *state = find_state(ui);
    if (state) {
        stats.bytes = sizeof *state;
        stats.links = state->header.bodies;
        if (ui->scroll_layout == &state->header && (state->header.flags & CUI_CORE_SL_READY))
            for (size_t i = 0; i < state->header.bodies; ++i)
                stats.commits += !state->clamps[state->bodies[i].body_clamp].flags;
    }
    return stats;
}
