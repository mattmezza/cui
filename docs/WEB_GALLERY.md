# Browser gallery

The static site uses the same portable CUI C implementation as native applications, compiled to a WebAssembly reactor. The browser supplies Canvas text measurement, Canvas painting of CUI draw commands, and pointer/keyboard events. HTML supplies documentation navigation, search, themes and code snippets. It is an optional host, not a dependency of native CUI applications.

Each live catalog entry has a public C usage excerpt. These are frame-level fragments: create a context and theme, retain any state and IDs, and provide named callbacks before invoking them. `tests/web/test_snippets.py` checks all 78 entries against public function names, argument counts and constants, then compiles every displayed excerpt with typed setup scaffolds under C11 warnings as errors. The Chromium suite runs that check before exercising the copy button.

Build with Python 3 and **Zig 0.16.0**:

```sh
python scripts/build_web.py
python -m http.server --directory build/web 8080
```

Open `http://localhost:8080/`. Every asset uses a relative URL, so the same artifact works beneath GitHub Pages' `/cui/` project path. The output contains `.nojekyll` and needs no server or build-time Node dependencies. GitHub Actions owns publication.

All 78 reference catalog entries appear in searchable navigation, with per-feature evidence statuses and links to the pinned comparison reference. All 78 entries have live CUI browser stories: Button, Checkbox, Switch, Toggle, Radio, Slider, Input, Label, Badge, Progress, Theme, Alert, Avatar, Bubble, Empty, GroupBox, Kbd, Skeleton, Spinner, Accordion, Collapsible, Tabs, List, Select, Menu, Combobox, NumberInput, Pagination, Rating, Stepper, Clipboard, Calendar, DatePicker, TimeField, ColorPicker, Tree, Scrollable, Table, DataTable, Tooltip, HoverCard, Dialog, AlertDialog, Sheet, Popover, DropdownButton, Form, Input Group, OtpInput, Tag, Marker, Notification, Shimmer, Message, MessageScroller, Image, Attachment, DescriptionList, StatusBar, Resizable, Icon, Toolbar, TitleBar, Sidebar, Textarea, Settings, Carousel, Command, Root View, TextView, Chart, Speech and Questionnaire, plus explicitly labeled partial base-model stories for Editor, VirtualList, Dock, Plot and Focus Trap. The navigation and feature table retain actual implementation status, including partial APIs. A browser story does not establish full reference parity. Skeleton and Spinner animate using browser frame time and respect prefers-reduced-motion. Tooltip and HoverCard demonstrate real delayed floating portals and Escape dismissal.

Use pointer input or focus the preview and use Tab, Enter/Space and arrow keys. Input routes UTF-8 text into the shared editor model. Disabled, comfortable/compact density, single/multiple selection where supported and theme options rebuild actual CUI draw lists. The site follows the system light/dark preference by default, including later changes; a named light or dark palette or the separate High contrast choice overrides it until Follow System is selected again. Canvas draws at the browser device pixel ratio; CUI receives logical CSS pixels. Browser fonts are measured and painted with the same system font settings. RGBA8 bitmap commands support source crops, opacity, grayscale, tint, rounded corners and nested clips.

Checkbox and Radio demonstrate four sizes, required and invalid semantics, skipped Tab stops, disabled behavior, and graphical selection marks. Switch demonstrates label placement, a custom active color, spring travel and reduced-motion settling. Toggle includes standalone selection and a three-item toolbar with single or multiple selection, segmented edges, and ghost or outline styling.
The Canvas host applies the CUI core's resolved pointer cursor after each frame and mirrors toolbar and separator roles with orientation and range metadata when present.

The Canvas mesh consumer decodes actual CUI expanded triangles, including analytic
ST coverage and linear premultiplied colors. It rasterizes physical pixel centers
at the gallery's positive uniform DPR scale plus translation, preserves ordered
translucent triangles and nested pixel-center clips, and consumes Core ancestor
opacity once. Other Canvas matrices and composition modes are rejected. Bounded
preflight completes before resizing or clearing the target; invalid geometry or
resource exhaustion leaves the existing image intact. The host limits and memory
accounting are specified in [the adapter design](design/CANVAS_MESH.md).
`tests/web/test_mesh_canvas.py` checks exact Canvas reference images at DPR1/2 and
a Core-emitted opacity fixture. This establishes this browser host's mesh path;
it does not establish exact GPUI path rasterization or support in native adapters.

