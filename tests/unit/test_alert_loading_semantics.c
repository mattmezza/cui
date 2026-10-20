/* SPDX-License-Identifier: MIT */
#include "../../examples/loading_region/scene.h"
#include <assert.h>
#include <string.h>
static const cui_semantic *find(cui_context *ui, cui_id id) {
    size_t count;
    const cui_semantic *nodes = cui_semantics(ui, &count);
    for (size_t i = 0; i < count; i++)
        if (nodes[i].id == id)
            return &nodes[i];
    return NULL;
}
static bool named(const cui_semantic *node, const char *name) {
    return node && node->name.length == strlen(name) &&
           !memcmp(node->name.data, name, node->name.length);
}
static void frame(cui_context *ui, loading_scene *scene, double now) {
    cui_theme theme = cui_theme_light();
    assert(cui_begin_frame(ui, 400, 400, 0));
    loading_scene_frame(ui, &theme, scene, now);
    cui_end_frame(ui);
    assert(cui_last_error(ui) == CUI_OK);
}
int main(void) {
    cui_context *ui = cui_create((cui_config){.max_elements = 128});
    assert(ui);
    loading_scene scene = {.alert_visible = true, .alert_name = "Records unavailable"};
    size_t count;
    for (unsigned i = 0; i < 12; i++) {
        frame(ui, &scene, i * .17);
        cui_semantics(ui, &count);
        assert(count == 2);
        assert(find(ui, 10)->role == CUI_ROLE_REGION && find(ui, 10)->details.busy);
        assert(find(ui, 11)->role == CUI_ROLE_STATUS && find(ui, 11)->tab_index == -1);
        assert(named(find(ui, 11), "Loading records") && cui_semantic_parent(ui, 11) == 10);
        for (cui_id id = 20; id < 23; id++)
            assert(!find(ui, id));
    }
    scene.phase = LOADING_REGION_READY;
    frame(ui, &scene, 2.1);
    assert(!find(ui, 10)->details.busy && named(find(ui, 11), "Records loaded"));
    scene.phase = LOADING_REGION_FAILED;
    for (unsigned i = 0; i < 2; i++) {
        frame(ui, &scene, 2.2 + i * .1);
        const cui_semantic *alert = find(ui, 30);
        assert(named(alert, "Records unavailable") && alert->role == CUI_ROLE_ALERT);
        assert(alert->tab_index == -1 && !find(ui, 10)->details.busy && !find(ui, 11));
        assert(cui_semantic_parent(ui, 30) == 10 && cui_semantic_parent(ui, 31) == 30);
        assert(named(find(ui, 31), "Retry records") && find(ui, 31)->role == CUI_ROLE_BUTTON);
        const cui_semantic *nodes = cui_semantics(ui, &count);
        assert(count == 4);
        unsigned dismiss = 0;
        for (size_t j = 0; j < count; j++)
            if (named(&nodes[j], "Dismiss")) {
                assert(nodes[j].role == CUI_ROLE_BUTTON &&
                       cui_semantic_parent(ui, nodes[j].id) == 30);
                dismiss++;
            }
        assert(dismiss == 1);
    }
    scene.alert_name = "Retry failed";
    frame(ui, &scene, 2.5);
    assert(named(find(ui, 30), "Retry failed"));
    scene.alert_visible = false;
    frame(ui, &scene, 2.6);
    cui_semantics(ui, &count);
    assert(count == 1 && !find(ui, 30) && !find(ui, 31));
    scene.alert_visible = true;
    frame(ui, &scene, 2.7);
    assert(named(find(ui, 30), "Retry failed") && find(ui, 31));
    cui_destroy(ui);
    return 0;
}
