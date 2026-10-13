# Component composition, state and interaction

CUI composes visible UI each frame between `cui_begin_frame` and `cui_end_frame`. The context retains focus, prior hit regions, resources and transient routing. Application-owned values and explicit models persist between frames. A persistent model is not a retained application widget tree. Model-only families require application composition; this guide does not claim complete upstream parity. See the [implementation manifest](gpui-parity.json) for per-family limitations.

This guide mirrors [component-usage.json](component-usage.json), the website metadata. “Immediate frame submission” means values/descriptors suffice; “application model + frame composition” highlights explicit persistent model, behavior or resource storage. Both use the same frame engine.

Keep stable IDs for the same logical controls. Retain borrowed strings, arrays, buffers and callback user data for the lifetime documented by each header, including through `cui_end_frame` when deferred layout/painting uses them. Model-bound arrays may need to survive until the next successful bind/rebuild; notification and host registrations outlive a submission. Never retain callback-borrowed URLs without copying. See [frames and callbacks](IMMEDIATE_AND_STATE.md) and [core lifecycle](CORE.md).

Component action/change callbacks run synchronously at the API's event-reduction point; model operations may also emit events. Speech recognizer/input callbacks may originate outside frame submission; queue reports and pump on the owning thread. User pointers must be valid for that call and for any longer registered-model lifetime. Rendering another frame alone must not replay application side effects. Content/render/paint/measure callbacks are for composition and may run during measurement; never put saving, network requests, deletion or other application actions in paint-only callbacks.

For example, inside an active frame, save only on activation:

```c
if (cui_button(ui, save_id, theme, cui_string("Save"), false))
    save_document(app);
if (cui_checkbox(ui, notify_id, theme, cui_string("Notifications"),
                 &app->notifications, false))
    persist_preferences(app);
```

The checkbox updates the application's bool. Controlled Switch/Toggle `on_change` receives the requested bool; accept by updating the authoritative value or reject by leaving it unchanged. See the complete [controlled callback example](IMMEDIATE_AND_STATE.md#use-a-controlled-callback). Initialize models once, retain them across frames, and destroy/dispose allocated models explicitly. External model changes should invalidate the context; honor animation deadlines and reduced motion.

## Component index

