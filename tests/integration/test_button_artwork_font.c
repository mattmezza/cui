/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#define main artwork_portable_main
#include "../unit/test_button_artwork.c"
#undef main
int main(int argc, char **argv) {
    assert(argc == 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    fixture_measure = cui_font_measure;
    fixture_measure_user = font;
    artwork_portable_main();
    cui_font_close(font);
    return 0;
}
