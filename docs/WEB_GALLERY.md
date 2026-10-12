# Browser gallery

The static site uses the same portable CUI C implementation as native applications, compiled to a WebAssembly reactor. The browser supplies Canvas text measurement, Canvas painting of CUI draw commands, and pointer/keyboard events. HTML supplies documentation navigation, search, themes and code snippets. It is an optional host, not a dependency of native CUI applications.

Build with Python 3 and **Zig 0.16.0**:

```sh
python scripts/build_web.py
python -m http.server --directory build/web 8080
```

Open `http://localhost:8080/`. Every asset uses a relative URL, so the same artifact works beneath GitHub Pages' `/cui/` project path. The output contains `.nojekyll` and needs no server or build-time Node dependencies. GitHub Actions owns publication.

All 78 reference catalog entries appear in searchable navigation. Twenty-nine entries have live CUI browser stories: Button, Checkbox, Switch, Toggle, Radio, Slider, Input, Label, Badge, Progress, Theme, Alert, Avatar, Bubble, Empty, GroupBox, Kbd, Skeleton, Spinner, Accordion, Collapsible Tabs, Tooltip and HoverCard, plus explicitly labeled partial base-model stories for Editor, VirtualList, Dock, Plot and Calendar. Others show their actual repository implementation status and an explicit pending browser story. A browser story does not establish full reference parity. Skeleton and Spinner animate using browser frame time and respect prefers-reduced-motion. Tooltip and HoverCard demonstrate real delayed floating portals and Escape dismissal.

Use pointer input or focus the preview and use Tab, Enter/Space and arrow keys. Input routes UTF-8 text into the shared editor model. Disabled and light/dark/high-contrast options rebuild actual CUI draw lists. Canvas draws at the browser device pixel ratio; CUI receives logical CSS pixels. Browser fonts are measured and painted with the same system font settings.

Run the meaningful browser checks with pinned Playwright 1.58.0:

```sh
uv run --with playwright==1.58.0 playwright install chromium
uv run tests/web/test_browser.py
```

CI may use `playwright install --with-deps chromium` on its disposable runner. An installed Chromium can instead be selected with `--chromium /absolute/path/to/chrome`. Tests serve the artifact beneath `/cui/`, instantiate the real WASM, exercise pointer activation, keyboard activation, disabled behavior, checkbox state, slider stepping and input editing; exercise delayed tooltip open/Escape and hover-card close; inspect all live stories; verify all 78 entries, search, themes, pending entries and narrow layout. There is no mocked C implementation.

Limitations: Canvas text uses browser system fonts rather than native FreeType/HarfBuzz. IME composition, native clipboard routing, mobile soft keyboard, rich caret/selection painting, complete screen-reader focus synchronization and browser accessibility parity are pending. The host mirrors current interactive semantics into hidden ARIA elements, but that is experimental and does not establish a complete accessibility bridge. Images, shadows, device-loss handling and unrelated unimplemented draw capabilities are not simulated.

The minimal WASI host grants no filesystem or environment access. WebAssembly exports expose draw and semantic scalar accessors instead of relying on C structure offsets or leaking platform types into the public CUI API. The shader/native GPU adapters are not linked into this Canvas host.
