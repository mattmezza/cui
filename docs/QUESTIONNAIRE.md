# Questionnaire source audit

Questionnaire remains unimplemented. The next implementation must preserve the frozen source contract, rather than treating it as a generic next/previous form. Reference commit: `87d10ae5e1299d1be18670a37570c5402f952e34`, `crates/base/src/questionnaire/{types,state,keyboard,control}.rs` and `crates/component/src/questionnaire/components.rs`.

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
