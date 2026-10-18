# Async URL fetch: optional Linux host example

This example uses an existing synchronous CUI Button `on_click` handler to queue
an HTTPS fetch and return immediately. It shows loading, disables duplicate
fetches, supports cancellation/retry and applies completions on the UI thread.
It adds no event API, scheduler, networking or thread pool to Core. The worker
and transport interfaces in this directory are private example code.

## Build and run

Supported and tested host: Linux x86-64, POSIX pthreads, glibc 2.44, SDL 3.4.18,
libcurl 8.22.0, C11, GCC 16.2.1. macOS/Windows/WASM hosts are not supported by
this example's CMake file. Native CUI still has its own broader host support;
this example makes no new claim about those hosts or GPU hardware.

The standalone build uses the repository's CUI libraries and installed
FreeType/HarfBuzz/SDL packages. It downloads nothing and sets `CUI_FETCH_SDL=OFF`.
SDL and curl pkg-config versions are checked exactly, with these tested defaults.
Supply already installed dependencies matching them; explicit
`AF_SDL_VERSION` / `AF_CURL_VERSION` overrides select another exact version and
require revalidation. They do not mean untested versions are verified.

From the repository root:

```sh
pkg-config --modversion sdl3 libcurl
cmake -S examples/async_fetch -B build/async-fetch -G Ninja \
  -DCMAKE_BUILD_TYPE=Debug
cmake --build build/async-fetch \
  --target cui_async_fetch test_async_fetch test_async_fetch_curl
python scripts/test_local.py --build-dir build/async-fetch -- -R '^async_fetch_'
```

`AF_TEST_FONT` is a CMake file-path override for the offline SDL tests; by default
it uses the repository's discovered `CUI_TEST_FONT`. If absent, SDL tests are not
registered: inspect the configure output/CTest list instead of claiming they ran.
Use a known Latin font such as Noto Sans. Normal use opens a window only when
explicitly launched:

```sh
build/async-fetch/cui_async_fetch \
  --font /path/to/NotoSans-Regular.ttf --url https://example.com/
```

Choose **Fetch URL** to perform the network request. No request starts at launch.
The URL is a command-line value, not a new Input callback API. The example
accepts printable ASCII HTTPS URLs up to 2048 bytes (percent-encode other bytes).
A success displays received byte count/HTTP status; binary response data is not
passed to CUI as unvalidated UTF-8. Failure, including a response exceeding the body limit, displays an error and
labels the action **Retry fetch**. Retrying an oversized response may fail again
unless the server returns a smaller body.
The retry button uses the same existing synchronous handler as initial fetch.
Cancel invalidates the request generation and enables a subsequent attempt.
If the single pending job slot is still occupied, the UI reports busy and the
caller can retry; it never waits for transport in the handler.

To build/test without libcurl, while retaining the actual SDL offline example:

```sh
cmake -S examples/async_fetch -B build/async-fetch-offline -G Ninja \
  -DAF_WITH_CURL=OFF
cmake --build build/async-fetch-offline --target cui_async_fetch test_async_fetch
python scripts/test_local.py --build-dir build/async-fetch-offline -- -R '^async_fetch_'
```

For only the worker/model tests, with neither SDL nor Core/network dependencies,
set both `AF_BUILD_UI=OFF` and `AF_WITH_CURL=OFF` and build `test_async_fetch`.
Interactive live fetch is unavailable without curl; the host reports this and
requires `--offline-smoke`. All automated tests use injected transport or
pre-transfer cancellation, never an external network.

## Ownership and dispatch contract

| Boundary | Owner / lifetime |
| --- | --- |
| Button callback | Synchronous UI thread; validates model and copies URL into the worker's one pending slot. No fetch, wait, join or transport call here. |
| Worker transport | One pthread, one active request. Receives its own URL buffer and fixed result storage. Transport userdata remains alive through worker join. |
| Completion mailbox | Two fixed slots protected by a mutex/condition variable; body at most 8192 bytes, error at most 159 bytes plus NUL. A full mailbox blocks the worker publisher, not the UI callback. Shutdown wakes this wait. |
| Wakeup | Worker calls `SDL_PushEvent` with a registered user-event type and no pointer payload. This is a notification, never a CUI call or render callback. |
| UI model | Created and applied on its owning thread. `af_model_start/cancel/apply/drain` reject use from another thread. Only UI code applies completions or calls CUI. |
| Shutdown | Signal stop, wake worker waits, request cooperative abort, join, then release curl globals, CUI, SDL host and font. No callback can use app/host storage after join. |

`fetch_clicked` in [main.c](main.c) is the actual callback wired to
`cui_button_options.on_click`; it calls `af_model_start` and invalidates the UI.
The returned frame result is not used for a second side-effect path. The
callback can affect loading text later in that submission, while already-painted
frames remain unchanged. Existing button semantics and callback timing are not
replaced or moved to a new queue.

The worker never owns a pointer to the UI model. It owns request/result bytes;
the UI takes a completion copy before applying it. A copied result can be held
and applied later, but matching generation plus current loading state is
required. Stale, duplicate and deliberately reordered completions cannot
replace the current result. Each accepted start gets a new generation; cancel
also advances it. Exhaustion rejects starts before uint64 wrap. Workers execute
requests serially: the out-of-order application test deliberately reorders
copied completions, not two claimed concurrent network operations.

For a removed view, invalidate its generation before omitting it and stop
applying that model. Do not free transport/wakeup userdata while its worker can
still run: join first, or keep that data in an app-owned object that outlives the
view. The sample closes its entire host using the join-first path. Worker
functions are synchronized, but callers must stop using a worker before
`af_worker_destroy`; a freed worker pointer cannot be reused or destroyed twice.
`af_worker_destroy(NULL)` is safe. No arbitrary-thread CUI/destruction support is
implied.

