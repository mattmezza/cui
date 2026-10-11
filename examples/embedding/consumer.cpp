#include "cui/cui.h"
#include <cstdio>
int main() {
    cui_config config = {};
    config.width = 320;
    config.height = 200;
    cui_context *ui = cui_create(config);
    if (!ui)
        return 1;
    if (!cui_begin_frame(ui, 320, 200, 0))
        return 2;
    cui_box box = {};
    box.width = -1;
    box.height = -1;
    box.padding = 16;
    cui_begin_box(ui, cui_id_from(ui, "cpp-host"), box);
    cui_text(ui, cui_string("C ABI from C++"), 18, cui_color{230, 230, 230, 255});
    cui_end_box(ui);
    auto list = cui_end_frame(ui);
    std::printf("C++ host consumed %zu C draw commands\n", list.count);
    cui_destroy(ui);
    return list.count ? 0 : 3;
}
