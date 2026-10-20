# Avatar activation owner

`main.c` is a complete portable model example using `cui_avatar_events`.
Its profile-picker owner opens a profile once from the typed callback, copies
pointer-free evidence and requests deferred dismissal. The host applies that
request after frame completion; the next submission omits the picker. Returned
counts are inspected without repeating the action. No callback frees its own
userdata or destroys the context.

This executable has no native window or renderer. It supplies a synthetic Core
accessibility action after semantic registration, rather than claiming a real
screen-reader or browser test. To use the same owner with a native host, submit
`draw_picker` inside the host's normal frame, consume the returned draw list with
its renderer, then apply `dismiss_pending`. The existing native starter documents
host/font setup; no new host abstraction or dependency is introduced here.

Build the current library, then compile against that library (distribution and
SDK regeneration is owned by the integrator):

```sh
cmake -S . -B /tmp/cui-avatar-example -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build /tmp/cui-avatar-example --target cui
cc -std=c11 -Wall -Wextra -Wpedantic -Werror -I include \
  examples/avatar_events/main.c /tmp/cui-avatar-example/libcui.a -lm \
  -o /tmp/cui-avatar-example/avatar_owner
/tmp/cui-avatar-example/avatar_owner
```

The executable is portable-only and opens no desktop windows. Registered native
checks must still use `python scripts/test_local.py --build-dir <private-build>`.
This example uses initials; photo/icon resources in a real application must
remain alive until the renderer consumes their borrowed draw data.