An earlier light/dark Chromium audit of the first 62 live stories identified ASCII stand-ins in C components. The subsequent source icon artwork implementation replaced those glyphs in Button, Disclosure, Tree, DropdownButton, Input Group and Notification. The gallery’s custom disclosure header still uses `+`/`-` in `web/gallery.c`; it is sample-owned content rather than component painting. The Message avatar is intentionally bottom-aligned: `src/components/cui_message.c` follows the frozen reference's `message.rs` bottom alignment. Input Group's separate block addon rows follow `src/components/cui_form.c`; the gallery gives its inline action enough width to keep “Open address” on one line.
An idle-frame audit found continuous frames only for Skeleton, Spinner, Shimmer, and Attachment's brief entrance animation; Image schedules a load deadline. With reduced motion, all 62 stories in that audit stopped scheduling continuous frames.

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

Canvas also reads each text command's weight, italic and monospace flags. The browser's styled measurement callback and paint use the same CSS font specification and a style-keyed baseline cache. Theme includes semibold, italic and monospace samples; Chromium checks both their actual Canvas font selection and measured layout widths.

Form composes application-owned name/email editors with required labels, descriptions, validation errors and a Save footer. Its responsive grid switches between two columns and one; valid, email-error, read-only and disabled variants retain genuine input behavior and semantic associations. The sample validates a nonempty name and an email containing @ and a dot; submission and validation belong to the application.

The Input Group story uses the retained editor with inline and block addons, an actual action button, and read-only, disabled and loading variants. The application owns the address and action count.

OtpInput normalizes ASCII and fullwidth digits through the actual C reducer, supports groups and masking, reports completion counts, and schedules caret wakes at the component-provided deadline while leaving other frames idle.

Tag demonstrates actual intent and named palette colors, outline/pill styling, passive labels and rich children. Marker demonstrates plain/separator/border layouts, alignment, typed text/icon and rich content, spinner suppression by typed icons, shimmer and reduced motion.

Notification demonstrates actual in-app retained lifecycle timers, all eight anchors, collapsed/expanded stacks, arbitrary focusable content, actions and dismissal. Browser deadlines drive frames; OS notification delivery is not simulated.

Accordion and Collapsible include optional retained spring reveals, interactive panels, custom header/box composition and multiple accordion panels. Closing subtrees continue painting while their semantics and input are inert; reduced motion snaps panels and leaves settled frames idle.

Theme also demonstrates passive and explicitly focusable Log semantics. The host projects the core focusable flag into Canvas Tab routing while leaving passive logs and list items outside the tab sequence.

Shimmer renders standalone animated text with repeat, reverse, once and absolute spread controls, clipped width and reduced motion.

Tabs exposes all five variants and four source sizes, automatic/manual activation, bounded horizontal/vertical strips, selected-tab reveal, an all-tabs menu and keyboard/pointer scrollbars. Structured slots and caller-applied drag reordering still need dedicated browser coverage.

DropdownButton exposes all twelve variants and four sizes, custom palettes, outline/selected paint, independently disabled/loading actions, split/menu-only/action-only composition and four anchors. Browser checks cover independent action/menu routing and captured pressed colors. Tooltip composition and inherited action properties still need dedicated browser coverage.

Message composes real avatars, metadata, multiple bodies, seven Bubble styles and interactive content with alignment/inset controls. MessageScroller retains a variable-height application collection with tail following, append/prepend/removal, a focusable Log, optional scrollbar and animated jump/fade behavior.

Image uses application-owned RGBA8 frames with a cached browser raster keyed by pointer, dimensions, stride, revision, grayscale and tint. Its story exercises all five fit modes, crop, delayed loading, fallback, opacity, grayscale, animation and reduced motion. Attachment composes six application-owned cards in a horizontal scroll group with lifecycle states, image media, actions, remove/retry callbacks, vertical layout and edge fades.