- [Accordion](#accordion)
- [AlertDialog](#alertdialog)
- [Alert](#alert)
- [Attachment](#attachment)
- [Avatar](#avatar)
- [Badge](#badge)
- [Bubble](#bubble)
- [Button](#button)
- [Checkbox](#checkbox)
- [Clipboard](#clipboard)
- [Collapsible](#collapsible)
- [DropdownButton](#dropdownbutton)
- [Empty](#empty)
- [Focus Trap](#focus-trap)
- [GroupBox](#groupbox)
- [HoverCard](#hovercard)
- [Icon](#icon)
- [Image](#image)
- [Kbd](#kbd)
- [Label](#label)
- [Marker](#marker)
- [Message](#message)
- [MessageScroller](#messagescroller)
- [Notification](#notification)
- [Pagination](#pagination)
- [Progress](#progress)
- [Radio](#radio)
- [Rating](#rating)
- [Shimmer](#shimmer)
- [Skeleton](#skeleton)
- [Spinner](#spinner)
- [Stepper](#stepper)
- [Switch](#switch)
- [Tag](#tag)
- [Toggle](#toggle)
- [Tooltip](#tooltip)
- [Calendar](#calendar)
- [ColorPicker](#colorpicker)
- [Combobox](#combobox)
- [DatePicker](#datepicker)
- [Editor](#editor)
- [Form](#form)
- [Input](#input)
- [Input Group](#input-group)
- [NumberInput](#numberinput)
- [OtpInput](#otpinput)
- [Questionnaire](#questionnaire)
- [Select](#select)
- [Slider](#slider)
- [Speech](#speech)
- [Textarea](#textarea)
- [TimeField](#timefield)
- [Carousel](#carousel)
- [DescriptionList](#descriptionlist)
- [Dialog](#dialog)
- [Dock](#dock)
- [List](#list)
- [Menu](#menu)
- [Popover](#popover)
- [Resizable](#resizable)
- [Root View](#root-view)
- [Scrollable](#scrollable)
- [Settings](#settings)
- [Sheet](#sheet)
- [Sidebar](#sidebar)
- [StatusBar](#statusbar)
- [Tabs](#tabs)
- [Theme](#theme)
- [TitleBar](#titlebar)
- [Toolbar](#toolbar)
- [Tree](#tree)
- [VirtualList](#virtuallist)
- [Chart](#chart)
- [Command](#command)
- [DataTable](#datatable)
- [Plot](#plot)
- [Table](#table)
- [TextView](#textview)

## Accordion

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains bool expansion values and optional per-panel cui_disclosure_motion arrays.

**Interaction:** Returns whether expansion changed; cui_disclosure_panel_fn composes panel children and cui_disclosure_header_fn custom headers.

**Usage:** Submit stable item IDs and current expansion each frame; keep motion and style arrays alive through end_frame.

API: `cui_accordion`. [Public header](../include/cui/cui_disclosure.h); [guide](DISCLOSURE.md).

## AlertDialog

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_surface_state and child form/application data.

**Interaction:** cui_surface_decision_fn may veto close; cui_surface_closed_fn observes close; take_result consumes the decision.

**Usage:** Initialize the appropriate surface kind, open from an action, and pair end only when begin succeeds. Consume result once.

API: `cui_surface_init`, `cui_alert_dialog`, `cui_surface_take_result`. [Public header](../include/cui/cui_surface.h); [guide](SURFACES.md).

## Alert

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller optionally retains a bool visible flag referenced by cui_alert_options.visible.

**Interaction:** Returns dismissal; writes supplied visibility.

**Usage:** Submit the current title/message and visibility; handle dismissal in the returned event branch.

API: `cui_alert`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Attachment

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_attachment_state and owns attachment model, metadata and resource lifetime.

**Interaction:** cui_attachment_result reports actions; cui_attachment_action_fn receives remove/retry activation.

**Usage:** Submit transfer status and progress; perform removal/retry in action callbacks, not render callbacks.

API: `cui_attachment`. [Public header](../include/cui/cui_media.h); [guide](MEDIA.md).

## Avatar

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller retains name, borrowed RGBA pixels, bounded circular cache and destination storage through rendering.

**Interaction:** Interactive avatars return activation; interactive stacks return the selected index or SIZE_MAX. Application opens the selected profile.

**Usage:** Initialize a circular image cache once; submit borrowed image pixels with a revision. Missing images fall back to initials; stacks use stable application keys.

API: `cui_avatar`, `cui_avatar_ex`, `cui_avatar_stack`, `cui_avatar_image_init`. [Public header](../include/cui/cui_content.h); [content guide](CONTENT.md); [profile images and conversations](AVATAR_MESSAGE_EXAMPLES.md).

## Badge

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns counts, badge options and child state; callbacks compose arbitrary child content synchronously.

**Interaction:** Child controls return their own actions; corner indicators provide nonfocusable status semantics.

**Usage:** Initialize options with cui_badge_defaults, select count/dot/icon and call cui_badge_ex with a content callback every frame.

API: `cui_badge_defaults`, `cui_badge_ex`, `cui_badge`. Callback: `cui_button_content_fn`. [Public header](../include/cui/cui_components.h); [guide](BADGE.md), [guide](COMPONENTS.md).

## Bubble

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns child data; no separate bubble model.

**Interaction:** Child controls handle their own events.

**Usage:** Begin the surface, compose child controls, then end it in the same frame.

API: `cui_begin_bubble`, `cui_end_bubble`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Button

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns application data; cui_button_options controls presentation.

**Interaction:** Returns true on activation; invoke the application action in that branch.

**Usage:** For Save, call save_document only when cui_button returns true. content is a composition callback, not an action handler.

API: `cui_button`, `cui_button_ex`. [Public header](../include/cui/cui_components.h); [guide](BUTTON.md).

## Checkbox

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller retains bool or cui_check_state value; context owns focus.

**Interaction:** Returns true when the caller-owned value changes.

**Usage:** Pass the current value by pointer and persist preferences in the change branch.

API: `cui_checkbox`, `cui_checkbox_ex`. [Public header](../include/cui/cui_components.h); [guide](CONTROLS.md).

## Clipboard

**Composition:** application model + frame composition. **Scope:** host adapter facility; no standalone styled clipboard widget.

**State:** Host retains backend and application owns clipboard buffers; cui_platform carries portable callbacks.

**Interaction:** Clipboard reads/writes are host interface calls, not a rendered widget action callback.

**Usage:** Obtain the host clipboard interface and supply it to editing/copy controls; compose a Copy button to trigger application copying.

API: `cui_sdl3_clipboard_interface`. [Public header](../include/cui/cui_sdl3.h); [guide](INTERFACES.md).

## Collapsible

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains bool expansion values and optional per-panel cui_disclosure_motion arrays.

**Interaction:** Returns whether expansion changed; cui_disclosure_panel_fn composes panel children and cui_disclosure_header_fn custom headers.

**Usage:** Submit stable item IDs and current expansion each frame; keep motion and style arrays alive through end_frame.

API: `cui_collapsible`, `cui_collapsible_ex`. [Public header](../include/cui/cui_disclosure.h); [guide](DISCLOSURE.md).

## DropdownButton

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains menu tree state and menu entries.

**Interaction:** cui_dropdown_button_result separates main action activation from menu interaction.

**Usage:** Handle the primary action result separately from the adjacent menu trigger.

API: `cui_dropdown_button`. [Public header](../include/cui/cui_choice.h); [guide](CHOICE.md).

## Empty

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns title and description.

**Interaction:** Presentation only; compose separate action buttons when needed.

**Usage:** Submit title, description and width each frame.

API: `cui_empty`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Focus Trap

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_overlay with modal=true; context retains focus routing/restoration.

**Interaction:** Overlay update handles dismissal/focus; child controls return their own events.

**Usage:** Open a modal overlay, submit children inside its active scope and close via application actions.

API: `cui_overlay_init`, `cui_overlay_begin`, `cui_overlay_end`. [Public header](../include/cui/cui_overlay.h); [guide](OVERLAYS.md).

## GroupBox

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns title and child data; no group model.

**Interaction:** Child controls handle their own events.

**Usage:** Pair begin/end around application-composed children.

API: `cui_begin_group_box`, `cui_end_group_box`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## HoverCard

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_hover_state for delay/open lifecycle.

**Interaction:** Hover, focus and escape update lifecycle; child controls handle card actions.

**Usage:** Submit the trigger first; Tooltip submits text, while HoverCard pairs end only after begin returns true.

API: `cui_begin_hover_card`, `cui_end_hover_card`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Icon

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_icon_state for custom SVG/resource preparation; named helper uses context resources.

**Interaction:** cui_icon_loader_fn resolves SVG resources; no click callback.

**Usage:** Initialize custom icon state, load data/name/path, submit each frame and destroy prepared resources after use.

API: `cui_icon`, `cui_icon_named`, `cui_icon_state_init`, `cui_icon_state_destroy`. [Public header](../include/cui/cui_icon.h); [guide](ICON.md).

## Image

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_image_state and owns loader/resource backing storage.

**Interaction:** cui_image_loader_fn resolves resources; cui_image_result reports load status, not a click event.

**Usage:** Submit a source and loader each frame; retain loaded frame resources for their documented paint lifetime.

API: `cui_image`. [Public header](../include/cui/cui_media.h); [guide](MEDIA.md).

## Kbd

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns shortcut text and optional formatting buffer.

**Interaction:** Presentation only; shortcut dispatch belongs to the application.

**Usage:** Format host conventions explicitly and submit the chord label.

API: `cui_kbd`, `cui_kbd_format`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Label

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns label text.

**Interaction:** Presentation only; no action callback.

**Usage:** Submit text inside the current parent layout.

API: `cui_label`. [Public header](../include/cui/cui_components.h); [guide](COMPONENTS.md).

## Marker

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_marker_state and child descriptors; owns animation time.

**Interaction:** cui_status_render_fn paints rich content; no action callback.

**Usage:** Supply elapsed time/loading status and compose children using resolved style and opacity.

API: `cui_marker`. [Public header](../include/cui/cui_status.h); [guide](STATUS_COMPONENTS.md).

## Message

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns message data and slot content.

**Interaction:** cui_message_render_fn composes slots; action controls inside slots supply events.

**Usage:** Submit body and slot options each frame; compose application actions in designated slots.

API: `cui_message`. [Public header](../include/cui/cui_message.h); [guide](MESSAGE.md).

## MessageScroller

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains allocated cui_message_scroller_state and message data; destroy model after use.

**Interaction:** cui_message_scroller_result reports scrolling/jump behavior; cui_message_row_render_fn composes visible rows.

**Usage:** Append/prepend through model operations, then submit visible messages through the row callback each frame.

API: `cui_message_scroller_create`, `cui_message_scroller_destroy`, `cui_message_scroller`. [Public header](../include/cui/cui_message.h); [guide](MESSAGE.md).

## Notification

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_notification_state, entry storage, notification content/user pointers and host binding.

**Interaction:** cui_notification_callback handles click/close/action; cui_notification_send_fn delegates system delivery to host.

**Usage:** Push on an application event, render the stack once after content each frame, and honor returned scheduling deadlines.

API: `cui_notification_init`, `cui_notification_push`, `cui_notifications`. [Public header](../include/cui/cui_status.h); [guide](STATUS_COMPONENTS.md).

## Pagination

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller retains size_t page.

**Interaction:** Returns true when page changes.

**Usage:** On a page change, load or select application data for the new page.

API: `cui_pagination`. [Public header](../include/cui/cui_numeric.h); [guide](NUMERIC.md).

## Progress

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns the target fraction, host clock and optional zero-initialized cui_progress_motion.

**Interaction:** Presentation only; the extended return reports whether another animation frame is needed.

**Usage:** Submit the controlled fraction each frame; use cui_progress_defaults for the source loading cycle and retain motion state for smooth value changes. Circles need caller-owned pixel storage retained through rendering.

API: `cui_progress_defaults`, `cui_progress`, `cui_progress_ex`, `cui_progress_circle_defaults`, `cui_progress_circle`. [Public header](../include/cui/cui_components.h); [guide](PROGRESS.md).

## Radio

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** For grouped choices, retain one selected cui_id and cui_radio_group_state per collection; standalone radios retain one shared integer.

**Interaction:** cui_radio_group resolves controlled callback requests before painting and provides arrow/Home/End navigation with one Tab stop; standalone functions return true when selection changes.

**Usage:** Submit stable unique item IDs to cui_radio_group; synchronously accept or reject on_change requests in your application model. Keep selected and navigation between frames.

API: `cui_radio`, `cui_radio_ex`. [Public header](../include/cui/cui_components.h); [group guide](RADIO_GROUP.md); [standalone guide](CONTROLS.md).

## Rating

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller retains float rating.

**Interaction:** Returns true when rating changes.

**Usage:** Pass the current rating by pointer and save it in the change branch.

API: `cui_rating`. [Public header](../include/cui/cui_numeric.h); [guide](NUMERIC.md).

## Shimmer

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_shimmer_state through end_frame and supplies elapsed time.

**Interaction:** Presentation only; no action callback.

**Usage:** Submit text, timing and reduced-motion preference; honor animation wake deadlines.

API: `cui_shimmer_text`. [Public header](../include/cui/cui_status.h); [guide](STATUS_COMPONENTS.md).

## Skeleton

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns elapsed time/phase and reduced-motion preference.

**Interaction:** Presentation only; no action callback.

**Usage:** Derive phase from elapsed time and schedule redraws while loading.

API: `cui_skeleton`, `cui_loading_phase`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Spinner

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns elapsed time/phase and reduced-motion preference.

**Interaction:** Presentation only; no action callback.

**Usage:** Derive phase from elapsed time and schedule redraws while loading.

API: `cui_spinner`, `cui_loading_phase`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Stepper

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns step descriptors and size_t active step.

**Interaction:** Returns true when active step changes.

**Usage:** Submit step descriptors and the current step; validate application data before progressing elsewhere.

API: `cui_stepper`. [Public header](../include/cui/cui_numeric.h); [guide](NUMERIC.md).

## Switch

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains bool value and optional cui_switch_state motion.

**Interaction:** Returns activation/change; cui_boolean_change_fn receives requested value when on_change is set, leaving acceptance to the caller.

**Usage:** Accept or reject requested changes in a synchronous on_change callback; resubmit the accepted value next frame.

API: `cui_switch`, `cui_switch_ex`. [Public header](../include/cui/cui_components.h); [guide](CONTROLS.md).

## Tag

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns tag text/content; no tag model.

**Interaction:** Presentation only; child controls provide actions.

**Usage:** Use the text helper or pair successful begin/end and apply returned foreground/font style to children.

API: `cui_tag`, `cui_begin_tag`, `cui_end_tag`. [Public header](../include/cui/cui_status.h); [guide](STATUS_COMPONENTS.md).

## Toggle

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller retains bool pressed value.

**Interaction:** Returns activation/change; optional cui_boolean_change_fn receives the requested value.

**Usage:** Keep the authoritative bool in application state; on_change may validate before accepting.

API: `cui_toggle`, `cui_toggle_ex`. [Public header](../include/cui/cui_components.h); [guide](CONTROLS.md).

## Tooltip

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_hover_state for delay/open lifecycle.

**Interaction:** Hover, focus and escape update lifecycle; child controls handle card actions.

**Usage:** Submit the trigger first; Tooltip submits text, while HoverCard pairs end only after begin returns true.

API: `cui_tooltip`. [Public header](../include/cui/cui_content.h); [guide](CONTENT.md).

## Calendar

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_calendar_state and cui_date_selection.

**Interaction:** cui_calendar_result reports selection/navigation; cui_date_disabled_fn excludes dates.

**Usage:** Initialize a month, submit calendar each frame, and consume selection changes in the result.

API: `cui_calendar_init`, `cui_calendar`. [Public header](../include/cui/cui_picker.h); [guide](PICKERS.md).

## ColorPicker

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_color_state and cui_color value.

**Interaction:** Returns true when color changes.

**Usage:** Submit persistent channel/editor state; use the accepted color in the next frame theme or preview.

API: `cui_color_picker`. [Public header](../include/cui/cui_picker.h); [guide](PICKERS.md).

## Combobox

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_edit query/buffer, popup state, items, selection and filtered scratch array with capacity at least the item count.

**Interaction:** cui_combobox_result distinguishes query and selection behavior.

**Usage:** Retain the query editor and popup; filter application choices using the query and process returned changes.

API: `cui_combobox`. [Public header](../include/cui/cui_choice.h); [guide](CHOICE.md).

## DatePicker

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_date_picker_state and cui_date_selection.

**Interaction:** Returns selection changes; optional cui_date_disabled_fn restricts dates.

**Usage:** Initialize stable popup identity once; resubmit state and accepted selection each frame.

API: `cui_date_picker_init`, `cui_date_picker`. [Public header](../include/cui/cui_picker.h); [guide](PICKERS.md).

## Editor

**Composition:** application model + frame composition. **Scope:** styled frame adapter + retained base model.

**State:** Application retains the multiline edit, visual-row workspace, editor/search state, matcher ranges, query/replacement edit models and optional replacement scratch. Multicursor editing also borrows live selections, history snapshots and cursor anchors; adapters borrow this storage.

**Interaction:** cui_editor returns edit, selection and scroll results; retained search supports ASCII-insensitive navigation and atomic replacement. Optional multicursor interaction supports adding cursors, measured navigation, selection painting, atomic typing and undo/redo; cui_editor_highlight_fn supplies syntax colors.

**Usage:** Initialize editor and optional search resources once, call cui_editor_find_bar every frame, and pass the same matcher in cui_editor_options.search for visible match highlights. Attach selection storage with cui_edit_attach_selections and initialize borrowed anchors with cui_editor_multicursor_init to enable multicursor interaction. Keep replacement scratch disjoint from model storage.

API: `cui_editor_state_init`, `cui_editor_multicursor_init`, `cui_editor_matcher_init`, `cui_editor_search_enable_replace`, `cui_editor_find_bar`, `cui_editor_search_open`, `cui_editor`. [Public header](../include/cui/cui_editor.h); [guide](EDITOR.md).

## Form

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns field descriptors, labels, validation messages and each control value/model.

**Interaction:** cui_form_content_fn composes field controls; those controls supply action/change events.

**Usage:** Describe associations and validation in fields; submit controls in content callbacks and handle their events there.

API: `cui_form`. [Public header](../include/cui/cui_form.h); [guide](FORMS.md).

## Input

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_edit and its backing buffer; cui_input_ex additionally uses caller-owned cui_input_state.

**Interaction:** cui_input_result reports changed, submitted, rejected and errors. Optional whole-value validate callback decides whether a proposed edit is accepted.

**Usage:** Create an editor once; submit it each frame and perform application submission only on the result event.

API: `cui_input`, `cui_input_ex`. [Public header](../include/cui/cui_components.h); [guide](INPUT.md).

## Input Group

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_edit, cui_input_group_state, and addon descriptors.

**Interaction:** cui_input_group_result reports edit behavior; cui_input_group_content_fn composes addons.

**Usage:** Keep the editor and state across frames and compose prefix/suffix addon controls.

API: `cui_input_group`. [Public header](../include/cui/cui_form.h); [guide](FORMS.md).

## NumberInput

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains double value, cui_number_state and its borrowed cui_edit editor/buffer.

**Interaction:** cui_number_result reports changed/submitted/invalid; cui_number_step_fn customizes stepping.

**Usage:** Retain editor draft across frames; consume submitted/changed separately from invalid input.

API: `cui_number_input`. [Public header](../include/cui/cui_numeric.h); [guide](NUMERIC.md).

## OtpInput

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_otp_state and initialized character buffer.

**Interaction:** cui_otp_result distinguishes changed and completion events.

**Usage:** Initialize bounded storage once, submit cells each frame and consume the returned completion event.

API: `cui_otp_init`, `cui_otp`. [Public header](../include/cui/cui_otp.h); [guide](OTP.md).

## Questionnaire

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains question definitions, state and all workspace buffers.

**Interaction:** cui_questionnaire_event_fn emits model events; cui_questionnaire_validate_fn validates answers.

**Usage:** Initialize definitions/workspace once; compose current question controls each frame and handle answer/submit events.

API: `cui_questionnaire_init`, `cui_questionnaire_begin`, `cui_questionnaire_end`. [Public header](../include/cui/cui_questionnaire.h); [guide](QUESTIONNAIRE.md).

## Select

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_choice_popup, selected cui_id and item array.

**Interaction:** Returns selection change; choice content/render callbacks customize presentation.

**Usage:** Initialize popup once; submit trigger and selected value each frame.

API: `cui_select`. [Public header](../include/cui/cui_choice.h); [guide](CHOICE.md).

## Slider

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller retains float value and supplies min/max/step.

**Interaction:** Returns true when the value changes.

**Usage:** Pass a volume value by pointer; apply audio settings only in the change branch.

API: `cui_slider`. [Public header](../include/cui/cui_components.h); [guide](CONTROLS.md).

## Speech

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains speech state/workspace and recognizer/input implementations.

**Interaction:** cui_speech_event_fn reports recognition events; host reports queue messages and application pumps them.

**Usage:** Initialize provider bindings, pump recognition on the owning thread and compose button/waveform each frame; dispose after use.

API: `cui_speech_init`, `cui_speech_pump`, `cui_speech_button`, `cui_speech_dispose`. [Public header](../include/cui/cui_speech.h); [guide](SPEECH.md).

## Textarea

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_edit, buffer, cui_textarea_state and line workspace.

**Interaction:** cui_textarea_result reports edit events and layout errors.

**Usage:** Initialize line storage once and submit the multiline editor each frame.

API: `cui_textarea`. [Public header](../include/cui/cui_textarea.h); [guide](TEXTAREA.md).

## TimeField

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_time_state and cui_time value.

**Interaction:** Returns true when the time value changes.

**Usage:** Submit segment state and time by pointer; persist accepted time in the change branch.

API: `cui_time_field`. [Public header](../include/cui/cui_picker.h); [guide](PICKERS.md).

## Carousel

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains state, geometry workspace and slide data.

**Interaction:** cui_carousel_change_fn observes selection; cui_carousel_render_fn composes slides.

**Usage:** Initialize capacity once, submit slides each frame and handle selected-index changes separately from slide rendering.

API: `cui_carousel_state_init`, `cui_carousel`. [Public header](../include/cui/cui_carousel.h); [guide](CAROUSEL.md).

## DescriptionList

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns item descriptors/content; no separate list model.

**Interaction:** cui_description_render_fn composes custom values; child controls provide actions.

**Usage:** Submit label/value descriptors each frame; respect the resolved layout width in custom content.

API: `cui_description_list`. [Public header](../include/cui/cui_description.h); [guide](DESCRIPTION_LIST.md).

## Dialog

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_surface_state and child form/application data.

**Interaction:** cui_surface_decision_fn may veto close; cui_surface_closed_fn observes close; take_result consumes the decision.

**Usage:** Initialize the appropriate surface kind, open from an action, and pair end only when begin succeeds. Consume result once.

API: `cui_surface_init`, `cui_surface_begin`, `cui_surface_end`, `cui_surface_take_result`. [Public header](../include/cui/cui_surface.h); [guide](SURFACES.md).

## Dock

**Composition:** application model + frame composition. **Scope:** styled frame adapter + retained base model.

**State:** Application retains cui_dock, cui_dock_view and the panel registry; keep callback data alive through renderer completion.

**Interaction:** Panel render callbacks compose active content; on_event reports select, close, reorder, move, split and resize; allow_operation may veto structural actions.

**Usage:** Create the model and view once, register stable panel keys, submit cui_dock_view_render each frame and serialize the model between frames.

API: `cui_dock_view_create`, `cui_dock_view_render`, `cui_dock_view_destroy`. [Public header](../include/cui/cui_dock.h); [guide](DOCK.md), [guide](DOCK_MODEL.md).

## List

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_list_state, selected cui_id and item array.

**Interaction:** Returns changes; cui_choice_activation_fn / cui_choice_confirm_fn / cui_choice_cancel_fn handle list interaction.

**Usage:** Keep keys stable; submit current items and selection, then handle activation/confirmation callbacks synchronously.

API: `cui_list`. [Public header](../include/cui/cui_choice.h); [guide](CHOICE.md).

## Menu

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains menu entries and popup/menu-tree state.

**Interaction:** Returns activation and writes the activated cui_id through the action pointer; check/radio entries update caller values. cui_menu_render_fn composes custom content.

**Usage:** Initialize menu-tree state once and submit stable entry IDs each frame; invoke application actions from the returned activation and action key.

API: `cui_menu_tree`, `cui_menu`. [Public header](../include/cui/cui_choice.h); [guide](CHOICE.md).

## Popover

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_surface_state and child form/application data.

**Interaction:** cui_surface_decision_fn may veto close; cui_surface_closed_fn observes close; take_result consumes the decision.

**Usage:** Initialize the appropriate surface kind, open from an action, and pair end only when begin succeeds. Consume result once.

API: `cui_surface_init`, `cui_surface_begin`, `cui_surface_end`, `cui_surface_take_result`. [Public header](../include/cui/cui_surface.h); [guide](SURFACES.md).

## Resizable

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains state, slots and panel descriptors.

**Interaction:** cui_resizable_resize_fn observes size changes; panel content composes controls.

**Usage:** Initialize slot storage, submit panel descriptors each frame and save sizes after resize events.

API: `cui_resizable_state_init`, `cui_resizable`. [Public header](../include/cui/cui_resizable.h); [guide](RESIZABLE.md).

## Root View

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_root, plugin instance storage, registry and bound surface/notification storage.

**Interaction:** cui_root_content_fn composes application content; cui_root_copy_fn handles copy; plugins have explicit lifecycle hooks.

**Usage:** Initialize root/plugin storage, render content and facilities each frame, then dispose after use.

API: `cui_root_init`, `cui_root_render`, `cui_root_dispose`. [Public header](../include/cui/cui_root.h); [guide](ROOT.md).

## Scrollable

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_scroll_state offsets and supplies content/viewport dimensions.

**Interaction:** Begin/end reduce scroll input; child controls handle application actions.

**Usage:** Pair active scrollable begin/end around children and preserve updated offsets across frames.

API: `cui_scrollable_begin`, `cui_scrollable_end_ex`. [Public header](../include/cui/cui_scroll.h); [guide](SCROLL.md).

## Settings

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains page state/workspace, descriptors and actual setting values.

**Interaction:** cui_setting_render_fn composes controls; cui_setting_dirty_fn and cui_setting_reset_fn implement application dirty/reset behavior.

**Usage:** Initialize page/query storage; submit descriptors and implement each setting through its render/dirty/reset functions.

API: `cui_settings_state_init`, `cui_settings`. [Public header](../include/cui/cui_settings.h); [guide](SETTINGS.md).

## Sheet

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_surface_state and child form/application data.

**Interaction:** cui_surface_decision_fn may veto close; cui_surface_closed_fn observes close; take_result consumes the decision.

**Usage:** Initialize the appropriate surface kind, open from an action, and pair end only when begin succeeds. Consume result once.

API: `cui_surface_init`, `cui_surface_begin`, `cui_surface_end`, `cui_surface_take_result`. [Public header](../include/cui/cui_surface.h); [guide](SURFACES.md).

## Sidebar

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains sidebar state/cache, entry descriptors and selected application route.

**Interaction:** cui_sidebar_content_fn composes regions; menu-item results expose selection/activation.

**Usage:** Initialize measurement cache, submit current entries, and update routing from menu-item events.

API: `cui_sidebar_state_init`, `cui_sidebar`. [Public header](../include/cui/cui_sidebar.h); [guide](SIDEBAR.md).

## StatusBar

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns left/center/right descriptors and displayed data.

**Interaction:** cui_chrome_render_fn composes regions; child controls handle actions.

**Usage:** Submit region content each frame; preserve application data outside the bar.

API: `cui_status_bar`. [Public header](../include/cui/cui_chrome.h); [guide](CHROME.md).

## Tabs

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains selected cui_id, item array and optional motion/scroll state.

**Interaction:** Returns selection changes; cui_tab_panel_fn composes selected panel content.

**Usage:** Submit tab descriptors and current selected key; keep optional animation state across frames.

API: `cui_tabs`. [Public header](../include/cui/cui_choice.h); [guide](CHOICE.md).

## Theme

**Composition:** application model + frame composition. **Scope:** application style data; passed during frame submission.

**State:** Caller retains cui_theme value and any icon-loader user/backing storage.

**Interaction:** cui_theme_icon_loader_fn resolves icon SVG; theme selection is an application event.

**Usage:** Choose/copy a preset and pass its address to styled controls; invalidate after changing it.

API: `cui_theme_light`, `cui_theme_preset_get`. [Public header](../include/cui/cui_theme.h); [guide](THEME_GUIDE.md).

## TitleBar

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_title_bar_state and window interface binding.

**Interaction:** on_close callback handles application close; window host functions implement native window behavior.

**Usage:** Submit current window information each frame; keep host/user pointers valid during calls.

API: `cui_title_bar`. [Public header](../include/cui/cui_chrome.h); [guide](CHROME.md).

## Toolbar

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns item/command descriptors and command user data.

**Interaction:** cui_toolbar_render_fn composes items; cui_toolbar_command.on_activate handles command activation.

**Usage:** Submit command descriptors each frame and perform application actions in on_activate.

API: `cui_toolbar`, `cui_toolbar_command_render`. [Public header](../include/cui/cui_toolbar.h); [guide](TOOLBAR.md).

## Tree

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains model/state, selected cui_id and borrowed node/expansion arrays until next successful rebuild.

**Interaction:** cui_tree_result separates selection_changed, expansion_changed and activated key.

**Usage:** Rebuild preorder data when it changes; submit visible rows each frame and open the activated application item from the result.

API: `cui_tree_create`, `cui_tree_rebuild`, `cui_tree`, `cui_tree_destroy`. [Public header](../include/cui/cui_tree.h); [guide](TREE.md).

## VirtualList

**Composition:** application model + frame composition. **Scope:** styled frame adapter + retained base model.

**State:** Application retains cui_virtual_list and cui_virtual_widget_state; keyed extents live in the model and row controls remain application-owned.

**Interaction:** cui_virtual_render_item_fn composes only the visible window; cui_virtual_measure_item_fn supplies optional intrinsic measurement; scrollbars and owner keys update state.

**Usage:** Rebuild keyed item extents between frames, then call cui_virtual_list_draw with a viewport and row callback; keep model keys stable while callbacks run.

API: `cui_virtual_list_draw`, `cui_base_virtual_list`. [Public header](../include/cui/cui_virtual_scroll.h); [guide](VIRTUAL_LIST.md), [guide](VIRTUALIZATION.md).

## Chart

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains cui_chart_state, its RGBA pixel buffer/byte capacity, data arrays and resources used by options.

**Interaction:** Hit testing identifies data; tooltip/format/paint callbacks customize output and are not application action callbacks.

**Usage:** Submit data/options each frame; route application actions from explicit hit testing or child controls.

API: `cui_chart`, `cui_chart_hit_test`. [Public header](../include/cui/cui_chart.h); [guide](CHART.md).

## Command

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains command state, row workspace, cui_edit query and command descriptors.

**Interaction:** on_select/on_query/on_confirm/on_cancel observe synchronous interaction; command item action invokes application work.

**Usage:** Initialize bounded query/row storage, submit entries each frame, and execute actions only from confirmation/action events.

API: `cui_command_init`, `cui_command`. [Public header](../include/cui/cui_command.h); [guide](COMMAND.md).

## DataTable

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains allocated model/state, data rows/columns and any borrowed label storage.

**Interaction:** cui_table_result reports interaction; cui_table_label_fn provides accessibility text and cui_table_cell_fn composes visible cells.

**Usage:** Bind data when changed, submit virtual cells each frame, and handle selection/sort actions from the result.

API: `cui_table_create`, `cui_data_table`, `cui_table_destroy`. [Public header](../include/cui/cui_table.h); [guide](TABLE.md).

## Plot

**Composition:** application model + frame composition. **Scope:** styled frame adapter + standalone numeric model; bounded software rasterizer.

**State:** Caller retains scales, samples, path commands, RGBA pixels, tick labels and callback state; no retained widget tree.

**Interaction:** cui_plot_activate_fn receives validated pointer positions; pure numeric operations return cui_plot_result and the application routes pan/zoom.

**Usage:** Build paths and render into caller-owned pixels, then submit a themed bitmap and axes each frame; keep labels and pixels alive through renderer completion.

API: `cui_plot_view`, `cui_plot_axis`, `cui_plot_raster`, `cui_plot_path`, `cui_plot_map`. [Public header](../include/cui/cui_plot_view.h); [guide](PLOT.md); [numeric model](PLOT_MODEL.md).

## Table

**Composition:** immediate frame submission. **Scope:** styled frame component.

**State:** Caller owns row/cell hierarchy and data; no retained table model is required.

**Interaction:** Composition only; child controls handle actions.

**Usage:** Submit sections, rows and cells using balanced begin/end nesting each frame.

API: `cui_table_begin`, `cui_table_begin_row`, `cui_table_begin_cell`, `cui_table_end`. [Public header](../include/cui/cui_table.h); [guide](TABLE.md).

## TextView

**Composition:** application model + frame composition. **Scope:** styled frame component.

**State:** Caller retains state and all source/rendered/layout workspace buffers.

**Interaction:** on_link/open_url handle link interaction; image resolves resources; other content/measurement hooks compose or paint.

**Usage:** Initialize bounded workspace, set text on changes, then submit each frame. Decoded link URLs are borrowed only during callbacks: copy for deferred use.

API: `cui_text_view_init`, `cui_text_view_set_text`, `cui_text_view`. [Public header](../include/cui/cui_text_view.h); [guide](TEXT_VIEW.md).
