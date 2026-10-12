# Questionnaire source audit

Questionnaire is registered in the main library and distribution. Its interactive gallery story is implemented; full source parity remains unfinished. The retained model and initial composable styled parts are implemented; this is not full parity. Reference commit: `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/base/src/questionnaire/{types,state,keyboard,control}.rs` and `crates/component/src/questionnaire/components.rs`.

## State and answers

Ordered item definitions have a unique name, accessible label, optional description, required/multiple/disabled flags, choices, an optional retained Input editor and a validator. Choice values are unique within an item. Single-answer definitions reject multiple defaults even if one default is disabled. Disabled default choices do not enter the initial answer. A nonblank enabled initial input replaces defaults for single-answer items. Preserve the original input draft separately from the effective answer, including when a choice supersedes the freeform answer.

Effective answers always use schema choice order. Disabling a choice hides it from the effective answer without deleting the retained selection; enabling restores it. Disabled items have empty effective answers and are excluded from totals and submission. Multiple-answer items may include both choices and freeform; single-answer items may include only one source. Whitespace-only freeform is absent. Setters validate names, choices and capacities atomically before changing model/editor/history. Programmatic setters are silent; user changes emit AnswerChanged. Reset restores initial answers and original input drafts while retaining current disabled flags and external errors.

## Validation and navigation

Unanswered, Answered and Skipped are distinct states. An empty optional item is invalid until explicitly skipped. Internal errors distinguish Required, Unanswered and validator text; external errors take precedence. Disabled and skipped items suppress errors. After the first validation attempt, answer edits revalidate immediately. Any answer change clears completion and cancels pending confirmation.

Current position and total refer to enabled items. Disabling the current item chooses the next enabled item, then the previous one; enabling an item when none is current selects it. Previous navigation does not validate; Next validates the current item. Submit validates every enabled item and returns to the first invalid one. A valid submission emits Completed once per completed state, then Submit every time. Changes allow a later Completed again. CurrentItemChanged is emitted by user navigation but not silent setters. Submission and validation snapshots preserve schema order.

## Activation, focus and timing

Single-choice user activation selects then confirms after 150 ms, letting the selection become visible. Reduced motion still defers confirmation until after AnswerChanged callbacks, with no time delay. Activating the already-selected answer confirms immediately. Multiple-choice activation only toggles. Radio arrow movement changes selection without confirming. Another answer/current-item change cancels the pending confirmation; event subscribers may enable follow-up items before it runs.

Enabled choices receive A–Z or 1–9 shortcuts by enabled position, skipping disabled choices. Focus moves between choices and the optional Input with the source's radio-group behavior. A selected choice owns Enter confirmation; an unselected choice owns activation. A focused nonempty Input retains arrows and text, while empty Input Up/Down may leave it. Ctrl/Command+Enter confirms; unrelated modifiers, repeats, IME composition and preferred character input must not trigger shortcuts. Rendered controls must use real Checkbox/Radio/Input behavior and graphical marks, with accessible names, description, set position and focus ownership.

## Verification requirements

Port source behavioral cases for schema rejection, silent setters, draft preservation, reset, disabled restoration, schema order, external errors, cross-item validation, first-invalid focus, completion ordering, shortcuts, disabled-current fallback, delayed choice confirmation and no confirmation from radio arrows/typing. Add transactional capacity/UTF-8 and standalone base-link checks. Styled composition needs its own source audit and actual pointer/keyboard, four-size, disabled/error and focus tests before being called implemented. Full parity is not established by a state reducer or a static wizard screenshot.

## Current implementation evidence

The portable retained model tests cover schema errors, disabled answer restoration, effective-answer change detection, input drafts, capacity/UTF-8 atomicity, optional skip validation, delayed/reduced-motion confirmation, and completion-before-submit ordering. Styled tests exercise all four card sizes, pointer activation and updated checkbox semantics, minimum card height, error/action visibility, Skip completion, actual text editing into a freeform answer, and disabled input suppression. Standalone strict C11 and ASan/UBSan checks pass; the existing branch CTest suite passes73/73.

Remaining work includes root keyboard capture and focus choreography, complete composable item/choices/action containers, custom indicator/content/shortcut slots, localization hooks, exact selected/hover/focus colors and source radius refinements, root shortcut routing and cross-item focus transfer, comprehensive keyboard coverage and platform accessibility integration. Existing styled code is a development checkpoint, not evidence that these gaps are complete.

Input Enter confirmation and radio Left/Right wrapping now have native and sanitizer tests, including disabled-choice skipping, repeat suppression, event consumption, and no delayed confirmation from arrow selection. The explicit radio key helper must be called before choice rendering; full root keyboard routing remains unfinished.

The root now owns a nonmodal focus scope. The explicit answer-key helper traverses enabled choices and an empty input with Up/Down, wraps in schema order, selects radios without confirming, and preserves cursor movement for nonempty drafts. Call it inside the root before answer parts. Native and sanitizer tests cover the choice/input boundary. Automatic root capture and initially focused-answer restoration remain unfinished.

The explicit confirm-key helper supports Enter on a focused filled choice/input and a host-selected Ctrl/Meta+Enter modifier. Modified submission owns and cancels the core activation token even when validation fails, preventing accidental radio selection. The native regression verifies this fallback failure and successful confirmation; sanitizer and existing73CTest gates pass.

The browser story now exercises required validation, delayed radio navigation, optional Skip, typed freeform submission, reset, disabled controls and all four sizes. The integrated library passes 77 native and 75 sanitizer suites. Gallery coverage does not establish complete keyboard, styling, accessibility or composition parity.

`cui_questionnaire_shortcut_keys` routes bare A–Z or 1–9 key identities to enabled choices by their visible shortcut position, focuses the chosen card, and uses the same delayed/repeated-choice confirmation model as pointer activation. Invoke it inside the root before input and choice parts. It ignores consumed events, repeat, modifiers, focused text input, disabled root/item, IME composition and preferred character input. The other explicit key helpers also yield to the two host keyboard-context flags. SDL tracks composition start/end when forwarding keys, and core default Enter/Tab/Space behavior yields to those flags. This prevents shortcut/default activation during composition; complete IME preedit rendering and automatic root keyboard capture remain unfinished.

Arrow entry from a non-answer control now restores the first (Down) or last (Up) filled answer in schema order before choosing an unfilled answer. Restoring focus leaves the answer unchanged and does not schedule confirmation. A failing-before/passing-after headless regression covers both directions from an action button.

Choice-card paint now follows the pinned selected/hover/invalid/disabled colors, transparent light or 20%-input dark idle background, size-specific card/indicator/shortcut radii and shortcut font sizes. The shared theme exposes explicit palette mode and the muted background token, populated from all 38 pinned palettes. Headless draw assertions exercise four sizes and idle/selected/hover/disabled states across every palette. Focus-ring composition, custom slots and instance style refinements remain unfinished.
