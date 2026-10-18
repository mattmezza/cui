# Native control events: a small settings panel

This standalone example uses the shipped `cui_button_events` and
`cui_checkbox_events` APIs. It does not use proposed Styled Switch/Toggle event
APIs or a new binding facade. All state, CUI calls and callbacks stay on the UI
thread of the existing `cui_sdl3_run` callback runner.

The panel demonstrates:

- **Enable notifications** requests a new checkbox value. The application owner
  rejects it while **Allow setting changes** is unchecked and accepts it when
  that permission is checked. Rejection is an owner policy, not an event
  unconsume/propagation operation.
- **Apply settings** increments one application counter from its activation
  callback. Returned frame counts are inspected and displayed; they do not
  perform the action again.
- **Close settings panel** only records a deferred request. After `cui_end_frame`
  and rendering consume the frame's borrowed draw data, the host applies that
  request. The next submission omits the panel. **Reopen settings** restores it.
  No callback destroys the context, font, host or its own userdata.

## Copy and run

Tested host: Linux x86-64, GCC 16.2.1, SDL 3.4.18, installed FreeType/HarfBuzz and
the repository's native text/Core libraries. The optional standalone CMake file
currently accepts Linux only. No new dependencies or downloads are introduced.
SDL's installed version is checked exactly, matching the async-fetch example
convention. An explicit `CE_SDL_VERSION` override selects another exact installed
version and requires revalidation; it does not establish support for it.

From the repository root:

```sh
pkg-config --modversion sdl3
cmake -S examples/control_events -B build/control-events -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/control-events \
  --target cui_control_events test_control_events_app
python scripts/test_local.py --build-dir build/control-events -- \
  -R '^control_events_example_'
```

The smoke test uses the discovered `CUI_TEST_FONT`. Override it with
`-DCE_TEST_FONT=/absolute/path/to/NotoSans-Regular.ttf` when configuring. If no
font is found, the SDL smoke is not registered; inspect the test list rather
than claiming it ran. The model test needs no font or window.

To explicitly open the interactive example yourself:

```sh
build/control-events/cui_control_events \
  --font /absolute/path/to/NotoSans-Regular.ttf
```

Only this explicit interactive command opens a normal window. Automated native
checks use the repository wrapper's private Xvfb display. There is no desktop
fallback. Close the window normally to quit. No network or asynchronous job is
involved in this example.

## Callback ownership and timing

The actual setting handler in [app.c](app.c) is deliberately small:

```c
void ce_setting_request(void *user, const cui_component_check_request *request) {
    ce_app *a = user;
    ++a->requests;
    a->last_request = *request; /* Pointer-free payload: safe owned copy. */
    if (a->allow_toggle)
        a->setting = request->requested;
    else
        ++a->rejected;
}
```

The Checkbox call supplies a value snapshot rather than a mutable value pointer:

```c
result.setting = cui_checkbox_events(
    ui, CE_SETTING, theme, cui_string("Enable notifications"), a->setting,
    (cui_checkbox_options){0},
    (cui_component_check_handler){ce_setting_request, a});
```

The control retains its entry snapshot for that call's paint/semantics and all
requests. An accepted owner assignment appears in that control on the next
submission; the host invalidates after owner changes to submit it. A request
callback's return value cannot reject anything: its type returns void. Rejection
means the owner leaves its value unchanged. A missing handler is read-only.

Handlers and userdata are borrowed for the component call. This sample's app
object and Button states outlive every frame and are released only after the
runner returns through its quit callback. Request/activation metadata can be
copied by value. This sample copies it for inspection; it does not retain a
borrowed callback pointer. Frame/index identity is context-local, presence flags
must be honored, and a source field is not proof of arbitrary platform input.

Result `activation_count` and `request_count` describe that invocation. They are
not a queue, accepted-owner count or command to dispatch again. `valid` and
`submitted` do not mean an owner accepted a value or that later rendering cannot
fail. Legacy Checkbox change fields are zero, so there is no competing handler.
The Button likewise uses one side-effect callback path. Do not attach a second
legacy handler and then accidentally repeat the action from its result.

[main.c](main.c) applies `ce_finish_frame` only after rendering. Omission preserves
the app-owned setting for reopening; it is not context destruction or a promise
that active callbacks can free resources. Window shutdown remains the runner's
normal quit path. This is an example of safe application sequencing, not a new
runtime guard against callback reentrancy/destruction.

## Offline verification and scope

`control_events_example_model` tests the actual owner callbacks with rejected and
accepted requests, copied request independence, activation and deferred
close/reopen. It is not evidence of dispatch ordering by itself.

`control_events_example_sdl` uses the real scene, native font and SDL renderer.
It pushes SDL mouse motion/down/up events to exercise both checkboxes and Button,
then explicitly focuses the setting and pushes SDL Space down/up events. The
source Checkbox intentionally does not take pointer focus, so explicit focus is
part of this deterministic keyboard fixture. The test verifies pointer/keyboard
metadata with frame/index/activation evidence, requested-versus-owner values,
exact action counts, an intervening frame with zero counts/no duplicate effects,
and absence of omitted controls from the following semantic tree. It captures
`control-events.bmp` while the actual settings panel is visible and checks ink
pixels. Only successful completed rendering precedes omission.

These tests establish the listed Linux native paths. They do not establish all
component event families, bindings, capture/bubble routing, arbitrary teardown,
full accessibility, macOS/Windows live resize or GPU hardware acceptance. The
example source is MIT under the repository license; existing SDL/font-stack
licenses and pinned dependency notices remain applicable. It vendors nothing.
