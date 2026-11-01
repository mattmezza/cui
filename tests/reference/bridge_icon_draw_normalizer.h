/* SPDX-License-Identifier: MIT */
/* Existing lossless V1 active-field normalizer, source dbfb7945; snippet SHA256
 * 75a10c20804c58c9e9bccc2a6c3baa4dddfc1cd498f03a17b76fc17610b74c6c. */
static void scalar(FILE *f, float x) {
    uint32_t bits = 0;
    if (x != 0)
        memcpy(&bits, &x, sizeof bits); /* normalize signed zero, never padding */
    fprintf(f, " %08" PRIx32, bits);
}
static void rect(FILE *f, cui_rect r) {
    scalar(f, r.x);
    scalar(f, r.y);
    scalar(f, r.w);
    scalar(f, r.h);
}
static void color(FILE *f, cui_color c) {
    fprintf(f, " %u %u %u %u", c.r, c.g, c.b, c.a);
}
static void radii(FILE *f, cui_corner_radii r) {
    scalar(f, r.top_left);
    scalar(f, r.top_right);
    scalar(f, r.bottom_left);
    scalar(f, r.bottom_right);
}
static void string(FILE *f, cui_str s) {
    fprintf(f, " %zu:", s.length);
    for (size_t i = 0; i < s.length; i++)
        fprintf(f, "%02x", (unsigned char)s.data[i]);
}
static void shape(FILE *f, const cui_draw_command *c) {
    fprintf(f, " %u", c->custom_corners);
    if (c->custom_corners)
        radii(f, c->corner_radii);
    else
        scalar(f, c->radius);
}
static void command(FILE *f, const cui_draw_command *c) {
    fprintf(f, "C %u", (unsigned)c->type);
    if (c->type != CUI_DRAW_CLIP_POP)
        rect(f, c->bounds);
    switch (c->type) {
    case CUI_DRAW_RECT:
        color(f, c->color);
        shape(f, c);
        break;
    case CUI_DRAW_BORDER:
        color(f, c->color);
        shape(f, c);
        fprintf(f, " %u", c->custom_border_edges);
        if (c->custom_border_edges) {
            scalar(f, c->border_edges.left);
            scalar(f, c->border_edges.right);
            scalar(f, c->border_edges.top);
            scalar(f, c->border_edges.bottom);
        } else
            scalar(f, c->stroke);
        break;
    case CUI_DRAW_CLIP_PUSH:
    case CUI_DRAW_CLIP_POP:
        break;
    case CUI_DRAW_TEXT:
        if (c->text_paint.kind == CUI_TEXT_PAINT_SOLID)
            color(f, c->color);
        scalar(f, c->font_size);
        fprintf(f, " %u %u %u %" PRIu32, (unsigned)c->text_style.weight, c->text_style.italic,
                c->text_style.monospace, c->text_style.family);
        scalar(f, c->text_style.letter_spacing);
        string(f, c->text);
        fprintf(f, " %u", c->text_source.paragraph.data ? 1u : 0u);
        if (c->text_source.paragraph.data) {
            string(f, c->text_source.paragraph);
            fprintf(f, " %zu %zu", c->text_source.byte_start, c->text_source.byte_end);
        }
        fprintf(f, " %zu", c->text_color_count);
        for (size_t i = 0; i < c->text_color_count; i++) {
            fprintf(f, " %zu %zu", c->text_colors[i].byte_start, c->text_colors[i].byte_end);
            color(f, c->text_colors[i].color);
        }
        fprintf(f, " %u", (unsigned)c->text_paint.kind);
        if (c->text_paint.kind == CUI_TEXT_PAINT_LINEAR) {
            scalar(f, c->text_paint.x0);
            scalar(f, c->text_paint.y0);
            scalar(f, c->text_paint.x1);
            scalar(f, c->text_paint.y1);
            fprintf(f, " %u %zu", (unsigned)c->text_paint.anchor, c->text_paint.stop_count);
            for (size_t i = 0; i < c->text_paint.stop_count; i++) {
                scalar(f, c->text_paint.stops[i].position);
                color(f, c->text_paint.stops[i].color);
            }
        }
        break;
    case CUI_DRAW_IMAGE:
        shape(f, c);
        fprintf(f, " %u %u %" PRIu64, c->bitmap.bitmap.width, c->bitmap.bitmap.height,
                c->bitmap.bitmap.revision);
        rect(f, c->bitmap.source);
        scalar(f, c->bitmap.opacity);
        fprintf(f, " %u %u", c->bitmap.grayscale, c->bitmap.custom_tint);
        if (c->bitmap.custom_tint)
            color(f, c->bitmap.tint);
        /* Row padding and addresses are representation, actual pixel bytes are owned semantics. */
        for (unsigned y = 0; y < c->bitmap.bitmap.height; y++)
            for (unsigned x = 0; x < c->bitmap.bitmap.width * 4; x++)
                fprintf(f, " %02x",
                        c->bitmap.bitmap.pixels[(size_t)y * c->bitmap.bitmap.stride + x]);
        break;
    case CUI_DRAW_MESH:
        fprintf(f, " %zu", c->mesh.vertex_count);
        scalar(f, c->mesh.origin_x);
        scalar(f, c->mesh.origin_y);
        scalar(f, c->mesh.scale);
        scalar(f, c->mesh.color.r);
        scalar(f, c->mesh.color.g);
        scalar(f, c->mesh.color.b);
        scalar(f, c->mesh.color.a);
        for (size_t i = 0; i < c->mesh.vertex_count; i++) {
            scalar(f, c->mesh.vertices[i].x);
            scalar(f, c->mesh.vertices[i].y);
            scalar(f, c->mesh.vertices[i].s);
            scalar(f, c->mesh.vertices[i].t);
        }
        break;
    case CUI_DRAW_DROP_SHADOW:
    case CUI_DRAW_INSET_SHADOW:
        rect(f, c->shadow.caster);
        radii(f, c->shadow.radii);
        scalar(f, c->shadow.layer.offset_x);
        scalar(f, c->shadow.layer.offset_y);
        scalar(f, c->shadow.layer.sigma);
        scalar(f, c->shadow.layer.spread);
        color(f, c->shadow.layer.color);
        break;
    default:
        assert(!"unknown V1 kind");
    }
    fputc('\n', f);
}
