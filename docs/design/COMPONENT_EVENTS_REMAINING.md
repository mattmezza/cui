# Remaining component event coverage

Source checkpoint: `07d26c6d` (78 frozen manifest families). Documentation-only
audit, not a parity/status promotion or a claim of 78 uniform event APIs. The
[contract](COMPONENT_EVENTS_CONTRACT.md), original [matrix](COMPONENT_EVENTS_COVERAGE.md)
and [binding audit](COMPONENT_EVENTS_BINDINGS.md) retain their historical scope;
current headers and implemented guides take precedence over their older absence
statements. No production behavior or tests changed and no runtime builds were
run for this audit. Linked tests are evidence to inspect, not newly executed proof.

## User-facing priority

Button, Input, Checkbox, Switch, Toggle, controlled double Slider, selections,
menus and completed dialog close already have additive C APIs. The remaining
objective is discoverable use and honest boundaries, rather than inventing a
new callback for every render family. Standalone legacy Radio/float Slider and
legacy entrypoints are preserved alternatives, not automatically adapted APIs.
Controlled requests permit owner rejection; applied notifications do not. Enter
submission, gesture commit and completed surface close are distinct events.

Immediate handlers are submission-borrowed and payloads callback-borrowed unless
their guide says otherwise. Text/arrays require checked owned copies. Shared
metadata never establishes an unavailable cause. Context destruction, recursive
composition and freeing active storage during callbacks remain unsupported.
Surface is retained; Speech is pumped; retained models and deferred rendering
retain their longer lifetimes. None is silently retimed to end-frame.

## Exhaustive disposition (78/78)

A = additive facade exists for the stated scope; T = existing typed API with a
justified timing/lifetime exception; N = render/child composition; H = host,
resource or deferred-render exception; G = genuine gap or pending slice. A is not
full-family verification. Each family links its authoritative public header.

