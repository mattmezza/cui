/* SPDX-License-Identifier: MIT */
#include "cui/cui_bridge.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    const char *isolated = getenv("CUI_TEST_VIRTUAL_DISPLAY");
    if (!isolated || strcmp(isolated, "1") != 0)
        return 77;
    const char *primary = getenv("CUI_TEST_FONT_PATH");
    const char *fallback = getenv("CUI_TEST_FALLBACK_FONT_PATH");
    if (!primary || !fallback)
        return 77;
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .backend = CUI_BRIDGE_BACKEND_SDL,
                                  .width = 640,
                                  .height = 480,
                                  .font_path_utf8 = primary};
    cui_bridge_status status = CUI_BRIDGE_OK;
    cui_bridge_app *app = cui_bridge_create(&options, &status);
    assert(app && status == CUI_BRIDGE_OK);
    cui_bridge_node root = {0}, input = {0};
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_COLUMN, "root", &root) == CUI_BRIDGE_OK);
    assert(cui_bridge_node_create(app, root, CUI_BRIDGE_INPUT, "input", &input) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    cui_bridge_rect bounds = {0};
    assert(cui_bridge_node_bounds(app, input, &bounds) == CUI_BRIDGE_OK);
    float x = bounds.x + bounds.width / 2, y = bounds.y + bounds.height / 2;
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, x, y) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    assert(cui_bridge_queue_text(app, "\xE2\x9C\x93") == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_CORE_ERROR);
    const char *bad[] = {fallback, ""};
    assert(cui_bridge_set_font_fallbacks(app, bad, 2) == CUI_BRIDGE_INVALID_UTF8);
    assert(cui_bridge_set_font_fallbacks(app, &fallback, 33) == CUI_BRIDGE_INVALID_ARGUMENT);
    assert(cui_bridge_set_font_fallbacks(app, &fallback, 1) == CUI_BRIDGE_OK);
    assert(cui_bridge_step(app, .016f) == CUI_BRIDGE_OK);
    char text[16] = {0};
    uint32_t needed = 0;
    assert(cui_bridge_get_text(app, input, text, sizeof text, &needed) == CUI_BRIDGE_OK);
    assert(strcmp(text, "\xE2\x9C\x93") == 0);
    cui_bridge_destroy(app);
    return 0;
}
