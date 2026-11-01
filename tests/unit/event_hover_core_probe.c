/* SPDX-License-Identifier: MIT */
/* Exact production Core in this TU; test-only poison/topology witnesses below. */
#ifdef CUI_EVENT_HOVER_DISTRIBUTION
#define CUI_IMPLEMENTATION
#include "cui.h"
#else
#include "../../src/core/cui.c"
#endif
/* Test-only poison witness; preserves event record and query has no mutation. */
bool event_hover_nonfinite(cui_context *ui, size_t index, cui_id id) {
    if (!ui || index >= ui->event_count)
        return false;
    float x = ui->events[index].x, y = ui->events[index].y;
    ui->events[index].x = NAN;
    bool rejected = !cui_event_hovered(ui, index, id);
    ui->events[index].x = x;
    ui->events[index].y = INFINITY;
    rejected &= !cui_event_hovered(ui, index, id);
    ui->events[index].y = y;
    return rejected;
}
/* Read-only actual retained topology witness, independent of query admission.
 * Test-only outputs; no corresponding public declaration or state. */
bool event_hover_topology(const cui_context *ui, cui_id a, cui_id b, uint32_t roots[2],
                          int16_t layers[2], bool *descendant) {
    if (!ui || !roots || !layers || !descendant)
        return false;
    size_t indices[2] = {SIZE_MAX, SIZE_MAX};
    for (size_t i = 0; i < ui->previous_count; ++i) {
        if (ui->previous[i].id == a)
            indices[0] = i;
        if (ui->previous[i].id == b)
            indices[1] = i;
    }
    if (indices[0] == SIZE_MAX || indices[1] == SIZE_MAX)
        return false;
    bool related = false;
    size_t parent = ui->previous_parents[indices[1]];
    for (size_t steps = 0; parent != SIZE_MAX; ++steps) {
        if (parent >= ui->previous_count || steps >= ui->previous_count)
            return false;
        related |= parent == indices[0];
        parent = ui->previous_parents[parent];
    }
    for (size_t i = 0; i < 2; ++i) {
        roots[i] = (uint32_t)(ui->previous_order[indices[i]] >> 32);
        layers[i] = ui->previous_layers[indices[i]];
    }
    *descendant = related;
    return true;
}
