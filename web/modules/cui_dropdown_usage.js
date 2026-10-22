/* SPDX-License-Identifier: MIT */
export function dropdownUsage(policy, shadow) {
 return `#include "cui/cui_dropdown_button_styles.h"

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
    /* Shadow requires BUTTON policy, CUSTOM variant and an enabled half.
     * Callback refinements are optional; see DropdownButton styles docs. */
    return cui_dropdown_button_styled_ex(
        ui, cui_id_from(ui, "export"), cui_id_from(ui, "export-action"),
        cui_id_from(ui, "export-menu"), theme, cui_string("Export workspace"),
        cui_string("Export options"), entries, count, menu_action, menu,
        options, &styles);
}
`;
}
