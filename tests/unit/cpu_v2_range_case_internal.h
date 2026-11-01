/* SPDX-License-Identifier: MIT */
/* Included independently after the original/candidate private validator. No
 * private plan crosses the TU boundary; only explicit active fields/diagnostics. */
#include "cpu_v2_range_reference_internal.h"
#include <assert.h>
#include <string.h>
static cpu_range_result CPU_RANGE_RESULT(const cui_draw_list_v2 *list,
                                         const cui_render_v2_options *options, unsigned flags,
                                         const void *owner, size_t owner_bytes,
                                         unsigned output_alias) {
    _Alignas(cui_draw_list_v2) _Alignas(cui_render_v2_options) _Alignas(cui_draw_command_v2)
        cui_cpu_v2_plan plan;
    memset(&plan, 0xa5, sizeof plan);
    volatile uintptr_t address = (uintptr_t)&plan;
    void *alias = (void *)address;
    cui_draw_list_v2 local_list;
    cui_render_v2_options local_options;
    cui_draw_command_v2 local_command;
    if (output_alias) {
        local_list = *list;
        local_options = *options;
        assert(list->count == 1);
        local_command = list->commands[0];
        local_list.commands = &local_command;
        list = &local_list;
        options = &local_options;
        switch (output_alias) {
        case RANGE_OUT_LIST:
            list = alias;
            break;
        case RANGE_OUT_OPTIONS:
            options = alias;
            break;
        case RANGE_OUT_COMMANDS:
            local_list.commands = alias;
            break;
        case RANGE_OUT_IMAGE:
            local_command.payload.legacy.bitmap.bitmap.pixels = alias;
            break;
        case RANGE_OUT_MESH:
            local_command.payload.legacy.mesh.vertices = alias;
            break;
        case RANGE_OUT_TEXT:
            local_command.payload.legacy.text.data = alias;
            break;
        case RANGE_OUT_SPANS:
            local_command.payload.legacy.text_colors = alias;
            break;
        case RANGE_OUT_STOPS:
            local_command.payload.legacy.text_paint.stops = alias;
            /* Preserve the original pre-paint owner rejection path: do not ask
             * the original to dereference poisoned active gradient data. */
            owner = &plan;
            owner_bytes = sizeof plan;
            break;
        default:
            assert(false);
        }
    }
    unsigned char before[sizeof plan];
    memcpy(before, &plan, sizeof plan);
    const cui_cpu_v2_exclusion range = {owner, owner_bytes};
    cpu_range_result result = {0};
    result.error = cui_cpu_v2_validate_ranges(list, options, flags & 1, flags & 2, flags & 4, &plan,
                                              &range, 1);
    result.unchanged = memcmp(before, &plan, sizeof plan) == 0;
    if (result.error == CUI_OK) {
        const size_t fields[] = {plan.commands,         plan.backgrounds,   plan.background_pixels,
                                 plan.background_bytes, plan.scratch_bytes, plan.shadow_bytes,
                                 plan.metadata_bytes,   plan.cpu_bytes,     plan.inspected_bytes};
        memcpy(result.fields, fields, sizeof fields);
        result.text = plan.needs_text_preparation;
        result.shadow = plan.needs_shadow_preparation;
    }
    return result;
}
