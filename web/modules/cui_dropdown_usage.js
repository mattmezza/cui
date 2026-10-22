/* SPDX-License-Identifier: MIT */
export function dropdownUsage(policy, shadow, names=0, order=0, skip=0, toggle=0) {
 return `#include "cui/cui_dropdown_button_styles.h"
#include "cui/cui_dropdown_button_metadata.h"

/* Retain the initialized menu state and action across frames.
 * Call inside a CUI frame. The caller handles result.action_activated once.
 * Menu entries and their strings must remain alive through frame completion. */
cui_dropdown_button_result draw_export(
    cui_context *ui, const cui_theme *theme, cui_menu_tree_state *menu,
    cui_id *menu_action, const cui_menu_entry *entries, size_t count,
    cui_dropdown_button_options options) {
    cui_dropdown_button_styles styles = {
        .paint_policy = ${policy ? 'CUI_DROPDOWN_BUTTON_PAINT_BUTTON' : 'CUI_DROPDOWN_BUTTON_PAINT_LEGACY'},
        .custom_shadow = ${shadow ? 'true' : 'false'},
        .host_shadows_enabled = true
    };
    const cui_dropdown_button_metadata metadata = {
        .action = {.custom_name = ${names ? 'true' : 'false'},
                   .name = {"${names===2 ? '' : 'Exporter le projet'}", ${names===2 ? 0 : 18}},
                   .custom_tab_index = ${order ? 'true' : 'false'},
                   .tab_index = ${order===2 ? 2 : -2}, .skip_tab = ${skip===1 ? 'true' : 'false'}},
        .trigger = {.custom_name = ${names ? 'true' : 'false'},
                    .name = {"${names===2 ? '' : 'Autres exports'}", ${names===2 ? 0 : 14}},
                    .custom_tab_index = ${order ? 'true' : 'false'},
                    .tab_index = ${order===2 ? -2 : 2}, .skip_tab = ${skip===2 ? 'true' : 'false'}},
        .toggle_semantics = ${toggle ? 'true' : 'false'}, .toggled = ${toggle===2 ? 'true' : 'false'}
    };
    /* Checked/value follows CUI Button; optional pressed AT presence is not exposed.
     * Names are immutable borrowed UTF-8 through submission.
     * Shadow requires BUTTON policy, CUSTOM variant and an enabled half.
     * Callback refinements are optional; see DropdownButton styles docs. */
    return cui_dropdown_button_with_metadata_ex(
        ui, cui_id_from(ui, "export"), cui_id_from(ui, "export-action"),
        cui_id_from(ui, "export-menu"), theme, cui_string("Export workspace"),
        cui_string("Export options"), entries, count, menu_action, menu,
        options, &styles, &metadata);
}
`;
}
