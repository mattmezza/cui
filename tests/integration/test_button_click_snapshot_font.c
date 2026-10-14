/* SPDX-License-Identifier: MIT */
#include "cui/cui_text.h"
#define CUI_BUTTON_SNAPSHOT_MAIN snapshot_portable_main
#include "../unit/test_button_click_snapshot.c"
#undef CUI_BUTTON_SNAPSHOT_MAIN
int main(int argc, char **argv) {
    assert(argc == 2);
    cui_font *font = cui_font_open(argv[1]);
    assert(font);
    fixture_measure = cui_font_measure;
    fixture_measure_user = font;
    snapshot_portable_main();
    cui_font_close(font);
    return 0;
}