DescriptionList demonstrates source row packing across one to three columns, vertical and bordered variants, and all four sizes. StatusBar composes left, center and optional right regions with default or caller styling. Resizable demonstrates constrained horizontal and vertical panels, keyboard and pointer resizing, and disabled handles. Icon renders bundled SVG masks through the Canvas bitmap path with source assets, sizes, transform and inherited or explicit theme tint; its caller-owned raster cache is collected only after the prior draw list has finished painting.

Toolbar demonstrates the source size metrics, command activation, nested formatting group, optional input, scoped Left/Right navigation, wraparound, and editor caret priority. The Disabled host control disables its commands and input.

TitleBar paints the source-height gradient and title slots. Browser mode hides window controls as in the reference WASM environment; an optional simulated client-decorated host exercises minimize, maximize/restore, close override, title drag, context menu and double-click actions without changing the browser window.

The top-right theme picker uses the 38 pinned GPUI Kit theme presets, grouped by light and dark mode, plus Follow System and a separate high-contrast choice. Follow System starts from the browser color-scheme preference and responds to changes; an explicit named choice persists by ID in local storage. The page CSS and WASM preview use the same resolved preset. The picker supports arrows, Home/End, Enter, Escape and outside-click dismissal. `themes.json` is generated from `third_party/gpui_themes/presets.json`, and the site license bundle includes its attribution. Aurora's source gradient tokens currently use their first stop in the portable C theme; this is a documented adaptation, not full visual parity.

Input and NumberInput expose all four source sizes through the gallery size selector. Input keeps its accessible name while painting only the single-line field, with no extra label row. NumberInput paints real SVG increment/decrement glyphs; the browser exercises their CUI pointer interactions and controlled value changes. The native size APIs define the height and font metrics; the gallery uses those actual styled components.

Settings composes two application-owned pages and six fields through `cui_settings`: search, sidebar navigation, Checkbox, Switch, Input, NumberInput and a checked Menu-backed workspace choice. The gallery exposes its three variants and four sizes. Browser checks exercise field changes, menu selection, page navigation and option changes; values remain owned by the sample application.

Carousel uses four real CUI slide items with source controls and pagination. Browser checks step in both axes, select a page, and wrap from the last slide when looping is enabled. Slide selection and its callback count come from the retained C state; animation is driven by the host frame clock and reduced-motion preference.

Command renders the real inline palette with a retained query editor, filtered virtual rows, icon/shortcut/checked/disabled entries, keyboard navigation and action callbacks. The optional dialog mode hosts the same palette in a CUI Surface. Browser checks filtering, pointer and keyboard confirmation, loading semantics and two-stage Escape behavior: first clear the query, then cancel the dialog.

Root View uses a caller-owned per-window plugin registry and automatically mounts a Dialog, Sheet and Notification layer after application content. Its sample plugin changes Root padding and paints an ordinary overlay at an absolute location without taking application layout space. Browser checks actual callbacks, separate overlay bounds, Dialog and Sheet interaction, and notification mounting; global text selection and OS window facilities remain outside this module.

TextView renders a bounded caller-owned document in Markdown, safe HTML, or plain text, with real styled text, links, selection, and optional scrolling. The gallery highlights a Markdown range and checks link activation; the underlying parser is an explicit subset and does not provide complete CommonMark or HTML5 support.

Chart rasterizes caller-owned data into RGBA pixels for line, area, bar, pie, radar, candlestick and Sankey stories. The gallery exercises each family, curve and label controls, with source-style optional appear animation and pointer hit behavior. The seven C adapters remain partial against the pinned source; see [CHART.md](CHART.md) for the exact method audit.

Speech is a deterministic provider demonstration. Caller-owned callbacks feed ready/interim/phrase/final/error events and PCM samples into the real Speech state, SVG icon button, and waveform. Browser checks provider lifecycle, transcript, teardown, sizes and reduced-motion stability. The site does not request a microphone or provide system recognition; [SPEECH.md](SPEECH.md) tracks those platform gaps.