| Family | Class | Current boundary and next work |
| --- | --- | --- |
| [Accordion](../../include/cui/cui_disclosure.h) | A | [DISCLOSURE_EVENTS](../DISCLOSURE_EVENTS.md): Every applied expansion transition; no controlled veto added. |
| [AlertDialog](../../include/cui/cui_surface.h) | A | [SURFACE_EVENTS](../SURFACE_EVENTS.md): Retained completed-close only; existing decision policy separate; open/request/veto not notifications. |
| [Alert](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Attachment](../../include/cui/cui_media.h) | A | [ATTACHMENT_EVENTS](../ATTACHMENT_EVENTS.md): Coalesced OPEN/REMOVE/RETRY after old callbacks in category order, not raw-input multiplicity. |
| [Avatar](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Badge](../../include/cui/cui_components.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Bubble](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Button](../../include/cui/cui_components.h) | A | [CONTROL_EVENTS](../CONTROL_EVENTS.md): Per-activation Button; immutable controlled Check requests, not confirmed owner changes. |
| [Checkbox](../../include/cui/cui_components.h) | A | [CONTROL_EVENTS](../CONTROL_EVENTS.md): Per-activation Button; immutable controlled Check requests, not confirmed owner changes. |
| [Clipboard](../../include/cui/cui_sdl3.h) | H | Host get/set service; no editable component event. Preserve platform capability/failure semantics. |
| [Collapsible](../../include/cui/cui_disclosure.h) | A | [DISCLOSURE_EVENTS](../DISCLOSURE_EVENTS.md): Every applied expansion transition; no controlled veto added. |
| [DropdownButton](../../include/cui/cui_choice.h) | A | [DROPDOWN_EVENTS](../DROPDOWN_EVENTS.md): Action phase then every menu command; indices can decrease across phases. |
| [Empty](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Focus Trap](../../include/cui/cui_overlay.h) | G | Explicit overlay/focus-scope operations and close-input result; no trap lifecycle callback. Child events and Surface completed-close are separate. |
| [GroupBox](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [HoverCard](../../include/cui/cui_content.h) | G | Visibility reduction has no lifecycle observer; content callback is rendering, not opening notification. |
| [Icon](../../include/cui/cui_icon.h) | H | Resource loading/style/rendering, not activation; loader users/resources follow resource lifetime. |
| [Image](../../include/cui/cui_media.h) | H | Loader/result and borrowed frame resources through render completion; no implied image activation or async load-change event. |
| [Kbd](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Label](../../include/cui/cui_components.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Marker](../../include/cui/cui_status.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Message](../../include/cui/cui_message.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [MessageScroller](../../include/cui/cui_message.h) | G | Typed visible-window/scroll result and explicit append/splice bookkeeping, no message-added/read event; row collection remains stable through end_frame. |
| [Notification](../../include/cui/cui_status.h) | T | Retained descriptor click/action/close callbacks with typed ID and host send/retract operations; descriptor strings/users survive use, not just submission. |
| [Pagination](../../include/cui/cui_numeric.h) | A | [NUMERIC_SELECTION_EVENTS](../NUMERIC_SELECTION_EVENTS.md): Aggregate applied page/index/rating; normalization is not automatically notification. |
| [Progress](../../include/cui/cui_components.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Radio](../../include/cui/cui_components.h) | A | [SELECTION_EVENTS](../SELECTION_EVENTS.md): RadioGroup Base/Styled transitions covered; standalone legacy int Radio has no new facade. |
| [Rating](../../include/cui/cui_numeric.h) | A | [NUMERIC_SELECTION_EVENTS](../NUMERIC_SELECTION_EVENTS.md): Aggregate applied page/index/rating; normalization is not automatically notification. |
| [Shimmer](../../include/cui/cui_status.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Skeleton](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Spinner](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Stepper](../../include/cui/cui_numeric.h) | A | [NUMERIC_SELECTION_EVENTS](../NUMERIC_SELECTION_EVENTS.md): Aggregate applied page/index/rating; normalization is not automatically notification. |
| [Switch](../../include/cui/cui_components.h) | A | [BOOLEAN_EVENTS](../BOOLEAN_EVENTS.md): Base/Styled immutable entry requests; ToggleGroup transitions are separately covered by LIST_EVENTS. |
| [Tag](../../include/cui/cui_status.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Toggle](../../include/cui/cui_components.h) | A | [BOOLEAN_EVENTS](../BOOLEAN_EVENTS.md): Base/Styled immutable entry requests; ToggleGroup transitions are separately covered by LIST_EVENTS. |
| [Tooltip](../../include/cui/cui_content.h) | G | Visibility reduction has no lifecycle observer; hover/focus timing and omitted submissions require distinct lifecycle design. |
| [Calendar](../../include/cui/cui_picker.h) | A | [PICKER_EVENTS](../PICKER_EVENTS.md): Aggregate applied dates/RGBA; navigation/latent HSV/popup lifecycle excluded. |
| [ColorPicker](../../include/cui/cui_picker.h) | A | [PICKER_EVENTS](../PICKER_EVENTS.md): Aggregate applied dates/RGBA; navigation/latent HSV/popup lifecycle excluded. |
| [Combobox](../../include/cui/cui_choice.h) | A | [COMBOBOX_EVENTS](../COMBOBOX_EVENTS.md): Selection only; query editing and submission are not derived from selection notifications. |
| [DatePicker](../../include/cui/cui_picker.h) | A | [PICKER_EVENTS](../PICKER_EVENTS.md): Aggregate applied dates/RGBA; navigation/latent HSV/popup lifecycle excluded. |
| [Editor](../../include/cui/cui_editor.h) | A | [MULTILINE_EVENTS](../MULTILINE_EVENTS.md): Aggregate revision CHANGE after whole reducer; Editor completion included, no invented cause or submit. |
| [Form](../../include/cui/cui_form.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Input](../../include/cui/cui_components.h) | A | [TEXT_INPUT_STYLED_EVENTS](../TEXT_INPUT_STYLED_EVENTS.md): Base/Styled committed per-event text CHANGE and eligible SUBMIT; secure payload redaction. |
| [Input Group](../../include/cui/cui_form.h) | A | [INPUT_GROUP_EVENTS](../INPUT_GROUP_EVENTS.md): Addon activations plus nested Input changes/submits; custom children remain independent. |
| [NumberInput](../../include/cui/cui_numeric.h) | A | [NUMBER_EVENTS](../NUMBER_EVENTS.md): Aggregate exact-double applied change; draft revisions distinct, no inferred commit. |
| [OtpInput](../../include/cui/cui_otp.h) | A | [OTP_EVENTS](../OTP_EVENTS.md): Aggregate final borrowed text and operation/completion counts; masked redaction, no submit claim. |
| [Questionnaire](../../include/cui/cui_questionnaire.h) | T | Retained typed current/answer/completed/submit events on explicit operations; validators are policy, errors borrowed until next validation. |
| [Select](../../include/cui/cui_choice.h) | A | [SELECT_EVENTS](../SELECT_EVENTS.md): Per-transition selection; OPTION accessibility ACTIVATE remains unsupported. |
| [Slider](../../include/cui/cui_components.h) | A | [STYLED_SLIDER_EVENTS](../STYLED_SLIDER_EVENTS.md): Controlled double Base/Styled BEGIN/UPDATE/COMMIT/CANCEL; legacy float path stays separate. |
| [Speech](../../include/cui/cui_speech.h) | T | Typed bounded provider reports delivered by owner-thread pump only; generation/session checks and pump lifetime cannot become immediate handlers. |
| [Textarea](../../include/cui/cui_textarea.h) | A | [MULTILINE_EVENTS](../MULTILINE_EVENTS.md): Aggregate revision CHANGE after whole reducer; Editor completion included, no invented cause or submit. |
| [TimeField](../../include/cui/cui_picker.h) | A | [TIME_FIELD_EVENTS](../TIME_FIELD_EVENTS.md): Final aggregate typed time; draft/submit/cause excluded. |
| [Carousel](../../include/cui/cui_carousel.h) | T | Retained index-change callback already reports confirmed selection; no common metadata envelope. Preserve state/user lifetime and operation-time dispatch. |
| [DescriptionList](../../include/cui/cui_description.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Dialog](../../include/cui/cui_surface.h) | A | [SURFACE_EVENTS](../SURFACE_EVENTS.md): Retained completed-close only; existing decision policy separate; open/request/veto not notifications. |
| [Dock](../../include/cui/cui_dock.h) | T | Typed event/panel/node, resize and transfer requests, explicit lifecycle capture/dispatch and retained descriptors; no blanket submission borrowing. |
| [List](../../include/cui/cui_choice.h) | A | [LIST_EVENTS](../LIST_EVENTS.md): Ordered ID transitions and confirmation; ToggleGroup per-item changes separate. |
| [Menu](../../include/cui/cui_choice.h) | A | [MENU_EVENTS](../MENU_EVENTS.md): MenuTree, SimpleMenu and ContextMenu forwarding implemented; opening is not command activation. |
| [Popover](../../include/cui/cui_surface.h) | A | [SURFACE_EVENTS](../SURFACE_EVENTS.md): Retained completed-close only; existing decision policy separate; open/request/veto not notifications. |
| [Resizable](../../include/cui/cui_resizable.h) | T | Retained on_resize receives borrowed live state; typed changed/completed/phase results. Immutable snapshot facade would need independent lifetime review. |
| [Root View](../../include/cui/cui_root.h) | H | Retained plugin build/render/destroy/copy services; explicit dispose ordering, not value notifications. |
| [Scrollable](../../include/cui/cui_scroll.h) | G | No common scroll observer. Begin/reduce and finish/deferred-child paths can both update offsets; cannot infer one callback per wheel input from final bool. |
| [Settings](../../include/cui/cui_settings.h) | A | [SETTINGS_EVENTS](../SETTINGS_EVENTS.md): Applied navigation/built-in values/text/reset; custom controls and custom reset hooks separate. |
| [Sheet](../../include/cui/cui_surface.h) | A | [SURFACE_EVENTS](../SURFACE_EVENTS.md): Retained completed-close only; existing decision policy separate; open/request/veto not notifications. |
| [Sidebar](../../include/cui/cui_sidebar.h) | A | [SIDEBAR_EVENTS](../SIDEBAR_EVENTS.md): Rows, expansion, collapse and header/footer complete; GROUP pointer_activatable supports a11y, TREEITEM limitation remains. |
| [StatusBar](../../include/cui/cui_chrome.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Tabs](../../include/cui/cui_choice.h) | A | [TABS_EVENTS](../TABS_EVENTS.md): Aggregate stable-ID selection; reorder/overflow/scroll lifecycle excluded. |
| [Theme](../../include/cui/cui_theme.h) | H | Explicit value projection/loading and icon-loader policy; no ambient theme-change event bus. |
| [TitleBar](../../include/cui/cui_chrome.h) | G | Design proposal only (TITLE_BAR_EVENTS_SLICE); host dispatch acceptance is not confirmed OS window state. Preserve Base gestures before Styled controls and existing coalescing. |
| [Toolbar](../../include/cui/cui_toolbar.h) | A | [TOOLBAR_EVENTS](../TOOLBAR_EVENTS.md): Aggregate command activation after paint/legacy callback; custom renderer opt-in. |
| [Tree](../../include/cui/cui_tree.h) | A | [TREE_EVENTS](../TREE_EVENTS.md): Applied selection/expansion and invocation; intermediate reducer state preserved. |
| [VirtualList](../../include/cui/cui_virtual_scroll.h) | G | Typed scroll_changed/window_pending result and child renderers; no scroll/window notification facade. Deferred window can lag one frame; rows own item events. |
| [Chart](../../include/cui/cui_chart.h) | H | Typed hover/result and tooltip/render policy; callbacks may run during layout/prepaint/paint with current-frame borrowed data. |
| [Command](../../include/cui/cui_command.h) | G | Base operation observers implemented (COMMAND_EVENTS); Styled forwarding in flight, not shipped at this checkpoint. Query-before-old-hook exception preserves borrowed query lifetime; cancel separate. |
| [DataTable](../../include/cui/cui_table.h) | A | [TABLE_EVENTS](../TABLE_EVENTS.md): Sort/selection/set snapshots/columns/actions/ROW requests; viewport/load-edge still result-only. |
| [Plot](../../include/cui/cui_plot_view.h) | H | Existing prepaint appearance callback and hover/result; deferred resolver/paint lifetimes must remain through render completion. |
| [Table](../../include/cui/cui_table.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [TextView](../../include/cui/cui_text_view.h) | T | Existing on_link borrows decoded URI/event, on_reveal requests viewport movement. No common selection/link metadata facade; copying struct does not own URI. |

## Independently reviewable follow-ups

1. **Finish Styled Command forwarding**, already in flight; do not implement a
   competing path. [Base reducer](../../src/base/cui_command.c),
   [Styled reducer](../../src/components/cui_command.c),
   [current tests](../../tests/unit/test_command_events.c) and
   [checkpoint](COMMAND_EVENTS_SLICE.md) establish query/selection/confirmation
   ordering. Require multiple confirmations, old-hook model/query replacement,
   cancellation, passive work, borrowed query copies and actual Styled evidence.
   Base PROGRAMMATIC metadata must not acquire synthetic UI evidence.
2. **Review TitleBar proposal before code**: [exact seams](TITLE_BAR_EVENTS_SLICE.md),
   [Base](../../src/base/cui_chrome.c), [Styled](../../src/components/cui_chrome.c),
   [fake-host tests](../../tests/unit/test_chrome.c). A typed dispatch observer
   should report the host operation and actual return, not window destruction or
   accepted physical drag. Preserve coalesced control actions, close override,
   decoration/capability gates, gesture-before-controls ordering and callback
   mutation behavior. Headless fake-host proof is distinct from OS proof.
3. **Scroll observation checkpoint**: [scroll API](../../include/cui/cui_scroll.h),
   [Base](../../src/base/cui_scroll.c), [tests](../../tests/unit/test_scroll.c),
   [virtual implementation](../../src/base/cui_virtual_scroll.c),
   [virtual tests](../../tests/unit/test_virtual_styled.c) and
   [message implementation](../../src/components/cui_message.c). Start with one
   explicitly bounded final-offset observer after finish; separately review
   virtual-window completion and MessageScroller. Prove wheel, keyboard,
   scrollbar drag, child reveal/deferred scroll, clamping/normalization, no-op,
   inert work and repeated events. Do not turn final offsets into fabricated
   per-input transitions or equate row rendering with data insertion.
4. **Optional visibility lifecycle**, only if applications need it: use
   [content reduction](../../src/components/cui_content.c) and
   [overlay operations](../../src/base/cui_overlay.c), with
   [content](../../tests/unit/test_content.c) and
   [overlay tests](../../tests/unit/test_overlay.c). Audit hover/focus delay,
   dismissal, omitted submission and motion completion before defining OPEN/CLOSE.
   Focus Trap is a child scope, not another Dialog; avoid duplicate close delivery.
5. **TextView narrowly typed link/selection observation**: audit
   [link and reveal dispatch](../../src/components/cui_text_view.c) and
   [tests](../../tests/unit/test_text_view.c). Keep decoded URI ownership explicit;
   selection changes, reveal requests and link activation need separate payloads.
   Require pointer/key supported paths, copy bounds, callback mutation constraints
   and selection changes returning to their initial state. Existing typed callback
   is useful now; absence of the shared envelope is not absence of interaction.
6. **Retained APIs only on demonstrated need**: Carousel, Notification,
   Questionnaire, Resizable and Dock already have typed notifications. Their
   [original ownership evidence](COMPONENT_EVENTS_COVERAGE.md) remains relevant.
   Any shared-metadata adapter must first specify registration replacement,
   retained user lifetime, snapshot versus live-state borrowing, programmatic
   operations and detach/dispose. Speech must preserve bounded owner-thread pump
   delivery and provider generation checks. No blanket immediate wrapper is safe.

These are proposals, not authorization or implemented contracts. Each production
slice needs strict C11/C++ public-header checks, meaningful legacy comparisons,
multiple/no-op/passive inputs, correct counts and owned-copy tests, plus CTest via
`python scripts/test_local.py --build-dir <isolated-build>` on Linux. Browser
checks remain headless; private Xvfb is not hardware/Wayland/physical-drag proof.

## Binding coverage is a separate axis

[Initial V1](../BRIDGE_NOTIFICATIONS.md) exposes CLICK, audited boolean/table and
surface-close views. [Opt-in values](../BRIDGE_VALUE_NOTIFICATIONS.md) add only:

- SCALAR: Slider, NumberInput, Rating, Pagination.
- TEXT: Input, Textarea, Editor, InputGroup.
- SELECTION: RadioGroup, Tabs, Select, List, Stepper.

Python, JS/TypeScript, Go and Rust have owning manual value polls documented in
those guides. They preserve the existing shared queue, aggregate producer timing,
full historical handles and uint64 frames, bounded capacity negotiation, optional
symbol availability and LEGACY downgrade. Rust static builds require their paired
SDK; dynamic availability does not promise mismatched static-library compatibility.
These are applied bridge notifications, not exports of the new synchronous C
request handlers, per-transition C selection observers or Slider gesture phases.
No foreign synchronous rejection, callback auto-fanout or independent queue exists.

Menu/Dropdown, Combobox, dates/time/color, Tree/Sidebar/Command/Dock/Settings and
other unaudited values are not newly typed by resemblance. Plain bridge text has
no secure/masked model configuration: direct C secure redaction is not a promise
about legacy bridge bytes. Future secure support must cover both typed and legacy
copies before exposure. Bridge Item slot/generation handles are not Core uint64
IDs. C selection intermediate transitions and query/submit reasons cannot be
recovered by parsing the existing final JSON payload.

The next binding slices should audit actual enqueue producers individually,
capture typed data at enqueue without JSON parsing, negotiate categories on the
same queue, and test real queued UI input separately from synthetic protocol
fixtures. Preserve old/new polling interleave, FIFO/capacity atomicity, removal and
historical generations, malformed-tag failure, older-library no-drain, UTF-8
ownership and numeric precision in every language. A new C facade alone does not
close any of these binding gaps.
