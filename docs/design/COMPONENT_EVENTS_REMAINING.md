# Remaining component event coverage

Source checkpoint: `18d3c17d` (78 frozen manifest families). Documentation-only
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
| [Alert](../../include/cui/cui_content.h) | A | [ALERT_EVENTS](../ALERT_EVENTS.md): coalesced completed Dismiss action after legacy visibility assignment; frame-only metadata, no per-input observer or close/veto promise. Child actions remain separate. |
| [Attachment](../../include/cui/cui_media.h) | A | [ATTACHMENT_EVENTS](../ATTACHMENT_EVENTS.md): Coalesced OPEN/REMOVE/RETRY after old callbacks in category order, not raw-input multiplicity. |
| [Avatar](../../include/cui/cui_content.h) | A | [AVATAR_EVENTS](../AVATAR_EVENTS.md): single and visible-stack per-event activation observers preserve genuine child evidence and legacy aggregate order. Legacy void Avatar remains presentational; foreign callback facades are omitted. |
| [Badge](../../include/cui/cui_components.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Bubble](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Button](../../include/cui/cui_components.h) | A | [CONTROL_EVENTS](../CONTROL_EVENTS.md): Per-activation Button; immutable controlled Check requests, not confirmed owner changes. |
| [Checkbox](../../include/cui/cui_components.h) | A | [CONTROL_EVENTS](../CONTROL_EVENTS.md): Per-activation Button; immutable controlled Check requests, not confirmed owner changes. |
| [Clipboard](../../include/cui/cui_sdl3.h) | H | Host get/set service; no editable component event. Preserve platform capability/failure semantics. |
| [Collapsible](../../include/cui/cui_disclosure.h) | A | [DISCLOSURE_EVENTS](../DISCLOSURE_EVENTS.md): Every applied expansion transition; no controlled veto added. |
| [DropdownButton](../../include/cui/cui_choice.h) | A | [DROPDOWN_EVENTS](../DROPDOWN_EVENTS.md): Action phase then every menu command; indices can decrease across phases. |
| [Empty](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Focus Trap](../../include/cui/cui_overlay.h) | N | Reviewed primitive exception in [checkpoint](LIFECYCLE_RESULT_EVENTS_SLICE.md): caller-owned scope begin/end is composition, not open/close lifecycle. Optional overlay-operation observer absent; Surface close separate. |
| [GroupBox](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [HoverCard](../../include/cui/cui_content.h) | A | [HOVER_EVENTS](../HOVER_EVENTS.md): Pure Base and Styled final open-model transitions; not actual visibility or animation completion. Child composition remains separate. |
| [Icon](../../include/cui/cui_icon.h) | H | Resource loading/style/rendering, not activation; loader users/resources follow resource lifetime. |
| [Image](../../include/cui/cui_media.h) | H | Loader/result and borrowed frame resources through render completion; no implied image activation or async load-change event. |
| [Kbd](../../include/cui/cui_content.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Label](../../include/cui/cui_components.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Marker](../../include/cui/cui_status.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Message](../../include/cui/cui_message.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [MessageScroller](../../include/cui/cui_message.h) | A | [MESSAGE_EVENTS](../MESSAGE_EVENTS.md): Base/Styled exact double-offset and following-tail observers after legacy composition; explicit mutators and deferred bounds remain outside call-local notification. |
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
| [Tooltip](../../include/cui/cui_content.h) | A | [HOVER_EVENTS](../HOVER_EVENTS.md): Aggregate applied open-model transition, distinct from composition bool; omission retains no listener or synthetic close. |
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
| [Scrollable](../../include/cui/cui_scroll.h) | A | [SCROLL_EVENTS](../SCROLL_EVENTS.md): Paired Base/Styled final-offset observer after children/finish; aggregate normalization included, no fabricated per-input transitions. Overlay, virtual and message paths remain separate. |
| [Settings](../../include/cui/cui_settings.h) | A | [SETTINGS_EVENTS](../SETTINGS_EVENTS.md): Applied navigation/built-in values/text/reset; custom controls and custom reset hooks separate. |
| [Sheet](../../include/cui/cui_surface.h) | A | [SURFACE_EVENTS](../SURFACE_EVENTS.md): Retained completed-close only; existing decision policy separate; open/request/veto not notifications. |
| [Sidebar](../../include/cui/cui_sidebar.h) | A | [SIDEBAR_EVENTS](../SIDEBAR_EVENTS.md): Rows, expansion, collapse and header/footer complete; GROUP pointer_activatable supports a11y, TREEITEM limitation remains. |
| [StatusBar](../../include/cui/cui_chrome.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [Tabs](../../include/cui/cui_choice.h) | A | [TABS_EVENTS](../TABS_EVENTS.md): Aggregate stable-ID selection; reorder/overflow/scroll lifecycle excluded. |
| [Theme](../../include/cui/cui_theme.h) | H | Explicit value projection/loading and icon-loader policy; no ambient theme-change event bus. |
| [TitleBar](../../include/cui/cui_chrome.h) | H | Reviewed host-service exception in [checkpoint](TITLE_BAR_EVENTS_SLICE.md): existing perform/close hooks remain synchronous services, not confirmed OS lifecycle or value notifications. No proposed observer implemented. |
| [Toolbar](../../include/cui/cui_toolbar.h) | A | [TOOLBAR_EVENTS](../TOOLBAR_EVENTS.md): Aggregate command activation after paint/legacy callback; custom renderer opt-in. |
| [Tree](../../include/cui/cui_tree.h) | A | [TREE_EVENTS](../TREE_EVENTS.md): Applied selection/expansion and invocation; intermediate reducer state preserved. |
| [VirtualList](../../include/cui/cui_virtual_scroll.h) | A | [VIRTUAL_EVENTS](../VIRTUAL_EVENTS.md): Base/Styled aggregate final float-offset observers; deferred rendered window remains result-only and can lag one frame. Rows own item events. |
| [Chart](../../include/cui/cui_chart.h) | H | Typed hover/result and tooltip/render policy; callbacks may run during layout/prepaint/paint with current-frame borrowed data. |
| [Command](../../include/cui/cui_command.h) | A | [COMMAND_EVENTS](../COMMAND_EVENTS.md): Base operations and Styled forwarding implemented. Query-before-old-hook exception preserves borrowed query lifetime; cancel is separate. |
| [DataTable](../../include/cui/cui_table.h) | A | [TABLE_EVENTS](../TABLE_EVENTS.md): Sort/selection/set snapshots/columns/actions/ROW requests; viewport/load-edge still result-only. |
| [Plot](../../include/cui/cui_plot_view.h) | H | Existing prepaint appearance callback and hover/result; deferred resolver/paint lifetimes must remain through render completion. |
| [Table](../../include/cui/cui_table.h) | N | Rendering/container or child composition; successful submission is not activation. Child controls retain their own event contracts. |
| [TextView](../../include/cui/cui_text_view.h) | T | Existing on_link borrows decoded URI/event, on_reveal requests viewport movement. No common selection/link metadata facade; copying struct does not own URI. |

## Updated implementation disposition

Command Base/Styled forwarding and Scrollable Base/Styled pairing are implemented. TitleBar remains an explicitly reviewed host-service exception: host dispatch success is not confirmed OS state, and existing host/close hooks already provide synchronous extension points. TextView keeps its typed link/reveal callbacks; focused contract tests and copying guidance were added without changing dispatch. The original coverage matrix remains a historical checkpoint, not a current absence claim.

HoverCard and Tooltip now have final open-model observers. Focus Trap bracketing and virtual-window completion are explicitly accepted primitive/result exceptions in the [lifecycle checkpoint](LIFECYCLE_RESULT_EVENTS_SLICE.md). VirtualList now exposes aggregate final-offset observers. MessageScroller now exposes exact double-offset/following-tail observers. Deferred window completion remains result-only and is not a claimed callback API. No universal lifecycle coverage is asserted. Foreign binding allowlists remain separately bounded. The follow-up list records the original review plan; the status above supersedes completed/in-flight descriptions in that historical plan.

## Remaining review boundaries

The earlier follow-up plan has landed for Command forwarding, Scrollable,
HoverCard/Tooltip, VirtualList and MessageScroller. TitleBar, Focus Trap and
TextView retain the documented typed/host/composition exceptions. Current
classification is A45, N18, H8, T7, G0 across exactly 78 families. This is a
**disposition count**, not uniform event semantics or complete family parity.

Further work must target demonstrated application needs rather than repeat those
slices. Deferred virtual-window/render completion stays result-only; applied
notification is not a controlled request or veto. Optional visibility observers
describe the open model, not pixels or animation completion. Retained Carousel,
Notification, Questionnaire, Resizable, Dock and Speech keep their explicit
registration/pump/user-lifetime rules. A future metadata adapter must separately
preserve those rules and existing mutation/ordering behavior.

Foreign binding coverage below remains narrower than direct C. Full platform and
accessibility acceptance also remains separate from headless event evidence. Each
future behavior change needs strict public headers, legacy comparisons, owned-copy
and ordering tests plus private-display CTest; no desktop or GPU proof is implied.

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

## Verified delivery checkpoints

The Scroll/Command/Sidebar checkpoint at `5da9648f` passed the warning-enabled
native build and all **292/292** CTest cases through the private-display wrapper
(87.12 seconds). Scroll additionally passed 5/5 WASI and 5/5 ASan/UBSan cases.
The subsequent Hover Base/Styled and standalone Content dependency changes passed
6/6 applicable native cases, 5/5 ASan/UBSan cases, 4/4 WASI suites, strict generated
all-implementation C11 syntax, 11/11 documentation tests and 11/11 skill tests.
This is broad baseline plus focused subsequent proof, not a claim that the earlier
292-case run included APIs added afterward. All 78 manifest entries remain intact.

The separately rebuilt optional async host example passed 5/5 deterministic offline
cases at `5da9648f`, including actual SDL frames and pending-work shutdown under
private Xvfb. Cancellation means immediate logical invalidation plus cooperative
transport cancellation subject to configured timeouts, not a hard real-time abort.
No networking or worker pool was added to Core. Unsupported GPU hardware and
physical macOS/Wayland behavior remain outside this headless proof.

Go/Rust owning notification integration also passed a fresh 52/52-case run after
the Command/Sidebar/InputGroup fixes and before Scroll/Hover additions. That proof
includes actual older-library no-drain behavior and static/dynamic SDK cases; it
neither exports the newer native observers nor implies foreign synchronous request
acceptance. Binding omissions above remain explicit. No new hash-pair claim is
made for that run.
