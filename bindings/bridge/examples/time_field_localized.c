/* SPDX-License-Identifier: MIT */
/* Complete shared authored UI: windowless test or optional interactive SDL host.
 * SOURCE ONLY: both paths await compile/runtime proof. */
#include "cui/cui_bridge.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define CHECK(e)                                                                                   \
    do {                                                                                           \
        if (!(e)) {                                                                                \
            fprintf(stderr, "line%d: %s\n", __LINE__, #e);                                         \
            goto failed;                                                                           \
        }                                                                                          \
    } while (0)

typedef struct {
    cui_bridge_node english, arabic, summary, clear, readonly_button, disabled_button,
        bounds_button;
    uint32_t english_disabled;
    cui_bridge_time_field_config_v1 english_config, arabic_config;
    cui_bridge_time latest;
    unsigned changes;
} appointments;
static int node(cui_bridge_app *app, cui_bridge_node parent, cui_bridge_kind kind, const char *key,
                const char *text, cui_bridge_node *out) {
    return cui_bridge_node_create(app, parent, kind, key, out) == CUI_BRIDGE_OK &&
           (!*text || cui_bridge_set_text(app, *out, text) == CUI_BRIDGE_OK);
}
static int token(cui_bridge_time_token_v1 *out, const char *s) {
    size_t length = strlen(s);
    if (!length || length > sizeof out->bytes)
        return 0;
    memset(out, 0, sizeof *out);
    out->length = (uint32_t)length;
    memcpy(out->bytes, s, length);
    return 1;
}
static int configure(cui_bridge_time_field_config_v1 *out, int arabic) {
    *out = (cui_bridge_time_field_config_v1){
        .struct_size = sizeof *out,
        .version = 1,
        .cycle = arabic ? CUI_BRIDGE_TIME_H12_V1 : CUI_BRIDGE_TIME_H23_V1,
        .precision = CUI_BRIDGE_TIME_SECOND_V1,
        .edit_generation = 1,
        .bounds_flags = CUI_BRIDGE_TIME_HAS_MINIMUM_V1 | CUI_BRIDGE_TIME_HAS_MAXIMUM_V1,
        .minimum = {8, 0, 0},
        .maximum = {18, 0, 0}};
    const char *ascii[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ":", " ", "AM", "PM"};
    const char *localized[] = {"٠", "١", "٢", "٣", "٤", "٥", "٦",
                               "٧", "٨", "٩", ":", " ", "ص", "م"};
    for (unsigned i = 0; i < 14; i++)
        if (!token(&out->tokens[i], arabic ? localized[i] : ascii[i]))
            return 0;
    if (arabic) {
        const char *names[] = {"ساعة", "دقيقة", "ثانية", "فترة"};
        for (unsigned i = 0; i < 4; i++) {
            size_t length = strlen(names[i]);
            if (length > sizeof out->names[i].bytes)
                return 0;
            out->names[i].length = (uint32_t)length;
            memcpy(out->names[i].bytes, names[i], length);
        }
    }
    return 1;
}
/* Called only after bridge_step returns, including a non-OK frame. All records
 * below are owned copies. Setters run outside the Core borrow/submission. */
static int dispatch(cui_bridge_app *app, appointments *owner) {
    cui_bridge_event_ex generic;
    char text[4097];
    uint32_t required;
    cui_bridge_status status;
    while ((status = cui_bridge_poll_event_ex(app, &generic, text, sizeof text, &required)) ==
           CUI_BRIDGE_OK) {
        if (generic.type != CUI_BRIDGE_EVENT_CLICK)
            continue;
        if (generic.node.slot == owner->clear.slot &&
            generic.node.generation == owner->clear.generation) {
            if (cui_bridge_time_field_clear_validation_v1(app, owner->english) != CUI_BRIDGE_OK ||
                cui_bridge_time_field_clear_validation_v1(app, owner->arabic) != CUI_BRIDGE_OK)
                return 0;
        } else if (generic.node.slot == owner->readonly_button.slot &&
                   generic.node.generation == owner->readonly_button.generation) {
            owner->english_config.flags ^= CUI_BRIDGE_TIME_FIELD_READONLY_V1;
            if (cui_bridge_time_field_configure_v1(app, owner->english, &owner->english_config) !=
                CUI_BRIDGE_OK)
                return 0;
        } else if (generic.node.slot == owner->disabled_button.slot &&
                   generic.node.generation == owner->disabled_button.generation) {
            owner->english_disabled ^= 1u;
            if (cui_bridge_set_disabled(app, owner->english, owner->english_disabled) !=
                CUI_BRIDGE_OK)
                return 0;
        } else if (generic.node.slot == owner->bounds_button.slot &&
                   generic.node.generation == owner->bounds_button.generation) {
            if (owner->english_config.edit_generation == UINT64_MAX)
                return 0;
            owner->english_config.edit_generation++;
            owner->english_config.bounds_flags ^=
                CUI_BRIDGE_TIME_HAS_MINIMUM_V1 | CUI_BRIDGE_TIME_HAS_MAXIMUM_V1;
            owner->english_config.minimum = owner->english_config.bounds_flags
                                                ? (cui_bridge_time){8, 0, 0}
                                                : (cui_bridge_time){0};
            owner->english_config.maximum = owner->english_config.bounds_flags
                                                ? (cui_bridge_time){18, 0, 0}
                                                : (cui_bridge_time){0};
            if (cui_bridge_time_field_configure_v1(app, owner->english, &owner->english_config) !=
                CUI_BRIDGE_OK)
                return 0;
        }
    }
    if (status != CUI_BRIDGE_EMPTY)
        return 0;
    cui_bridge_time_field_change_v1 event = {.struct_size = sizeof event, .version = 1};
    while ((status = cui_bridge_time_field_poll_change_v1(app, &event)) == CUI_BRIDGE_OK) {
        owner->latest = event.value;
        owner->changes++;
        char label[160];
        snprintf(label, sizeof label,
                 "Applied %02" PRIu32 ":%02" PRIu32 ":%02" PRIu32
                 "; observations %u; frame status %" PRIu32,
                 event.value.hour, event.value.minute, event.value.second, owner->changes,
                 event.frame_status);
        if (cui_bridge_set_text(app, owner->summary, label) != CUI_BRIDGE_OK)
            return 0;
        printf("node=%" PRIu32 ":%" PRIu32 " frame=%" PRIu64 " previous=%" PRIu32 ":%" PRIu32
               ":%" PRIu32 " value=%" PRIu32 ":%" PRIu32 ":%" PRIu32 " frame_status=%" PRIu32 "\n",
               event.node.slot, event.node.generation, event.frame_sequence, event.previous.hour,
               event.previous.minute, event.previous.second, event.value.hour, event.value.minute,
               event.value.second, event.frame_status);
    }
    return status == CUI_BRIDGE_EMPTY;
}
static int frame(cui_bridge_app *app, appointments *owner) {
    cui_bridge_status status = cui_bridge_step(app, .016f);
    /* Observe any applied changes even when a later render/submission failed. */
    int dispatched = dispatch(app, owner);
    return dispatched && status == CUI_BRIDGE_OK;
}
static int paste(cui_bridge_app *app, cui_bridge_node field, const char *text) {
    cui_bridge_rect bounds;
    return cui_bridge_node_bounds(app, field, &bounds) == CUI_BRIDGE_OK &&
           cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_DOWN, bounds.x + bounds.width / 2,
                                    bounds.y + bounds.height / 2) == CUI_BRIDGE_OK &&
           cui_bridge_queue_pointer(app, CUI_BRIDGE_POINTER_UP, bounds.x + bounds.width / 2,
                                    bounds.y + bounds.height / 2) == CUI_BRIDGE_OK &&
           cui_bridge_queue_text(app, text) == CUI_BRIDGE_OK;
}
typedef struct {
    cui_bridge_app *app;
    appointments *owner;
} interactive;
static cui_bridge_status after_frame(void *user) {
    interactive *run = user;
    return dispatch(run->app, run->owner) ? CUI_BRIDGE_OK : CUI_BRIDGE_CORE_ERROR;
}
int main(int argc, char **argv) {
    int native = argc == 2 && strcmp(argv[1], "--native") == 0;
    if (argc > 2 || (argc == 2 && !native)) {
        fprintf(stderr, "usage: %s [--native]\n", argv[0]);
        return EXIT_FAILURE;
    }
    const char *font = native ? getenv("CUI_TEST_FONT") : NULL;
    if (native && (!font || !*font)) {
        fputs("--native requires CUI_TEST_FONT=/absolute/path/font.ttf\n", stderr);
        return EXIT_FAILURE;
    }
    cui_bridge_app *app = NULL;
    appointments owner = {0};
    cui_bridge_options options = {.struct_size = sizeof options,
                                  .abi_version = CUI_BRIDGE_ABI_VERSION,
                                  .width = 760,
                                  .height = 500,
                                  .max_nodes = 32,
                                  .max_events = 32,
                                  .backend =
                                      native ? CUI_BRIDGE_BACKEND_SDL : CUI_BRIDGE_BACKEND_NONE,
                                  .title_utf8 = "Localized appointments",
                                  .font_path_utf8 = font};
    cui_bridge_status status;
    CHECK(cui_bridge_time_field_versions() & 1u);
    app = cui_bridge_create(&options, &status);
    CHECK(app && status == CUI_BRIDGE_OK);
    /* Two configured live fields reserve two possible observations every frame. */
    CHECK(cui_bridge_time_field_enable_v1(app, 16) == CUI_BRIDGE_OK);
    cui_bridge_node root, row, controls;
    CHECK(node(app, (cui_bridge_node){0}, CUI_BRIDGE_COLUMN, "appointments", "", &root));
    CHECK(node(app, root, CUI_BRIDGE_LABEL, "heading", "Appointments; same-day08:00–18:00",
               &owner.summary));
    CHECK(node(app, root, CUI_BRIDGE_ROW, "english-row", "", &row));
    CHECK(node(app, row, CUI_BRIDGE_TIME_FIELD, "english", "English24h", &owner.english));
    CHECK(node(app, root, CUI_BRIDGE_ROW, "arabic-row", "", &row));
    CHECK(node(app, row, CUI_BRIDGE_TIME_FIELD, "arabic", "Arabic12h", &owner.arabic));
    CHECK(configure(&owner.english_config, 0) && configure(&owner.arabic_config, 1));
    CHECK(cui_bridge_set_time(app, owner.english, (cui_bridge_time){9, 30, 0}) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_set_time(app, owner.arabic, (cui_bridge_time){10, 15, 0}) == CUI_BRIDGE_OK);
    CHECK(cui_bridge_time_field_configure_v1(app, owner.english, &owner.english_config) ==
          CUI_BRIDGE_OK);
    CHECK(cui_bridge_time_field_configure_v1(app, owner.arabic, &owner.arabic_config) ==
          CUI_BRIDGE_OK);
    CHECK(node(app, root, CUI_BRIDGE_ROW, "controls", "", &controls));
    CHECK(node(app, controls, CUI_BRIDGE_BUTTON, "clear", "Clear validation", &owner.clear));
    CHECK(node(app, controls, CUI_BRIDGE_BUTTON, "readonly", "Toggle English read-only",
               &owner.readonly_button));
    CHECK(node(app, root, CUI_BRIDGE_ROW, "more-controls", "", &controls));
    CHECK(node(app, controls, CUI_BRIDGE_BUTTON, "disabled", "Toggle English disabled",
               &owner.disabled_button));
    CHECK(node(app, controls, CUI_BRIDGE_BUTTON, "bounds", "Toggle English bounds",
               &owner.bounds_button));
    if (native) {
        /* Same nodes, values, controls and copied-observation dispatch; no scripted
         * input/step inside the runner callback. Existing host polls real input. */
        interactive run = {app, &owner};
        status = cui_bridge_run(app, after_frame, &run);
        CHECK(status == CUI_BRIDGE_CLOSED || status == CUI_BRIDGE_OK);
        cui_bridge_destroy(app);
        return EXIT_SUCCESS;
    }
    CHECK(frame(app, &owner) && frame(app, &owner));
    CHECK(paste(app, owner.english, "11:45:30") && frame(app, &owner));
    CHECK(paste(app, owner.arabic, "٠١:٢٠:٠٠ م") && frame(app, &owner));
    /* A rejected edit is validation, not a controlled owner proposal. */
    CHECK(paste(app, owner.english, "23:00:00") && frame(app, &owner));
    cui_bridge_time_field_result_v1 result = {.struct_size = sizeof result, .version = 1};
    CHECK(cui_bridge_time_field_get_result_v1(app, owner.english, &result) == CUI_BRIDGE_OK);
    CHECK(result.validation == CUI_BRIDGE_TIME_OUT_OF_RANGE_V1 && !result.changed);
    CHECK(cui_bridge_time_field_clear_validation_v1(app, owner.english) == CUI_BRIDGE_OK);
    CHECK(frame(app, &owner));
    CHECK(owner.changes == 2);
    cui_bridge_destroy(app);
    return EXIT_SUCCESS;
failed:
    cui_bridge_destroy(app);
    return EXIT_FAILURE;
}
