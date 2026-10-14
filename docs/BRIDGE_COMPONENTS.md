# C bridge component coverage

The bridge ABI remains version 1. Existing numeric enum values, option layout,
event layout, and functions are unchanged. New kinds are appended.

| Kind | Native CUI renderer | State and events | Current limit |
| --- | --- | --- | --- |
| Row, Column | `cui_begin_box` | gap | One root; depth 128 |
| Label | `cui_text` | copied UTF-8 text | Text only |
| Button | `cui_button_ex` | copied text, click | Existing options |
| Checkbox | `cui_checkbox` | checked, change, disabled | Boolean only |
| Radio | `cui_radio` | checked, change, disabled | Standalone radio; no group exclusivity |
| Switch | `cui_switch_ex` | checked, change, disabled | Boolean only |
| Toggle | `cui_toggle` | checked, change, disabled | Boolean only |
| Slider | `cui_slider` | value, range, step, change, disabled | Float native precision |
| Progress | `cui_progress` | value in [0,1] | Width fixed at 160 |
| Badge | `cui_badge` | copied UTF-8 text | Legacy text badge |
| Input | `cui_input_ex` | per-node edit model, text, change, disabled | 4096 UTF-8 bytes |
| Textarea | `cui_textarea` | per-node multiline edit model, text, change, disabled | 4096 UTF-8 bytes, 512 layout lines |
| NumberInput | `cui_number_input` | per-node numeric editor, value, range, step, change, disabled | Finite values; 127-byte draft |
| Collapsible | `cui_collapsible` | open state, change, disabled | Child subtree is panel |
| Alert | `cui_alert` | copied title/message | Static informational alert |
| RadioGroup | `cui_radio_group` | selected Item, change, disabled | Up to 64 Items |
| Tabs | `cui_tabs` | selected Item, change, disabled | Up to 64 Items; Item subtree is panel |
| Accordion | `cui_accordion` | per-Item open state, change, disabled | Up to 64 Items; multiple expansion |
| Item | Composite descriptor | label, disabled, optional panel children | Must belong to group |
| Select | `cui_select` | selected Item, change, disabled | Up to 64 Items; viewport is app surface |
| ComboBox | `cui_combobox` | query edit model, selected Item, change, disabled | Up to 64 Items, 4096 UTF-8 query bytes |
| DropdownButton | `cui_dropdown_button` | action click, selected menu Item, change, disabled | Up to 64 leaf menu actions |

`cui_bridge_poll_event_ex` copies queued CHANGE snapshots, including editable
text. A capacity failure preserves the event for retry. The legacy event poll
returns CLICK events and skips CHANGE events.

Remaining manifest entries are unimplemented by the bridge. No language binding
should advertise those entries until they have state, interaction, and tests.
