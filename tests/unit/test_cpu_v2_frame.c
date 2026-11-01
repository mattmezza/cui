/* SPDX-License-Identifier: MIT */
#include "cpu_v2_common_reference_internal.h"
#include "cpu_v2_frame_layout_proposal.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SAME_LAYOUT(field)                                                                         \
    _Static_assert(offsetof(cui_cpu_v2_frame, field) ==                                            \
                       offsetof(cui_cpu_v2_frame_proposed, field),                                 \
                   "measured frame offset changed")
_Static_assert(sizeof(cui_cpu_v2_frame) == sizeof(cui_cpu_v2_frame_proposed),
               "measured frame size changed");
_Static_assert(_Alignof(cui_cpu_v2_frame) == _Alignof(cui_cpu_v2_frame_proposed),
               "measured alignment changed");
_Static_assert(sizeof(cui_cpu_v2_frame_request) == sizeof(cui_cpu_v2_frame_request_proposed),
               "measured request changed");
SAME_LAYOUT(state);
SAME_LAYOUT(self);
SAME_LAYOUT(workspace);
SAME_LAYOUT(pixels);
SAME_LAYOUT(width);
SAME_LAYOUT(height);
SAME_LAYOUT(stride);
SAME_LAYOUT(bytes);
#ifdef CUI_HAS_TEXT
SAME_LAYOUT(text_owner);
SAME_LAYOUT(text_session);
#endif
#undef SAME_LAYOUT
typedef struct {
    cui_cpu_v2_frame *frame;
    const cui_cpu_v2_frame_request *request;
    const cui_draw_list_v2 *list;
    const cui_render_v2_options *options;
    cui_font *font;
    size_t allocations, frees, live, fail;
    void *workspace_blocks[2];
    bool owner_busy;
} trace;
static void *allocate(void *, size_t);
static void release(void *, void *);
static void reenter(trace *t, cui_cpu_v2_frame_state state) {
    assert(t->owner_busy && t->frame->state == state);
    unsigned char saved[sizeof(*t->frame)];
    memcpy(saved, t->frame, sizeof saved);
    cui_cpu_v2_frame_release(t->frame);
    cui_cpu_v2_frame_init(t->frame);
    assert(memcmp(saved, t->frame, sizeof saved) == 0);
    cui_cpu_v2_frame distinct = {0};
    unsigned char distinct_saved[sizeof distinct];
    memcpy(distinct_saved, &distinct, sizeof distinct);
    assert(cui_cpu_v2_frame_prepare(&distinct, t->request, t->list, (cui_color){0}, t->options,
                                    t->font, allocate, release, t) == CUI_ERROR_LIFECYCLE);
    assert(memcmp(distinct_saved, &distinct, sizeof distinct) == 0);

    assert(cui_cpu_v2_frame_prepare(t->frame, t->request, t->list, (cui_color){0}, t->options,
                                    t->font, allocate, release, t) == CUI_ERROR_LIFECYCLE);
#ifdef CUI_HAS_TEXT
    assert(!t->frame->text_session.owner || t->frame->text_session.owner == &t->frame->text_owner);
#endif
}
static void *allocate(void *user, size_t bytes) {
    trace *t = user;
    reenter(t, CUI_CPU_V2_FRAME_PREPARING);
    if (++t->allocations == t->fail)
        return NULL;
    void *p = malloc(bytes);
    assert(p);
    if (t->allocations <= 2)
        t->workspace_blocks[t->allocations - 1] = p;
    ++t->live;
    return p;
}
static void release(void *user, void *p) {
    trace *t = user;
    /* Frame-owned workspace always tears down in RELEASING, including failed
     * second-block reservation. Counted shaping temporaries may normally free
     * during PREPARING inside the text recorder before it returns. */
    if (p == t->workspace_blocks[0] || p == t->workspace_blocks[1])
        assert(t->frame->state == CUI_CPU_V2_FRAME_RELEASING);
    else
        assert(t->frame->state == CUI_CPU_V2_FRAME_PREPARING ||
               t->frame->state == CUI_CPU_V2_FRAME_RELEASING);
    reenter(t, t->frame->state);
    assert(t->live);
    --t->live;
    ++t->frees;
    free(p);
}
static void *original_allocate(void *user, size_t bytes) {
    (void)user;
    return malloc(bytes);
}
static void original_release(void *user, void *p) {
    (void)user;
    free(p);
}
static cui_render_v2_options options(void) {
    return (cui_render_v2_options){.size = sizeof(cui_render_v2_options),
                                   .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                                   .viewport_width = 16,
                                   .viewport_height = 12,
                                   .scale_x = 1.25f,
                                   .scale_y = .75f,
                                   .origin_x = .25f,
                                   .origin_y = -.25f,
                                   .cpu_bytes = 32 * 1024 * 1024,
                                   .metadata_bytes = 1024 * 1024,
                                   .texture_bytes = 16 * 1024 * 1024,
                                   .transfer_bytes = 16 * 1024 * 1024,
                                   .max_pixels = 8 * 1024 * 1024,
                                   .max_pixels_per_background = 4 * 1024 * 1024};
}
static cui_draw_list_v2 list(cui_draw_command_v2 *commands, size_t count) {
    return (cui_draw_list_v2){.header_size = sizeof(cui_draw_list_v2),
                              .abi_version = CUI_DRAW_ABI_V2,
                              .command_stride = sizeof(*commands),
                              .background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1,
                              .commands = commands,
                              .count = count,
                              .width = 16,
                              .height = 12};
}
static cui_error prepare(trace *t) {
    return cui_cpu_v2_frame_prepare(t->frame, t->request, t->list, (cui_color){7, 19, 31, 53},
                                    t->options, t->font, allocate, release, t);
}
static void invalid_unchanged(trace *t) {
    unsigned char saved[sizeof(*t->frame)];
    memcpy(saved, t->frame, sizeof saved);
    size_t allocations = t->allocations;
    assert(prepare(t) == CUI_ERROR_INVALID_ACTION);
    assert(!t->owner_busy && t->allocations == allocations && !t->live);
    assert(memcmp(saved, t->frame, sizeof saved) == 0);
}
int main(int argc, char **argv) {
    cui_font *font = NULL;
#ifdef CUI_HAS_TEXT
    assert(argc == 2);
    font = cui_font_open(argv[1]);
    assert(font);
#else
    (void)argc;
    (void)argv;
#endif
    cui_draw_command_v2 commands[2] = {{.kind = CUI_DRAW_RECT,
                                        .payload.legacy = {.type = CUI_DRAW_RECT,
                                                           .bounds = {1, 2, 8, 6},
                                                           .color = {91, 33, 201, 173},
                                                           .radius = 2}}};
    size_t count = 1;
#ifdef CUI_HAS_TEXT
    commands[count++] = (cui_draw_command_v2){.kind = CUI_DRAW_TEXT,
                                              .payload.legacy = {.type = CUI_DRAW_TEXT,
                                                                 .bounds = {0, 0, 0, 0},
                                                                 .text = {"Affi", 4},
                                                                 .font_size = 9,
                                                                 .color = {233, 61, 17, 193}}};
#endif
    cui_draw_list_v2 input = list(commands, count);
    cui_render_v2_options o = options();
    cui_cpu_v2_frame f = {0};
    cui_cpu_v2_frame_request request = {.width = 16, .height = 12};
    trace t = {.frame = &f, .request = &request, .list = &input, .options = &o, .font = font};
    request.owner_busy = &t.owner_busy;
    cui_cpu_v2_frame_init(&f);
    cui_cpu_v2_frame_release(&f);
    cui_cpu_v2_frame_release(&f);
    unsigned char expected[768];
    bool original_busy = false;
    assert(cpu_common_original_paint(expected, 16, 12, 64, sizeof expected, &input,
                                     (cui_color){7, 19, 31, 53}, &o, font, original_allocate,
                                     original_release, NULL, &original_busy) == CUI_OK);
    assert(prepare(&t) == CUI_OK && f.state == CUI_CPU_V2_FRAME_READY && t.owner_busy);
    assert(memcmp(expected, f.pixels, sizeof expected) == 0);
    assert(f.stride == 64 && f.bytes == 768 && f.width == 16 && f.height == 12 && t.live);

    size_t allocations = t.allocations;
    cui_cpu_v2_frame moved = f;
    assert(cui_cpu_v2_frame_prepare(&moved, &request, &input, (cui_color){0}, &o, font, allocate,
                                    release, &t) == CUI_ERROR_LIFECYCLE);
    cui_cpu_v2_frame_release(&moved);
    cui_cpu_v2_frame_init(&moved);
    assert(t.allocations == allocations && memcmp(expected, f.pixels, sizeof expected) == 0);
    assert(prepare(&t) == CUI_ERROR_LIFECYCLE);
    cui_cpu_v2_frame_release(&f);
    assert(f.state == CUI_CPU_V2_FRAME_EMPTY && !f.pixels && !t.live && t.owner_busy);
    assert(t.frees == allocations);
    t.owner_busy = false;
    for (size_t fail = 1; fail <= allocations; ++fail) {
        t.allocations = t.frees = t.live = 0;
        t.fail = fail;
        t.workspace_blocks[0] = t.workspace_blocks[1] = NULL;
        assert(prepare(&t) == CUI_ERROR_CAPACITY && f.state == CUI_CPU_V2_FRAME_EMPTY &&
               !f.pixels && !t.live);
        assert(t.owner_busy);
        t.owner_busy = false;
        cui_cpu_v2_frame_release(&f);
    }
    t.workspace_blocks[0] = t.workspace_blocks[1] = NULL;
    t.fail = 0;
    t.allocations = t.frees = 0;
    assert(prepare(&t) == CUI_OK && memcmp(expected, f.pixels, sizeof expected) == 0);
    cui_cpu_v2_frame_release(&f);
    t.owner_busy = false;
    /* Sources and control records are rejected before the first callback and
     * before resource dereference, including malformed command bytes in frame. */
    input.commands = (const cui_draw_command_v2 *)&f;
    input.count = 1;
    invalid_unchanged(&t);
    input.commands = commands;
    input.count = count;
    commands[0].kind = UINT32_MAX;
    invalid_unchanged(&t);
    commands[0] = (cui_draw_command_v2){
        .kind = CUI_DRAW_IMAGE,
        .payload.legacy = {
            .type = CUI_DRAW_IMAGE,
            .bounds = {0, 0, 1, 1},
            .bitmap = {
                .bitmap = {.pixels = (unsigned char *)&f, .width = 1, .height = 1, .stride = 4},
                .source = {0, 0, 1, 1},
                .opacity = 1}}};
    invalid_unchanged(&t);
    commands[0] =
        (cui_draw_command_v2){.kind = CUI_DRAW_MESH,
                              .payload.legacy = {.type = CUI_DRAW_MESH,
                                                 .mesh = {.vertices = (const cui_mesh_vertex *)&f,
                                                          .vertex_count = 3,
                                                          .scale = 1,
                                                          .color = {1, 1, 1, 1}}}};
    invalid_unchanged(&t);
#ifdef CUI_HAS_TEXT
    commands[0] = (cui_draw_command_v2){
        .kind = CUI_DRAW_TEXT,
        .payload.legacy = {.type = CUI_DRAW_TEXT, .text = {(const char *)&f, 1}, .font_size = 9}};
    invalid_unchanged(&t);
    commands[0].payload.legacy.text = (cui_str){"g", 1};
    commands[0].payload.legacy.text_paint =
        (cui_text_paint){.kind = CUI_TEXT_PAINT_LINEAR,
                         .stops = (const cui_text_gradient_stop *)&f,
                         .stop_count = 2,
                         .x1 = 1};
    invalid_unchanged(&t);
#endif
    commands[0] = (cui_draw_command_v2){.kind = CUI_DRAW_RECT,
                                        .payload.legacy = {.type = CUI_DRAW_RECT,
                                                           .bounds = {1, 2, 8, 6},
                                                           .color = {91, 33, 201, 173},
                                                           .radius = 2}};
    request.owner_busy = (bool *)&commands[0];
    invalid_unchanged(&t);
    request.owner_busy = &t.owner_busy;
    request.owner_busy = (bool *)&request;
    invalid_unchanged(&t);
    request.owner_busy = &t.owner_busy;
    request.owner_busy = (bool *)&f;
    invalid_unchanged(&t);
    request.owner_busy = &t.owner_busy;
    request.exclusion_count = 4;
    invalid_unchanged(&t);
    request.exclusion_count = 0;
#ifdef CUI_HAS_TEXT
    cui_font_close(font);
#endif
    puts("owned frame layout/lifetime/reentry/aliases/failures/retry PASS");
    return 0;
}
