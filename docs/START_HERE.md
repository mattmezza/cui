# Start building with CUI

CUI composes a user interface in portable C11. Your application supplies state and calls components during a frame; CUI produces a draw list and a semantic tree. A renderer paints the list, and a host supplies window events, time, clipboard and other platform services.

CUI is experimental. The gallery contains all 78 comparison entries, but the [feature manifest](gpui-parity.json) records incomplete behavior. Begin with the controls you need and check their documented limits.

## Start with a native application

Follow [Build your first native CUI app](NATIVE_APP_STARTER.md) for a complete
three-page desktop starter, sidebar navigation and a bottom account menu. It
explains dependencies, compiler choice, CMake, Make, static linker flags and
native launch commands. The source is small enough to copy into your own app.

## Try a headless C program

From the repository root:

```sh
cmake -S . -B build -G Ninja -DCUI_BUILD_NATIVE=OFF
cmake --build build
python scripts/test_local.py --build-dir build
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

Use [Python, JavaScript, TypeScript, Go or Rust component builders](BINDING_BUILDERS.md)
to author UI trees and callbacks in your own language. The guide includes native
window build commands and examples; the [coverage table](BRIDGE_COMPONENTS.md)
records supported controls and the limits of each binding. The full component
catalog is still being bound and validated. The [C-host examples](LANGUAGE_BINDINGS.md)
also demonstrate embedding an existing C-authored application.

For composition, start with [Layout](LAYOUT_GUIDE.md), then [IDs and lifetimes](IDS_AND_LIFETIMES.md) and [Debugging](DEBUGGING.md). The [two-file native bundle](NATIVE_APP_STARTER.md) is the standalone consumer path; [agent guidance](AI_AGENTS.md) separates consumer and maintainer workflows.
