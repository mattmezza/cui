# Browser gallery

The static site uses the same portable CUI C implementation as native applications, compiled to a WebAssembly reactor. The browser supplies Canvas text measurement, Canvas painting of CUI draw commands, and pointer/keyboard events. HTML supplies documentation navigation, search, themes and code snippets. It is an optional host, not a dependency of native CUI applications.

Build with Python 3 and **Zig 0.16.0**:

```sh
python scripts/build_web.py
python -m http.server --directory build/web 8080
```

Open `http://localhost:8080/`. Every asset uses a relative URL, so the same artifact works beneath GitHub Pages' `/cui/` project path. The output contains `.nojekyll` and needs no server or build-time Node dependencies. GitHub Actions owns publication.

All 78 reference catalog entries appear in searchable navigation, with per-feature evidence statuses and links to the pinned comparison reference. Fifty-eight entries have live CUI browser stories: Button, Checkbox, Switch, Toggle, Radio, Slider, Input, Label, Badge, Progress, Theme, Alert, Avatar, Bubble, Empty, GroupBox, Kbd, Skeleton, Spinner, Accordion, Collapsible, Tabs, List, Select, Menu, Combobox, NumberInput, Pagination, Rating, Stepper, Clipboard, Calendar, DatePicker, TimeField, ColorPicker, Tree, Scrollable, Table, DataTable, Tooltip, HoverCard, Dialog, AlertDialog, Sheet, Popover, DropdownButton, Form, Input Group, OtpInput, Tag, Marker, Notification and Shimmer, plus explicitly labeled partial base-model stories for Editor, VirtualList, Dock, Plot and Focus Trap. Others show their actual repository implementation status and an explicit pending browser story. A browser story does not establish full reference parity. Skeleton and Spinner animate using browser frame time and respect prefers-reduced-motion. Tooltip and HoverCard demonstrate real delayed floating portals and Escape dismissal.

Use pointer input or focus the preview and use Tab, Enter/Space and arrow keys. Input routes UTF-8 text into the shared editor model. Disabled, comfortable/compact density, single/multiple selection where supported and light/dark/high-contrast options rebuild actual CUI draw lists. Canvas draws at the browser device pixel ratio; CUI receives logical CSS pixels. Browser fonts are measured and painted with the same system font settings.

Run the meaningful browser checks with pinned Playwright 1.58.0:

```sh
uv run --with playwright==1.58.0 playwright install chromium
uv run tests/web/test_browser.py
```

CI may use `playwright install --with-deps chromium` on its disposable runner. An installed Chromium can instead be selected with `--chromium /absolute/path/to/chrome`. Tests serve the artifact beneath `/cui/`, instantiate the real WASM, exercise pointer activation, keyboard activation, disabled behavior, checkbox state, slider stepping and input editing; exercise delayed tooltip open/Escape, hover-card close, popup selection/menu activation, modal Tab trapping, Escape trigger-focus restoration and tab arrow navigation; inspect all live stories; verify all 78 entries, search, themes, pending entries and narrow layout. There is no mocked C implementation.

Limitations: Canvas text uses browser system fonts rather than native FreeType/HarfBuzz. IME composition, clipboard support beyond the optional browser Clipboard API, mobile soft keyboard, complete screen-reader focus synchronization and browser accessibility parity are pending. The host mirrors current interactive semantics into hidden ARIA elements, but that is experimental and does not establish a complete accessibility bridge. Images, shadows, device-loss handling and unrelated unimplemented draw capabilities are not simulated.

The Clipboard story bridges actual CUI buttons to navigator.clipboard and reports denied/unavailable access; secure context and browser permission rules apply. Focus Trap demonstrates a core modal scope with Tab cycling and Escape/close trigger restoration.

The minimal WASI host grants no filesystem or environment access. WebAssembly exports expose draw and semantic scalar accessors instead of relying on C structure offsets or leaking platform types into the public CUI API. The shader/native GPU adapters are not linked into this Canvas host.

