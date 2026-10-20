# Application-owned loading semantics and Alert

This compiling C11 example uses existing CUI APIs. It opens no window and emits
Core snapshots as JSON. It demonstrates one meaningful Records region, one
polite status while loading/completing, three decorative Skeletons, and a failed
operation with a rich Alert, Retry button and controlled Dismiss button. The
application owns the operation state; the Skeleton animation is not the operation.

`scene.c` is the composition to adapt. The region's `cui_semantic_details.busy`
reflects the operation state. Skeletons add no focus stops or semantic nodes.
Alert's explicit accessible name summarizes its custom body; arbitrary painted
children are not automatically concatenated into that name. Retry is an ordinary
button; the application must start its real operation when `retries` changes.
Dismissing the failure Alert does not claim the operation succeeded. Stable IDs
preserve identity across UI frames. Do not change names every animation tick to
force announcements.

A visual host must submit the draw list, retain renderer resources as usual,
schedule the earliest successful motion result's `next_wake_seconds`, and poll
real application completion on the owner thread. This no-window snapshot driver
supplies deterministic times instead. It does not run a service or render a
complete application. `cui_skeleton_ex` is paint-only; use the existing application
region/status pattern when loading has meaning to users.

## Build and focused validation

No dependency is installed or downloaded. Use a dedicated build directory; root
CMake files are unchanged. The external harness reuses canonical sources but
builds only explicitly requested targets.

```sh
cmake -S examples/loading_region -B "$HOME/.cache/cui-validation/loading-region" -G Ninja
cmake --build "$HOME/.cache/cui-validation/loading-region" --target loading_region test_alert_loading_semantics
python scripts/test_local.py --build-dir "$HOME/.cache/cui-validation/loading-region" -R '^alert_loading_semantics$'
"$HOME/.cache/cui-validation/loading-region/loading_region"
```

On Linux, the optional native fixture uses only the existing checksum-verified
AccessKit cache and installed AT-SPI packages. It creates no SDL window. Run it
through the wrapper and existing private DBus/Xvfb runner; never use the desktop
accessibility bus. It does not launch a screen reader.

```sh
cmake -S examples/loading_region -B "$HOME/.cache/cui-validation/loading-region" -G Ninja -DLOADING_REGION_ACCESSKIT=ON
cmake --build "$HOME/.cache/cui-validation/loading-region" --target test_loading_native
python scripts/test_local.py --build-dir "$HOME/.cache/cui-validation/loading-region" -R '^alert_loading_(semantics|native)$' -V
```

The browser fixture needs the already installed Playwright 1.58.0 and Chromium
1208 used by repository tests. No package/browser installation occurs. It feeds
actual compiled Core snapshots into the production gallery's unchanged
`syncSemantics` function, executing the DOM adapter in headless Chromium. This is
a transport fixture, not a WASM execution or live-AT test.

```sh
python tests/web/test_alert_loading_dom.py --example "$HOME/.cache/cui-validation/loading-region/loading_region"
```

## Evidence and boundaries

Verified locally with strict C11 warnings, `-Werror`, assertions enabled and
clang-format 23.1.2: Core fixture and real native fixture passed 2/2 through the
private display wrapper, including a separate ASan/UBSan build; headless
Chromium DOM fixture passed. The Core fixture
checks decorative omission, busy/name transitions, stable parent relationships,
nonfocusable Alert/status roots, independent action descendants, hidden subtree
omission and reinsertion. The DOM fixture checks the same actual snapshot
transitions, stable element identity and removal/recreation.

The native fixture captures real `object:announcement` events on a private bus:
Alert insertion and changed name use assertive politeness 2; hiding emits no
repeat; reinsertion announces again; completion status uses polite politeness 1.
Repeated unchanged owner polls produce no duplicate tested messages. It queries
busy true/false and the actual platform role. The pinned adapter maps AccessKit
Alert to AT-SPI **Notification**, while AlertDialog maps to AT-SPI Alert. This is
upstream adapter behavior, not a component role change. The installed AT-SPI
client reports cache-signature warnings; direct role/state queries and event
payload assertions pass despite those warnings.

Source evidence: the checksum-verified AccessKit C 0.23.0 lockfile selects
`accesskit_atspi_common` 0.20.0 and `accesskit_unix` 0.23.0. The former's
`adapter.rs:add_node` emits named live-node announcements, and
`node.rs:notify_property_changes` emits them on name changes; `node.rs:role`
provides the Notification mapping. Unix `atspi/bus.rs` serializes the event with
message and politeness. The fixture uses those actual linked implementations;
it does not inject fake announcement events or require a new Core queue.

No audible delivery, interruption/coalescing policy or screen-reader preference
behavior is established. Browser role/name/busy DOM exposure does not prove that
initial insertion or an aria-label mutation will be spoken. The optional native
adapter remains Linux-only; macOS and Windows announcement delivery is deferred.
These results justify only the tested transport behavior, not full accessible
announcement parity or a new loading-semantics feature on Skeleton.
