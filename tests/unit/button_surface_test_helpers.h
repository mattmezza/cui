/* SPDX-License-Identifier: MIT */
/* Deterministic owned command/software hashing shared by two focused tests. */
#include "cui/cui_button_styles.h"
#include "cui/cui_software.h"
#include <assert.h>
#include <stdint.h>
#include <string.h>

static cui_size measure(void *user, cui_str text, float size) {
    (void)user;
    return (cui_size){(float)text.length * size * .5f, size};
}
static cui_context *create(size_t elements, size_t commands) {
    cui_context *ui = cui_create(
        (cui_config){.max_elements = elements, .max_commands = commands, .measure = measure});
    assert(ui);
    return ui;
}
static uint64_t word(uint64_t hash, uint32_t value) {
    for (unsigned i = 0; i < 4; i++) {
        hash ^= (value >> (8 * i)) & 255;
        hash *= UINT64_C(1099511628211);
    }
    return hash;
}
static uint64_t number(uint64_t hash, float value) {
    uint32_t bits;
    memcpy(&bits, &value, sizeof bits);
    return word(hash, bits);
}
static uint64_t commands(cui_draw_list list) {
    uint64_t h = word(UINT64_C(14695981039346656037), (uint32_t)list.count);
    for (size_t i = 0; i < list.count; i++) {
        const cui_draw_command *c = &list.commands[i];
        h = word(h, c->type);
        const float values[] = {c->bounds.x,
                                c->bounds.y,
                                c->bounds.w,
                                c->bounds.h,
                                c->radius,
                                c->stroke,
                                c->font_size,
                                c->corner_radii.top_left,
                                c->corner_radii.top_right,
                                c->corner_radii.bottom_left,
                                c->corner_radii.bottom_right};
        for (size_t j = 0; j < sizeof values / sizeof *values; j++)
            h = number(h, values[j]);
        h = word(h, c->color.r | (uint32_t)c->color.g << 8 | (uint32_t)c->color.b << 16 |
                        (uint32_t)c->color.a << 24);
        h = word(h, c->custom_corners);
        h = word(h, c->custom_border_edges);
        const float edges[] = {c->border_edges.left, c->border_edges.right, c->border_edges.top,
                               c->border_edges.bottom};
        for (size_t j = 0; j < 4; j++)
            h = number(h, edges[j]);
        h = word(h, c->text_style.weight);
        h = word(h, c->text_style.italic);
        h = word(h, c->text_style.monospace);
        h = word(h, c->text_style.family);
        h = number(h, c->text_style.letter_spacing);
        if (c->type == CUI_DRAW_IMAGE) {
            h = word(h, c->bitmap.bitmap.width);
            h = word(h, c->bitmap.bitmap.height);
            h = word(h, (uint32_t)c->bitmap.bitmap.revision);
            h = word(h, (uint32_t)(c->bitmap.bitmap.revision >> 32));
            const float image[] = {c->bitmap.source.x, c->bitmap.source.y, c->bitmap.source.w,
                                   c->bitmap.source.h, c->bitmap.opacity};
            for (size_t j = 0; j < 5; j++)
                h = number(h, image[j]);
            h = word(h, c->bitmap.grayscale);
            h = word(h, c->bitmap.custom_tint);
            h = word(h, c->bitmap.tint.r | (uint32_t)c->bitmap.tint.g << 8 |
                            (uint32_t)c->bitmap.tint.b << 16 | (uint32_t)c->bitmap.tint.a << 24);
            for (unsigned y = 0; y < c->bitmap.bitmap.height; y++)
                for (size_t x = 0; x < (size_t)c->bitmap.bitmap.width * 4; x++)
                    h = word(h, c->bitmap.bitmap.pixels[(size_t)y * c->bitmap.bitmap.stride + x]);
        }
        for (size_t j = 0; j < c->text.length; j++)
            h = word(h, (unsigned char)c->text.data[j]);
    }
    return h;
}
/* Actual software composition with text omitted; no fake glyph rasterization. */
static uint64_t pixels(cui_software *r, cui_draw_list list) {
    cui_draw_command filtered[512];
    size_t n = 0;
    for (size_t i = 0; i < list.count; i++)
        if (list.commands[i].type != CUI_DRAW_TEXT) {
            assert(n < 512);
            filtered[n++] = list.commands[i];
        }
    assert(n <= 512);
    cui_draw_list paint = {filtered, n, list.width, list.height};
    assert(cui_software_render(r, &paint, (cui_color){255, 255, 255, 255}));
    const unsigned char *rgba = cui_software_pixels(r, NULL, NULL);
    uint64_t h = UINT64_C(14695981039346656037);
    for (size_t i = 0; i < 320 * 160 * 4; i++) {
        h ^= rgba[i];
        h *= UINT64_C(1099511628211);
    }
    return h;
}
