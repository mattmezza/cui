# Application-controlled Stepper navigation

Include `cui/cui_stepper_navigation.h` for `cui_stepper_navigation` and its
unstyled Base sibling. They take a selected index by value and report a typed
REQUEST for each accepted item activation. The application decides whether to
update its own selection. The existing Stepper functions still mutate their
selected-index pointer and the existing aggregate CHANGE observers retain their
timing and meaning.

This closes a specific frozen-source gap: upstream's controlled selected_index
and on_click let the application decide navigation, including a click on the
already selected step. Upstream has no built-in validator or next/back controller.
The compiling [`examples/stepper_navigation/main.c`](../examples/stepper_navigation/main.c)
rejects a confirmation request while payment data is invalid, then accepts a
later request after the application validates it.

The callback receives captured `selected`, `requested` and `count`. A same-index
activation is still a request. Several accepted activations produce several
requests in event order, even when their final target equals the entry selection.
All requests use the same captured selected value. The current call registers
semantics and paints that captured selection throughout; owner updates become
visible on the next submission. A rejected request simply leaves the owner's
selection unchanged. There is no veto return, automatic selection, normalization,
async validation, form ownership or automatic error-status mutation.

`result.valid` means argument preflight succeeded, not completed layout or visible
rendering. `request_count` counts accepted requests independently of a handler.
A missing handler leaves selection untouched and still counts input. Preflight
requires nonzero ID, count <=100, structurally valid title/description pairs and
valid status enums. For nonempty steps selected must be in range; for empty steps
it must be zero. Invalid input registers nothing and dispatches nothing. This is
deliberately stricter than legacy clamping and upstream unchecked high indices;
invalid-input equivalence is not claimed.

Current semantics must successfully accept the activation. Current disabled or
omitted items, cancellation, inert/layout-only work and rejected accessibility
actions do not generate requests. Consumption follows Core's exact policy:
consuming accessibility input cancels its acceptance; consuming an ordinary
already generated Enter activation does not universally revoke it. This helper
does not consume input. Each request freezes its genuine accepted activation
snapshot before calling the application. Metadata targets the Stepper; the
embedded activation targets its item. Frame, index and pointer/keyboard/
accessibility source are evidence, not inferred cause. Copied payloads remain
usable after callback, frame and context lifetime.

Handler/user, steps, strings and theme are borrowed through the call and never
retained. Keep the borrowed array, strings and theme stable until return. The
handler and by-value arguments are captured before callbacks. Store validation
errors in separate owner fields, then rebuild step descriptors on the next
submission; changing a borrowed status in a callback would mix old semantics
with new paint and is unsupported. The example follows this owner pattern.
The callback runs after current semantic registration and before styled painting.
It may update unrelated application form/selection state. Do not mutate the
borrowed descriptors or active input routing/options, recursively submit the
component, destroy the UI/storage, or unwind a foreign exception. Updating the
application owner cannot change this call's captured selection. Further callbacks
still carry that captured value. Later mutation cannot rewrite a payload already
delivered; future input acceptance is not promised to survive forbidden routing
changes.

Styled rendering reuses the legacy private paint path, including horizontal and
vertical layout and explicit caller error/complete statuses. This API does not
add upstream icons, arbitrary children, sizing, checked separators, ListItem
hierarchy or live AT announcements. There is no Python/Node/TypeScript/Go/Rust
binding surface for these requests yet. Source and platform parity remain partial.
