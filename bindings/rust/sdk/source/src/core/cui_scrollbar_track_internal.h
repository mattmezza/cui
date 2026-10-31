/* SPDX-License-Identifier: MIT */
/* Private leaf geometry for already registered Y clamps; no input dispatch. */
_Static_assert(sizeof(cui_scrollbar_track_layout_v1) == 48, "scrollbar track V1 ABI");
_Static_assert(offsetof(cui_scrollbar_track_layout_v1, viewport) == 16, "track viewport offset");
_Static_assert(offsetof(cui_scrollbar_track_layout_v1, bar) == 24, "track bar offset");
_Static_assert(offsetof(cui_scrollbar_track_layout_v1, thumb) == 32, "track thumb offset");
_Static_assert(offsetof(cui_scrollbar_track_layout_v1, thickness) == 40, "track dimensions offset");
static cui_semantic *cui_core_sl_track_semantic(cui_context *ui, cui_id id) {
    cui_semantic *found = NULL;
    for (size_t i = 0; i < ui->semantic_count; ++i)
        if (ui->semantics[i].id == id) {
            if (found)
                return NULL;
            found = &ui->semantics[i];
        }
    return found;
}
static bool cui_core_sl_track_topology(cui_context *ui, const cui_scrollbar_track_layout_v1 *r,
                                       bool staged) {
    size_t v = cui_core_sl_node(ui, r->viewport), b = cui_core_sl_node(ui, r->bar),
           t = cui_core_sl_node(ui, r->thumb);
    cui_semantic *owner = cui_core_sl_track_semantic(ui, r->viewport),
                 *bar = cui_core_sl_track_semantic(ui, r->bar);
    if (v == SIZE_MAX || b == SIZE_MAX || t == SIZE_MAX || v == b || v == t || b == t ||
        !cui_core_sl_closed(ui, v) || !cui_core_sl_closed(ui, b) || !cui_core_sl_closed(ui, t) ||
        ui->nodes[b].parent != v || ui->nodes[t].parent != b ||
        cui_core_sl_track_semantic(ui, r->thumb))
        return false;
    bool inactive = ui->nodes[v].inert || ui->nodes[v].layout_only;
    if ((!inactive && (!owner || !bar)) ||
        (bar && (bar->role != CUI_ROLE_SCROLLBAR || bar->details.parent != r->viewport ||
                 bar->details.controls != r->viewport ||
                 bar->details.orientation != CUI_ORIENTATION_VERTICAL)))
        return false;
    cui_id ids[] = {r->viewport, r->bar, r->thumb};
    for (size_t i = 0; i < 3; ++i) {
        if (!ids[i] || !cui_core_folded(ids[i]))
            return false;
        for (size_t j = 0; j < i; ++j)
            if (cui_core_folded(ids[i]) == cui_core_folded(ids[j]))
                return false;
    }
    if (ui->nodes[b].layer != ui->nodes[v].layer || ui->nodes[t].layer != ui->nodes[b].layer ||
        ui->nodes[b].focus_scope != ui->nodes[v].focus_scope ||
        ui->nodes[t].focus_scope != ui->nodes[b].focus_scope)
        return false;
    size_t nodes[] = {b, t};
    for (size_t i = 0; i < 2; ++i) {
        Clay_LayoutElement *e = cui_core_sl_element_node(ui, nodes[i]);
        if (!e || ui->nodes[nodes[i]].prepaint != SIZE_MAX ||
            ui->nodes[nodes[i]].requested_max_width != 0 ||
            ui->nodes[nodes[i]].requested_max_height != 0 ||
            e->config.floating.attachTo != CLAY_ATTACH_TO_PARENT ||
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
bool cui_layout_scrollbar_track_v1(cui_context *ui, const cui_scrollbar_track_layout_v1 *r) {
    if (!cui_core_sl_begin(ui))
        return false;
    if (!r || r->struct_size != 48 || r->version != 1 || r->reserved || r->flags > 3 ||
        r->flags == CUI_SCROLLBAR_TRACK_INTERACTIVE || !isfinite(r->thickness) ||
        r->thickness <= 0 || !isfinite(r->minimum_thumb) || r->minimum_thumb <= 0 ||
        !ui->scroll_layout || !cui_core_sl_track_topology(ui, r, false))
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
    for (size_t i = 0; i < state->header.bodies; ++i) {
        cui_id structural[6];
        cui_core_sl_ids(&state->bodies[i].record, structural);
        for (size_t j = 0; j < 6; ++j)
            if (structural[j] == r->bar || structural[j] == r->thumb)
                return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    }
    for (size_t i = 0; i < state->tracks; ++i) {
        cui_scrollbar_track_layout_v1 old = state->track[i].record;
        if (old.viewport == r->viewport || old.bar == r->bar || old.bar == r->thumb ||
            old.thumb == r->bar || old.thumb == r->thumb)
            return cui_core_sl_fail(ui, CUI_ERROR_DUPLICATE_ID);
    }
    if (state->tracks == 64)
        return cui_core_sl_fail(ui, CUI_ERROR_CAPACITY);
    state->track[state->tracks++] =
        (cui_core_sl_track){.record = *r, .clamp_index = (uint32_t)clamp};
    return true;
}
static bool cui_core_sl_tracks_stage(cui_context *ui, cui_core_sl_state *state) {
    for (size_t i = 0; i < state->tracks; ++i) {
        cui_core_sl_track *p = &state->track[i];
        if (!cui_core_sl_track_topology(ui, &p->record, false))
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
        p->flags =
            (p->record.flags & CUI_SCROLLBAR_TRACK_VISIBLE) && height > 0 && clamp->maximum > 0;
        if (p->flags && (p->record.flags & CUI_SCROLLBAR_TRACK_INTERACTIVE)) {
            bool eligible = true;
            cui_semantic *bar_semantic = cui_core_sl_track_semantic(ui, p->record.bar);
            if (bar_semantic && bar_semantic->details.readonly)
                eligible = false;
            size_t node = cui_core_sl_node(ui, p->record.viewport), visits = 0;
            for (; node != SIZE_MAX && visits < ui->node_count; ++visits) {
                if (node >= ui->node_count)
                    return false;
                cui_semantic *ancestor = cui_core_sl_track_semantic(ui, ui->nodes[node].id);
                if (ui->nodes[node].inert || ui->nodes[node].layout_only ||
                    (ancestor && (ancestor->disabled || ancestor->details.readonly)))
                    eligible = false;
                node = ui->nodes[node].parent;
            }
            if (node != SIZE_MAX)
                return false;
            if (eligible)
                p->flags |= CUI_SCROLLBAR_TRACK_INTERACTIVE;
        }
        p->track =
            (cui_rect){width - p->record.thickness, 0, p->record.thickness, p->flags ? height : 0};
        p->thumb = p->flags ? cui_scrollbar_thumb_geometry(
                                  (cui_rect){0, 0, p->record.thickness, height}, clamp->staged,
                                  height, height + clamp->maximum, true, p->record.minimum_thumb)
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
static bool cui_core_sl_track_rect_equal(cui_rect a, cui_rect b) {
    return cui_core_sl_near(a.x, b.x) && cui_core_sl_near(a.y, b.y) && cui_core_sl_near(a.w, b.w) &&
           cui_core_sl_near(a.h, b.h);
}
static cui_rect cui_core_sl_track_bounds(cui_context *ui, cui_id id) {
    (void)ui;
    Clay_ElementData d = Clay_GetElementData((Clay_ElementId){.id = cui_core_folded(id)});
    return (cui_rect){d.boundingBox.x, d.boundingBox.y, d.boundingBox.width, d.boundingBox.height};
}
static bool cui_core_sl_tracks_validate(cui_context *ui, cui_core_sl_state *state) {
    for (size_t i = 0; i < state->tracks; ++i) {
        cui_core_sl_track *p = &state->track[i];
        if (!cui_core_sl_track_topology(ui, &p->record, true))
            return false;
        cui_rect viewport = cui_core_sl_track_bounds(ui, p->record.viewport);
        cui_rect expected = p->track;
        expected.x += viewport.x;
        expected.y += viewport.y;
        if (!cui_core_sl_track_rect_equal(cui_core_sl_track_bounds(ui, p->record.bar), expected))
            return false;
        expected = p->thumb;
        expected.x += viewport.x + p->track.x;
        expected.y += viewport.y + p->track.y;
        if (!cui_core_sl_track_rect_equal(cui_core_sl_track_bounds(ui, p->record.thumb), expected))
            return false;
    }
    return true;
}
static void cui_core_sl_tracks_publish(cui_context *ui, cui_core_sl_state *state) {
    for (size_t i = 0; i < state->tracks; ++i) {
        cui_core_sl_track *p = &state->track[i];
        cui_core_sl_clamp *clamp = &state->clamps[p->clamp_index];
        cui_semantic *bar = cui_core_sl_track_semantic(ui, p->record.bar);
        if (bar) {
            bar->disabled = !(p->flags & CUI_SCROLLBAR_TRACK_INTERACTIVE);
            bar->value = clamp->staged;
            bar->details.minimum = 0;
            bar->details.maximum = clamp->maximum;
        }
        if (!p->flags) {
            ui->nodes[cui_core_sl_node(ui, p->record.bar)].opacity = 0;
            ui->nodes[cui_core_sl_node(ui, p->record.thumb)].opacity = 0;
            ui->nodes[cui_core_sl_node(ui, p->record.bar)].scrollbar_hidden = true;
            ui->nodes[cui_core_sl_node(ui, p->record.thumb)].scrollbar_hidden = true;
        }
    }
}