Input includes editable, read-only and required/invalid state controls. It demonstrates the styled single-line editor with pointer caret placement, drag selection, horizontal scrolling and keyboard undo/redo. Tree demonstrates retained expansion, selection, keyboard traversal and typeahead over borrowed preorder nodes.

Scrollable renders actual clipped translated content and interactive horizontal/vertical scrollbars. DataTable renders a virtual window over 1,000 keyed rows, with selection, sort requests, column resize and reorder, bounded multi-selection/ranges, a pinned name-column option and two-axis wheel scrolling; sort requests are displayed without pretending that caller data was reordered. Table demonstrates stateless header/body sections and read-only header cells. DataTable exposes cell, row and column selection controls; cell Home/End stays within a row, while row Home/End reaches the first or last virtual row. Its composed mode uses real grouped headers, row headers, bounded row hooks, trailing Inspect buttons and scrollbars. Loading and empty modes invoke caller-provided placeholders over the actual model.

The Menu story provides dropdown and context triggers and uses real nested menu descriptors: disabled items, separators, labels, keyboard submenus, checkbox and radio items and shortcut hints. ContextMenu opens only from a secondary click within its receiving CUI area.

Dialog, AlertDialog, Sheet and Popover are composed by the portable C surface API in the WASI module and painted through the same CUI draw list as every other live story. Dialog demonstrates arbitrary child controls, nested focus restoration and an oversized clipped body that scrolls from wheel input above a fixed Save footer. The host schedules geometry correction frames from core invalidation and returns to idle once the layout settles. AlertDialog deliberately rejects its first destructive confirmation through a decision callback. Sheet exposes left, right, top and bottom geometry with captured pointer resizing and arrow/Home/End keyboard controls bounded by minimum and maximum dimensions. Popover is a nonmodal composed account card with interactive content, collision placement and outside/Escape dismissal. Browser ARIA projection includes dialog, alertdialog, rowheader and rowgroup roles; native adapters consume the same semantic tree and render text through their configured glyph pipeline.

DropdownButton demonstrates independently focusable primary export and options-menu halves, with the portable menu tree supplying selection, disabled options, checked items and nested menus. Dialog also demonstrates nested modal focus trapping and Escape restoration to its parent trigger.

Canvas consumes independent corner radii and edge widths from the public draw commands. Theme includes an asymmetric shape sample; Chromium pixel assertions verify square and rounded corners, unequal border widths and explicit absent edges at device scale two.

Form composes application-owned name/email editors with required labels, descriptions, validation errors and a Save footer. Its responsive grid switches between two columns and one; valid, email-error, read-only and disabled variants retain genuine input behavior and semantic associations. The sample validates a nonempty name and an email containing @ and a dot; submission and validation belong to the application.

The Input Group story uses the retained editor with inline and block addons, an actual action button, and read-only, disabled and loading variants. The application owns the address and action count.

OtpInput normalizes ASCII and fullwidth digits through the actual C reducer, supports groups and masking, reports completion counts, and schedules caret wakes at the component-provided deadline while leaving other frames idle.

Tag demonstrates actual intent and named palette colors, outline/pill styling, passive labels and rich children. Marker demonstrates plain/separator/border layouts, alignment, typed text/icon and rich content, spinner suppression by typed icons, shimmer and reduced motion.

Notification demonstrates actual in-app retained lifecycle timers, all eight anchors, collapsed/expanded stacks, arbitrary focusable content, actions and dismissal. Browser deadlines drive frames; OS notification delivery is not simulated.

Accordion and Collapsible include optional retained spring reveals, interactive panels, custom header/box composition and multiple accordion panels. Closing subtrees continue painting while their semantics and input are inert; reduced motion snaps panels and leaves settled frames idle.

Theme also demonstrates passive and explicitly focusable Log semantics. The host projects the core focusable flag into Canvas Tab routing while leaving passive logs and list items outside the tab sequence.

Shimmer renders standalone animated text with repeat, reverse, once and absolute spread controls, clipped width and reduced motion.
