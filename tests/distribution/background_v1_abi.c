/* SPDX-License-Identifier: MIT */
#ifdef CUI_TEST_V1_UMBRELLA
#include "cui_all.h"
#else
#include "cui.h"
#endif
#include "cui_renderer.h"
#include <stddef.h>

#include "../fixtures/background_v1/frozen_abi.h"
/* _Generic checks signatures without linking production or exposing test symbols. */
typedef bool (*begin_sig)(cui_context *, float, float, float);
typedef cui_draw_list (*end_sig)(cui_context *);
typedef cui_context *(*create_sig)(cui_config);
typedef void (*destroy_sig)(cui_context *);
typedef bool (*queue_sig)(cui_context *, cui_event);
typedef void (*box_sig)(cui_context *, cui_id, cui_box);
typedef void (*end_box_sig)(cui_context *);
typedef void (*portal_sig)(cui_context *, cui_id, cui_box, cui_rect, int16_t);
typedef void (*text_sig)(cui_context *, cui_str, float, cui_color);
typedef bool (*shadows_sig)(cui_context *, const cui_box_shadow *, size_t, cui_corner_radii);
typedef bool (*named_shadows_sig)(cui_context *, cui_id, const cui_box_shadow *, size_t,
                                  cui_corner_radii);
typedef bool (*underlay_sig)(cui_context *, cui_underlay_layout, cui_color, cui_corner_radii);
typedef bool (*bitmap_underlay_sig)(cui_context *, cui_underlay_layout, cui_bitmap_paint,
                                    cui_corner_radii);
typedef bool (*prepaint_sig)(cui_context *, cui_prepaint_options);
typedef bool (*emit_sig)(cui_prepaint_writer *, const cui_draw_command *, bool);
typedef cui_size (*measure_sig)(void *, cui_str, float);
typedef cui_size (*measure_styled_sig)(void *, cui_str, float, cui_text_style);
typedef bool (*render_sig)(void *, const cui_draw_list *, cui_color);
#define SIGNATURE(symbol, type)                                                                    \
    _Static_assert(_Generic(&(symbol), type: 1, default: 0), #symbol " V1 signature")
SIGNATURE(cui_begin_frame, begin_sig);
SIGNATURE(cui_end_frame, end_sig);
SIGNATURE(cui_create, create_sig);
SIGNATURE(cui_destroy, destroy_sig);
SIGNATURE(cui_queue_event, queue_sig);
SIGNATURE(cui_begin_box, box_sig);
SIGNATURE(cui_end_box, end_box_sig);
SIGNATURE(cui_begin_portal, portal_sig);
SIGNATURE(cui_text, text_sig);
SIGNATURE(cui_set_box_shadows, shadows_sig);
SIGNATURE(cui_element_set_box_shadows, named_shadows_sig);
SIGNATURE(cui_set_underlay_ex, underlay_sig);
SIGNATURE(cui_set_bitmap_underlay, bitmap_underlay_sig);
SIGNATURE(cui_set_prepaint, prepaint_sig);
SIGNATURE(cui_prepaint_emit, emit_sig);
_Static_assert(_Generic((cui_measure_fn)0, measure_sig: 1, default: 0),
               "measure callback V1 signature");
_Static_assert(_Generic((cui_measure_styled_fn)0, measure_styled_sig: 1, default: 0),
               "styled measure callback V1 signature");
_Static_assert(_Generic(((cui_renderer *)0)->render, render_sig: 1, default: 0),
               "renderer callback V1 signature");
int main(void) {
    return 0;
}
