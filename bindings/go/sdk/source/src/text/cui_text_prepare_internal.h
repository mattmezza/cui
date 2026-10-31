/* SPDX-License-Identifier: MIT */
#ifndef CUI_TEXT_PREPARE_INTERNAL_H
#define CUI_TEXT_PREPARE_INTERNAL_H
#include "cui/cui_text.h"
#include "cui_text_alloc_internal.h"
typedef struct cui_text_recorded_glyph_internal {
    struct cui_text_recorded_glyph_internal *next;
    int x, y;
    unsigned width, height;
    unsigned char *coverage; /* Owned tightly packed, top-to-bottom coverage. */
    cui_color color;
    bool colored;
    float source_density;
    cui_text_paint paint; /* Stops remain borrowed through session end/render completion. */
} cui_text_recorded_glyph_internal;
_Static_assert(_Alignof(cui_text_recorded_glyph_internal) <=
                   _Alignof(cui_fundamental_alignment_internal),
               "glyph recording exceeds allocation-prefix alignment");
_Static_assert(_Alignof(cui_text_glyph) <= _Alignof(cui_fundamental_alignment_internal),
               "shaped glyph exceeds allocation-prefix alignment");
_Static_assert(_Alignof(cui_text_run) <= _Alignof(cui_fundamental_alignment_internal),
               "text run exceeds allocation-prefix alignment");
_Static_assert(_Alignof(cui_text_cluster) <= _Alignof(cui_fundamental_alignment_internal),
               "text cluster exceeds allocation-prefix alignment");
typedef struct {
    cui_text_owner_internal *owner;
    cui_text_recorded_glyph_internal *first, *last;
    bool busy;
} cui_text_prepare_session;
/* Initialize a zero session with a live owner. The owner budget is the REMAINING
 * transaction envelope after other adapter reservations. No opaque dependency
 * byte guarantee. Failure appends nothing, becomes sticky until session end;
 * existing records survive for teardown only. All borrowed fonts/input remain
 * stable through session end/render completion. No public renderer capability follows from
 * recording. */
bool cui_text_prepare_line(cui_text_prepare_session *, cui_font *, const cui_draw_command *,
                           float source_density);
void cui_text_prepare_session_end(cui_text_prepare_session *);
#endif
