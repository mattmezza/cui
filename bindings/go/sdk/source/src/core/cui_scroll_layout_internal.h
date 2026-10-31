/* SPDX-License-Identifier: MIT */
/* Private stored-tree adapter. Included only after Core/Clay definitions. */
#define CUI_CORE_SL_BODY_MAX 32u
#define CUI_CORE_SL_CLAMP_MAX 64u
#define CUI_CORE_SL_FAILED 1u
#define CUI_CORE_SL_READY 2u
#define CUI_CORE_SL_REGIONS 4u
struct cui_core_scroll_layout_header {
    uint32_t bodies, clamps, flags, regions_slot;
};
typedef struct {
    cui_scroll_body_layout_v1 record;
    float panel_h, inner_h, column_h, body_h, outer_y, body_y;
    uint32_t depth;
    uint16_t outer_clamp, body_clamp;
} cui_core_sl_body;
typedef struct {
    cui_scroll_clamp_v1 record;
    float *destination;
    float staged, maximum;
    uint32_t owner, flags;
} cui_core_sl_clamp;
typedef struct {
    cui_scrollbar_track_layout_v1 record;
    cui_rect track, thumb;
    uint32_t clamp_index, flags;
} cui_core_sl_track;
typedef struct {
    cui_core_scroll_layout_header header;
    cui_core_sl_body bodies[CUI_CORE_SL_BODY_MAX];
    cui_core_sl_clamp clamps[CUI_CORE_SL_CLAMP_MAX];
    uint8_t order[CUI_CORE_SL_BODY_MAX];
    uint32_t tracks, track_reserved;
    cui_core_sl_track track[64];
} cui_core_sl_state;
/* V2-only sidecar: never enlarge the existing V1 base resource. */
typedef struct {
    cui_id content, footer;
    cui_scroll_region_sizing_v1 content_sizing, body_sizing;
    float content_h, content_natural, footer_h;
    uint32_t version;
} cui_core_sl_region;
typedef struct {
    cui_core_sl_region body[CUI_CORE_SL_BODY_MAX];
} cui_core_sl_regions;
#define CUI_CORE_SL_OWNER UINT64_C(0x729a60df47b12e51)
#define CUI_CORE_SL_REGIONS_NAMESPACE UINT64_C(0xc63157e218ad049b)
_Static_assert(sizeof(cui_core_sl_region) == 80, "bounded V2 region entry");
_Static_assert(sizeof(cui_core_sl_regions) == 2560, "bounded V2-only sidecar");
_Static_assert(sizeof(cui_core_sl_state) + sizeof(cui_core_sl_regions) <= 16384,
               "combined shared32/64/64 payload budget");
static const cui_core_scroll_layout_header cui_core_sl_failed = {0, 0, CUI_CORE_SL_FAILED, 0};
_Static_assert(sizeof(cui_scroll_body_layout_v1) == 80, "scroll body V1 ABI");
_Static_assert(offsetof(cui_scroll_body_layout_v1, panel) == 16, "scroll body V1 IDs");
_Static_assert(offsetof(cui_scroll_body_layout_v1, requested_height) == 64,
               "scroll body V1 heights");
_Static_assert(offsetof(cui_scroll_body_layout_v1, reserved2) == 76, "scroll body V1 tail");
_Static_assert(sizeof(cui_scroll_clamp_v1) == 32, "scroll clamp V1 ABI");
_Static_assert(offsetof(cui_scroll_clamp_v1, viewport) == 16, "scroll clamp V1 IDs");
_Static_assert(sizeof(cui_core_sl_state) <= 16384, "bounded scroll layout payload");

