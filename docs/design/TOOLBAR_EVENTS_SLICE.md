# Toolbar command activation checkpoint

Design only, audited at `28c3076c`. Preserve all 78 manifest entries and legacy
Toolbar options/items/presentation/command layouts and entrypoints.

## Existing contract

[Toolbar API](../../include/cui/cui_toolbar.h) is a composition container: its
items have render callbacks, not intrinsic actions. Sized wrappers prepare
presentation metrics; custom renderers own their child controls and events.
Container `disabled` suppresses navigation only; it does not disable hosted
children. The default command renderer has caller-owned selected/disabled state,
icon callback and a no-argument legacy `on_activate` with separate userdata.

[Styled command rendering](../../src/components/cui_toolbar.c) validates command,
name/presentation and metrics, calls `cui_interact` once as a Button, writes cursor
semantics, then submits its icon/label/box. Only after paint submission it invokes
legacy `on_activate`, at most once if the aggregate interaction activated flag is
true. Multiple accepted inputs coalesce; selection is external presentation,
not a Toggle value. Missing old callback does not make the command noninteractive.
The renderer returns void. Toolbar begin/end navigation and wrapper rendering
must remain unchanged; no container-wide command dispatch is needed.

## Proposed narrow additive observer

A new `cui_toolbar_events.h` may include Toolbar, shared metadata and the existing
`cui_component_activation_handler` convention, then declare:

```c
typedef struct {
    bool valid, submitted;
    size_t activation_count; /* Actual observer calls: zero or one. */
} cui_toolbar_command_event_result;
cui_toolbar_command_event_result cui_toolbar_command_events(
    cui_context *, const cui_theme *, const cui_toolbar_presentation *,
    cui_toolbar_command *, cui_component_activation_handler);
```

Refactor only the command renderer into a private common helper. Legacy void
entrypoint passes no observer and retains exact validation/work. The new helper
returns additive admission/submission/callback facts without reinterpreting the
old void. No public old layout change, allocation, retained callback or input
replay. New observer presence does not alter command availability or selected
state. Existing custom item renderers may call this function and store/read its
result in application state; provide that small adapter in the guide instead of
adding another retained command object merely for callback wiring.

At the original interaction seam, freeze category evidence before the icon or
legacy activation callbacks can alter focus/routing. Unique genuine accepted
activation copies shared ACTIVATE metadata with command ID target and its original
input facts. Multiple accepted activations coalesce to proven frame, UNKNOWN source,
no arbitrary index/snapshot. Missing genuine evidence must remain absent. Dispatch
old `on_activate` with unchanged userdata, then typed handler, after the same paint
submission. There is no CHANGE, REQUEST, COMMIT or container lifecycle event.
Each side effect should use one chosen callback/result path.

Disabled command, inert/layout-only/passive/omitted work emits no new callback;
legacy validation/paint/navigation still follows its old rules. A disabled Toolbar
container can still host an enabled command: preserve that documented distinction.
No new consumption filter, host default-prevention promise or rejection is added.

Handlers/users borrow through the call. Metadata copies by value; command,
presentation, strings/theme/icon resources and context remain live through outer
return plus renderer consumption. New observer cannot mutate/free/rebind active
resources, recurse or destroy context; unrelated flags and payload copies are
supported. Legacy icon/activation hooks keep existing timing/mutations but must
leave resources needed by remaining work and new observation live. Deferred
omission/teardown is the supported pattern.

## Verification and ownership

Before production, review exact common-helper admission/result semantics and
accepted-evidence handling against current source. Worker scope: new header,
Styled Toolbar source, new focused test/guide and checkpoint status. No Base/Core,
container behavior, bridge schema, root wiring or generated changes.

Tests need old/new paint/metrics/semantics/focus/legacy callback equivalence;
real pointer/key/touch/accessibility activation; repeated input coalescing;
unique/ambiguous/missing evidence; icon and old callbacks changing evidence after
capture; shared userdata correctness; missing observer/old callback; selected
state silence; disabled command versus disabled container; passive/inert/omitted
submission; copied payload and deferred removal. Keep legacy Toolbar regressions,
strict C11/C++ headers and private native wrapper checks, then portable WASI.
Existing foreign Toolbar child/command callbacks and generic bridge queues remain
separate; including this header in a source SDK does not expose a foreign facade.

## Implementation checkpoint

Implemented in the independent Toolbar command slice: one private renderer seam,
unchanged legacy entrypoint, additive aggregate observer/result. Capture occurs
immediately after the existing interaction; paint and legacy callback still
precede the typed observer. Unique accepted evidence is copied, ambiguous/missing
evidence reports only the proven frame. No Base/Core/container or foreign API
changes. Focused tests compare legacy/facade paint geometry, metrics, semantics,
focus and callbacks, and exercise pointer/touch/key/accessibility, coalescing,
mutable legacy hooks, passive/disabled policy and copied payload lifetime.
