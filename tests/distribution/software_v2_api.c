/* SPDX-License-Identifier: MIT */
#ifdef CUI_TEST_SOFTWARE_UMBRELLA
#include "cui_all.h"
#endif
#include "cui/cui_software.h"
#include <assert.h>
#include <string.h>
int main(void) {
    cui_software *renderer = cui_software_create(3, 2, NULL);
    assert(renderer);
    cui_software_v2_info info;
    memset(&info, 0, sizeof info);
    info.size = sizeof info;
    assert(cui_software_query_v2(renderer, &info) == CUI_OK);
    cui_draw_list_v2 list;
    memset(&list, 0, sizeof list);
    list.header_size = sizeof list;
    list.abi_version = CUI_DRAW_ABI_V2;
    list.command_stride = sizeof(cui_draw_command_v2);
    list.background_output = CUI_BACKGROUND_OUTPUT_SRGBA8_CENTER_V1;
    list.width = 3;
    list.height = 2;
    cui_color clear = {13, 27, 41, 59};
    assert(cui_software_render_v2(renderer, &list, clear, &info.defaults) == CUI_OK);
    const unsigned char *pixels = cui_software_pixels(renderer, NULL, NULL);
    for (unsigned i = 0; i < 6; ++i) {
        assert(pixels[i * 4] == 13 && pixels[i * 4 + 1] == 27 && pixels[i * 4 + 2] == 41 &&
               pixels[i * 4 + 3] == 59);
    }
    cui_software_destroy(renderer);
    return 0;
}