static void cui_core_sl_collect(void *payload) {
    memset(payload, 0, sizeof(cui_core_sl_state));
}
static bool cui_core_sl_fail(cui_context *ui, cui_error error) {
    ui->scroll_layout = &cui_core_sl_failed;
    cui_core_fail(ui, error);
    return false;
}
static void cui_core_sl_regions_collect(void *payload) {
    memset(payload, 0, sizeof(cui_core_sl_regions));
}
/* True+NULL means V1-only. False never downgrades corrupted V2 state to V1. */
static bool cui_core_sl_regions_read(cui_context *ui, const cui_core_sl_state *state,
                                     cui_core_sl_regions **out) {
    *out = NULL;
    if (!state->header.regions_slot)
        return (state->header.flags & CUI_CORE_SL_REGIONS) ? cui_core_sl_fail(ui, CUI_ERROR_LAYOUT)
                                                           : true;
    if (!(state->header.flags & CUI_CORE_SL_REGIONS))
        return cui_core_sl_fail(ui, CUI_ERROR_LAYOUT);
    size_t slot = (size_t)state->header.regions_slot - 1;
    if (slot >= ui->config.max_resources)
        return cui_core_sl_fail(ui, CUI_ERROR_LAYOUT);
    cui_core_resource *r = &ui->resources[slot];
    if (!r->payload || r->owner != CUI_CORE_SL_OWNER ||
        r->namespace_id != CUI_CORE_SL_REGIONS_NAMESPACE ||
        r->bytes != sizeof(cui_core_sl_regions) || r->collect != cui_core_sl_regions_collect ||
        r->destroy || !r->touched)
        return cui_core_sl_fail(ui, CUI_ERROR_LAYOUT);
    *out = r->payload;
    return true;
}
static cui_core_sl_regions *cui_core_sl_regions_acquire(cui_context *ui, cui_core_sl_state *state) {
    cui_core_sl_regions *regions;
    if (!cui_core_sl_regions_read(ui, state, &regions))
        return NULL;
    if (regions)
        return regions;
    regions = cui_resource_get(ui, CUI_CORE_SL_OWNER, CUI_CORE_SL_REGIONS_NAMESPACE,
                               sizeof(*regions), cui_core_sl_regions_collect, NULL);
    if (!regions) {
        cui_core_sl_fail(ui, ui->error != CUI_OK ? ui->error : CUI_ERROR_CAPACITY);
        return NULL;
    }
    for (size_t i = 0; i < ui->config.max_resources; ++i)
        if (ui->resources[i].payload == regions && ui->resources[i].owner == CUI_CORE_SL_OWNER &&
            ui->resources[i].namespace_id == CUI_CORE_SL_REGIONS_NAMESPACE) {
            /* cui_create caps max_resources at1,000,000. Stable until next begin. */
            state->header.regions_slot = (uint32_t)i + 1;
            state->header.flags |= CUI_CORE_SL_REGIONS;
            return regions;
        }
    cui_core_sl_fail(ui, CUI_ERROR_LAYOUT);
    return NULL;
}
static bool cui_core_sl_begin(cui_context *ui) {
    if (!cui_core_mutation(ui))
        return false;
    if (!cui_frame_is_submitting(ui)) {
        if (ui)
            cui_core_fail(ui, CUI_ERROR_LIFECYCLE);
        return false;
    }
    if (ui->error != CUI_OK || ui->scroll_layout == &cui_core_sl_failed)
        return cui_core_sl_fail(ui, ui->error != CUI_OK ? ui->error : CUI_ERROR_LAYOUT);
    if (ui->ambiguous_box_ids)
        return cui_core_sl_fail(ui, CUI_ERROR_DUPLICATE_ID);
    cui_core_select_context(ui);
    return true;
}
static cui_core_sl_state *cui_core_sl_state_get(cui_context *ui) {
    if (ui->scroll_layout == &cui_core_sl_failed)
        return NULL; /* Never reinterpret the shared immutable marker as a writable plan. */
    if (ui->scroll_layout)
        return (cui_core_sl_state *)(void *)ui->scroll_layout;
    cui_core_sl_state *state =
        cui_resource_get(ui, UINT64_C(0x729a60df47b12e51), UINT64_C(0x3286e4c53bd97501),
                         sizeof(cui_core_sl_state), cui_core_sl_collect, NULL);
    ui->scroll_layout = state ? &state->header : &cui_core_sl_failed;
    return state;
}
static size_t cui_core_sl_node(cui_context *ui, cui_id id) {
    size_t found = SIZE_MAX;
    if (!id)
        return found;
    for (size_t i = 0; i < ui->node_count; ++i)
        if (ui->nodes[i].id == id) {
            if (found != SIZE_MAX)
                return SIZE_MAX;
            found = i;
        }
    return found;
}
static bool cui_core_sl_closed(cui_context *ui, size_t node) {
    if (node == SIZE_MAX)
        return false;
    for (size_t i = 0; i < ui->depth; ++i)
        if (ui->node_stack[i] == node)
            return false;
    return true;
}
static bool cui_core_sl_descendant(cui_context *ui, size_t child, size_t ancestor) {
    for (size_t n = 0; child != SIZE_MAX && n < ui->node_count; ++n) {
        if (child >= ui->node_count)
            return false;
        if (child == ancestor)
            return true;
        child = ui->nodes[child].parent;
    }
    return false;
}
static void cui_core_sl_ids(const cui_scroll_body_layout_v1 *r, cui_id ids[6]) {
    ids[0] = r->panel;
    ids[1] = r->outer_viewport;
    ids[2] = r->column;
    ids[3] = r->body_frame;
    ids[4] = r->viewport;
    ids[5] = r->content;
}
static bool cui_core_sl_overlap(uintptr_t a, size_t an, uintptr_t b, size_t bn) {
    return a > UINTPTR_MAX - an || b > UINTPTR_MAX - bn || (a < b + bn && b < a + an);
}
static bool cui_core_sl_destination(cui_context *ui, const cui_core_sl_state *state,
                                    float *destination, size_t own) {
    uintptr_t address = (uintptr_t)destination;
    if (!destination || address % _Alignof(float) || address > UINTPTR_MAX - sizeof(float))
        return false;
    for (size_t i = 0; i < ui->bounds_track_count; ++i)
        if (cui_core_sl_overlap(address, sizeof(float), (uintptr_t)ui->bounds_tracks[i].retained,
                                sizeof(cui_rect)))
            return false;
    if (state) {
        cui_core_sl_regions *regions;
        if (!cui_core_sl_regions_read(ui, state, &regions) ||
            (regions &&
             cui_core_sl_overlap(address, sizeof(float), (uintptr_t)regions, sizeof(*regions))))
            return false;
        if (cui_core_sl_overlap(address, sizeof(float), (uintptr_t)state, sizeof *state))
            return false;
        for (size_t i = 0; i < state->header.clamps; ++i)
            if (i != own &&
                cui_core_sl_overlap(address, sizeof(float), (uintptr_t)state->clamps[i].destination,
                                    sizeof(float)))
                return false;
    }
    return isfinite(*destination);
}
static bool cui_core_sl_body_topology(cui_context *ui, const cui_scroll_body_layout_v1 *r,
                                      bool closed) {
    cui_id ids[6];
    size_t nodes[6];
    cui_core_sl_ids(r, ids);
    for (size_t i = 0; i < 6; ++i) {
        nodes[i] = cui_core_sl_node(ui, ids[i]);
        if (nodes[i] == SIZE_MAX || (closed && !cui_core_sl_closed(ui, nodes[i])))
            return false;
        for (size_t j = 0; j < i; ++j)
            if (ids[i] == ids[j])
                return false;
        if (i && ui->nodes[nodes[i]].parent != nodes[i - 1])
            return false;
    }
    return isfinite(ui->nodes[nodes[0]].requested_width) &&
           ui->nodes[nodes[0]].requested_width > 0 &&
           !ui->nodes[nodes[0]].requested_width_percent && ui->nodes[nodes[1]].clip &&
           ui->nodes[nodes[4]].clip && !ui->nodes[nodes[2]].clip && !ui->nodes[nodes[3]].clip &&
           ui->nodes[nodes[4]].layer == ui->nodes[nodes[3]].layer;
}
static bool cui_core_sl_conflicts(cui_context *ui, cui_core_sl_state *state,
                                  const cui_scroll_body_layout_v1 *body,
                                  const cui_scroll_clamp_v1 *clamp) {
    cui_id ids[6] = {0};
    size_t count = body ? 6 : 2;
    if (body)
        cui_core_sl_ids(body, ids);
    else {
        ids[0] = clamp->viewport;
        ids[1] = clamp->content;
    }
    for (size_t i = 0; i < state->header.bodies; ++i) {
        cui_id old[8];
        cui_core_sl_regions *regions;
        if (!cui_core_sl_regions_read(ui, state, &regions))
            return true;
        cui_core_sl_ids(&state->bodies[i].record, old);
        old[6] = regions ? regions->body[i].content : 0;
        old[7] = regions ? regions->body[i].footer : 0;
        for (size_t a = 0; a < count; ++a)
            for (size_t b = 0; b < 8; ++b)
                if (ids[a] && old[b] &&
                    (ids[a] == old[b] || cui_core_folded(ids[a]) == cui_core_folded(old[b])))
                    return true;
        if (clamp && cui_core_sl_descendant(ui, cui_core_sl_node(ui, old[0]),
                                            cui_core_sl_node(ui, clamp->content)))
            return true;
    }
    for (size_t i = 0; i < state->header.clamps; ++i) {
        cui_core_sl_clamp *old = &state->clamps[i];
        if (old->owner)
            continue;
        for (size_t a = 0; a < count; ++a)
            if (ids[a] == old->record.viewport || ids[a] == old->record.content)
                return true;
        if (body && cui_core_sl_descendant(ui, cui_core_sl_node(ui, body->panel),
                                           cui_core_sl_node(ui, old->record.content)))
            return true;
    }
    for (size_t i = 0; i < state->tracks; ++i)
        for (size_t a = 0; a < count; ++a)
            if (ids[a] == state->track[i].record.bar || ids[a] == state->track[i].record.thumb)
                return true;
    return false;
}
static cui_core_sl_clamp cui_core_sl_make_clamp(cui_id viewport, cui_id content, float *y,
                                                uint32_t owner) {
    return (cui_core_sl_clamp){.record = {32, 1, CUI_SCROLL_CLAMP_Y, 0, viewport, content},
                               .destination = y,
                               .owner = owner};
}
static bool cui_core_sl_body_valid(cui_context *ui, const cui_scroll_body_layout_v1 *r,
                                   float *outer_y, float *body_y) {
    return !(!r || r->struct_size != 80 || r->version != 1 || r->reserved || r->reserved2 ||
             r->mode > CUI_SCROLL_BODY_EXPLICIT || !isfinite(r->requested_height) ||
             !isfinite(r->min_height) || !isfinite(r->max_height) || r->min_height < 0 ||
             r->max_height <= 0 || r->min_height > r->max_height ||
             (r->mode == CUI_SCROLL_BODY_AUTO
                  ? r->requested_height != 0
                  : r->requested_height <= 0 || r->requested_height < r->min_height ||
                        r->requested_height > r->max_height) ||
             !cui_core_sl_body_topology(ui, r, true) ||
             !cui_core_sl_destination(ui, NULL, outer_y, SIZE_MAX) ||
             !cui_core_sl_destination(ui, NULL, body_y, SIZE_MAX) || outer_y == body_y);
}
bool cui_layout_scroll_body_v1(cui_context *ui, const cui_scroll_body_layout_v1 *r, float *outer_y,
                               float *body_y) {
    if (!cui_core_sl_begin(ui))
        return false;
    if (!cui_core_sl_body_valid(ui, r, outer_y, body_y))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    cui_core_sl_state *state = cui_core_sl_state_get(ui);
    if (!state)
        return false;
    if (cui_core_sl_conflicts(ui, state, r, NULL) ||
        !cui_core_sl_destination(ui, state, outer_y, SIZE_MAX) ||
        !cui_core_sl_destination(ui, state, body_y, SIZE_MAX))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    if (state->header.bodies == CUI_CORE_SL_BODY_MAX ||
        state->header.clamps > CUI_CORE_SL_CLAMP_MAX - 2)
        return cui_core_sl_fail(ui, CUI_ERROR_CAPACITY);
    uint32_t index = state->header.bodies;
    uint16_t outer = (uint16_t)state->header.clamps;
    state->bodies[index] =
        (cui_core_sl_body){.record = *r, .outer_clamp = outer, .body_clamp = (uint16_t)(outer + 1)};
    state->clamps[outer] = cui_core_sl_make_clamp(r->outer_viewport, r->column, outer_y, index + 1);
    state->clamps[outer + 1] = cui_core_sl_make_clamp(r->viewport, r->content, body_y, index + 1);
    state->header.clamps += 2;
    ++state->header.bodies;
    return true;
}
bool cui_layout_scroll_clamp_v1(cui_context *ui, const cui_scroll_clamp_v1 *r, float *y) {
    if (!cui_core_sl_begin(ui))
        return false;
    if (!r || r->struct_size != 32 || r->version != 1 || r->axes != CUI_SCROLL_CLAMP_Y ||
        r->reserved || r->viewport == r->content ||
        !cui_core_sl_closed(ui, cui_core_sl_node(ui, r->viewport)) ||
        !cui_core_sl_closed(ui, cui_core_sl_node(ui, r->content)) ||
        ui->nodes[cui_core_sl_node(ui, r->content)].parent != cui_core_sl_node(ui, r->viewport) ||
        !ui->nodes[cui_core_sl_node(ui, r->viewport)].clip ||
        ui->nodes[cui_core_sl_node(ui, r->viewport)].requested_height <= 0 ||
        ui->nodes[cui_core_sl_node(ui, r->viewport)].requested_height_percent ||
        !cui_core_sl_destination(ui, NULL, y, SIZE_MAX))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    cui_core_sl_state *state = cui_core_sl_state_get(ui);
    if (!state)
        return false;
    if (cui_core_sl_conflicts(ui, state, NULL, r) ||
        !cui_core_sl_destination(ui, state, y, SIZE_MAX))
        return cui_core_sl_fail(ui, CUI_ERROR_INVALID_ACTION);
    if (state->header.clamps == CUI_CORE_SL_CLAMP_MAX)
        return cui_core_sl_fail(ui, CUI_ERROR_CAPACITY);
    state->clamps[state->header.clamps++] = cui_core_sl_make_clamp(r->viewport, r->content, y, 0);
    return true;
}
static Clay_LayoutElement *cui_core_sl_element_node(cui_context *ui, size_t node) {
    if (node >= ui->node_count)
        return NULL;
    Clay_LayoutElementHashMapItem *item = Clay__GetHashMapItem(ui->nodes[node].clay_id);
    Clay_LayoutElement *e =
        item == &Clay_LayoutElementHashMapItem_DEFAULT ? NULL : item->layoutElement;
    if (!e || e->isTextElement || e->id != ui->nodes[node].clay_id || e->exiting ||
        e->config.userData != (ui->nodes[node].layout_only ? (void *)ui : &ui->nodes[node]))
        return NULL;
    return e;
}
static Clay_LayoutElement *cui_core_sl_element(cui_context *ui, cui_id id) {
    return cui_core_sl_element_node(ui, cui_core_sl_node(ui, id));
}
static bool cui_core_sl_floating(cui_context *ui, const cui_core_sl_state *state,
                                 size_t root_node) {
    for (size_t n = 0; n < ui->node_count; ++n) {
        Clay_LayoutElement *e = cui_core_sl_element_node(ui, n);
        if (!e || n == root_node || e->config.floating.attachTo == CLAY_ATTACH_TO_NONE ||
            !cui_core_sl_descendant(ui, n, root_node))
            continue;
        size_t parent = ui->nodes[n].parent;
        bool registered = false;
        for (size_t b = 0; b < state->header.bodies; ++b)
            registered |= state->bodies[b].record.viewport == ui->nodes[n].id;
        if (parent >= ui->node_count || e->config.floating.attachTo != CLAY_ATTACH_TO_PARENT ||
            e->config.floating.parentId != ui->nodes[parent].clay_id ||
            e->config.floating.clipTo != CLAY_CLIP_TO_ATTACHED_PARENT ||
            (!registered && (e->config.layout.sizing.width.type != CLAY__SIZING_TYPE_FIXED ||
                             e->config.layout.sizing.height.type != CLAY__SIZING_TYPE_FIXED)))
            return false;
    }
    return true;
}
static bool cui_core_sl_only_child(Clay_Context *context, Clay_LayoutElement *parent,
                                   Clay_LayoutElement *child) {
    return parent->children.length == 1 && parent->children.elements[0] >= 0 &&
           parent->children.elements[0] < context->layoutElements.length &&
           Clay_LayoutElementArray_Get(&context->layoutElements, parent->children.elements[0]) ==
               child;
}
static bool cui_core_sl_no_padding(Clay_LayoutElement *e) {
    Clay_Padding p = e->config.layout.padding;
    return !p.left && !p.right && !p.top && !p.bottom;
}
static bool cui_core_sl_tree(cui_context *ui, const cui_core_sl_state *state, Clay_LayoutElement *e,
                             size_t depth) {
    Clay_Context *context = ui->clay;
    if (!e || depth > CUI_CORE_STACK_MAX)
        return false;
    if (e->isTextElement)
        return true;
    if (e->config.aspectRatio.aspectRatio)
        return false;
    if (e->config.layout.sizing.height.type == CLAY__SIZING_TYPE_PERCENT) {
        bool fixed_zero = false;
        for (size_t n = 0; n < ui->node_count; ++n)
            if (ui->nodes[n].clay_id == e->id)
                fixed_zero = ui->nodes[n].requested_fixed_height &&
                             !ui->nodes[n].requested_height_percent &&
                             ui->nodes[n].requested_height == 0;
        if (!fixed_zero)
            return false;
    }
    for (int32_t i = 0; i < e->children.length; ++i) {
        int32_t index = e->children.elements[i];
        if (index < 0 || index >= context->layoutElements.length ||
            !cui_core_sl_tree(
                ui, state, Clay_LayoutElementArray_Get(&context->layoutElements, index), depth + 1))
            return false;
    }
    /* Floating roots are excluded from ordinary flow children. Only actual parent
     * attachments with inherited clips are admitted, never a portal/root fallback. */
    if (e->floatingChildrenCount)
        for (int32_t i = 0; i < context->layoutElementTreeRoots.length; ++i) {
            Clay__LayoutElementTreeRoot *root =
                Clay__LayoutElementTreeRootArray_Get(&context->layoutElementTreeRoots, i);
            if (root->parentId != e->id)
                continue;
            if (root->layoutElementIndex < 0 ||
                root->layoutElementIndex >= context->layoutElements.length)
                return false;
            Clay_LayoutElement *child = Clay_LayoutElementArray_Get(
                &context->layoutElements, (int32_t)root->layoutElementIndex);
            bool registered = false;
            for (size_t b = 0; b < state->header.bodies; ++b) {
                size_t node = cui_core_sl_node(ui, state->bodies[b].record.viewport);
                registered |= node != SIZE_MAX && ui->nodes[node].clay_id == child->id;
            }
            if (child->config.floating.attachTo != CLAY_ATTACH_TO_PARENT ||
                child->config.floating.parentId != e->id ||
                child->config.floating.clipTo != CLAY_CLIP_TO_ATTACHED_PARENT ||
                (!registered &&
                 (child->config.layout.sizing.width.type != CLAY__SIZING_TYPE_FIXED ||
                  child->config.layout.sizing.height.type != CLAY__SIZING_TYPE_FIXED)) ||
                !cui_core_sl_tree(ui, state, child, depth + 1))
                return false;
        }
    return true;
}
static float cui_core_sl_intrinsic(Clay_LayoutElement *e, size_t depth, bool *valid) {
    if (!e || depth > CUI_CORE_STACK_MAX) {
        *valid = false;
        return 0;
    }
    if (e->isTextElement)
        return e->dimensions.height;
    Clay_LayoutConfig *layout = &e->config.layout;
    if (layout->sizing.height.type == CLAY__SIZING_TYPE_FIXED)
        return layout->sizing.height.size.minMax.min;
    if (layout->sizing.height.type == CLAY__SIZING_TYPE_PERCENT)
        return 0; /* Only a canonical fixed-zero axis passed tree validation. */
    float h = 0;
    for (int32_t i = 0; i < e->children.length; ++i) {
        int32_t index = e->children.elements[i];
        if (index < 0 || index >= Clay_GetCurrentContext()->layoutElements.length) {
            *valid = false;
            return 0;
        }
        float child_h = cui_core_sl_intrinsic(
            Clay_LayoutElementArray_Get(&Clay_GetCurrentContext()->layoutElements, index),
            depth + 1, valid);
        h = layout->layoutDirection == CLAY_TOP_TO_BOTTOM ? h + child_h : fmaxf(h, child_h);
    }
    if (layout->layoutDirection == CLAY_TOP_TO_BOTTOM && e->children.length)
        h += (float)(e->children.length - 1) * layout->childGap;
    h += layout->padding.top + layout->padding.bottom;
    h = fminf(fmaxf(h, layout->sizing.height.size.minMax.min),
              layout->sizing.height.size.minMax.max);
    *valid &= isfinite(h);
    return h;
}
static void cui_core_sl_fixed_height(Clay_LayoutElement *e, float height) {
    e->config.layout.sizing.height =
        (Clay_SizingAxis){.size = {.minMax = {height, height}}, .type = CLAY__SIZING_TYPE_FIXED};
    e->dimensions.height = e->minDimensions.height = height;
}
static Clay_RenderCommandArray cui_core_sl_abort(cui_context *ui) {
    cui_core_sl_fail(ui, ui->error == CUI_OK ? CUI_ERROR_LAYOUT : ui->error);
    ui->prepaint_capacity = true; /* No prepaint receives discarded opt-in geometry. */
    return (Clay_RenderCommandArray){0};
}
static bool cui_core_sl_near(float a, float b) {
    return isfinite(a) && isfinite(b) && fabsf(a - b) < .05f;
}
static uint64_t cui_core_sl_widths(Clay_Context *context) {
    uint64_t hash = UINT64_C(1469598103934665603);
    for (int32_t i = 0; i < context->layoutElements.length; ++i) {
        uint32_t bits;
        memcpy(&bits, &context->layoutElements.internalArray[i].dimensions.width, sizeof bits);
        hash = (hash ^ bits) * UINT64_C(1099511628211);
    }
    return hash;
}
#include "cui_scroll_regions_internal.h"
#include "cui_scrollbar_track_internal.h"
static Clay_RenderCommandArray cui_core_scroll_layout_end(cui_context *ui) {
    Clay_RenderCommandArray trial = Clay_EndLayout(ui->dt);
    if (!ui->scroll_layout)
        return trial;
    if (ui->scroll_layout == &cui_core_sl_failed || ui->error != CUI_OK || ui->ambiguous_box_ids)
        return cui_core_sl_abort(ui);
    cui_core_sl_state *state = (cui_core_sl_state *)(void *)ui->scroll_layout;
    cui_core_sl_regions *regions;
    if (!cui_core_sl_regions_read(ui, state, &regions))
        return cui_core_sl_abort(ui);
    for (size_t i = 0; i < state->header.bodies; ++i) {
        uint32_t version = state->bodies[i].record.version;
        if ((version != 1 && version != 2) ||
            (version == 2 && (!regions || regions->body[i].version != 2)) ||
            (version == 1 && regions && regions->body[i].version != 0))
            return cui_core_sl_abort(ui);
    }
    Clay_Context *context = Clay_GetCurrentContext();
    if (context != ui->clay || context->debugModeEnabled || context->transitionDatas.length)
        return cui_core_sl_abort(ui);
    for (size_t i = 0; i < state->header.clamps; ++i)
        if (!cui_core_sl_destination(ui, state, state->clamps[i].destination, i))
            return cui_core_sl_abort(ui);
    for (size_t i = 0; i < state->header.bodies; ++i) {
        cui_core_sl_body *p = &state->bodies[i];
        if (!cui_core_sl_body_topology(ui, &p->record, false))
            return cui_core_sl_abort(ui);
        size_t n = cui_core_sl_node(ui, p->record.panel);
        for (; n != SIZE_MAX; n = ui->nodes[n].parent) {
            if (n >= ui->node_count || ++p->depth > ui->node_count)
                return cui_core_sl_abort(ui);
        }
        state->order[i] = (uint8_t)i;
        for (size_t j = i; j && state->bodies[state->order[j - 1]].depth < p->depth; --j) {
            state->order[j] = state->order[j - 1];
            state->order[j - 1] = (uint8_t)i;
        }
    }
    bool valid = true;
    for (size_t i = 0; i < state->header.bodies; ++i) {
        cui_core_sl_body *p = &state->bodies[state->order[i]];
        const cui_scroll_body_layout_v1 *r = &p->record;
        cui_core_sl_region *v = r->version == 2 ? &regions->body[state->order[i]] : NULL;
        Clay_LayoutElement *panel = cui_core_sl_element(ui, r->panel);
        Clay_LayoutElement *outer = cui_core_sl_element(ui, r->outer_viewport);
        Clay_LayoutElement *column = cui_core_sl_element(ui, r->column);
        Clay_LayoutElement *frame = cui_core_sl_element(ui, r->body_frame);
        Clay_LayoutElement *viewport = cui_core_sl_element(ui, r->viewport);
        Clay_LayoutElement *content = cui_core_sl_element(ui, r->content);
        if (!panel || !outer || !column || !frame || !viewport || !content ||
            panel->config.layout.sizing.width.type != CLAY__SIZING_TYPE_FIXED ||
            outer->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
            column->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
            frame->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
            content->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
            viewport->config.floating.attachTo != CLAY_ATTACH_TO_PARENT ||
            viewport->config.floating.parentId != frame->id ||
            viewport->config.floating.clipTo != CLAY_CLIP_TO_ATTACHED_PARENT ||
            column->config.layout.layoutDirection != CLAY_TOP_TO_BOTTOM ||
            outer->config.layout.childAlignment.y != CLAY_ALIGN_Y_TOP ||
            viewport->config.layout.childAlignment.y != CLAY_ALIGN_Y_TOP ||
            viewport->config.layout.sizing.width.type != CLAY__SIZING_TYPE_GROW ||
            viewport->config.layout.sizing.height.type != CLAY__SIZING_TYPE_GROW ||
            viewport->config.floating.offset.x != 0 || viewport->config.floating.offset.y != 0 ||
            !cui_core_sl_no_padding(outer) || !cui_core_sl_no_padding(frame) ||
            !cui_core_sl_no_padding(viewport) || frame->children.length != 0 ||
            (!v && !cui_core_sl_only_child(context, panel, outer)) ||
            (v && !cui_core_sl_region_panel(ui, p, v)) ||
            !cui_core_sl_only_child(context, outer, column) ||
            !cui_core_sl_only_child(context, viewport, content) ||
            !cui_core_sl_floating(ui, state, cui_core_sl_node(ui, r->panel)) ||
            !cui_core_sl_tree(ui, state, panel, 0))
            return cui_core_sl_abort(ui);
        float natural = cui_core_sl_intrinsic(content, 0, &valid);
        float fixed = column->config.layout.padding.top + column->config.layout.padding.bottom;
        bool found_frame = false;
        for (int32_t j = 0; j < column->children.length; ++j) {
            Clay_LayoutElement *child =
                Clay_LayoutElementArray_Get(&context->layoutElements, column->children.elements[j]);
            if (child == frame)
                found_frame = true;
            else if (!v || child != cui_core_sl_element(ui, v->content))
                fixed += cui_core_sl_intrinsic(child, 0, &valid);
        }
        fixed += (float)(column->children.length ? column->children.length - 1 : 0) *
                 column->config.layout.childGap;
        float padding = panel->config.layout.padding.top + panel->config.layout.padding.bottom;
        if (v) {
            if (!cui_core_sl_region_solve(ui, p, v, natural, fixed, padding, &valid))
                return cui_core_sl_abort(ui);
        } else {
            p->panel_h = r->mode == CUI_SCROLL_BODY_AUTO
                             ? fminf(r->max_height, fmaxf(r->min_height, padding + fixed + natural))
                             : r->requested_height;
            p->inner_h = fmaxf(0, p->panel_h - padding);
            p->body_h = fixed > p->inner_h ? natural : fmaxf(0, p->inner_h - fixed);
            p->column_h = fmaxf(p->inner_h, fixed + p->body_h);
        }
        cui_core_sl_clamp *a = &state->clamps[p->outer_clamp], *b = &state->clamps[p->body_clamp];
        a->maximum = fmaxf(0, p->column_h - p->inner_h);
        b->maximum = fmaxf(0, natural - p->body_h);
        a->staged = p->outer_y = fminf(fmaxf(0, *a->destination), a->maximum);
        b->staged = p->body_y = fminf(fmaxf(0, *b->destination), b->maximum);
        if (!valid || !found_frame || !isfinite(p->column_h) || !isfinite(fixed))
            return cui_core_sl_abort(ui);
        cui_core_sl_fixed_height(panel, p->panel_h);
        cui_core_sl_fixed_height(outer, p->inner_h);
        cui_core_sl_fixed_height(column, p->column_h);
        cui_core_sl_fixed_height(frame, p->body_h);
        outer->config.clip.childOffset.y = -a->staged;
        viewport->config.clip.childOffset.y = -b->staged;
    }
    for (size_t i = 0; i < state->header.clamps; ++i) {
        cui_core_sl_clamp *p = &state->clamps[i];
        size_t node = cui_core_sl_node(ui, p->record.viewport);
        Clay_LayoutElement *viewport = cui_core_sl_element(ui, p->record.viewport);
        Clay_LayoutElement *content = cui_core_sl_element(ui, p->record.content);
        if (node == SIZE_MAX || !viewport || !content || !ui->nodes[node].clip)
            return cui_core_sl_abort(ui);
        p->flags = ui->nodes[node].inert || ui->nodes[node].layout_only;
        if (!p->owner) {
            if (viewport->config.layout.sizing.height.type != CLAY__SIZING_TYPE_FIXED ||
                viewport->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
                content->config.floating.attachTo != CLAY_ATTACH_TO_NONE ||
                viewport->config.layout.childAlignment.y != CLAY_ALIGN_Y_TOP ||
                !cui_core_sl_floating(ui, state, node) || !cui_core_sl_tree(ui, state, viewport, 0))
                return cui_core_sl_abort(ui);
            p->maximum = fmaxf(0, content->dimensions.height - viewport->dimensions.height);
            p->staged = fminf(fmaxf(0, *p->destination), p->maximum);
            viewport->config.clip.childOffset.y = -p->staged;
        }
    }
    if (!cui_core_sl_tracks_stage(ui, state))
        return cui_core_sl_abort(ui);
    uint64_t widths = cui_core_sl_widths(context);
    size_t strings = ui->string_used, sources = ui->text_source_count,
           styles = ui->text_style_count;
    size_t resource_bytes = ui->resource_bytes;
    int32_t wrapped = context->wrappedTextLines.length;
    int32_t dynamic_strings = context->dynamicStringData.length;
    context->wrappedTextLines.length = 0;
    Clay__CalculateFinalLayout(0, false, true);
    cui_core_sl_regions *verified_regions;
    if (!cui_core_sl_regions_read(ui, state, &verified_regions) || verified_regions != regions)
        return cui_core_sl_abort(ui);
    if (!cui_core_sl_tracks_validate(ui, state))
        return cui_core_sl_abort(ui);
    if (ui->error != CUI_OK || ui->wrap_failed || widths != cui_core_sl_widths(context) ||
        strings != ui->string_used || sources != ui->text_source_count ||
        styles != ui->text_style_count || resource_bytes != ui->resource_bytes ||
        wrapped != context->wrappedTextLines.length ||
        dynamic_strings != context->dynamicStringData.length)
        return cui_core_sl_abort(ui);
    for (size_t i = 0; i < state->header.bodies; ++i) {
        cui_core_sl_body *p = &state->bodies[i];
        if (p->record.version == 2 && (!regions || regions->body[i].version != 2 ||
                                       !cui_core_sl_region_final(ui, &regions->body[i])))
            return cui_core_sl_abort(ui);
        if (!cui_core_sl_near(cui_core_sl_element(ui, p->record.panel)->dimensions.height,
                              p->panel_h) ||
            !cui_core_sl_near(cui_core_sl_element(ui, p->record.outer_viewport)->dimensions.height,
                              p->inner_h) ||
            !cui_core_sl_near(cui_core_sl_element(ui, p->record.column)->dimensions.height,
                              p->column_h) ||
            !cui_core_sl_near(cui_core_sl_element(ui, p->record.body_frame)->dimensions.height,
                              p->body_h) ||
            !cui_core_sl_near(cui_core_sl_element(ui, p->record.viewport)->dimensions.height,
                              p->body_h))
            return cui_core_sl_abort(ui);
    }
    for (size_t i = 0; i < state->header.clamps; ++i) {
        cui_core_sl_clamp *p = &state->clamps[i];
        Clay_LayoutElement *viewport = cui_core_sl_element(ui, p->record.viewport);
        Clay_LayoutElement *content = cui_core_sl_element(ui, p->record.content);
        if (!viewport || !content ||
            !cui_core_sl_near(fmaxf(0, content->dimensions.height - viewport->dimensions.height),
                              p->maximum) ||
            !cui_core_sl_destination(ui, state, p->destination, i))
            return cui_core_sl_abort(ui);
    }
    /* All plans validated. No application/input/resource callback occurs in this commit. */
    for (size_t i = 0; i < state->header.clamps; ++i) {
        cui_core_sl_clamp *p = &state->clamps[i];
        if (!p->flags)
            *p->destination = p->staged;
        for (size_t n = 0; n < ui->semantic_count; ++n)
            if (ui->semantics[n].id == p->record.viewport) {
                ui->semantics[n].details.scroll_y = p->staged;
                ui->semantics[n].details.scroll_max_y = p->maximum;
            }
    }
    cui_core_sl_tracks_publish(ui, state);
    state->header.flags = CUI_CORE_SL_READY | (state->header.flags & CUI_CORE_SL_REGIONS);
    return context->renderCommands;
}
