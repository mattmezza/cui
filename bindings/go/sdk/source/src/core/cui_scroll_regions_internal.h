/* SPDX-License-Identifier: MIT */
/* V2 current intrinsic peer allocation; Core-only, no input or composition replay. */
_Static_assert(sizeof(cui_scroll_region_sizing_v1) == 24, "region sizing V1 ABI");
_Static_assert(sizeof(cui_scroll_body_layout_v2) == 144, "body layout V2 ABI");
_Static_assert(offsetof(cui_scroll_body_layout_v2, content_region) == 80, "V2 Content ID");
_Static_assert(offsetof(cui_scroll_body_layout_v2, footer) == 88, "V2 Footer ID");
_Static_assert(offsetof(cui_scroll_body_layout_v2, content_sizing) == 96, "V2 Content sizing");
_Static_assert(offsetof(cui_scroll_body_layout_v2, body_sizing) == 120, "V2 Body sizing");
static bool cui_core_sl_region_policy_valid(cui_scroll_region_sizing_v1 p, bool absent) {
    if (p.reserved || p.mode > CUI_SCROLL_REGION_FIXED || p.minimum_policy > 1)
        return false;
    if (p.mode == CUI_SCROLL_REGION_OMIT)
        return absent && p.minimum_policy == 0 && p.requested_height == 0 && p.min_height == 0 &&
               p.max_height == 0;
    return !absent && isfinite(p.requested_height) && isfinite(p.min_height) &&
           isfinite(p.max_height) && p.min_height >= 0 && p.max_height > 0 &&
           p.min_height <= p.max_height &&
           (p.mode == CUI_SCROLL_REGION_FIXED
                ? p.requested_height > 0 && p.requested_height >= p.min_height &&
                      p.requested_height <= p.max_height
                : p.requested_height == 0);
}
static bool cui_core_sl_region_panel(cui_context *ui, const cui_core_sl_body *p,
                                     const cui_core_sl_region *v) {
    Clay_Context *context = Clay_GetCurrentContext();
    Clay_LayoutElement *panel = cui_core_sl_element(ui, p->record.panel),
                       *outer = cui_core_sl_element(ui, p->record.outer_viewport),
                       *column = cui_core_sl_element(ui, p->record.column),
                       *footer = v->footer ? cui_core_sl_element(ui, v->footer) : NULL,
                       *content = v->content ? cui_core_sl_element(ui, v->content) : NULL;
    if (!panel || !outer || !column || column->config.layout.childGap ||
        panel->children.length != (footer ? 2 : 1) ||
        Clay_LayoutElementArray_Get(&context->layoutElements, panel->children.elements[0]) != outer)
        return false;
    if (v->footer && (!footer || !cui_core_sl_closed(ui, cui_core_sl_node(ui, v->footer)) ||
                      ui->nodes[cui_core_sl_node(ui, v->footer)].parent !=
                          cui_core_sl_node(ui, p->record.panel) ||
                      Clay_LayoutElementArray_Get(&context->layoutElements,
                                                  panel->children.elements[1]) != footer ||
                      footer->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
                      footer->config.layout.sizing.height.type == CLAY__SIZING_TYPE_GROW))
        return false;
    if (v->content && (!content || !cui_core_sl_closed(ui, cui_core_sl_node(ui, v->content)) ||
                       ui->nodes[cui_core_sl_node(ui, v->content)].parent !=
                           cui_core_sl_node(ui, p->record.column) ||
                       content->config.floating.attachTo != CLAY_ATTACH_TO_NONE))
        return false;
    bool found_content = !content, found_frame = false;
    for (int32_t i = 0; i < column->children.length; ++i) {
        int32_t child = column->children.elements[i];
        if (child < 0 || child >= context->layoutElements.length)
            return false;
        Clay_LayoutElement *e = Clay_LayoutElementArray_Get(&context->layoutElements, child);
        if (e == content) {
            if (found_frame || found_content)
                return false;
            found_content = true;
        }
        if (e == cui_core_sl_element(ui, p->record.body_frame))
            found_frame = true;
    }
    return found_content && found_frame;
}
static float cui_core_sl_region_clamp(float value, float minimum, float maximum) {
    return fminf(maximum, fmaxf(minimum, value));
}
static float cui_core_sl_region_demand(cui_scroll_region_sizing_v1 p, float natural) {
    return p.mode == CUI_SCROLL_REGION_OMIT ? 0
           : p.mode == CUI_SCROLL_REGION_FIXED
               ? p.requested_height
               : cui_core_sl_region_clamp(natural, p.min_height, p.max_height);
}
static bool cui_core_sl_region_solve(cui_context *ui, cui_core_sl_body *p, cui_core_sl_region *v,
                                     float natural_body, float fixed, float padding, bool *valid) {
    Clay_LayoutElement *panel = cui_core_sl_element(ui, p->record.panel),
                       *content = v->content ? cui_core_sl_element(ui, v->content) : NULL,
                       *footer = v->footer ? cui_core_sl_element(ui, v->footer) : NULL;
    float natural[2] = {content ? cui_core_sl_intrinsic(content, 0, valid) : 0, natural_body};
    v->content_natural = natural[0];
    v->footer_h = footer ? cui_core_sl_intrinsic(footer, 0, valid) : 0;
    float footer_total = v->footer_h + (footer ? panel->config.layout.childGap : 0);
    cui_scroll_region_sizing_v1 policy[2] = {v->content_sizing, v->body_sizing};
    float demand = cui_core_sl_region_demand(policy[0], natural[0]) +
                   cui_core_sl_region_demand(policy[1], natural[1]);
    if (!*valid || !isfinite(footer_total) || !isfinite(demand) ||
        !isfinite(padding + fixed + footer_total + demand))
        return false;
    p->panel_h = p->record.mode == CUI_SCROLL_BODY_AUTO
                     ? cui_core_sl_region_clamp(padding + fixed + footer_total + demand,
                                                p->record.min_height, p->record.max_height)
                     : p->record.requested_height;
    p->inner_h = fmaxf(0, p->panel_h - padding - footer_total);
    float remaining = p->inner_h - fixed, heights[2] = {0}, minimum[2] = {0}, maximum[2] = {0};
    bool active[2] = {false};
    unsigned count = 0;
    for (unsigned i = 0; i < 2; ++i) {
        if (policy[i].mode == CUI_SCROLL_REGION_FLEX1) {
            minimum[i] = fmaxf(policy[i].min_height,
                               policy[i].minimum_policy == CUI_SCROLL_REGION_MIN_INTRINSIC
                                   ? fminf(natural[i], policy[i].max_height)
                                   : 0);
            maximum[i] = policy[i].max_height;
            active[i] = true;
            ++count;
        } else {
            heights[i] = cui_core_sl_region_demand(policy[i], natural[i]);
            remaining -= heights[i];
        }
    }
    for (unsigned pass = 0; count && pass < 3; ++pass) {
        float trial = fmaxf(0, remaining) / (float)count, violation = 0, proposed[2] = {0};
        for (unsigned i = 0; i < 2; ++i)
            if (active[i]) {
                proposed[i] = cui_core_sl_region_clamp(trial, minimum[i], maximum[i]);
                violation += proposed[i] - trial;
            }
        unsigned frozen = 0;
        for (unsigned i = 0; i < 2; ++i)
            if (active[i] && (violation == 0 || (violation > 0 && proposed[i] > trial) ||
                              (violation < 0 && proposed[i] < trial))) {
                heights[i] = proposed[i];
                remaining -= proposed[i];
                active[i] = false;
                --count;
                ++frozen;
            }
        if (!frozen)
            return false;
    }
    if (count || !*valid || !isfinite(p->panel_h) || !isfinite(p->inner_h) ||
        !isfinite(heights[0]) || !isfinite(heights[1]))
        return false;
    v->content_h = heights[0];
    p->body_h = heights[1];
    p->column_h = p->inner_h; /* Source inner clip; Footer is outside this group. */
    if (content)
        cui_core_sl_fixed_height(content, heights[0]);
    if (footer)
        cui_core_sl_fixed_height(footer, v->footer_h);
    return true;
}
static bool cui_core_sl_region_final(cui_context *ui, const cui_core_sl_region *v) {
    return (!v->content || cui_core_sl_near(cui_core_sl_element(ui, v->content)->dimensions.height,
                                            v->content_h)) &&
           (!v->footer ||
            cui_core_sl_near(cui_core_sl_element(ui, v->footer)->dimensions.height, v->footer_h));
}