Questionnaire composes the retained two-step model with item, choice and action containers, a radio group, graphical choices, optional freeform input, validation, Skip/Next/Previous/Submit actions, progress and four source sizes. Its browser story exercises real CUI pointer events, custom indicator/content/shortcut callbacks, and grouped-choice accessibility semantics. Automatic root shortcut routing, cross-item focus transfer, localization and exact source styling remain incomplete; see [QUESTIONNAIRE.md](QUESTIONNAIRE.md).

The browser host forwards all ASCII letter and digit key identities to CUI alongside text events, preserving the original special-key values. Questionnaire routes alphabetic or numeric shortcut hints through the explicit C keyboard helper when its answer scope owns focus. Browser composition flags suppress shortcut activation; committed composition text reaches the retained input. Root-wide shortcut capture, cross-item focus transfer and a complete IME bridge remain pending.

The browser provides `cui_measure_font_metrics` from Canvas font-wide ascent and descent for the selected weight, italic style and family. Its line spacing remains the gallery's `font_size * 1.25`, matching text layout and the alphabetic `fillText` baseline. If the browser lacks `fontBoundingBoxAscent` or `fontBoundingBoxDescent`, the callback reports unavailable and painting falls back to Canvas's top baseline; it does not treat sample-glyph ink bounds as font metrics.

The browser host forwards mouse click counts from MouseEvent.detail because PointerEvent.detail is zero in Chromium. Touch and pen pointer events use one click and suppress compatibility mouse duplicates. Physical Chromium single, double and triple clicks and a touch tap verify raw WASM down/up counts. This supplies metadata; TextView multi-click selection remains under development.

## Shareable URLs and product media

