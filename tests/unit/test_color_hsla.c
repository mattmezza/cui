/* SPDX-License-Identifier: MIT */
#include "cui/cui_color_hsla.h"
#include <assert.h>
#include <math.h>
#include <string.h>
static cui_str text(const char *s) {
    return (cui_str){s, strlen(s)};
}
static bool near(float a, float b) {
    return fabsf(a - b) < .00001f;
}
int main(void) {
    cui_color_hsla h = {.1234567f, .2345678f, .3456789f, .5f}, out = h;
    assert(cui_color_hsla_valid(h));
    assert(cui_color_hsla_to_rgba(h, NULL) == CUI_COLOR_HSLA_INVALID_ARGUMENT);
    assert(cui_color_hsla_from_color((cui_color){0}, NULL) == CUI_COLOR_HSLA_INVALID_ARGUMENT);
    assert(cui_color_hsla_to_color(h, NULL) == CUI_COLOR_HSLA_INVALID_ARGUMENT);
    assert(cui_color_hsla_from_rgba((cui_color_encoded_rgba){0, 0, 0, NAN}, &out) ==
           CUI_COLOR_HSLA_INVALID_ARGUMENT);
    assert(!memcmp(&h, &out, sizeof h));
    float invalid[] = {NAN, INFINITY, -INFINITY, -.01f, 1.01f};
    for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i) {
        cui_color_hsla bad = h;
        bad.h = invalid[i];
        assert(!cui_color_hsla_valid(bad));
        cui_color_encoded_rgba sentinel = {.1f, .2f, .3f, .4f}, rgba = sentinel;
        assert(cui_color_hsla_to_rgba(bad, &rgba) == CUI_COLOR_HSLA_INVALID_ARGUMENT);
        assert(!memcmp(&rgba, &sentinel, sizeof rgba));
    }
    for (unsigned sector = 0; sector <= 6; ++sector) {
        cui_color_hsla c = {sector / 6.f, 1, .5f, 1};
        cui_color_encoded_rgba rgb;
        assert(cui_color_hsla_to_rgba(c, &rgb) == CUI_COLOR_HSLA_OK);
        assert(cui_color_hsla_from_rgba(rgb, &out) == CUI_COLOR_HSLA_OK);
        assert(near(out.h, sector == 6 ? 0 : c.h) && near(out.s, 1) && near(out.l, .5f));
    }
    for (unsigned i = 0; i < 1000; ++i) {
        cui_color_encoded_rgba rgb = {(i % 101) / 100.f, ((i * 17) % 101) / 100.f,
                                      ((i * 37) % 101) / 100.f, .123456f},
                               back;
        assert(cui_color_hsla_from_rgba(rgb, &out) == CUI_COLOR_HSLA_OK);
        assert(cui_color_hsla_to_rgba(out, &back) == CUI_COLOR_HSLA_OK);
        assert(near(rgb.r, back.r) && near(rgb.g, back.g) && near(rgb.b, back.b) &&
               rgb.a == back.a);
    }
    const char *valid[] = {"#abc",    "aBc",    "#abcd",     "abcd",
                           "#AABBCC", "aabbcc", "#AABBCCDD", "aabbccdd"};
    for (size_t i = 0; i < sizeof valid / sizeof *valid; ++i) {
        assert(cui_color_hsla_parse_hex(text(valid[i]), &out) == CUI_COLOR_HSLA_HEX_COMPLETE);
        assert(near(out.a, i == 2 || i == 3 || i >= 6 ? 221 / 255.f : 1));
    }
    const char *incomplete[] = {"", "#", "1", "#12", "12345", "#1234567"};
    for (size_t i = 0; i < sizeof incomplete / sizeof *incomplete; ++i) {
        out = h;
        assert(cui_color_hsla_parse_hex(text(incomplete[i]), &out) ==
               CUI_COLOR_HSLA_HEX_INCOMPLETE);
        assert(!memcmp(&h, &out, sizeof h));
    }
    const char *bad[] = {"+abc", "#-fff", "#éff", "#123456789", " abc", "##abc", "#abc\n"};
    for (size_t i = 0; i < sizeof bad / sizeof *bad; ++i) {
        out = h;
        assert(cui_color_hsla_parse_hex(text(bad[i]), &out) == CUI_COLOR_HSLA_HEX_INVALID);
        assert(!memcmp(&h, &out, sizeof h));
    }
    assert(cui_color_hsla_parse_hex((cui_str){NULL, 1}, &out) == CUI_COLOR_HSLA_HEX_INVALID);
    for (unsigned alpha = 0; alpha < 2; ++alpha) {
        cui_color_hsla red = {0, 1, .5f, alpha ? .5f : 1};
        size_t required = alpha ? 9 : 7;
        for (size_t cap = 0; cap <= required + 1; ++cap) {
            char storage[12];
            memset(storage, 'x', sizeof storage);
            cui_color_hsla_format_result r = cui_color_hsla_format_hex(red, storage, cap);
            assert(r.required == required);
            if (cap <= required) {
                assert(r.status == CUI_COLOR_HSLA_CAPACITY);
                for (size_t j = 0; j < sizeof storage; ++j)
                    assert(storage[j] == 'x');
            } else {
                assert(r.status == CUI_COLOR_HSLA_OK &&
                       !strcmp(storage, alpha ? "#FF00007F" : "#FF0000"));
                assert(storage[cap] == 'x');
            }
        }
        assert(cui_color_hsla_format_hex(red, NULL, 0).required == required);
        assert(cui_color_hsla_format_hex(red, NULL, 1).status == CUI_COLOR_HSLA_INVALID_ARGUMENT);
    }
    cui_color a, b;
    assert(cui_color_hsla_to_color((cui_color_hsla){0, 0, 0, 127.99f / 255.f}, &a) ==
           CUI_COLOR_HSLA_OK);
    assert(cui_color_hsla_to_color((cui_color_hsla){0, 0, 0, 128.01f / 255.f}, &b) ==
           CUI_COLOR_HSLA_OK);
    assert(a.a == 127 && b.a == 128);
    return 0;
}
