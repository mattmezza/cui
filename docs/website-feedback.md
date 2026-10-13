# Website and component feedback — 2026-10-12

This audit extends the 78-entry parity manifest. An item is closed only after library tests and an actual WASM interaction or visual check; API presence alone is insufficient. All items below are open unless evidence is linked explicitly.

## Rendering and behavior

- [x] AlertDialog: center content within the dialog. — 593bbb1 passes native75, sanitizers75, WASI67 and Chromium78. Native checks centered content, overflow start/end scrolling and recovery in four themes; browser checks visible title/action centering. Published in f6c8cf8; HTTPS78 and twenty artifact files verified. The width fix c965b3d also passes all four gates and reproduces/fixes the CI-only wide-text confirmation miss.
- [x] Attachment and Image: show a real image, replacing the tiny blue fixture. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Avatar: center initials. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Marker: keep “Synchronizing 3 tasks” on the intended horizontal line. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Message: remove the stray renderer test bitmap from the normal body layout. — 0e4ee4a passes Chromium78 and retains the explicit crop/tint/clip pixel fixture checks. Native Message interactions and remaining family limitations are unchanged. Published in f6c8cf8; HTTPS78 and twenty artifact files verified.
- [x] Pagination: center button text vertically. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Progress: indeterminate host-clock animation and reduced-motion fallback. — 18e1ec4 passes native75, sanitizers75, WASI67 and Chromium78. Determinate progress remains application-owned; indeterminate semantics are busy with no numeric current value in the browser. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] Radio: one click clears the previous selection visually and semantically. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Rating: proper empty, filled and partially filled star graphics. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Stepper: make navigation and its current status visibly work. — 9741448 passes native75, sanitizers75, WASI67 and Chromium78; rendered-selection regression fails old code. Published in f6c8cf8; HTTPS78 and twenty artifact files verified.
- [x] Tooltip: center text in the panel. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Calendar and DatePicker: use SVG navigation icons; center popup content. — d55aeb7 passes native75, sanitizers75, WASI67 and Chromium78. Day cells are checked at three sizes; DatePicker trigger/opened icons are checked headlessly. Published in f6c8cf8; HTTPS78 and twenty artifact files verified.
- [x] ColorPicker: browser screen eyedropper with explicit availability, cancellation and controlled RGBA completion. — 0bb3670 passes native75, sanitizers75, WASI67 and Chromium78. Native adapters do not capture the screen; documented applications supply their platform picker. Published in a136c07; HTTPS78 and thirty-two artifact files verified.
- [x] Combobox and Select: options span the field width. — 76c36ba passes native75, sanitizers75, WASI67 and Chromium78; popup-width regression fails old code. Published in f6c8cf8; HTTPS78 and twenty artifact files verified.
- [x] Editor and Input: word navigation and vertical caret navigation. — Native/WASI/browser checks pass in the navigation batch; this fixes the gallery editor model, not complete code-editor parity.
- [x] Input Group: constrain the field so addons stay on the same row. — 87721c1 passes native75, sanitizers75, WASI67 and Chromium78. Native checks all four sizes; browser checks 390px/1280px inline layout and block-mode opt-in. Published in f6c8cf8; HTTPS78 and twenty artifact files verified.
- [x] Textarea: word/vertical navigation and repeated Down/content visibility. — 00a4f3d moves the scroll offset to the clipped viewport; native drawing regression fails the old component. Native77, sanitizers75, WASI67 and Chromium78 pass in the combined batch. Published in 917f17d; HTTPS78 and thirty artifact files verified.
- [x] Dock: visible composed panes, tab selection, split and reset controls with instructions. — 1f4dd88 passes Chromium78 with physical interactions and pane geometry checks. The public styled Dock and tab/divider dragging remain incomplete. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] Resizable: a composed workspace visibly resizes Explorer/editor panes and switches example files. — 1ceb567 passes Chromium78 with physical drag and visible geometry assertions; portable code is unchanged from validated 41de104. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] Scrollable and Sidebar: natural wheel sensitivity and moving sidebar content. — 917f17d positions the sidebar content by its scroll offset and transports precise browser CSS pixels for scrollable/sidebar/virtual previews. Geometry and real-wheel browser regressions pass. Native77, sanitizers75, WASI67 and Chromium78 pass in the combined batch. Published in 917f17d; HTTPS78 and thirty artifact files verified.
- [x] Sheet: opt-in host-time enter/exit animation on all four edges, reversal and reduced-motion handling. — 8777cc4 passes native77, sanitizer75, WASI67 and Chromium78 after integration. Old base fails the offscreen geometry regression. Close/focus/result callbacks wait for exit; input is blocked while closing. Published in 1dc98a9; HTTPS78 and all 147 page artifacts verified.
- [x] Theme: six labeled design-token swatches, inherited typography/controls and palette instructions. — dc12d14 passes native75, strict C11 web build, Chromium78 and quality checks. Documentation now describes system defaults, stored choices and host-owned native appearance. Published in a136c07; HTTPS78 and thirty-two artifact files verified.
- [x] VirtualList: wheel/scrollbar/Home/End scroll real task rows from the 100,000-key model. — 85531a4 passes Chromium78 and submits at most nine rows; native model75 passes. Offscreen focus, asynchronous measurement and a public styled widget remain incomplete. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] Chart: configurable 2px default strokes with smooth edges. — d564c70 passes native75, sanitizers75, WASI67 and Chromium78. Coverage regression fails the old raster; width/cache/clamping checks pass. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] Command: content spans panel width. — 76c36ba checks full scroll/row width at two panel sizes, passes the same gates and fails old code. Published in f6c8cf8; HTTPS78 and twenty artifact files verified.
- [x] DataTable: headers sort actual gallery rows and use themed SVG indicators. — 41de104 passes native75, sanitizers75, WASI67 and Chromium78; the graphic regression fails old code. Application-owned model sorting is documented. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] Plot: visible projected sine wave with zoom/pan/reset and domain readout. — 8f1497a passes Chromium78 with changed/restored bitmap signatures. This composes the numerical model with Chart; a separate public Plot widget remains incomplete. Published in 9c87b99; HTTPS78 and twenty-seven artifact files verified.
- [x] TextView: the existing fenced C code appears directly below the heading. — dc12d14 checks visible initial monospace bounds in Chromium78; native75 and quality checks pass. Published in a136c07; HTTPS78 and thirty-two artifact files verified.

