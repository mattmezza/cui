# Website and component feedback — 2026-10-12

This audit extends the 78-entry parity manifest. An item is closed only after library tests and an actual WASM interaction or visual check; API presence alone is insufficient. All items below are open unless evidence is linked explicitly.

## Rendering and behavior

- [ ] AlertDialog: center content within the dialog.
- [x] Attachment and Image: show a real image, replacing the tiny blue fixture. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Avatar: center initials. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Marker: keep “Synchronizing 3 tasks” on the intended horizontal line. — first polish batch; native/browser geometry and interaction evidence below.
- [ ] Message: inspect and correct body/action layout.
- [x] Pagination: center button text vertically. — first polish batch; native/browser geometry and interaction evidence below.
- [ ] Progress: demonstrate animated progress and reduced motion.
- [x] Radio: one click clears the previous selection visually and semantically. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Rating: proper empty, filled and partially filled star graphics. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Stepper: make navigation and its current status visibly work. — 9741448 passes native75, sanitizers75, WASI67 and Chromium78; rendered-selection regression fails old code. Publication pending.
- [x] Tooltip: center text in the panel. — first polish batch; native/browser geometry and interaction evidence below.
- [x] Calendar and DatePicker: use SVG navigation icons; center popup content. — d55aeb7 passes native75, sanitizers75, WASI67 and Chromium78. Day cells are checked at three sizes; DatePicker trigger/opened icons are checked headlessly. Publication pending.
- [ ] ColorPicker: screen eyedropper through an explicit host capability.
- [x] Combobox and Select: options span the field width. — 76c36ba passes native75, sanitizers75, WASI67 and Chromium78; popup-width regression fails old code. Publication pending.
- [x] Editor and Input: word navigation and vertical caret navigation. — Native/WASI/browser checks pass in the navigation batch; this fixes the gallery editor model, not complete code-editor parity.
- [x] Input Group: constrain the field so addons stay on the same row. — 87721c1 passes native75, sanitizers75, WASI67 and Chromium78. Native checks all four sizes; browser checks 390px/1280px inline layout and block-mode opt-in. Publication pending.
- [ ] Textarea: word/vertical navigation is implemented/tested in the navigation batch; repeated Down/content visibility remains under investigation.
- [ ] Dock: a visible, interactive docking demonstration and instructions.
- [ ] Resizable: divider drag resizes panes correctly.
- [ ] Scrollable and Sidebar: natural wheel sensitivity and working scrollbar.
- [ ] Sheet: enter/exit animation with reduced-motion handling.
- [ ] Theme: understandable, functional token demonstration.
- [ ] VirtualList: visibly scroll a genuinely virtualized large list.
- [ ] Chart: continuous, adequately thick curved line rendering.
- [x] Command: content spans panel width. — 76c36ba checks full scroll/row width at two panel sizes, passes the same gates and fails old code. Publication pending.
- [ ] DataTable: clickable header sorting with SVG indicators.
- [ ] Plot: visible, interactive plot with clear instructions.
- [ ] TextView: Markdown fenced-code example.

## Design system

- [ ] Shared size scale: xs, sm, base, md, lg, xl, 2xl, 3xl, 4xl and larger where useful.
- [ ] Rounded-corner tokens and interactive examples.
- [ ] Typography: fonts, styles, weights and sizes.
- [ ] Animation examples and configurable motion.
- [ ] Backgrounds: colors, gradients, opacity and blur, with truthful renderer capabilities.
- [ ] Spacing tokens with explicit unit-to-pixel dimensions.
- [ ] Professional composed examples with copyable working code: app shell, sidebar/hamburger, account menu, hero and pricing CTAs.

## Documentation and publishing

- [ ] Every component explains its immediate/retained state and ownership.
- [ ] Callback, event and host integration examples.
- [ ] Reproducible Go, Rust, JavaScript and TypeScript examples with actual support limits.
- [ ] Improved browsable documentation and intuitive getting-started path.
- [ ] Copyable agent prompt and generated llms.txt / llms-full.txt (including requested aliases).
- [ ] Original professional media with recorded provenance.
- [ ] Complete Open Graph coverage, validated share images and metadata.

## Existing work still in scope

Full GPUI Kit 0.7.1 parity remains unfinished. See `gpui-parity.json` and `swarm-status.md`. Questionnaire batched input/confirmation event ordering remains under investigation in the isolated event-order worktree.

## First polish batch evidence

Native75, sanitizer75, WASI67, full Chromium78 and targeted visual regression checks pass. The Radio redraw regression fails against the previous base implementation. The Image gallery uses an original generated landscape; image-state, fit, grayscale, opacity and failure checks remain active. See [native](validation/native-3d05c2a.txt), [sanitizers](validation/sanitizers-3d05c2a.txt), [WASI](validation/wasi-3d05c2a.txt), [browser](validation/browser-3d05c2a.txt) and [visual regressions](validation/visual-3d05c2a.txt). These checks close the specific defects above; they do not certify full component parity.

Navigation evidence: [native](validation/native-a679582.txt), [sanitizers](validation/sanitizers-a679582.txt), [WASI](validation/wasi-a679582.txt), [browser](validation/browser-a679582.txt), and [failing-before word motion](validation/edit-failing-before-a679582.txt).