static bool cui_core_sl_region_alias(cui_id a, cui_id b) {
    return a && b && (a == b || cui_core_folded(a) == cui_core_folded(b));
}
bool cui_layout_scroll_body_v2(cui_context *ui, const cui_scroll_body_layout_v2 *r, float *outer_y,
                               float *body_y) {
    if (!cui_core_sl_begin(ui))
        return false;
    if (!r || r->base.struct_size != 144 || r->base.version != 2 ||
        !cui_core_sl_region_policy_valid(r->content_sizing, !r->content_region) ||
        !cui_core_sl_region_policy_valid(r->body_sizing,
                                         r->body_sizing.mode == CUI_SCROLL_REGION_OMIT))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    cui_scroll_body_layout_v1 base = r->base;
    base.struct_size = 80;
    base.version = 1;
    if (!cui_core_sl_body_valid(ui, &base, outer_y, body_y))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    cui_core_sl_region value = {.content = r->content_region,
                                .footer = r->footer,
                                .content_sizing = r->content_sizing,
                                .body_sizing = r->body_sizing,
                                .version = 2};
    cui_core_sl_body trial = {.record = base};
    cui_id ids[8];
    cui_core_sl_ids(&base, ids);
    ids[6] = value.content;
    ids[7] = value.footer;
    for (size_t i = 0; i < 8; ++i) {
        if (!ids[i])
            continue;
        if (!cui_core_folded(ids[i]) || !cui_core_sl_closed(ui, cui_core_sl_node(ui, ids[i])))
            return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
        for (size_t j = 0; j < i; ++j)
            if (cui_core_sl_region_alias(ids[i], ids[j]))
                return cui_core_sl_fail(ui, CUI_ERROR_DUPLICATE_ID);
    }
    if (!cui_core_sl_region_panel(ui, &trial, &value))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    cui_core_sl_state *state = cui_core_sl_state_get(ui);
    if (!state)
        return false;
    cui_core_sl_regions *regions;
    if (!cui_core_sl_regions_read(ui, state, &regions))
        return false;
    for (size_t i = 0; i < state->header.bodies; ++i) {
        cui_id old[8];
        cui_core_sl_ids(&state->bodies[i].record, old);
        old[6] = regions ? regions->body[i].content : 0;
        old[7] = regions ? regions->body[i].footer : 0;
        for (size_t a = 0; a < 8; ++a)
            for (size_t b = 0; b < 8; ++b)
                if (cui_core_sl_region_alias(ids[a], old[b]))
                    return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    }
    for (size_t i = 0; i < state->header.clamps; ++i)
        for (size_t a = 0; a < 8; ++a)
            if (cui_core_sl_region_alias(ids[a], state->clamps[i].record.viewport) ||
                cui_core_sl_region_alias(ids[a], state->clamps[i].record.content))
                return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    for (size_t i = 0; i < state->tracks; ++i)
        for (size_t a = 0; a < 8; ++a)
            if (cui_core_sl_region_alias(ids[a], state->track[i].record.bar) ||
                cui_core_sl_region_alias(ids[a], state->track[i].record.thumb))
                return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    if (state->header.bodies == CUI_CORE_SL_BODY_MAX ||
        state->header.clamps > CUI_CORE_SL_CLAMP_MAX - 2)
        return cui_core_sl_fail(ui, CUI_ERROR_CAPACITY);
    regions = cui_core_sl_regions_acquire(ui, state);
    if (!regions || !cui_layout_scroll_body_v1(ui, &base, outer_y, body_y))
        return false;
    size_t index = state->header.bodies - 1;
    state->bodies[index].record.version = 2; /* Independent marker prevents V1 downgrade. */
    regions->body[index] = value;
    return true;
}