## Design system

- [x] [Shared size scale](DESIGN_SYSTEM.md): xs through9xl, preserving inherited metrics. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Rounded-corner tokens and interactive examples](DESIGN_SYSTEM.md). — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Typography](TYPOGRAPHY.md): registered fonts, styles, weights and sizes. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Animation examples and configurable/reduced motion](DESIGN_SYSTEM.md#backgrounds-opacity-and-motion). — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Backgrounds](BACKGROUNDS.md): colors, gradients, opacity and bitmap blur, with documented renderer capabilities. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Spacing tokens](DESIGN_SYSTEM.md) with explicit unit-to-pixel dimensions. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Composed native app example](COMPOSED_APPS.md): sidebar/hamburger, account menu, hero, pricing CTAs and editable settings. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.

## Documentation and publishing

- [x] [All78 component usage guides](COMPONENT_USAGE.md) explain frame composition, retained state and ownership. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] [Callback/event/host examples](IMMEDIATE_AND_STATE.md), with per-component usage and synchronous lifetime rules. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] Reproducible Go, Rust, JavaScript and TypeScript examples with actual support limits. Integrated `fc56f33`; four headless clients and four SDL3/Xvfb native windows pass actual pointer-click callbacks. The small example host ABI is not a full component binding. See [LANGUAGE_BINDINGS.md](LANGUAGE_BINDINGS.md).
- [x] [Browsably published docs and getting-started path](START_HERE.md), including reproducible build, state/callback and host instructions. — Published9220c22: [full live-site evidence](validation/published-9220c22.txt), with actual C/WASM pixels and interaction checks. See the linked guide for capabilities and limits.
- [x] Copyable agent prompt and generated llms.txt / llms-full.txt, with llm.txt and llm-full.txt aliases. — Published f6c8cf8; actual HTTPS copy/export checks and artifact byte comparisons pass. Broader documentation and bindings remain open.
- [x] Original product media with recorded provenance. — 31847cc adds real CUI WASM workspace captures for light/dark, a 1200×630 original social card and a pinned regeneration script. The larger composed-app gallery remains open. Published in 1dc98a9; HTTPS78 and all 147 page artifacts verified.
- [x] Static Open Graph coverage for home, all 78 components and every published docs entry. — 31847cc validates 147 distinct canonical pages, required tags, share-image dimensions, sitemap and JS-disabled fallback content. Direct project-Pages visits/reloads and Chromium78 pass. External sharing-service caches are not asserted. Published in 1dc98a9; HTTPS78 and all 147 page artifacts verified.

