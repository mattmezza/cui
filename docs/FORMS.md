# Form and Field

The frozen 0.7.1 Form composes application-owned controls. It does not store
values, validate inputs, or submit automatically. CUI follows that contract:
`cui_form` accepts a borrowed array of typed `cui_form_field` descriptors and
callbacks for controls, labels, descriptions, and the optional footer. Callbacks
run each frame for visible fields. The application owns callback data and strings.

The default is one column, vertical labels, 140-pixel horizontal label width,
medium density, and no footer. Use `cui_form_options_default`, or zero initialize
options. Width defaults to 480. Columns and label orientation are independent.
Small and extra-small use a 6-pixel row gap, medium 8, and large 12; the default
column gap is three times that. Positive gap overrides and label text sizes are
supported. Field alignment controls the horizontal label/control alignment.

`column_span` defaults to one. `column_start` and `column_end` use one-based CSS
grid lines; negative lines count backwards from the end, so `-1` is the final
line. Zero means automatic placement. Explicit start/end determine the span.
Fields flow in descriptor order without overlapping: when a requested start is
behind the placement cursor or the span cannot fit, a new row starts. Hidden
fields occupy no cells and invoke no callbacks. Set `no_label_indent` to omit the
label slot and horizontal description indentation. The footer spans the full
width below all fields and aligns its callback content to the trailing edge.

`cui_form_plan` supplies allocation-free placement and widths for custom renderers.
It validates all descriptors before changing output. Limits are 64 columns and
256 fields; invalid spans, lines, widths, strings, duplicate IDs, and insufficient
output capacity are reported. Columns are explicit; applications choose their
responsive column count. This is ordinary row/column composition over core Clay
layout, with no additional general-purpose layout tree.

`cui_base_form` submits passive group/field/label semantics without painting or a
theme implementation dependency. Set each field's `control` to the actual input
ID. `cui_form_associate_control` preserves its existing semantics while attaching
field parent, label, description, required, invalid, and read-only metadata.
Styled composition invokes it after the control callback. When rendering a custom
base form, call it after submitting each control. Error text replaces the
ordinary description when invalid, and styled forms use the danger token.

For example, retain a `cui_edit` and `cui_input_state` in application state, and
provide a control callback that calls `cui_input_ex` at the supplied width. Pass
required, invalid, disabled, and read-only options to that control as appropriate.
Then supply `.control = input_id`, `.render_control = callback`, and `.user = state`
in the field. Arbitrary controls govern their own disabled behavior; passive form
metadata does not prevent a custom callback from changing application state.
Required fields show a themed asterisk. Supply an accessible label string even
when drawing a custom label callback. Attach submission to the footer's Button.

Tests cover validated placement and transactional failure, spans and negative
lines, conditional fields, four themes, both label orientations, shared row
geometry, full-width fields, dynamic descriptions, trailing footer, accessible
associations, and real Input edits/read-only/disabled events. The base test links
only core and form-base, with no styled or theme implementation.

This provides the Form/Field composition contract, not a validation engine or a
claim of full framework parity. Existing Input/Editor limitations carry through
the application-supplied control, including incomplete multiline presentation,
IME, grapheme navigation, and native accessibility bridges.

# InputGroup

`cui_input_group` composes exactly one retained `cui_edit` with up to 64 borrowed
addons. `cui_input_group_state` retains input drag state, horizontal scroll, and
measured viewport bounds. Programmatic editor updates use the ordinary Edit API;
only actual user text changes set the returned `input.changed` flag.

Each addon has a distinct stable ID, position (`INLINE_START`, `INLINE_END`,
`BLOCK_START`, `BLOCK_END`), optional width, and text/button/custom kind. Addons on
the same side keep array order. Custom render callbacks receive the inherited
disabled/read-only policy. Buttons default to ghost presentation, with primary,
secondary, outline, and danger variants. Disabled or loading actions cannot
activate. `activated_addon` returns the last activated built-in action in the
frame; the application performs its action. Custom interactive content owns its
callbacks and must respect the supplied disabled policy.

The group paints one shared theme frame; the input's own frame is transparent.
The field and addons share Input metrics: extra small 20px, small 24px, medium 32px and large 44px at the default 16px theme scale. The medium preset is the default. Theme scaling and an explicit group font size scale these dimensions together. Inline addons remain in the same row; block addons intentionally occupy rows above or below it. The gallery defaults to inline prefix/field/suffix/action and exposes block addons through its mode selector. Invalid edits
highlight the shared frame. Disabled input disables the group and built-in
buttons. Read-only preserves focus, selection/copy behavior, and addon actions.
Clicking an addon or empty group inset focuses the editor; clicking an action
keeps action focus. Root/child semantics retain input accessible name, hierarchy,
read-only/error state, and loading action status.

`cui_base_input_group` owns all input reduction and action behavior without theme
painting. Link core, base, edit, and form-base for this API. A custom renderer
supplies addon/root/input bounds and can retain its own input viewport rectangle.
The styled group uses ordinary Input's paint-only path and measured scalar hit
mapping, with no duplicate editor reducer. Its additional implementation link is
components/theme, already required by styled Input.

The group inherits the currently implemented Input/Editor slice. Multiline
editors compose with block addons, but sophisticated Textarea row auto-growth,
multiline caret/selection presentation, tokens, password toggle, attachment paste
hooks, tooltips, and built-in dropdown/popover addon builders remain outside this
module. Applications can compose existing menu/popover controls in custom addons;
this does not establish full InputGroup/editor family parity.
