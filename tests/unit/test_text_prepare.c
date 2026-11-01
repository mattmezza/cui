/* SPDX-License-Identifier: MIT */
#include "../../src/core/cui_text_paint_internal.h"
#include "cui/cui_software.h"
#ifndef CUI_TEXT_BASELINE_CAPTURE
#include "../../src/text/cui_text.c" /* Same-TU cache/owner proof; no fixture API. */
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef CUI_TEXT_BASELINE_CAPTURE
/* Runtime-derived address remains unreadable without O3 bounds constant propagation. */
static uintptr_t poison_address(void) {
    static volatile uintptr_t address = 1;
    return address;
}
bool cui_test_original_text_prepare(const char *, const char *, unsigned char *, size_t, size_t *);
bool cui_test_original_fallback(const char *, const char *, const cui_draw_command *,
                                unsigned char *, size_t, size_t *);
bool cui_test_original_density(const char *, const char *, const cui_draw_command *, float,
                               unsigned char *, size_t, size_t *);
#endif
#include "text_prepare_fixture_internal.h"
#ifndef CUI_TEXT_BASELINE_CAPTURE
static void *allocate(void *user, size_t n) {
    size_t *calls = user;
    ++*calls;
    return malloc(n);
}
static void release(void *user, void *p) {
    (void)user;
    free(p);
}
static cui_text_owner_internal owner(size_t *calls) {
    return (cui_text_owner_internal){.allocate = allocate,
                                     .release = release,
                                     .user = calls,
                                     .cpu_limit = (size_t)32 * 1024 * 1024,
                                     .metadata_limit = (size_t)1024 * 1024};
}
static void blend(unsigned char *p, cui_color c) {
    unsigned alpha = c.a * 255u + p[3] * (255u - c.a);
    if (!alpha) {
        memset(p, 0, 4);
        return;
    }
    unsigned src[3] = {c.r, c.g, c.b};
    for (unsigned i = 0; i < 3; i++)
        p[i] =
            (unsigned char)((src[i] * c.a * 255u + p[i] * p[3] * (255u - c.a) + alpha / 2) / alpha);
    p[3] = (unsigned char)((alpha + 127) / 255);
}
static void prepared(cui_font *font) {
    length = 0;
    bytes("CUI_FONT_V1", 11);
    for (unsigned i = 0; i < 8; i++) {
#ifdef CUI_TEXT_NO_BIDI_FIXTURE
        if (i != 0 && i != 6 && i != 7)
            continue;
#endif
        cui_draw_command c = fixture(i);
        float density = i == 7 ? 16 : 1;
        size_t calls = 0;
        cui_text_owner_internal o = owner(&calls);
        cui_text_prepare_session s = {.owner = &o};
        assert(cui_text_prepare_line(&s, font, &c, density));
        assert(!font->glyph_cache); /* Cold V2 never creates/inserts legacy cache. */
        number(i);
        size_t begin = length;
        number(0);
        for (cui_text_recorded_glyph_internal *g = s.first; g; g = g->next) {
            if (g->colored)
                colored(NULL, g->x, g->y, (int)g->width, (int)g->height, (int)g->width, g->coverage,
                        g->color);
            else
                record(NULL, g->x, g->y, (int)g->width, (int)g->height, (int)g->width, g->coverage);
        }
        uint32_t n = (uint32_t)(length - begin - 4);
        for (unsigned j = 0; j < 4; j++)
            result[begin + j] = (unsigned char)(n >> (8 * j));
        cui_text_prepare_session_end(&s);
        assert(!o.cpu_used && !o.metadata_used);
        /* Separate unit-density session for exact independently captured V1 pixels. */
        o = owner(&calls);
        s = (cui_text_prepare_session){.owner = &o};
        assert(cui_text_prepare_line(&s, font, &c, 1));
        unsigned char pixels[96 * 72 * 4];
        for (size_t p = 0; p < sizeof(pixels); p += 4) {
            pixels[p] = 9;
            pixels[p + 1] = 17;
            pixels[p + 2] = 25;
            pixels[p + 3] = 70;
        }
        for (cui_text_recorded_glyph_internal *g = s.first; g; g = g->next)
            for (unsigned y = 0; y < g->height; y++)
                for (unsigned x = 0; x < g->width; x++) {
                    int px = g->x + (int)x, py = g->y + (int)y;
                    if (px < 0 || py < 0 || px >= 96 || py >= 72)
                        continue;
                    cui_color color =
                        g->colored ? g->color
                                   : (c.text_paint.kind == CUI_TEXT_PAINT_LINEAR
                                          ? cui_text_paint_sample(c.text_paint, px + .5, py + .5)
                                          : c.color);
                    color.a =
                        (uint8_t)((unsigned)color.a * g->coverage[(size_t)y * g->width + x] / 255);
                    blend(pixels + ((size_t)py * 96 + (unsigned)px) * 4, color);
                }
        bytes(pixels, sizeof(pixels));
        cui_text_prepare_session_end(&s);
        assert(!o.cpu_used && !o.metadata_used);
    }
}
static void recording_boundaries(cui_font *font) {
    size_t calls = 0;
    cui_draw_command c = fixture(0);
    cui_text_owner_internal o = owner(&calls);
    text_recording r = {.records = {.owner = &o}, .command = &c, .density = 1};
    unsigned char source[6] = {1, 2, 99, 4, 5, 99};
    text_record_coverage(&r, 2, 3, 2, 2, -3, source + 3, c.color, false);
    assert(r.records.first && r.records.first->coverage[0] == 4 &&
           r.records.first->coverage[1] == 5 && r.records.first->coverage[2] == 1 &&
           r.records.first->coverage[3] == 2);
    memset(source, 0, sizeof(source));
    assert(r.records.first->coverage[0] == 4);
    text_records_clear(&r.records);
    assert(!o.cpu_used && !o.metadata_used);
    o = owner(&calls);
    r.records = (cui_text_prepare_session){.owner = &o};
    text_record_coverage(&r, 0, 0, 0, 2, 0, NULL, c.color, false);
    assert(!r.records.first && o.error == CUI_OK);
    text_record_coverage(&r, 0, 0, 4097, 1, 4097, (const unsigned char *)poison_address(), c.color,
                         false);
    assert(o.error == CUI_ERROR_CAPACITY && !r.records.first && !o.cpu_used);
    o = owner(&calls);
    o.coverage_pixels = (size_t)8 * 1024 * 1024;
    r.records = (cui_text_prepare_session){.owner = &o};
    text_record_coverage(&r, 0, 0, 1, 1, 1, (const unsigned char *)poison_address(), c.color,
                         false);
    assert(o.error == CUI_ERROR_CAPACITY && !r.records.first);
    o = owner(&calls);
    cui_text_prepare_session s = {.owner = &o};
    c.text_source = (cui_text_line_source){{(const char *)poison_address(), 65537}, 0, 65537};
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(!o.dependencies && !s.first);
    cui_text_prepare_session_end(&s);
#ifdef CUI_TEXT_BIDI
    o = owner(&calls);
    s = (cui_text_prepare_session){.owner = &o};
    c = fixture(4);
    c.text = (cui_str){(const char *)poison_address(), SIZE_MAX}; /* inactive contextual text */
    assert(cui_text_prepare_line(&s, font, &c, 1));
    cui_text_prepare_session_end(&s);
    o = owner(&calls);
    s = (cui_text_prepare_session){.owner = &o};
    c = fixture(0);
    c.text_source = (cui_text_line_source){(cui_str)CUI_STR(""), 0, 0};
    c.text = (cui_str){(const char *)poison_address(), SIZE_MAX};
    assert(cui_text_prepare_line(&s, font, &c, 1));
    assert(!s.first);
    cui_text_prepare_session_end(&s);
#else
    /* No-Bidi V2 rejects contextual mode before reading either borrowed string. */
    for (unsigned i = 0; i < 5; ++i) {
        size_t before = calls;
        o = owner(&calls);
        s = (cui_text_prepare_session){.owner = &o};
        c = fixture(0);
        c.text = (cui_str){(const char *)poison_address(), i & 1 ? SIZE_MAX : 1};
        if (i < 4)
            c.text_source = (cui_text_line_source){
                {(const char *)poison_address(), i < 2 ? 0 : 1}, 0, i < 2 ? 0 : 1};
        else {
            c.text_colors = (const cui_text_color_span *)poison_address();
            c.text_color_count = 1;
        }
        assert(!cui_text_prepare_line(&s, font, &c, i & 1 ? 64 : 1));
        assert(o.error == CUI_ERROR_INVALID_ACTION && !s.first && !s.last);
        assert(calls == before && !o.dependencies && !o.input_bytes && !o.face_count);
        assert(!o.cpu_used && !o.metadata_used && !s.busy);
        cui_text_prepare_session_end(&s);
        assert(calls == before);
    }
#endif
}
#ifdef CUI_TEXT_BIDI
static void fallback_recording(const char *regular) {
    const char *paths[] = {"/usr/share/fonts/noto/NotoSansArabic-Regular.ttf",
                           "/usr/share/fonts/noto/NotoSansHebrew-Regular.ttf"};
    const cui_str strings[] = {
        CUI_STR("abc \xd8\xa7\xd9\x84\xd8\xb9\xd8\xb1\xd8\xa8\xd9\x8a\xd8\xa9"),
        CUI_STR("abc \xd7\xa9\xd7\x9c\xd7\x95\xd7\x9d")};
    for (size_t i = 0; i < 2; ++i) {
        FILE *available = fopen(paths[i], "rb");
        if (!available) {
            puts("Optional system Arabic/Hebrew fallback recording probe unavailable.");
            continue;
        }
        fclose(available);
        cui_draw_command c = fixture(0);
        c.text = strings[i];
        unsigned char expected[sizeof(result)];
        size_t n = 0;
        assert(cui_test_original_fallback(regular, paths[i], &c, expected, sizeof(expected), &n));
        cui_font *font = cui_font_open(regular), *fallback = cui_font_open(paths[i]);
        assert(font && fallback);
        cui_font *chain[] = {fallback};
        assert(cui_font_set_fallbacks(font, chain, 1));
        size_t calls = 0;
        cui_text_owner_internal o = owner(&calls);
        cui_text_prepare_session session = {.owner = &o};
        assert(cui_text_prepare_line(&session, font, &c, 1));
        assert(o.face_count == 2 && !font->glyph_cache && !fallback->glyph_cache);
        length = 0;
        for (cui_text_recorded_glyph_internal *g = session.first; g; g = g->next)
            record(NULL, g->x, g->y, (int)g->width, (int)g->height, (int)g->width, g->coverage);
        assert(length == n && !memcmp(result, expected, n));
        cui_text_prepare_session_end(&session);
        assert(!o.cpu_used && !o.metadata_used);
        cui_font_close(font);
        cui_font_close(fallback);
        puts("Original-source Arabic/Hebrew fallback recording bytes verified.");
    }
}
#endif
static void owned_density64(cui_font *font, const char *regular, const char *bold_path) {
#ifdef CUI_TEXT_BIDI
    assert(cui_text_prepare_context_available_internal());
#else
    assert(!cui_text_prepare_context_available_internal());
#endif
    static unsigned char expected[sizeof result];
    const float fractional[] = {1.25f, 3.5f, 15.75f};
    size_t calls = 0;
    unsigned width16 = 0, height16 = 0;
    for (unsigned i = 0; i < 19; ++i) {
        float q = i < 16 ? i + 1.f : fractional[i - 16];
        cui_draw_command c = fixture(7);
        c.text_style.weight = i & 1 && q != 16 ? CUI_TEXT_WEIGHT_BOLD : CUI_TEXT_WEIGHT_NORMAL;
        size_t n = 0;
        assert(cui_test_original_density(regular, bold_path, &c, q, expected, sizeof expected, &n));
        cui_text_owner_internal o = owner(&calls);
        cui_text_prepare_session session = {.owner = &o};
        assert(cui_text_prepare_line(&session, font, &c, q));
        length = 0;
        for (cui_text_recorded_glyph_internal *g = session.first; g; g = g->next) {
            assert(g->source_density == q);
            record(NULL, g->x, g->y, (int)g->width, (int)g->height, (int)g->width, g->coverage);
            if (q == 16) {
                width16 = g->width;
                height16 = g->height;
            }
        }
        assert(length == n && !memcmp(result, expected, n));
        assert(!font->glyph_cache);
        cui_text_prepare_session_end(&session);
        assert(!o.cpu_used && !o.metadata_used);
    }
    cui_draw_command c = fixture(7);
    cui_text_owner_internal o = owner(&calls);
    cui_text_prepare_session session = {.owner = &o};
    assert(cui_text_prepare_line(&session, font, &c, 64));
    assert(session.first && !session.first->next && session.first->source_density == 64);
    assert(session.first->width > 3 * width16 && session.first->height > 3 * height16);
    assert(font->face->size->metrics.y_ppem == 320); /* 5 logical pixels *64, not16. */
    assert(!font->glyph_cache && o.coverage_pixels <= 8 * 1024 * 1024);
    printf("owned q64: q16=%ux%u q64=%ux%u ppem=%u CPUpeak=%zu metadataPeak=%zu\n", width16,
           height16, session.first->width, session.first->height, font->face->size->metrics.y_ppem,
           o.cpu_peak, o.metadata_peak);
    cui_text_prepare_session_end(&session);
    assert(!o.cpu_used && !o.metadata_used);
#ifdef CUI_TEXT_BIDI
    for (unsigned f = 4; f <= 5; ++f) {
        c = fixture(f);
        o = owner(&calls);
        session = (cui_text_prepare_session){.owner = &o};
        assert(cui_text_prepare_line(&session, font, &c, 64));
        assert(session.first && session.first->source_density == 64);
        if (f == 4)
            assert(session.first->colored);
        else
            assert(session.first->paint.kind == CUI_TEXT_PAINT_LINEAR);
        cui_text_prepare_session_end(&session);
        assert(!o.cpu_used && !o.metadata_used);
    }
#endif
    c = fixture(7);
    const float invalid[] = {nextafterf(64, INFINITY), INFINITY, NAN, 0, -1};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        size_t before = calls;
        o = owner(&calls);
        session = (cui_text_prepare_session){.owner = &o};
        assert(!cui_text_prepare_line(&session, font, &c, invalid[i]));
        assert(!session.first && !o.dependencies && calls == before);
        cui_text_prepare_session_end(&session);
    }
    /* All old public scaled entry guards stay<=16; callback output stays empty. */
    for (unsigned i = 0; i < 2; ++i) {
        float q = i ? 64 : nextafterf(16, INFINITY);
        length = 0;
        size_t n = 99;
        assert(
            !cui_test_original_density(regular, bold_path, &c, q, expected, sizeof expected, &n) &&
            !n);
        assert(!cui_font_draw_context_line_scaled(font, c.text, c.text_source, c.font_size,
                                                  c.text_style, c.bounds.x, c.bounds.y, q, record,
                                                  NULL));
        assert(!length && !font->glyph_cache);
#ifdef CUI_TEXT_BIDI
        cui_draw_command colored_command = fixture(4);
        assert(!cui_font_draw_context_line_colored_scaled(
            font, colored_command.text_source, colored_command.font_size,
            colored_command.text_style, 0, 0, colored_command.color, colored_command.text_colors,
            colored_command.text_color_count, q, colored, NULL));
        assert(!length && !font->glyph_cache);
        cui_text_layout *layout = NULL;
        assert(cui_font_layout_line(font, c.text, c.font_size, c.text_style, CUI_BIDI_AUTO_LTR, 0,
                                    c.text.length, NULL, &layout) == CUI_TEXT_LAYOUT_OK);
        assert(!cui_text_layout_draw_scaled(layout, 0, 0, q, record, NULL));
        cui_text_layout_destroy(layout);
        assert(!length && !font->glyph_cache);
#endif
    }
}
static void owned_format_and_casts(cui_font *font) {
    size_t calls = 0;
    cui_text_owner_internal o = owner(&calls);
    FT_GlyphSlotRec slot = {0};
    slot.bitmap.width = 1;
    slot.bitmap.rows = 1;
    slot.bitmap.buffer = (unsigned char *)poison_address();
    slot.bitmap.num_grays = 256;
    const unsigned char modes[] = {FT_PIXEL_MODE_NONE,  FT_PIXEL_MODE_MONO, FT_PIXEL_MODE_GRAY2,
                                   FT_PIXEL_MODE_GRAY4, FT_PIXEL_MODE_LCD,  FT_PIXEL_MODE_LCD_V,
                                   FT_PIXEL_MODE_BGRA};
    for (size_t i = 0; i < sizeof modes / sizeof *modes; ++i) {
        o = owner(&calls);
        slot.bitmap.pixel_mode = modes[i];
        assert(!text_owned_bitmap(&slot, &o) && o.error == CUI_ERROR_INVALID_ACTION && !calls);
    }
    slot.bitmap.pixel_mode = FT_PIXEL_MODE_GRAY;
    slot.bitmap.num_grays = 255;
    o = owner(&calls);
    assert(!text_owned_bitmap(&slot, &o) && !calls);
    slot.bitmap.num_grays = 256;
    o = owner(&calls);
    assert(text_owned_bitmap(&slot, &o));
    slot.bitmap.width = 0;
    slot.bitmap.pixel_mode = FT_PIXEL_MODE_BGRA;
    assert(text_owned_bitmap(&slot, &o)); /* zero work ignores format/buffer */
    FT_F26Dot6 size = 123;
    o = owner(&calls);
    assert(text_owned_char_size(4096.f * 64, &size, &o) && size == 16777216);
    const float invalid[] = {NAN, INFINITY, -INFINITY, 0, -1, nextafterf(4096.f * 64, INFINITY)};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        size = 123;
        o = owner(&calls);
        assert(!text_owned_char_size(invalid[i], &size, &o) && size == 123 && !calls &&
               !o.dependencies);
    }
    int position = 123;
    o = owner(&calls);
    assert(text_owned_origin(1e7f, 64, &position, &o) && position == 640000000);
    position = 123;
    o = owner(&calls);
    assert(!text_owned_origin(FLT_MAX, 64, &position, &o) && position == 123 && !calls);
    /* Seed actual hot V1 slot; owned lookup reads it without insertion/dependency.
     * Poisoned unsupported slot must reject before any byte or FT/HB operation. */
    cui_draw_command c = fixture(7);
    length = 0;
    assert(cui_font_draw_context_line_scaled(font, c.text, c.text_source, c.font_size, c.text_style,
                                             c.bounds.x, c.bounds.y, 16, record, NULL));
    FT_UInt glyph = FT_Get_Char_Index(font->face, 'g');
    FT_F26Dot6 key = (FT_F26Dot6)lroundf(c.font_size * 16 * 64);
    size_t index = ((size_t)glyph * 33u + (size_t)key) % GLYPH_CACHE_SLOTS;
    glyph_cache_entry *entry = &font->glyph_cache[index];
    assert(entry->valid && entry->glyph == glyph && entry->size == key);
    o = owner(&calls);
    assert(raster_glyph_owned(font, glyph, c.font_size * 16, &o) == &entry->slot);
    assert(!calls && !o.dependencies && entry->valid);
    FT_Bitmap saved = entry->slot.bitmap;
    for (size_t i = 0; i < sizeof modes / sizeof *modes; ++i) {
        entry->slot.bitmap.pixel_mode = modes[i];
        entry->slot.bitmap.buffer = (unsigned char *)poison_address();
        o = owner(&calls);
        assert(!raster_glyph_owned(font, glyph, c.font_size * 16, &o));
        assert(o.error == CUI_ERROR_INVALID_ACTION && !calls && !o.dependencies && entry->valid);
        assert(raster_glyph(font, glyph, c.font_size * 16) ==
               &entry->slot); /* NULL-owner unchanged */
    }
    entry->slot.bitmap = saved;
    /* Original live cached bytes remain unchanged after successful owned copy. */
    unsigned char before[GLYPH_CACHE_BITMAP_MAX];
    size_t n = (size_t)saved.pitch * saved.rows;
    assert(n <= sizeof before);
    memcpy(before, saved.buffer, n);
    o = owner(&calls);
    cui_text_prepare_session session = {.owner = &o};
    assert(cui_text_prepare_line(&session, font, &c, 16));
    assert(entry->valid && entry->slot.bitmap.buffer == saved.buffer &&
           !memcmp(before, saved.buffer, n));
    cui_text_prepare_session_end(&session);
    assert(!o.cpu_used && !o.metadata_used);
}
static size_t allocation_calls, allocation_fail, live_blocks;
static cui_text_prepare_session *reentrant_session;
static cui_font *reentrant_font;
static cui_draw_command *reentrant_command;
static void *failing_allocate(void *user, size_t n) {
    (void)user;
    allocation_calls++;
    if (reentrant_session) {
        assert(!cui_text_prepare_line(reentrant_session, reentrant_font, reentrant_command, 1));
        cui_text_prepare_session_end(reentrant_session);
        assert(reentrant_session->busy);
    }
    if (allocation_calls == allocation_fail)
        return NULL;
    void *p = malloc(n);
    assert(p);
    live_blocks++;
    return p;
}
static void failing_release(void *user, void *p) {
    (void)user;
    assert(p && live_blocks);
    if (reentrant_session) {
        assert(!cui_text_prepare_line(reentrant_session, reentrant_font, reentrant_command, 1));
        cui_text_prepare_session_end(reentrant_session);
        assert(reentrant_session->busy);
    }
    live_blocks--;
    free(p);
}
static void failures(cui_font *font) {
    cui_draw_command c = fixture(0);
    size_t calls = 0;
    cui_text_owner_internal o = owner(&calls);
    o.allocate = failing_allocate;
    o.release = failing_release;
    cui_text_prepare_session s = {.owner = &o};
    allocation_calls = 0;
    allocation_fail = 0;
    reentrant_session = &s;
    reentrant_font = font;
    reentrant_command = &c;
    assert(cui_text_prepare_line(&s, font, &c, 1));
    size_t count = allocation_calls, dependencies = o.dependencies;
    cui_text_prepare_session_end(&s);
    assert(!live_blocks);
    reentrant_session = NULL;
    for (size_t fail = 1; fail <= count; fail++) {
        o = owner(&calls);
        o.allocate = failing_allocate;
        o.release = failing_release;
        s = (cui_text_prepare_session){.owner = &o};
        allocation_calls = 0;
        allocation_fail = fail;
        assert(!cui_text_prepare_line(&s, font, &c, 1));
        assert(!s.first && !s.last && !live_blocks && !o.cpu_used && !o.metadata_used);
        cui_text_prepare_session_end(&s);
    }
    allocation_fail = 0;
    for (size_t fail = 1; fail <= dependencies; fail++) {
        o = owner(&calls);
        o.fail_dependency = fail;
        s = (cui_text_prepare_session){.owner = &o};
        assert(!cui_text_prepare_line(&s, font, &c, 1));
        assert(!s.first && !o.cpu_used && !o.metadata_used);
        cui_text_prepare_session_end(&s);
    }
    o = owner(&calls);
    s = (cui_text_prepare_session){.owner = &o};
    assert(cui_text_prepare_line(&s, font, &c, 1));
    cui_text_recorded_glyph_internal *first = s.first, *last = s.last;
    o.cpu_limit = o.cpu_used;
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(s.first == first && s.last == last && !last->next);
    cui_text_prepare_session_end(&s);
    assert(!o.cpu_used && !o.metadata_used);
    o = owner(&calls);
    o.metadata_limit = 1;
    s = (cui_text_prepare_session){.owner = &o};
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(!s.first && !o.cpu_used && !o.metadata_used);
    cui_text_prepare_session_end(&s);
    o = owner(&calls);
    o.input_bytes = 256 * 1024;
    s = (cui_text_prepare_session){.owner = &o};
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(!o.dependencies);
    cui_text_prepare_session_end(&s);
    o = owner(&calls);
    o.attempts = 8192;
    s = (cui_text_prepare_session){.owner = &o};
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(!s.first && !o.cpu_used);
    cui_text_prepare_session_end(&s);
    o = owner(&calls);
    o.segments = 4096;
    s = (cui_text_prepare_session){.owner = &o};
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(!s.first && !o.cpu_used && !o.metadata_used);
    cui_text_prepare_session_end(&s);
    o = owner(&calls);
    o.glyphs = 65536;
    s = (cui_text_prepare_session){.owner = &o};
    assert(!cui_text_prepare_line(&s, font, &c, 1));
    assert(!s.first && !o.cpu_used);
    cui_text_prepare_session_end(&s);
    o = owner(&calls);
    s = (cui_text_prepare_session){.owner = &o};
    assert(cui_text_prepare_line(&s, font, &c, 1));
    cui_text_prepare_session_end(&s);
    assert(!font->glyph_cache);
}
#endif
int main(int argc, char **argv) {
    assert(argc == 4);
    cui_font *font = cui_font_open(argv[1]), *bold = cui_font_open(argv[2]);
    assert(font && bold);
    assert(cui_font_set_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD}, bold));