## Cancellation, timeouts and waking an idle UI

Logical cancellation is immediate UI state/generation invalidation. It **does
not** promise the remote server stopped processing a request or that a blocked
transport was physically interrupted. The curl progress callback checks atomic
stop/generation flags and returns an abort request when curl invokes it. This is
cooperative cancellation, not forced thread cancellation. A replacement request
can occupy the single pending slot until the old transport returns.

The curl adapter permits HTTPS only, verifies TLS peer/host, follows at most
three HTTPS redirects, uses a 2000 ms connection timeout and an 8000 ms total
transfer timeout, and stops at the 8192-byte body limit. HTTP non-2xx outcomes
are failures. It sets `CURLOPT_NOSIGNAL=1` for its pthread and refuses curl builds
without TLS and asynchronous DNS: synchronous DNS with NOSIGNAL cannot honor
timeouts during name resolution. These are configured transport limits, not a
hard real-time shutdown deadline: resolver/backend cleanup and OS scheduling
can add latency. Curl cancellation need not run at the instant a DNS/connect
operation is blocked. No real network latency, TLS handshake or remote HTTP
behavior is claimed tested by the offline suite.

The callback-main host can enter its normal `waitevent` idle mode. A published
completion pushes an SDL user event; the next UI frame drains the mailbox and
renders the result. No event watch/filter renders, applies state or calls CUI.
SDL documents `SDL_PushEvent` as thread-safe, but it can fail if filtered or its
queue is full. This host installs no filter and leaves the registered event
enabled. On a failed push, the worker retries twice after condition-variable
waits of 10 ms and 20 ms (three total attempts). Shutdown interrupts these waits;
this adds no UI polling, heartbeat or extra thread. Failed attempts are counted
by `af_worker_wake_failures`. A transient failure can therefore recover without
user input; the offline SDL test injects exactly this case while Core is idle.
The delays are configured waits, not a wall-clock scheduling guarantee.

If all three attempts fail, the completion remains queued for the next UI event
and drain. **There is no unconditional idle-wake guarantee after persistent SDL
rejection.** The UI cannot inspect failure counters while asleep, and this sample
has no independent fallback timer. A host that adds filters must preserve the
completion event. Queue-full traffic provides other events to wake the loop,
but a permanently rejecting filter or failed SDL event subsystem cannot be
bypassed by retries. Replacing the wake implementation with one that silently
drops events violates the host's notification contract.

Authoritative behavior references:
[SDL_PushEvent](https://wiki.libsdl.org/SDL3/SDL_PushEvent),
[CURLOPT_NOSIGNAL](https://curl.se/libcurl/c/CURLOPT_NOSIGNAL.html),
[CURLOPT_TIMEOUT_MS](https://curl.se/libcurl/c/CURLOPT_TIMEOUT_MS.html), and the
installed 8.22.0 `CURLOPT_XFERINFOFUNCTION(3)` manual. Curl/SDL calls are confined
to their documented roles; no new Core dependency is introduced.

## Offline evidence and limits

- `async_fetch_worker`: blocked injected transport proves enqueue returns before
  completion; caller URL mutation leaves worker bytes intact; loading suppresses
  duplicates; injected failure/retry gets fresh generations; UI state stays
  unchanged until drain; wrong-thread apply/start reject. Tests logical cancel
  with a transport that ignores it, cooperative cancel, stale/duplicate/reversed
  completion application, pending jobs, full mailbox shutdown, bounded results,
  wake failures and joining an active transport before freeing userdata.
- `async_fetch_curl_offline`: exercises the real write callback with binary data,
  full body/overflow rejection, progress cancellation and cancellation before
  `curl_easy_perform`. It performs no DNS lookup or network I/O. Curl builds
  without required features return skip 77; live host initialization also refuses
  them. Compilation is not a real HTTPS integration claim.
- `async_fetch_sdl`: actual CUI button pointer activation starts the injected
  worker; a loading frame reaches Core idle before completion is released;
  the real SDL user event wakes the callback runner and the UI applies/renders
  completion. Saves `async-fetch-smoke.bmp` and checks visible text pixels.
- `async_fetch_sdl_wake_retry`: suppresses the first notification attempt, then
  verifies retry wakes the idle actual SDL runner, applies the retained result
  on the UI thread and reports exactly one failed/two total wake attempts.
- `async_fetch_sdl_pending_shutdown`: closes the actual host while its injected
  transport is blocked, then verifies cancellation/worker return before host
  teardown. All native tests use private Xvfb through the repository wrapper.

These tests are Linux software/SDL evidence. They do not cover macOS modal
loops, Windows, GPU hardware, actual network servers, every SDL filter failure,
or arbitrary third-party transports that ignore both cancellation and finite
timeouts. Such a transport can delay join indefinitely; it violates the injected
transport contract and is not made safe by this example.

## Dependency and license notes

No third-party source/binaries are vendored here. Example code uses the
repository MIT license. SDL 3.4.18 uses the zlib license (installed
`/usr/share/licenses/sdl3/LICENSE`); libcurl 8.22.0 uses the curl license
(`/usr/share/licenses/curl/COPYING`). The tested libc/pthreads implementation is
glibc 2.44 (LGPL-2.1-or-later); existing CUI/font stack licenses still apply.
Redistributing linked applications requires retaining applicable dependency
notices and satisfying their licenses; exact static vs dynamic dependencies
come from your selected build. This directory does not change repository
license metadata or package the optional executable into SDKs.
