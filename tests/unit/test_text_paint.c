/* SPDX-License-Identifier: MIT */
#include "cui/cui_typography.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#define CHECK(x)                                                                                   \
    do {                                                                                           \
        if (!(x)) {                                                                                \
            fprintf(stderr, "%d: %s\n", __LINE__, #x);                                             \
            exit(1);                                                                               \
        }                                                                                          \
    } while (0)
int main(void) {
    cui_context *ui = cui_create((cui_config){.frame_bytes = 16384});
    CHECK(ui);
    cui_text_gradient_stop stops[] = {{0, {255, 0, 0, 255}}, {1, {0, 0, 255, 255}}};
    cui_text_paint paint = {
        .kind = CUI_TEXT_PAINT_LINEAR, .x1 = 1, .stops = stops, .stop_count = 2};
    cui_typography_token token = {.size = 20, .line_height = 28, .wrap = true};
    size_t row_count[2] = {0};
    cui_rect solid[16];
    float block_start = 0, block_end = 0;
    for (unsigned pass = 0; pass < 2; ++pass) {
        CHECK(cui_begin_frame(ui, 200, 180, 0));
        cui_begin_box(ui, 1, (cui_box){.width = 90, .custom_opacity = true, .opacity = .5f});
        if (pass) {
            CHECK(cui_typography_text_painted(ui, cui_string("MMMM MMMM\nMM"), token, paint));
            stops[0].color = (cui_color){0}; /* Submission owns an independent copy. */
        } else
            CHECK(cui_typography_text(ui, cui_string("MMMM MMMM\nMM"), token,
                                      (cui_color){255, 255, 255, 255}));
        cui_end_box(ui);
        cui_draw_list list = cui_end_frame(ui);
        CHECK(cui_last_error(ui) == CUI_OK);
        for (size_t i = 0; i < list.count; ++i) {
            const cui_draw_command *c = &list.commands[i];
            if (c->type != CUI_DRAW_TEXT)
                continue;
            size_t row = row_count[pass]++;
            CHECK(row < 16);
            if (!pass)
                solid[row] = c->bounds;
            else {
                CHECK(c->bounds.x == solid[row].x && c->bounds.y == solid[row].y &&
                      c->bounds.w == solid[row].w && c->bounds.h == solid[row].h);
                CHECK(c->text_paint.kind == CUI_TEXT_PAINT_LINEAR);
                CHECK(c->text_paint.stops != stops && c->text_paint.stop_count == 2);
                CHECK(c->text_paint.stops[0].color.r == 255 &&
                      c->text_paint.stops[0].color.a == 127);
                if (!row) {
                    block_start = c->text_paint.x0;
                    block_end = c->text_paint.x1;
                    CHECK(block_end > block_start);
                } else
                    CHECK(c->text_paint.x0 == block_start && c->text_paint.x1 == block_end);
            }
        }
    }
    CHECK(row_count[0] >= 3 && row_count[0] == row_count[1]);
    CHECK(cui_begin_frame(ui, 200, 100, 0));
    paint.x1 = NAN;
    CHECK(!cui_typography_text_painted(ui, cui_string("invalid"), token, paint));
    paint.x1 = 0;
    CHECK(!cui_typography_text_painted(ui, cui_string("invalid"), token, paint));
    paint.x1 = 1;
    paint.stop_count = 1;
    CHECK(!cui_typography_text_painted(ui, cui_string("invalid"), token, paint));
    CHECK(cui_end_frame(ui).count == 0 && cui_last_error(ui) == CUI_OK);
    cui_destroy(ui);
    return 0;
}
