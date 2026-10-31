/* SPDX-License-Identifier: MIT */
// Complete C functions for the opt-in gallery routes; no private hashes.
const partHelper = `static void decorate(cui_context *ui, cui_id id, float radius, bool inset) {
    cui_box_shadow layer = {
        .kind = inset ? CUI_BOX_SHADOW_INSET : CUI_BOX_SHADOW_DROP,
        .layer = {.offset_y = inset ? 1 : 2, .sigma = 1,
                  .color = {20, 40, 80, 150}}};
    bool styled = cui_element_set_box_shadows(ui, id, &layer, 1,
        (cui_corner_radii){radius, radius, radius, radius});
    (void)styled; /* Check false: identity alone does not prove presence. */
}
`;
const buttonHelper = `static void button_shadow(void *user, const cui_button_style_info *info,
                          const cui_button_surface_paint *current,
                          cui_button_surface_patch *out) {
    (void)user; (void)current;
    out->fields = CUI_BUTTON_SURFACE_SHADOWS;
    if (info->disabled) return; /* Present empty list clears DROP and INSET. */
    bool pressed = info->phase == CUI_BUTTON_STYLE_ACTIVE;
    bool hovered = info->phase == CUI_BUTTON_STYLE_HOVER;
    out->value.shadow_count = 1;
    out->value.shadows[0] = (cui_box_shadow){
        .kind = pressed ? CUI_BOX_SHADOW_INSET : CUI_BOX_SHADOW_DROP,
        .layer = {.offset_y = pressed ? 2 : hovered ? 6 : 3,
                  .sigma = pressed ? 1 : hovered ? 3 : 2,
                  .color = {20, 40, 80, 150}}};
}
static unsigned actions;
static void activated(void *user) { (void)user; actions++; }
`;
export function controlShadowUsage(name, size=0, design=0) {
 const sizing=`.size = (cui_control_size)${size}, .design_size = (cui_design_size)${design},`;
 const extent=`static float indicator_extent(const cui_theme *theme) {
    cui_design_metrics metrics;
    if (${design} != CUI_DESIGN_INHERIT && cui_design_measure(
        (cui_design_size)${design}, theme->font_size / 16, &metrics))
        return metrics.indicator_size;
    return ${size===3?12:size===1?14:size===2?18:16} * theme->font_size / 16;
}
`; 
 let headers='#include "cui/cui_components.h"\n#include <math.h>\n';
 let helper=partHelper,body;
 if(name==='Button') {
  headers+='#include "cui/cui_button_styles.h"\n';helper=buttonHelper;
  body=`    cui_button_surface_status status;
    cui_button_surface_ex(ui, cui_id_from(ui, "action"), theme,
        cui_string("Run action"),
        (cui_button_options){${sizing} .disabled = disabled, .on_click = activated,
                            .prevent_pointer_focus = true},
        (cui_button_surface_styles){.instance = button_shadow,
            .hovered = button_shadow, .active = button_shadow,
            .disabled = button_shadow}, &status);
    /* status.rejected_phases reports invalid paint patches, not activation. */`;
 } else if(name==='Checkbox')body=`    static cui_check_state value;
    cui_id id = cui_id_from(ui, "check"), indicator = cui_id_from(ui, "indicator");
    cui_checkbox_ex_with_parts(ui, id, theme, cui_string("Receive updates"), &value,
        (cui_checkbox_options){${sizing} .disabled = disabled},
        (cui_selection_indicator_parts){indicator});
    decorate(ui, indicator, fminf(theme->radius, 4 * theme->font_size / 16),
             value != CUI_CHECK_UNCHECKED);`;
 else if(name==='Radio') {
  headers+='#include "cui/cui_radio_group.h"\n';helper+=extent;
  body=`    static cui_id selected;
    static cui_radio_group_state state;
    cui_radio_group_item items[] = {
        {cui_id_from(ui, "standard"), cui_string("Standard"), false},
        {cui_id_from(ui, "priority"), cui_string("Priority"), false},
        {cui_id_from(ui, "unavailable"), cui_string("Express unavailable"), true}};
    cui_selection_indicator_parts parts[] = {
        {cui_id_from(ui, "indicator-a")}, {cui_id_from(ui, "indicator-b")},
        {cui_id_from(ui, "indicator-c")}};
    if (!selected) selected = items[0].id;
    cui_radio_group_with_parts(ui, cui_id_from(ui, "delivery"), theme,
        items, 3, &selected, &state,
        (cui_radio_group_options){${sizing} .name = cui_string("Delivery options"),
                                 .vertical = true, .disabled = disabled}, parts, 3);
    for (size_t i = 0; i < 3; i++)
        decorate(ui, parts[i].indicator, indicator_extent(theme) / 2,
                 selected == items[i].id);`;
 } else if(name==='Switch'){helper+=extent;body=`    static bool value;
    static cui_switch_state state;
    cui_id id = cui_id_from(ui, "switch");
    cui_switch_options options = cui_switch_options_default();
    options.size = (cui_control_size)${size};
    options.design_size = (cui_design_size)${design};
    options.disabled = disabled;
    options.reduced_motion = true;
    cui_switch_ex(ui, id, theme, cui_string("Notifications"), &value, &state, options);
    float height = ${design} != CUI_DESIGN_INHERIT ? indicator_extent(theme) + 4 : ${size===1||size===3?16:size===2?24:20};
    float radius = theme->radius >= 4 ? height : theme->radius;
    decorate(ui, cui_switch_track_id(id), radius, value);
    decorate(ui, cui_switch_thumb_id(id), radius, false);
    /* Thumb DROP is clipped by the existing track. */`;} 
 else if(name==='Slider')body=`    static float value = 40;
    cui_id id = cui_id_from(ui, "slider");
    cui_slider(ui, id, theme, cui_string("Volume"), &value, 0, 100, 1, disabled);
    decorate(ui, cui_slider_track_id(id), theme->radius == 0 ? 0 : 4, true);
    /* Legacy Slider has no independently targetable thumb or fill. */`;
 else return '';
 return headers+'\n'+helper+'\n/* Call after begin_frame, before end_frame. */\n'+
  'void shadow_example(cui_context *ui, const cui_theme *theme, bool disabled) {\n'+body+'\n}\n';
}
