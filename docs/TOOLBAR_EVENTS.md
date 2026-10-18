# Toolbar command activation

Include `cui/cui_toolbar_events.h` and call `cui_toolbar_command_events` in a
custom Toolbar item renderer. Existing container and command entrypoints remain
unchanged. For example, an application-owned adapter can hold its command,
handler and last result:

```c
typedef struct {
    cui_toolbar_command command;
    cui_component_activation_handler handler;
    cui_toolbar_command_event_result result;
} command_item;
static void render_command(cui_context *ui, const cui_theme *theme,
                           const cui_toolbar_presentation *p, void *user) {
    command_item *item = user;
    item->result = cui_toolbar_command_events(ui, theme, p, &item->command,
                                               item->handler);
}
```

Use `render_command` as a `cui_toolbar_item.render`, with the adapter as its user.
The handler uses the shared user-first signature
`void callback(void *user, const cui_component_event_metadata *event)`.

`valid` means legacy command/presentation/name validation passed. `submitted`
means the command ran during ordinary active submission with a nonzero frame
sequence; it does not promise allocation success or paint delivery. Measurement,
inert/layout-only and out-of-frame work does not emit an observer callback.
`activation_count` counts actual observer calls, zero or one. A missing callback
ignores its user and leaves interaction/legacy callbacks intact.

The command still interacts once, submits its icon/label/box, calls legacy
`on_activate(activate_user)`, then calls the new observer. Repeated accepted inputs
coalesce exactly as before; this API does not provide per-input callbacks. Choose
one callback/result path for each application side effect to avoid doing it twice.
Selected state is caller-owned presentation, not a Toggle CHANGE. Disabled
commands do not activate; a disabled Toolbar container suppresses navigation only
and may still host enabled commands.

ACTIVATE metadata targets the command ID. Evidence freezes immediately after
interaction, before icon or legacy callbacks can consume events/change focus.
Exactly one accepted activation carries its genuine snapshot/index/source. If
multiple activations coalesce, or genuine evidence is absent, only the proven
frame is supplied: source UNKNOWN, no invented index or activation snapshot.
No REQUEST, rejection, commit, container event, dispatch or binding API is added.

Handler/user borrow through the synchronous call. Metadata is pointer-free and
copyable beyond frame/context lifetime. Command/presentation/theme/strings/icon
resources and context must remain live through outer return and renderer
consumption. New observers may copy payloads and set unrelated flags; they must
not mutate/free/rebind active resources, recurse, destroy context or unwind across
C. Legacy hooks retain their timing and mutations but must leave resources needed
by remaining rendering/observation live. Record deferred omission/teardown and
perform it after submission and renderer consumption. Omitting a command emits
nothing; there is no retained registration or replay.