## Existing work still in scope

Full GPUI Kit 0.7.1 parity remains unfinished. See `gpui-parity.json` and `swarm-status.md`. Questionnaire batched input/confirmation event ordering remains under investigation in the isolated event-order worktree.

## First polish batch evidence

Native75, sanitizer75, WASI67, full Chromium78 and targeted visual regression checks pass. The Radio redraw regression fails against the previous base implementation. The Image gallery uses an original generated landscape; image-state, fit, grayscale, opacity and failure checks remain active. See [native](validation/native-3d05c2a.txt), [sanitizers](validation/sanitizers-3d05c2a.txt), [WASI](validation/wasi-3d05c2a.txt), [browser](validation/browser-3d05c2a.txt) and [visual regressions](validation/visual-3d05c2a.txt). These checks close the specific defects above; they do not certify full component parity.

Navigation evidence: [native](validation/native-a679582.txt), [sanitizers](validation/sanitizers-a679582.txt), [WASI](validation/wasi-a679582.txt), [browser](validation/browser-a679582.txt), and [failing-before word motion](validation/edit-failing-before-a679582.txt).

## Feedback published on 2026-10-13

- [x] Refresh the README image using the current real composed C/WASM workspace.
- [x] Remove the redundant inherited-size label; use base by default and one shared-size selector.
- [x] Remove upstream branding from visible website copy while preserving source links and required license notices.
- [x] Center the Accordion caret in both open and closed states.
- [x] Fit unspecified AlertDialog height to wrapped content and actions instead of the fixed gallery height.

These changes and the composed overview wrapping correction are in `1f9ddda`.
See [local regression checks](validation/website-feedback-new.txt) and
[successful deployment and direct HTTPS checks](validation/published-1f9ddda.txt).
The overall component parity scope remains in progress.

## Additional feedback under active implementation

The following remain open until release and direct website verification:

- [ ] Wide-screen Resizable drag: responsive limits, both axes, actual pane geometry.
- [ ] Pagination counter and Select placeholder/value vertical alignment at every design size.
- [ ] Calendar/DatePicker locales and year/month grid selection for historical dates.
- [ ] All remaining legacy four-size controls use genuine shared design metrics.
- [ ] Sidebar nested icon/text clipping below fixed header/footer, including hidden-row hit tests.
- [ ] Bounded Tabs Shift-wheel scrolling and properly sized SVG overflow controls on both axes.
- [ ] TitleBar browser demonstration of native traffic lights, with explicit fullscreen behavior.
- [ ] Toolbar Find example searches real visible content and reports matches.
- [ ] Component switching preserves page and catalog scroll positions.
- [ ] Command dialog search/results fill available content width.
- [ ] Long DataTable scrolling matches panel pixel-based wheel behavior.
- [ ] Plot opt-in Shift-drag pan, Ctrl-wheel zoom, touch pan and pinch-to-zoom.
- [ ] Table alternating row colors and independently customizable header background.
- [ ] Avatar stack example with profile images and overflow count.
- [ ] Distinct Info, Success, Warning and Error Alert styling with SVG icons in both themes.

Focused local checks are evidence for individual fixes, not full parity.
## New feedback — native starter and remaining controls

- [x] Native starter source and website guide: dependencies, C11 compiler, CMake, Make, pkg-config static linker flags, actual native window, sidebar pages and bottom account menu. Published ddd0ae3; direct public screenshots and copied linker recipe verified.
- [ ] Resizable at wide desktop widths: both demo panes reach hard maxima, preventing drag; isolated fix verified before integration.
- [ ] Center Pagination compact counter (`x / y`) alongside button labels; previous button-label check did not cover counter. A new headless regression fails before the root fix and passes afterward.
- [ ] Apply xs/sm/base/md/lg/xl/2xl and larger design sizes to all remaining legacy four-size controls; dropdown, badge and other legacy selector families need coverage.
- [ ] DatePicker localized month/weekday labels, formatting and a clickable header year grid for quick selection of historical dates such as1980.

These points remain tracked until integrated and deployed verification, alongside the complete component manifest.

Additional open feedback: Alert/Bubble semantic styles; intrinsic HoverCard; reusable Message conversation; Notification action/close separation; shared optional scrollbar appearance/auto-hide across families; Python native example and clearly documented Node JavaScript/TypeScript startup.
