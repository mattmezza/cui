# Start building with CUI

CUI composes a user interface in portable C11. Your application supplies state and calls components during a frame; CUI produces a draw list and a semantic tree. A renderer paints the list, and a host supplies window events, time, clipboard and other platform services.

CUI is experimental. The gallery contains all 78 comparison entries, but the [feature manifest](gpui-parity.json) records incomplete behavior. Begin with the controls you need and check their documented limits.

## Run a small C program first

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build
ctest --test-dir build --output-on-failure
./build/hello_world
```

This portable example creates real CUI draw commands and prints their count. It exercises layout without opening a window. A C11 compiler, CMake, Ninja and Python3 are needed for these commands; core CUI itself uses the standard library and vendored Clay.

## Choose your host

| Path | What it does | Next reference |
| --- | --- | --- |
| Existing native application | Send events to a context and paint its draw list in your renderer | [Embedding contracts](INTERFACES.md) |
| SDL3 workbench | Opens the native gallery when SDL3, FreeType and HarfBuzz are available | [Repository setup](../README.md) |
| Browser | Compiles C to WebAssembly; JavaScript supplies browser input and canvas rendering | [Web gallery build](WEB_GALLERY.md) |
| Portable/headless | Runs layout, interaction and renderer tests without a window | [Core lifecycle](CORE.md) |

The browser gallery is an example host. Applications using CUI do not need a browser engine. Native adapter availability and actual runtime evidence are listed in the [platform matrix](PLATFORM_MATRIX.md).

## Compose a frame

The following function belongs in a host that already created `ui` and a theme. The host owns `clicks`, calls this function for each frame, then passes the returned draw list to its renderer before the next frame starts.

```c
#include "cui/cui_components.h"

cui_draw_list draw_counter(cui_context *ui, const cui_theme *theme,
                           unsigned *clicks, float width, float height,
                           float elapsed_seconds) {
    if (!cui_begin_frame(ui, width, height, elapsed_seconds))
        return (cui_draw_list){0};
    cui_begin_box(ui, cui_id_from(ui, "counter"),
        (cui_box){.width = -1, .height = -1,
                  .padding = 24, .gap = 12,
                  .background = theme->background});
    if (cui_button(ui, cui_id_from(ui, "increment"), theme,
                   cui_string("Increment"), false)) {
        ++*clicks;
        cui_invalidate(ui);
    }
    cui_end_box(ui);
    return cui_end_frame(ui);
}
```

Stable IDs connect this frame's controls to the previous frame's hit regions. Keep domain state outside this function. Frame-owned draw data remains borrowed until the next frame; render it before advancing the context.

## Learn the interaction model

Read [state and callbacks](IMMEDIATE_AND_STATE.md) for immediate frame composition, retained values/models, return-value events and controlled callbacks. Component headers and the gallery Usage section show the concrete types for each control. Each code block in website documentation has a Copy button.

## Work with an AI coding agent

Use the [copyable agent setup prompt](AI_AGENTS.md). The website also publishes `llms.txt` as a documentation index and `llms-full.txt` as a plain-text bundle. Both are generated from these repository documents, alongside the component-status manifest. The requested `llm.txt` and `llm-full.txt` names are aliases.

## Build with another language

Try the runnable [Go, Rust, JavaScript and TypeScript examples](LANGUAGE_BINDINGS.md).
They use a small C host ABI and real CUI controls with synchronous callbacks.
The guide includes headless checks, native-window commands and the observed
Linux support limits; these examples are a starting point for bindings.
