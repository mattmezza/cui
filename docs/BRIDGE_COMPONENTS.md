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
| Dialog | `cui_surface_begin` | open, close result, child body | Native modal surface |
| AlertDialog | `cui_alert_dialog` | open, accept/cancel/close result | Native confirm/cancel content |
| Sheet | `cui_surface_begin` | open, close result, child body | Default right edge |
| Popover | `cui_surface_begin` | open, close result, anchor, child body | Anchor must be a live node |
| Tooltip | `cui_tooltip` | native hover lifecycle, open/close change, disabled | Anchor must be a live node |
| HoverCard | `cui_begin_hover_card` | native hover lifecycle, open/close change, disabled, child body | Anchor must be a live node |
| Avatar | `cui_avatar` | copied name, size | Initials avatar |
| Bubble | `cui_bubble` | copied message | Default variant |
| Empty | `cui_empty` | copied title/description, width | Static state |
| GroupBox | `cui_begin_group_box` | copied title, width, child tree | Native group container |
| Kbd | `cui_kbd` | copied chord | Default platform-neutral text |
| Skeleton | `cui_skeleton` | width/height, frame-time motion | Default style |
| Spinner | `cui_spinner` | size, frame-time motion | Default style |
| Tag | `cui_tag` | copied text | Default style |
| Marker | `cui_marker` | copied text, retained state | Single text child |
| Shimmer | `cui_shimmer_text` | copied text, retained state | Single line |
| List | `cui_list` | selected Item, change, disabled | Up to 64 Items |
| Menu | `cui_menu` | menu Item action, change, disabled | Up to 64 leaf actions |
| Pagination | `cui_pagination` | current page, total, change, disabled | One-based page, total <= 1,000,000 |
| Rating | `cui_rating` | value, maximum, change, disabled | Maximum <= 10 |
| Stepper | `cui_stepper` | selected Item, status, change, disabled | Up to 64 Items |
| DescriptionList | `cui_description_list` | Item label/value pairs | Up to 64 Items, default columns |
| Scrollable | `cui_scrollable_begin/end_ex` | child tree, viewport/extents, wheel change | Vertical viewport |
| StatusBar | `cui_status_bar` | Item labels in left/center/right regions | Set Item step status to 0/1/2 for region |
| Toolbar | `cui_toolbar` | Item child controls, inherited disabled state | Up to 64 Items; child controls own actions |
| TitleBar | `cui_title_bar` | Item text labels, retained title state | No native window host controls |
| Icon | `cui_icon_named` | Copied catalog name, width/height | Embedded catalog assets; unknown name fails the frame |
| Message | `cui_message` | Copied body/header, width | Incoming filled bubble, one body |
| Attachment | `cui_attachment` | Copied title/description, status, progress, width | Metadata chip; no host file or remove/retry actions |
| Image | `cui_image` | Owned RGBA8 pixels, revision, description, size | At most 16 MiB; no path, URI, animated loader |
| Tree | `cui_tree` | 64 flat Item records with preorder depth, expansion and selected Item events | Item step status is depth 0..63; no custom row content |
| TimeField | `cui_time_field` | HH:MM:SS state, keyboard change, copied event text, disabled | 24-hour fixed format |
| ColorPicker | `cui_color_picker` | Packed RGBA, hue/plane/alpha input, copied hex event text, disabled | Default picker, no custom palette |
| Calendar | `cui_calendar` | Single selected date, pointer/keyboard change, disabled | Default English labels, one month |
| DatePicker | `cui_date_picker` | Retained popup, single selected date, open state and copied change text | Default English labels, no range mode |
| OtpInput | `cui_otp` | Owned value buffer, target length, text input and copied change text, disabled | 1..256 cells, 4096 UTF-8 bytes; default six cells |
| Form | `cui_form` | Item field labels/descriptions, child controls, inherited disabled state | Up to 64 fields; no built-in validation or nested Form |
| Input Group | `cui_input_group` | Owned editor, Item button addons, alignment, copied text/change, disabled | Up to 64 addons; Item step status 0..3 selects position |
| Notification | `cui_notifications` | Shared retained in-app stack, one card per node, copied title/message, open/dismiss, pointer click and close events | Up to 128 active cards; no OS delivery, action button, or auto-hide |
| Focus Trap | `cui_begin_focus_scope` | Modal scope around child nodes, explicit open state, pointer/focus exclusion outside scope | Child content only; no backdrop or automatic close |

`cui_bridge_poll_event_ex` copies queued CHANGE snapshots, including editable
text. A capacity failure preserves the event for retry. The legacy event poll
returns CLICK events and skips CHANGE events.

The table covers 59 manifest families plus four bridge-only structural kinds
(Row, Column, RadioGroup, Item). The remaining 19 manifest entries are:
Clipboard, MessageScroller,
Editor, Questionnaire, Speech, Carousel,
Dock, Resizable, Root View, Settings,
Sidebar, Theme, VirtualList, Chart, Command,
DataTable, Plot, Table, and TextView. These are unimplemented by the bridge.
No language binding should advertise them until they have state, interaction,
and tests.

The visual families above expose their documented default variants. Avatar is
an initials avatar with no image source. Marker is a single text child; Shimmer
is a single line. The bridge has no arbitrary C content callback entrypoint.
Scrollable currently uses a vertical viewport and caller-supplied extents.
All strings are copied and all event payloads are snapshots; none of these
limited variants implies full source component parity.

SDL applications can call `cui_bridge_set_font_fallbacks` with up to 32 explicit
UTF-8 font paths. The native font loader opens an owned replacement chain
transactionally; an invalid path leaves the old chain intact. A render failure
after backend creation returns `CUI_BRIDGE_CORE_ERROR` rather than reporting
the backend unavailable. The optional native regression test uses
`CUI_TEST_FONT_PATH` for a primary font without ✓ and
`CUI_TEST_FALLBACK_FONT_PATH` for a font with ✓; run it under isolated Xvfb.