The build generates a static page for every component and every published documentation entry. Component URLs use `/cui/components/checkbox/` and documentation URLs use `/cui/docs/start-here/`. Legacy fragment links continue to work; the browser updates the address to the matching real page. Each static page has its own canonical URL, title and description, Open Graph metadata and a large-image social card. These tags and fallback component/documentation content are present before JavaScript runs. `sitemap.xml` lists the generated routes, and `routes.json` provides their machine-readable index. Metadata follows the [Open Graph protocol](https://ogp.me/). Actual platform caching and preview appearance are controlled by the sharing service.

The homepage's light and dark workspace images show the actual CUI WebAssembly renderer. The social card uses the same original product capture. `web/assets/PROVENANCE.md` documents their source and the pinned regeneration script. `scripts/build_sharing.py` validates the share image dimensions and rejects duplicate or unsafe routes; `tests/test_sharing.py` checks complete page coverage and crawler-visible tags.

Dock's gallery opts into `cui_dock_create_nary`: repeated horizontal splits become siblings under a stable split node. The browser suite splits three panes and drags the second divider, checking that only its two neighbors resize; reset, zoom and cross-region tab transfer remain covered. Existing applications retain binary/v1 behavior with `cui_dock_create`.

Editor's gallery attaches eight live selection slots, bounded history snapshots and measured-column anchors. Ctrl/Alt+Up/Down or Alt-click adds cursors; text is inserted at each cursor as one undoable edit, and Escape collapses extras. The browser checks three-cursor typing and collection restoration through Undo/Redo. Further multi-range clipboard, indentation, rectangular dragging and IME behavior remain separate source work.

## Calendar and hex-entry examples

Calendar exposes independent **Month menu**, **Adjacent days** and **Mark today**
options through the real Calendar presentation API. The host supplies today's
local civil date; the component reads no clock. Selecting an adjacent date updates
selection and recenters when required. Month navigation preserves selection.
The control row stacks at narrow widths. See
[CALENDAR_PRESENTATION.md](CALENDAR_PRESENTATION.md) for the API and lifetimes.

ColorPicker also demonstrates the owned byte-RGBA hex entry from
[COLOR_ENTRY.md](COLOR_ENTRY.md). Editing changes its draft and preview; Enter
commits a complete value into the picker. Incomplete drafts remain editable.
Picker changes preserve a dirty draft until **Use picker color** explicitly
replaces it. Owner synchronization runs before the next frame, after the previous
draw list has been consumed. This demo is separate from the float-HSLA entry API;
it does not establish HSLA popup or tab coverage.

## CI execution

Pages builds the default and regex-enabled gallery once, then shares those exact
artifacts with three independent browser-check groups. All existing checks remain
required; deployment waits for the build and every group. Each group has a private
runner and headless Chromium, so probes and temporary servers remain isolated.
The transfer contains only generated gallery/fixture files, including `.nojekyll`;
its artifact expires after one day. Dependency versions and Action revisions remain
pinned. No cross-revision CUI gallery or fixture binary cache is used.

Pages caches dependency inputs separately from those tested artifacts. The existing
Python setup caches pip downloads using the workflow as its dependency key, covering
the pinned uv 0.10.8 and Zig 0.16.0 distributions. Browser groups cache uv's package
cache per OS, architecture, Python/uv/Playwright version, shard and test-source hash;
a fallback is restricted to that same host/version/shard prefix. Chromium downloads
use a separate exact OS/architecture/Playwright 1.58.0 key, shared by the shards.
Only the optional Rust toolchain's manifest and two input archives are cached against
`backends/regex/pin.json`; installed Rust executables, regex libraries and gallery
build outputs are excluded. The preparation script still verifies each archive's
checksum and reinstalls the private toolchain on every run. Pip installation,
Playwright installation with Linux OS dependencies, every test and deployment gates
also run on cache hits. `tests/test_pages_tool_cache.py` checks this policy offline.

Before this cache change, run
[39769287993](https://github.com/mattmezza/cui/actions/runs/39769287993)
recorded 6 seconds for build-tool installation, 45 seconds for private Rust
preparation and 50 seconds for the regex build. Browser-tool installation took
25 and 29 seconds on shards 1 and 2. Shard 3's same step eventually completed in
11 minutes 53 seconds (2026-10-22 11:19:25–11:31:18 UTC).
Its full job logs were still unavailable, so these observations do not identify whether
package downloads, browser downloads or Linux dependency installation caused the
delay. No warm-cache timing has been measured for this change. Cold misses still
download dependencies; simultaneous cold shards can download the same browser
before any job has saved its cache. Cache transfer and save overhead can offset
download savings. Playwright specifically cautions that browser-cache restoration
can take as long as downloading, and Linux OS dependencies still require normal
installation ([upstream CI guidance](https://playwright.dev/python/docs/ci#caching-browsers)).

The baseline Pages run [39397271176](https://github.com/mattmezza/cui/actions/runs/39397271176)
spent 25.6 minutes in its sequential build/check job. The three groups were balanced
using that run's recorded step durations. The first parallel run
[39401871665](https://github.com/mattmezza/cui/actions/runs/39401871665) passed
all 67 checks: build plus browser validation took 11 minutes 42 seconds
(09:36:33–09:48:15 UTC), about 54% less elapsed time than the baseline.
This is one hosted-runner observation, not a guarantee or a reduction in total
runner minutes. Deployment was intentionally skipped on the experiment branch.
Hosted runner capacity and queue time also affect latency.
Portable matrix builds use two concurrent compilation jobs; tests keep their
existing execution order. Core and native-binding workflows cancel superseded
runs on the same ref. No test coverage or deployment acceptance gate was removed.

DatePicker retains its default extended-API example and locale selector. Enable
**Date-only actions** for two application-owned preset dates, then optionally
enable its clear button and scoped Delete key. The copy button follows the active
mode and includes retained state/value initialization for the actions example.
Dismiss an open modal popup before using its outside clear button. Desktop and
touch tests exercise these actions and compile both copied C snippets. This
coverage excludes date-time editing, arbitrary styling, observers and bindings.

DatePicker additionally offers an explicit **Date + time** mode using the optional single civil date/time C facade. Minute/second, 12/24-hour and clear/Delete controls appear only in that mode; date-only/Date-only actions modes remain available when it is off. See `docs/DATE_TIME_PICKER.md` for state transfer, preset/default-time and source-event limits. Headless desktop and CDP touch proof lives in `tests/web/test_date_time_picker_gallery.py`; copied snippets strictly compile as C11.
