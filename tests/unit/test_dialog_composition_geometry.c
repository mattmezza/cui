/* SPDX-License-Identifier: MIT */
#include "dialog_composition_helpers.h"
#include <stdlib.h>
/* Values are the independently executed pinned Taffy source_* rows in
 * tests/reference/dialog_flex/golden.csv, not recaptured CUI output. */
static const float expected[10][6] = {{260, 172, 44, 44, 92, 204}, {260, 172, 96, 0, 92, 204},
                                      {260, 172, 44, 44, 92, 204}, {260, 172, 0, 0, 0, 204},
                                      {260, 172, 240, 0, 92, 204}, {202, 114, 15, 15, 92, 146},
                                      {260, 228, 72, 72, 92, 0},   {260, 0, 40, 0, 92, 32},
                                      {260, 0, 40, 0, 92, 32},     {562, 114, 15, 15, 92, 146}};
static bool red_at(cui_software *renderer, unsigned x, unsigned y, unsigned red) {
    unsigned width, height;
    const unsigned char *pixels = cui_software_pixels(renderer, &width, &height);
    assert(pixels && x < width && y < height);
    const unsigned char *p = pixels + ((size_t)y * width + x) * 4;
    return p[0] == red && p[1] == 0 && p[2] == 0 && p[3] == 255;
}
static void source_pixels(dialog_fixture *f, const cui_draw_list *draw, unsigned row) {
    if (row != 4 && row != 7 && row != 8)
        return;
    cui_software *renderer = cui_software_create((unsigned)f->width, (unsigned)f->height, NULL);
    assert(renderer && cui_software_render(renderer, draw, (cui_color){1, 2, 3, 255}));
    unsigned x = (unsigned)(f->layout.completed[0].x + 20);
    unsigned top = (unsigned)f->layout.completed[0].y;
    if (row == 4) {
        assert(red_at(renderer, x, top + 97, 120));   /* Content within Inner clip */
        assert(!red_at(renderer, x, top + 195, 120)); /* Content clipped before Footer */
        assert(red_at(renderer, x, top + 209, 180));  /* Footer remains independently visible */
    } else {
        assert(red_at(renderer, x, top + 37, 180));
        assert(red_at(renderer, x, top + 270, 180)); /* explicit source Footer overflow */
    }
    cui_software_destroy(renderer);
}
static void source_rows(bool styled) {
    for (unsigned k = 0; k < 10; ++k) {
        dialog_fixture f;
        dialog_fixture_init(&f, styled, NULL);
        f.height = 800;
        f.sizing.requested_height = 260;
        f.sizing.min_height = 96;
        f.sizing.max_height = k == 9 ? 600 : 260;
        f.composition.omit = 1u << CUI_DIALOG_DESCRIPTION;
        f.composition.present = (1u << CUI_DIALOG_HEADER) | (1u << CUI_DIALOG_TITLE) |
                                (1u << CUI_DIALOG_CONTENT) | (1u << CUI_DIALOG_BODY) |
                                (1u << CUI_DIALOG_FOOTER);
        f.metric[CUI_DIALOG_HEADER] = 40;
        f.metric[CUI_DIALOG_TITLE] = 20;
        f.metric[CUI_DIALOG_CONTENT] = 40;
        f.metric[CUI_DIALOG_BODY] = 800;
        f.metric[CUI_DIALOG_FOOTER] = 40;
        if (k == 1 || k == 3)
            f.composition.present &= ~(1u << CUI_DIALOG_BODY);
        if (k == 3)
            f.composition.present &= ~(1u << CUI_DIALOG_CONTENT);
        if (k == 2)
            f.metric[CUI_DIALOG_BODY] = 0;
        if (k == 4)
            f.metric[CUI_DIALOG_CONTENT] = 240;
        if (k == 5 || k == 9) {
            f.metric[CUI_DIALOG_CONTENT] = 10;
            f.metric[CUI_DIALOG_BODY] = 20;
        }
        if (k == 5 || k == 8 || k == 9)
            f.sizing.mode = CUI_SCROLL_BODY_AUTO;
        if (f.sizing.mode == CUI_SCROLL_BODY_AUTO)
            f.sizing.requested_height = 0;
        if (k == 6)
            f.composition.present &= ~(1u << CUI_DIALOG_FOOTER);
        if (k >= 7)
            f.metric[CUI_DIALOG_FOOTER] = 400;
        cui_draw_list draw = dialog_fixture_frame(&f);
        source_pixels(&f, &draw, k);
        float y = f.layout.completed[0].y;
        dialog_equal(f.layout.completed[0].h, expected[k][0], "source Panel");
        dialog_equal(f.layout.completed[8].h, expected[k][1], "source Inner");
        dialog_equal(f.layout.completed[4].h, expected[k][2], "source Content");
        dialog_equal(f.layout.completed[5].h, expected[k][3], "source Body");
        if (f.composition.present & (1u << CUI_DIALOG_CONTENT))
            dialog_equal(f.layout.completed[4].y - y, expected[k][4], "source Content Y");
        if (f.composition.present & (1u << CUI_DIALOG_FOOTER))
            dialog_equal(f.layout.completed[7].y - y, expected[k][5], "source Footer Y");
        for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
            assert(f.calls[i] == ((f.composition.present >> i) & 1u));
        dialog_fixture_destroy(&f);
    }
}
static void current_font(const char *path, const char *bold_path, bool styled) {
    cui_font *font = cui_font_open(path);
    assert(font);
    assert(cui_font_register_style(font, (cui_text_style){.weight = CUI_TEXT_WEIGHT_SEMIBOLD},
                                   bold_path));
    dialog_fixture f;
    dialog_fixture_init(&f, styled, font);
    f.text_title = true;
    f.composition.present = 1u << CUI_DIALOG_TITLE;
    f.composition.omit = 1u << CUI_DIALOG_DESCRIPTION;
    f.sizing.mode = CUI_SCROLL_BODY_AUTO;
    f.sizing.requested_height = 0;
    f.sizing.max_height = 700;
    f.height = 800;
    const float widths[] = {240, 150, 360, 150};
    float narrow = 0;
    for (unsigned i = 0; i < 4; ++i) {
        f.sizing.width = widths[i];
        dialog_fixture_frame(&f);
        float title = f.layout.completed[2].h;
        assert(title > 0);
        dialog_equal(f.layout.completed[0].h, fmaxf(96, title + 32), "current-font intrinsic");
        assert(f.calls[CUI_DIALOG_TITLE] == 1);
        if (i == 1)
            narrow = title;
        if (i == 2)
            assert(title < narrow);
        if (i == 3)
            dialog_equal(title, narrow, "repeat narrow wrapping");
    }
    /* Changed borrowed title text is measured on its first presented frame. */
    f.title_text = "Short alert";
    dialog_fixture_frame(&f);
    float short_title = f.layout.completed[2].h;
    assert(short_title > 0);
    dialog_equal(f.layout.completed[0].h, fmaxf(96, short_title + 32),
                 "new short title first frame");
    f.title_text = "Changed title\nSecond current line\nThird current line";
    f.sizing.width = 360;
    dialog_fixture_frame(&f);
    assert(f.layout.completed[2].h > short_title * 2);
    dialog_equal(f.layout.completed[0].h, fmaxf(96, f.layout.completed[2].h + 32),
                 "new multiline title first frame");
    assert(f.calls[CUI_DIALOG_TITLE] == 1);
    f.text_title = false;
    f.composition.present |= (1u << CUI_DIALOG_BODY) | (1u << CUI_DIALOG_FOOTER);
    f.slot_text[CUI_DIALOG_TITLE] = "Short alert title";
    f.slot_text[CUI_DIALOG_BODY] = "Short body.";
    f.slot_text[CUI_DIALOG_FOOTER] = "Confirm";
    f.sizing.width = 300;
    f.sizing.max_height = 240;
    dialog_fixture_frame(&f);
    float short_panel = f.layout.completed[0].h;
    assert(short_panel < 240 && f.surface.body_maximum == 0);
    f.slot_text[CUI_DIALOG_BODY] =
        "Long current body with paragraphs that wrap at the current available width. "
        "Changing the viewport must change these measured lines immediately. "
        "The header and footer remain separate while this body scrolls. "
        "Another sentence forces enough lines to exercise the current frame range. "
        "Repeated content remains ordinary application text and is submitted once.";
    f.slot_text[CUI_DIALOG_FOOTER] = "Confirm changed action label with wrapping";
    f.width = 210;
    f.height = 300;
    f.sizing.width = 170;
    dialog_fixture_frame(&f);
    assert(f.surface.body_maximum > 0);
    assert(f.layout.completed[7].h > 0 && f.layout.completed[2].h > 0);
    assert(f.calls[CUI_DIALOG_BODY] == 1 && f.calls[CUI_DIALOG_FOOTER] == 1);
    f.slot_text[CUI_DIALOG_BODY] = "Short body.";
    f.slot_text[CUI_DIALOG_FOOTER] = "Confirm";
    f.width = 400;
    f.height = 800;
    f.sizing.width = 300;
    dialog_fixture_frame(&f);
    dialog_equal(f.layout.completed[0].h, short_panel, "current-font body/action recovery");
    assert(f.surface.body_maximum == 0);
    dialog_fixture_destroy(&f);
    cui_font_close(font);
}
/* Each assertion observes the first completed frame for that revision of the
 * open owner. No warm-up is allowed between viewport/content changes. */