#ifdef CUI_TEXT_BASELINE_CAPTURE
    baseline(font);
    FILE *f = fopen(argv[3], "wb");
    assert(f);
    assert(fwrite(result, 1, length, f) == length);
    assert(!fclose(f));
#else
    /* First reproduce untouched legacy route against immutable pre-refactor bytes. */
    baseline(font);
    FILE *f = fopen(argv[3], "rb");
    assert(f);
    unsigned char frozen[sizeof(result)], expected[sizeof(result)];
    size_t frozen_length = fread(frozen, 1, sizeof(frozen), f), expected_length = 0;
    assert(feof(f));
    fclose(f);
    assert(cui_test_original_text_prepare(argv[1], argv[2], expected, sizeof(expected),
                                          &expected_length));
    assert(expected_length == length && !memcmp(expected, result, length));
    if (getenv("CUI_TEXT_FROZEN_EXACT") &&
        (frozen_length != expected_length || memcmp(frozen, expected, expected_length))) {
        fputs("Captured font fixture mismatch.\n", stderr);
        cui_font_close(font);
        cui_font_close(bold);
        return 2; /* Normal failure permits headless mutation rejection on every host. */
    } else if (frozen_length != expected_length || memcmp(frozen, expected, expected_length))
        puts("Captured-toolchain bytes differ; same-runtime original-source oracle remains "
             "mandatory.");
    cui_font_close(font);
    cui_font_close(bold);
    bold = cui_font_open(argv[2]);
    font = cui_font_open(argv[1]);
    assert(font);
    assert(cui_font_set_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_BOLD}, bold));
    prepared(font);
    assert(!bold->glyph_cache);
    assert(expected_length == length && !memcmp(expected, result, length));
    failures(font);
    recording_boundaries(font);
    owned_density64(font, argv[1], argv[2]);
    owned_format_and_casts(font);
#ifdef CUI_TEXT_BIDI
    fallback_recording(argv[1]);
#endif
#endif
    cui_font_close(font);
    cui_font_close(bold);
    return 0;
}