static void first_presented_regions(bool styled) {
    dialog_fixture f;
    dialog_fixture_init(&f, styled, NULL);
    f.surface.kind = CUI_SURFACE_ALERT;
    f.composition.present = (1u << CUI_DIALOG_HEADER) | (1u << CUI_DIALOG_TITLE) |
                            (1u << CUI_DIALOG_DESCRIPTION) | (1u << CUI_DIALOG_BODY) |
                            (1u << CUI_DIALOG_FOOTER);
    f.composition.omit = 0;
    f.sizing.mode = CUI_SCROLL_BODY_AUTO;
    f.sizing.requested_height = 0;
    f.sizing.max_height = 500;
    f.height = 600;
    f.metric[CUI_DIALOG_HEADER] = 10;
    f.metric[CUI_DIALOG_TITLE] = 20;
    f.metric[CUI_DIALOG_DESCRIPTION] = 12;
    f.metric[CUI_DIALOG_BODY] = 30;
    f.metric[CUI_DIALOG_FOOTER] = 24;
    dialog_fixture_frame(&f);
    float short_height = f.layout.completed[0].h;
    assert(short_height < 260 && short_height >= 96);
    assert(f.layout.completed[1].h == 10 && f.layout.completed[2].h == 20);
    assert(f.layout.completed[3].h == 12 && f.layout.completed[7].h == 24);
    assert(f.surface.body_maximum == 0);
    for (unsigned i = 0; i < CUI_DIALOG_SLOT_COUNT; ++i)
        assert(f.calls[i] == (i == CUI_DIALOG_CONTENT ? 0u : 1u));
    /* Body growth and Footer/action growth must affect this same frame. */
    f.metric[CUI_DIALOG_BODY] = 1200;
    f.metric[CUI_DIALOG_FOOTER] = 48;
    f.metric[CUI_DIALOG_TITLE] = 36;
    f.width = 220;
    f.height = 320;
    f.sizing.width = 180;
    f.sizing.max_height = 260;
    dialog_fixture_frame(&f);
    dialog_equal(f.layout.completed[0].h, 260, "long current Panel");
    dialog_equal(f.layout.completed[2].h, 36, "changed current Title");
    dialog_equal(f.layout.completed[7].h, 48, "changed current Footer");
    assert(f.surface.body_maximum > 1000);
    assert(f.layout.completed[7].y >= f.layout.completed[5].y + f.layout.completed[5].h);
    /* Shrink content and resize back without retaining the tall old Panel. */
    f.metric[CUI_DIALOG_BODY] = 30;
    f.metric[CUI_DIALOG_FOOTER] = 24;
    f.metric[CUI_DIALOG_TITLE] = 20;
    f.width = 400;
    f.height = 600;
    f.sizing.width = 240;
    f.sizing.max_height = 500;
    dialog_fixture_frame(&f);
    dialog_equal(f.layout.completed[0].h, short_height, "same-frame short recovery");
    assert(f.surface.body_maximum == 0);
    dialog_fixture_destroy(&f);
}
int main(int argc, char **argv) {
    assert(argc == 3);
    first_presented_regions(false);
    first_presented_regions(true);
    source_rows(false);
    source_rows(true);
    current_font(argv[1], argv[2], false);
    current_font(argv[1], argv[2], true);
    puts("Dialog source slots/current-font geometry passed");
    return 0;
}
