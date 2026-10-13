# Nightly report — live development checkpoint

Repository: https://github.com/mattmezza/cui. Branch `main`. Evidence/source commit before this report update: `2d291fe9741addc322a6e107e15d03bf97e0d650`. See swarm-status.md for branch promotion and push/CI observations.

## Working architecture

Explicit C11 contexts, pinned Clay layout, stable scoped IDs, bounded frame strings, ordered previous-frame hit routing, capture/focus/modal scopes, independent semantics and draw lists. Unstyled controlled primitives feed theme-driven components. UTF-8 scalar editing with bounded undo/redo, retained keyed Fenwick virtualization, split/tab docking with transactional persistence and plot scales/clipped geometry are substantive headless models. Optional FreeType/HarfBuzz shapes runs. Software, SDL_Renderer compatibility, SDL_GPU Vulkan and independent Sokol GLCORE are separate renderers; actual hardware/runtime selection is reported.

GPUI Kit baseline 0.7.1 commit `87d10ae5e1299d1be18670a37570c5402f952e34`; Clay e6cc36941ab2af5d81107617039d6f527a1c660b. See upstream-baseline.md and DECISIONS.md. Source code and generated one/few headers are tested independently.

## Catalog counts

| Status | Families |
|---|---:|
| not_started | 0 |
| scaffolded | 0 |
| implemented | 2 |
| tested | 76 |
| verified | 0 |

Every non-verified family remains incomplete; tested means only its documented subset has tests. **No full family is verified.** Full feature backlog: [gpui-parity.json](gpui-parity.json), [COMPONENTS.md](COMPONENTS.md).

## Validation evidence

Integrated next17 compiled source 2d291fe9741addc322a6e107e15d03bf97e0d650 passes native95/95, ASan/UBSan95/95, WASI79/79, all78 browser stories and compiled excerpts, extensions, native Chromium composition across seven consumers, touch42/42 plus16/16 and page navigation19/19. Explicit native font fallback, Numeric/OTP composition, ListMulti callback ownership and page-swipe fallback are integrated. Published source remains30deb99 until CI/deployment verification. Full family verification remains0/78; physical devices, native accessibility, bidi editing/wrapping and full platform/ecosystem acceptance remain unfinished. See [next17 proof](validation/fallback-ime-touch-next17.txt) and [previous publication](validation/published-30deb99.txt).

Latest stored logs (including the preserved initial WASI failure and corrected rerun):

- [adapters-d95f727.txt](validation/adapters-d95f727.txt)
- [alert-failing-before-593bbb1.txt](validation/alert-failing-before-593bbb1.txt)
- [asan-1c1322e.txt](validation/asan-1c1322e.txt)
- [asan-2190f5b.txt](validation/asan-2190f5b.txt)
- [asan-56.txt](validation/asan-56.txt)
- [asan-57.txt](validation/asan-57.txt)
- [asan-58.txt](validation/asan-58.txt)
- [asan-60.txt](validation/asan-60.txt)
- [asan-62-source.txt](validation/asan-62-source.txt)
- [asan-62.txt](validation/asan-62.txt)
- [asan-71a64d5.txt](validation/asan-71a64d5.txt)
- [asan-7970367.txt](validation/asan-7970367.txt)
- [asan-controls-753be98.txt](validation/asan-controls-753be98.txt)
- [asan-form.txt](validation/asan-form.txt)
- [asan-measurement-input-group.txt](validation/asan-measurement-input-group.txt)
- [asan-otp.txt](validation/asan-otp.txt)
- [asan-recovery.txt](validation/asan-recovery.txt)
- [asan-surfaces.txt](validation/asan-surfaces.txt)
- [asan-table-dropdown.txt](validation/asan-table-dropdown.txt)
- [attachment-semantics-before-fb4464b.txt](validation/attachment-semantics-before-fb4464b.txt)
- [bidi-next16.txt](validation/bidi-next16.txt)
- [browser-0df7626.txt](validation/browser-0df7626.txt)
- [browser-0e4ee4a.txt](validation/browser-0e4ee4a.txt)
- [browser-18e1ec4.txt](validation/browser-18e1ec4.txt)
- [browser-1ceb567.txt](validation/browser-1ceb567.txt)
- [browser-1f38787.txt](validation/browser-1f38787.txt)
- [browser-1f4dd88.txt](validation/browser-1f4dd88.txt)
- [browser-2af3f39.txt](validation/browser-2af3f39.txt)
- [browser-3605e0b.txt](validation/browser-3605e0b.txt)
- [browser-39.txt](validation/browser-39.txt)
- [browser-3c08749.txt](validation/browser-3c08749.txt)
- [browser-3d05c2a.txt](validation/browser-3d05c2a.txt)
- [browser-41de104.txt](validation/browser-41de104.txt)
- [browser-42.txt](validation/browser-42.txt)
- [browser-43.txt](validation/browser-43.txt)
- [browser-46.txt](validation/browser-46.txt)
- [browser-51-integrated.txt](validation/browser-51-integrated.txt)
- [browser-51-measurement.txt](validation/browser-51-measurement.txt)
- [browser-51.txt](validation/browser-51.txt)
- [browser-53-integrated.txt](validation/browser-53-integrated.txt)
- [browser-55f78c3.txt](validation/browser-55f78c3.txt)
- [browser-56-integrated.txt](validation/browser-56-integrated.txt)
- [browser-57-integrated.txt](validation/browser-57-integrated.txt)
- [browser-58-integrated.txt](validation/browser-58-integrated.txt)
- [browser-593bbb1.txt](validation/browser-593bbb1.txt)
- [browser-5b87567.txt](validation/browser-5b87567.txt)
- [browser-60.txt](validation/browser-60.txt)
- [browser-62-source.txt](validation/browser-62-source.txt)
- [browser-62.txt](validation/browser-62.txt)
- [browser-6e023e8.txt](validation/browser-6e023e8.txt)
- [browser-71a64d5.txt](validation/browser-71a64d5.txt)
- [browser-71f6695.txt](validation/browser-71f6695.txt)
- [browser-76c36ba.txt](validation/browser-76c36ba.txt)
- [browser-85531a4.txt](validation/browser-85531a4.txt)
- [browser-87721c1.txt](validation/browser-87721c1.txt)
- [browser-882a5af.txt](validation/browser-882a5af.txt)
- [browser-8e4e15b.txt](validation/browser-8e4e15b.txt)
- [browser-8e79c46.txt](validation/browser-8e79c46.txt)
- [browser-8f1497a.txt](validation/browser-8f1497a.txt)
- [browser-93d6706.txt](validation/browser-93d6706.txt)
- [browser-94503eb.txt](validation/browser-94503eb.txt)
- [browser-9741448.txt](validation/browser-9741448.txt)
- [browser-a5d5f23.txt](validation/browser-a5d5f23.txt)
- [browser-a679582.txt](validation/browser-a679582.txt)
- [browser-ae43b53.txt](validation/browser-ae43b53.txt)
- [browser-alert-settle.txt](validation/browser-alert-settle.txt)
- [browser-b219eee.txt](validation/browser-b219eee.txt)
- [browser-bfb9f4f.txt](validation/browser-bfb9f4f.txt)
- [browser-c1cc854.txt](validation/browser-c1cc854.txt)
- [browser-c56fdb0.txt](validation/browser-c56fdb0.txt)
- [browser-c8d5893.txt](validation/browser-c8d5893.txt)
- [browser-c965b3d.txt](validation/browser-c965b3d.txt)
- [browser-cd531cc.txt](validation/browser-cd531cc.txt)
- [browser-cf11f39.txt](validation/browser-cf11f39.txt)
- [browser-click-host-e33586a.txt](validation/browser-click-host-e33586a.txt)
- [browser-d55aeb7.txt](validation/browser-d55aeb7.txt)
- [browser-d564c70.txt](validation/browser-d564c70.txt)
- [browser-d60b8ef.txt](validation/browser-d60b8ef.txt)
- [browser-d6aa5bd.txt](validation/browser-d6aa5bd.txt)
- [browser-d6fe7b0.txt](validation/browser-d6fe7b0.txt)
- [browser-d85d2a5.txt](validation/browser-d85d2a5.txt)
- [browser-d972c06.txt](validation/browser-d972c06.txt)
- [browser-e1c2b5b.txt](validation/browser-e1c2b5b.txt)
- [browser-e96fa1a.txt](validation/browser-e96fa1a.txt)
- [browser-f100df2.txt](validation/browser-f100df2.txt)
- [browser-f248d6e.txt](validation/browser-f248d6e.txt)
- [browser-failure-30bde26.txt](validation/browser-failure-30bde26.txt)
- [browser-plot-frame-wait.txt](validation/browser-plot-frame-wait.txt)
- [browser-pre-surfaces.txt](validation/browser-pre-surfaces.txt)
- [browser-recovery.txt](validation/browser-recovery.txt)
- [calendar-failing-before-d55aeb7.txt](validation/calendar-failing-before-d55aeb7.txt)
- [carousel-motion-before-fix.txt](validation/carousel-motion-before-fix.txt)
- [chart-sankey-portability-9ea51e5.txt](validation/chart-sankey-portability-9ea51e5.txt)
- [choice-failing-before-76c36ba.txt](validation/choice-failing-before-76c36ba.txt)
- [ci-01865d5.txt](validation/ci-01865d5.txt)
- [ci-alert-overflow-1836045.txt](validation/ci-alert-overflow-1836045.txt)
- [circle-editor-dock-plot-ccbade3.txt](validation/circle-editor-dock-plot-ccbade3.txt)
- [clang-zig-headless.txt](validation/clang-zig-headless.txt)
- [color-screen.txt](validation/color-screen.txt)
- [combined-color-theme-dc12d14.txt](validation/combined-color-theme-dc12d14.txt)
- [combined-scroll-917f17d.txt](validation/combined-scroll-917f17d.txt)
- [combined-sharing-31847cc.txt](validation/combined-sharing-31847cc.txt)
- [combined-sheet-8777cc4.txt](validation/combined-sheet-8777cc4.txt)
- [command-failing-before-76c36ba.txt](validation/command-failing-before-76c36ba.txt)
- [core-ancestry-before-fb4464b.txt](validation/core-ancestry-before-fb4464b.txt)
- [cursor-x11.txt](validation/cursor-x11.txt)
- [deployed-42.txt](validation/deployed-42.txt)
- [design-scale-7da1a00.txt](validation/design-scale-7da1a00.txt)
- [design-site-1a8185d.txt](validation/design-site-1a8185d.txt)
- [edit-failing-before-a679582.txt](validation/edit-failing-before-a679582.txt)
- [editor-dock-plot-3e60050.txt](validation/editor-dock-plot-3e60050.txt)
- [editor-ime-before-17cf96b.txt](validation/editor-ime-before-17cf96b.txt)
- [failing-before-1f38787.txt](validation/failing-before-1f38787.txt)
- [failing-before-41de104.txt](validation/failing-before-41de104.txt)
- [failing-before-c965b3d.txt](validation/failing-before-c965b3d.txt)
- [failing-before-d564c70.txt](validation/failing-before-d564c70.txt)
- [gcc-56.txt](validation/gcc-56.txt)
- [gcc-57.txt](validation/gcc-57.txt)
- [gcc-58.txt](validation/gcc-58.txt)
- [gcc-60.txt](validation/gcc-60.txt)
- [gcc-button-intents.txt](validation/gcc-button-intents.txt)
- [gcc-choice-wheel.txt](validation/gcc-choice-wheel.txt)
- [gcc-clip-lifecycle.txt](validation/gcc-clip-lifecycle.txt)
- [gcc-combo-table.txt](validation/gcc-combo-table.txt)
- [gcc-focus-proxy.txt](validation/gcc-focus-proxy.txt)
- [gcc-focus-within.txt](validation/gcc-focus-within.txt)
- [gcc-form.txt](validation/gcc-form.txt)
- [gcc-gallery46.txt](validation/gcc-gallery46.txt)
- [gcc-inert.txt](validation/gcc-inert.txt)
- [gcc-input-group.txt](validation/gcc-input-group.txt)
- [gcc-input-paint.txt](validation/gcc-input-paint.txt)
- [gcc-input-tree.txt](validation/gcc-input-tree.txt)
- [gcc-integrated-51.txt](validation/gcc-integrated-51.txt)
- [gcc-intent-theme.txt](validation/gcc-intent-theme.txt)
- [gcc-link.txt](validation/gcc-link.txt)
- [gcc-measurement.txt](validation/gcc-measurement.txt)
- [gcc-offset.txt](validation/gcc-offset.txt)
- [gcc-otp.txt](validation/gcc-otp.txt)
- [gcc-palette-caps.txt](validation/gcc-palette-caps.txt)
- [gcc-pickers.txt](validation/gcc-pickers.txt)
- [gcc-pointer.txt](validation/gcc-pointer.txt)
- [gcc-pressed.txt](validation/gcc-pressed.txt)
- [gcc-reference-controls.txt](validation/gcc-reference-controls.txt)
- [gcc-resumed.txt](validation/gcc-resumed.txt)
- [gcc-scroll-table.txt](validation/gcc-scroll-table.txt)
- [gcc-shapes.txt](validation/gcc-shapes.txt)
- [gcc-sheet-resize.txt](validation/gcc-sheet-resize.txt)
- [gcc-status.txt](validation/gcc-status.txt)
- [gcc-surface-footer.txt](validation/gcc-surface-footer.txt)
- [gcc-surface-scroll.txt](validation/gcc-surface-scroll.txt)
- [gcc-surface-tracking.txt](validation/gcc-surface-tracking.txt)
- [gcc-surfaces-choice.txt](validation/gcc-surfaces-choice.txt)
- [gcc-surfaces.txt](validation/gcc-surfaces.txt)
- [gcc-table-dropdown.txt](validation/gcc-table-dropdown.txt)
- [gcc-table-navigation.txt](validation/gcc-table-navigation.txt)
- [gcc-tabs-scrollbar.txt](validation/gcc-tabs-scrollbar.txt)
- [gcc-wheel.txt](validation/gcc-wheel.txt)
- [gcc-xorg.txt](validation/gcc-xorg.txt)
- [group-failing-before-87721c1.txt](validation/group-failing-before-87721c1.txt)
- [ime-bidi-touch-next16.txt](validation/ime-bidi-touch-next16.txt)
- [input-current-frame-before-confirmed.txt](validation/input-current-frame-before-confirmed.txt)
- [input-current-frame-final-quality.txt](validation/input-current-frame-final-quality.txt)
- [input-current-frame-final-tests.txt](validation/input-current-frame-final-tests.txt)
- [language-examples-5c8b3bc.txt](validation/language-examples-5c8b3bc.txt)
- [limited-stack-c8d5893.txt](validation/limited-stack-c8d5893.txt)
- [nary-multicursor-plot-86d8f9e.txt](validation/nary-multicursor-plot-86d8f9e.txt)
- [native-18e1ec4.txt](validation/native-18e1ec4.txt)
- [native-1c1322e.txt](validation/native-1c1322e.txt)
- [native-1da5aca.txt](validation/native-1da5aca.txt)
- [native-1f38787.txt](validation/native-1f38787.txt)
- [native-25b84dc.txt](validation/native-25b84dc.txt)
- [native-283a328.txt](validation/native-283a328.txt)
- [native-2af3f39.txt](validation/native-2af3f39.txt)
- [native-30bde26.txt](validation/native-30bde26.txt)
- [native-3605e0b.txt](validation/native-3605e0b.txt)
- [native-3c08749.txt](validation/native-3c08749.txt)
- [native-3d05c2a.txt](validation/native-3d05c2a.txt)
- [native-3d769d8.txt](validation/native-3d769d8.txt)
- [native-41de104.txt](validation/native-41de104.txt)
- [native-46f31e6.txt](validation/native-46f31e6.txt)
- [native-50ffcc4.txt](validation/native-50ffcc4.txt)
- [native-55f78c3.txt](validation/native-55f78c3.txt)
- [native-593bbb1.txt](validation/native-593bbb1.txt)
- [native-6061cc5.txt](validation/native-6061cc5.txt)
- [native-62-source.txt](validation/native-62-source.txt)
- [native-62.txt](validation/native-62.txt)
- [native-71a64d5.txt](validation/native-71a64d5.txt)
- [native-71f6695.txt](validation/native-71f6695.txt)
- [native-75ef24f.txt](validation/native-75ef24f.txt)
- [native-76c36ba.txt](validation/native-76c36ba.txt)
- [native-7970367.txt](validation/native-7970367.txt)
- [native-85531a4.txt](validation/native-85531a4.txt)
- [native-87721c1.txt](validation/native-87721c1.txt)
- [native-882a5af.txt](validation/native-882a5af.txt)
- [native-8e4e15b.txt](validation/native-8e4e15b.txt)
- [native-8e79c46.txt](validation/native-8e79c46.txt)
- [native-9741448.txt](validation/native-9741448.txt)
- [native-9769858.txt](validation/native-9769858.txt)
- [native-a679582.txt](validation/native-a679582.txt)
- [native-b219eee.txt](validation/native-b219eee.txt)
- [native-bb4d462.txt](validation/native-bb4d462.txt)
- [native-bfb9f4f.txt](validation/native-bfb9f4f.txt)
- [native-c1cc854.txt](validation/native-c1cc854.txt)
- [native-c36210d.txt](validation/native-c36210d.txt)
- [native-c8d5893.txt](validation/native-c8d5893.txt)
- [native-c965b3d.txt](validation/native-c965b3d.txt)
- [native-ceb6f72.txt](validation/native-ceb6f72.txt)
- [native-cf11f39.txt](validation/native-cf11f39.txt)
- [native-cf513f5.txt](validation/native-cf513f5.txt)
- [native-controls-753be98.txt](validation/native-controls-753be98.txt)
- [native-d55aeb7.txt](validation/native-d55aeb7.txt)
- [native-d564c70.txt](validation/native-d564c70.txt)
- [native-d5c0631.txt](validation/native-d5c0631.txt)
- [native-d6fe7b0.txt](validation/native-d6fe7b0.txt)
- [native-db79e1e.txt](validation/native-db79e1e.txt)
- [native-e5b414b.txt](validation/native-e5b414b.txt)
- [native-editor-ime-17cf96b.txt](validation/native-editor-ime-17cf96b.txt)
- [native-f100df2.txt](validation/native-f100df2.txt)
- [native-f248d6e.txt](validation/native-f248d6e.txt)
- [native-font-metrics-7fdfe35.txt](validation/native-font-metrics-7fdfe35.txt)
- [native-pointer-clicks-2c524e4.txt](validation/native-pointer-clicks-2c524e4.txt)
- [native-questionnaire-a11y-f19c062.txt](validation/native-questionnaire-a11y-f19c062.txt)
- [native-questionnaire-containers-74c9347.txt](validation/native-questionnaire-containers-74c9347.txt)
- [native-questionnaire-focus-5914945.txt](validation/native-questionnaire-focus-5914945.txt)
- [native-recovery.txt](validation/native-recovery.txt)
- [native-root-overlay-5ccee5a.txt](validation/native-root-overlay-5ccee5a.txt)
- [native-semantic-ancestry-fb4464b.txt](validation/native-semantic-ancestry-fb4464b.txt)
- [native-textview-0773cd4.txt](validation/native-textview-0773cd4.txt)
- [pages-157961a-failure.txt](validation/pages-157961a-failure.txt)
- [pages-9e72440-failure.txt](validation/pages-9e72440-failure.txt)
- [pages-a96fd34-failure.txt](validation/pages-a96fd34-failure.txt)
- [pages-daf43b9-failure.txt](validation/pages-daf43b9-failure.txt)
- [pages-failure-1edb47d.txt](validation/pages-failure-1edb47d.txt)
- [pages-failure-2b917ab.txt](validation/pages-failure-2b917ab.txt)
- [pages-failure-5ced994.txt](validation/pages-failure-5ced994.txt)
- [portable-stack-limited.txt](validation/portable-stack-limited.txt)
- [portable-stack-portable.txt](validation/portable-stack-portable.txt)
- [portable-stack-quality.txt](validation/portable-stack-quality.txt)
- [progress-dock-plot-6747044.txt](validation/progress-dock-plot-6747044.txt)
- [published-04e044e.txt](validation/published-04e044e.txt)
- [published-0cbfcbb.txt](validation/published-0cbfcbb.txt)
- [published-0e61b2f.txt](validation/published-0e61b2f.txt)
- [published-15885d6.txt](validation/published-15885d6.txt)
- [published-1dc98a9.txt](validation/published-1dc98a9.txt)
- [published-1f9ddda.txt](validation/published-1f9ddda.txt)
- [published-2be466c.txt](validation/published-2be466c.txt)
- [published-30deb99.txt](validation/published-30deb99.txt)
- [published-3386332.txt](validation/published-3386332.txt)
- [published-3aecd89.txt](validation/published-3aecd89.txt)
- [published-49d4bfe.txt](validation/published-49d4bfe.txt)
- [published-4fa4dad.txt](validation/published-4fa4dad.txt)
- [published-54e3410.txt](validation/published-54e3410.txt)
- [published-55a0b6a.txt](validation/published-55a0b6a.txt)
- [published-55e904a.txt](validation/published-55e904a.txt)
- [published-5608e51.txt](validation/published-5608e51.txt)
- [published-596742d.txt](validation/published-596742d.txt)
- [published-59a0988.txt](validation/published-59a0988.txt)
- [published-5eb21f6.txt](validation/published-5eb21f6.txt)
- [published-611631b.txt](validation/published-611631b.txt)
- [published-65da7b2.txt](validation/published-65da7b2.txt)
- [published-917f17d.txt](validation/published-917f17d.txt)
- [published-9220c22.txt](validation/published-9220c22.txt)
- [published-9c87b99.txt](validation/published-9c87b99.txt)
- [published-a136c07.txt](validation/published-a136c07.txt)
- [published-a17b9d3.txt](validation/published-a17b9d3.txt)
- [published-a4737f6.txt](validation/published-a4737f6.txt)
- [published-a70afe9.txt](validation/published-a70afe9.txt)
- [published-ab46d9d.txt](validation/published-ab46d9d.txt)
- [published-d552763.txt](validation/published-d552763.txt)
- [published-e49dc95.txt](validation/published-e49dc95.txt)
- [published-e6a5395.txt](validation/published-e6a5395.txt)
- [published-edea484.txt](validation/published-edea484.txt)
- [published-f6c8cf8.txt](validation/published-f6c8cf8.txt)
- [published-ff475e6.txt](validation/published-ff475e6.txt)
- [published-picker-1b453ab.txt](validation/published-picker-1b453ab.txt)
- [quality-0e4ee4a.txt](validation/quality-0e4ee4a.txt)
- [quality-18e1ec4.txt](validation/quality-18e1ec4.txt)
- [quality-1ceb567.txt](validation/quality-1ceb567.txt)
- [quality-1f38787.txt](validation/quality-1f38787.txt)
- [quality-1f4dd88.txt](validation/quality-1f4dd88.txt)
- [quality-25b84dc.txt](validation/quality-25b84dc.txt)
- [quality-2af3f39.txt](validation/quality-2af3f39.txt)
- [quality-3605e0b.txt](validation/quality-3605e0b.txt)
- [quality-3c08749.txt](validation/quality-3c08749.txt)
- [quality-3d05c2a.txt](validation/quality-3d05c2a.txt)
- [quality-41de104.txt](validation/quality-41de104.txt)
- [quality-55f78c3.txt](validation/quality-55f78c3.txt)
- [quality-593bbb1.txt](validation/quality-593bbb1.txt)
- [quality-6061cc5.txt](validation/quality-6061cc5.txt)
- [quality-6e023e8.txt](validation/quality-6e023e8.txt)
- [quality-75ef24f.txt](validation/quality-75ef24f.txt)
- [quality-76c36ba.txt](validation/quality-76c36ba.txt)
- [quality-85531a4.txt](validation/quality-85531a4.txt)
- [quality-87721c1.txt](validation/quality-87721c1.txt)
- [quality-882a5af.txt](validation/quality-882a5af.txt)
- [quality-8e4e15b.txt](validation/quality-8e4e15b.txt)
- [quality-8e79c46.txt](validation/quality-8e79c46.txt)
- [quality-8f1497a.txt](validation/quality-8f1497a.txt)
- [quality-9741448.txt](validation/quality-9741448.txt)
- [quality-9769858.txt](validation/quality-9769858.txt)
- [quality-a5d5f23.txt](validation/quality-a5d5f23.txt)
- [quality-a679582.txt](validation/quality-a679582.txt)
- [quality-bfb9f4f.txt](validation/quality-bfb9f4f.txt)
- [quality-c1cc854.txt](validation/quality-c1cc854.txt)
- [quality-c36210d.txt](validation/quality-c36210d.txt)
- [quality-c8d5893.txt](validation/quality-c8d5893.txt)
- [quality-c965b3d.txt](validation/quality-c965b3d.txt)
- [quality-cf11f39.txt](validation/quality-cf11f39.txt)
- [quality-cf513f5.txt](validation/quality-cf513f5.txt)
- [quality-d55aeb7.txt](validation/quality-d55aeb7.txt)
- [quality-d564c70.txt](validation/quality-d564c70.txt)
- [quality-d6fe7b0.txt](validation/quality-d6fe7b0.txt)
- [quality-f248d6e.txt](validation/quality-f248d6e.txt)
- [quality-pointer-clicks-2c524e4.txt](validation/quality-pointer-clicks-2c524e4.txt)
- [questionnaire-a11y-before-f19c062.txt](validation/questionnaire-a11y-before-f19c062.txt)
- [questionnaire-badge-before-761fff8.txt](validation/questionnaire-badge-before-761fff8.txt)
- [questionnaire-focus-before-5914945.txt](validation/questionnaire-focus-before-5914945.txt)
- [questionnaire-inherited-size-asan-tests.txt](validation/questionnaire-inherited-size-asan-tests.txt)
- [questionnaire-inherited-size-before.txt](validation/questionnaire-inherited-size-before.txt)
- [questionnaire-inherited-size-quality.txt](validation/questionnaire-inherited-size-quality.txt)
- [questionnaire-inherited-size-tests.txt](validation/questionnaire-inherited-size-tests.txt)
- [questionnaire-root-asan-tests.txt](validation/questionnaire-root-asan-tests.txt)
- [questionnaire-root-before.txt](validation/questionnaire-root-before.txt)
- [questionnaire-root-quality.txt](validation/questionnaire-root-quality.txt)
- [questionnaire-root-tests.txt](validation/questionnaire-root-tests.txt)
- [radio-failing-before-3d05c2a.txt](validation/radio-failing-before-3d05c2a.txt)
- [release-headless.txt](validation/release-headless.txt)
- [release-resumed.txt](validation/release-resumed.txt)
- [root-overlay-before-fix.txt](validation/root-overlay-before-fix.txt)
- [sanitizers-18e1ec4.txt](validation/sanitizers-18e1ec4.txt)
- [sanitizers-1da5aca.txt](validation/sanitizers-1da5aca.txt)
- [sanitizers-1f38787.txt](validation/sanitizers-1f38787.txt)
- [sanitizers-25b84dc.txt](validation/sanitizers-25b84dc.txt)
- [sanitizers-283a328.txt](validation/sanitizers-283a328.txt)
- [sanitizers-2af3f39.txt](validation/sanitizers-2af3f39.txt)
- [sanitizers-30bde26.txt](validation/sanitizers-30bde26.txt)
- [sanitizers-3605e0b.txt](validation/sanitizers-3605e0b.txt)
- [sanitizers-3d05c2a.txt](validation/sanitizers-3d05c2a.txt)
- [sanitizers-3d769d8.txt](validation/sanitizers-3d769d8.txt)
- [sanitizers-41de104.txt](validation/sanitizers-41de104.txt)
- [sanitizers-46f31e6.txt](validation/sanitizers-46f31e6.txt)
- [sanitizers-50ffcc4.txt](validation/sanitizers-50ffcc4.txt)
- [sanitizers-55f78c3.txt](validation/sanitizers-55f78c3.txt)
- [sanitizers-593bbb1.txt](validation/sanitizers-593bbb1.txt)
- [sanitizers-71f6695.txt](validation/sanitizers-71f6695.txt)
- [sanitizers-75ef24f.txt](validation/sanitizers-75ef24f.txt)
- [sanitizers-76c36ba.txt](validation/sanitizers-76c36ba.txt)
- [sanitizers-87721c1.txt](validation/sanitizers-87721c1.txt)
- [sanitizers-882a5af.txt](validation/sanitizers-882a5af.txt)
- [sanitizers-8e4e15b.txt](validation/sanitizers-8e4e15b.txt)
- [sanitizers-8e79c46.txt](validation/sanitizers-8e79c46.txt)
- [sanitizers-9741448.txt](validation/sanitizers-9741448.txt)
- [sanitizers-a679582.txt](validation/sanitizers-a679582.txt)
- [sanitizers-bb4d462.txt](validation/sanitizers-bb4d462.txt)
- [sanitizers-bfb9f4f.txt](validation/sanitizers-bfb9f4f.txt)
- [sanitizers-c1cc854.txt](validation/sanitizers-c1cc854.txt)
- [sanitizers-c8d5893.txt](validation/sanitizers-c8d5893.txt)
- [sanitizers-c965b3d.txt](validation/sanitizers-c965b3d.txt)
- [sanitizers-ceb6f72.txt](validation/sanitizers-ceb6f72.txt)
- [sanitizers-cf513f5.txt](validation/sanitizers-cf513f5.txt)
- [sanitizers-d55aeb7.txt](validation/sanitizers-d55aeb7.txt)
- [sanitizers-d564c70.txt](validation/sanitizers-d564c70.txt)
- [sanitizers-d6fe7b0.txt](validation/sanitizers-d6fe7b0.txt)
- [sanitizers-e5b414b.txt](validation/sanitizers-e5b414b.txt)
- [sanitizers-f100df2.txt](validation/sanitizers-f100df2.txt)
- [sanitizers-f248d6e.txt](validation/sanitizers-f248d6e.txt)
- [sanitizers-font-metrics-7fdfe35.txt](validation/sanitizers-font-metrics-7fdfe35.txt)
- [sanitizers-input-tree.txt](validation/sanitizers-input-tree.txt)
- [sanitizers-pickers.txt](validation/sanitizers-pickers.txt)
- [sanitizers-questionnaire-a11y-f19c062.txt](validation/sanitizers-questionnaire-a11y-f19c062.txt)
- [sanitizers-questionnaire-containers-74c9347.txt](validation/sanitizers-questionnaire-containers-74c9347.txt)
- [sanitizers-questionnaire-focus-5914945.txt](validation/sanitizers-questionnaire-focus-5914945.txt)
- [sanitizers-resumed.txt](validation/sanitizers-resumed.txt)
- [sanitizers-root-overlay-5ccee5a.txt](validation/sanitizers-root-overlay-5ccee5a.txt)
- [sanitizers-scroll-table.txt](validation/sanitizers-scroll-table.txt)
- [sanitizers-semantic-ancestry-fb4464b.txt](validation/sanitizers-semantic-ancestry-fb4464b.txt)
- [sanitizers-textview-0773cd4.txt](validation/sanitizers-textview-0773cd4.txt)
- [sanitizers.txt](validation/sanitizers.txt)
- [scroll-presentation.txt](validation/scroll-presentation.txt)
- [sheet-motion.txt](validation/sheet-motion.txt)
- [site-sharing.txt](validation/site-sharing.txt)
- [snippets-22061e5.txt](validation/snippets-22061e5.txt)
- [sokol-asan-xorg.txt](validation/sokol-asan-xorg.txt)
- [stepper-failing-before-9741448.txt](validation/stepper-failing-before-9741448.txt)
- [text-multi-click-asan.txt](validation/text-multi-click-asan.txt)
- [text-multi-click-before.txt](validation/text-multi-click-before.txt)
- [text-multi-click-native.txt](validation/text-multi-click-native.txt)
- [text-multi-click-quality.txt](validation/text-multi-click-quality.txt)
- [theme-textview.txt](validation/theme-textview.txt)
- [touch-mesh-chart-f7343a5.txt](validation/touch-mesh-chart-f7343a5.txt)
- [typed-dock-block-editor-726a730.txt](validation/typed-dock-block-editor-726a730.txt)
- [unicode-badge-160cee1.txt](validation/unicode-badge-160cee1.txt)
- [visual-3c08749.txt](validation/visual-3c08749.txt)
- [visual-3d05c2a.txt](validation/visual-3d05c2a.txt)
- [visual-8e4e15b.txt](validation/visual-8e4e15b.txt)
- [visual-cf11f39.txt](validation/visual-cf11f39.txt)
- [wasi-18e1ec4.txt](validation/wasi-18e1ec4.txt)
- [wasi-1c1322e.txt](validation/wasi-1c1322e.txt)
- [wasi-1da5aca.txt](validation/wasi-1da5aca.txt)
- [wasi-1f38787.txt](validation/wasi-1f38787.txt)
- [wasi-25b84dc.txt](validation/wasi-25b84dc.txt)
- [wasi-283a328.txt](validation/wasi-283a328.txt)
- [wasi-2af3f39.txt](validation/wasi-2af3f39.txt)
- [wasi-30bde26.txt](validation/wasi-30bde26.txt)
- [wasi-3605e0b.txt](validation/wasi-3605e0b.txt)
- [wasi-3d05c2a.txt](validation/wasi-3d05c2a.txt)
- [wasi-3d769d8.txt](validation/wasi-3d769d8.txt)
- [wasi-41de104.txt](validation/wasi-41de104.txt)
- [wasi-50ffcc4.txt](validation/wasi-50ffcc4.txt)
- [wasi-55f78c3.txt](validation/wasi-55f78c3.txt)
- [wasi-56-fuel-limit.txt](validation/wasi-56-fuel-limit.txt)
- [wasi-56.txt](validation/wasi-56.txt)
- [wasi-57.txt](validation/wasi-57.txt)
- [wasi-58.txt](validation/wasi-58.txt)
- [wasi-593bbb1.txt](validation/wasi-593bbb1.txt)
- [wasi-60.txt](validation/wasi-60.txt)
- [wasi-62-source.txt](validation/wasi-62-source.txt)
- [wasi-62.txt](validation/wasi-62.txt)
- [wasi-71a64d5.txt](validation/wasi-71a64d5.txt)
- [wasi-71f6695.txt](validation/wasi-71f6695.txt)
- [wasi-75ef24f.txt](validation/wasi-75ef24f.txt)
- [wasi-76c36ba.txt](validation/wasi-76c36ba.txt)
- [wasi-7970367.txt](validation/wasi-7970367.txt)
- [wasi-87721c1.txt](validation/wasi-87721c1.txt)
- [wasi-882a5af.txt](validation/wasi-882a5af.txt)
- [wasi-8e4e15b.txt](validation/wasi-8e4e15b.txt)
- [wasi-8e79c46.txt](validation/wasi-8e79c46.txt)
- [wasi-9741448.txt](validation/wasi-9741448.txt)
- [wasi-a679582.txt](validation/wasi-a679582.txt)
- [wasi-bb4d462.txt](validation/wasi-bb4d462.txt)
- [wasi-bfb9f4f.txt](validation/wasi-bfb9f4f.txt)
- [wasi-c1cc854.txt](validation/wasi-c1cc854.txt)
- [wasi-c8d5893.txt](validation/wasi-c8d5893.txt)
- [wasi-c965b3d.txt](validation/wasi-c965b3d.txt)
- [wasi-ceb6f72.txt](validation/wasi-ceb6f72.txt)
- [wasi-cf513f5.txt](validation/wasi-cf513f5.txt)
- [wasi-controls-753be98.txt](validation/wasi-controls-753be98.txt)
- [wasi-d55aeb7.txt](validation/wasi-d55aeb7.txt)
- [wasi-d564c70.txt](validation/wasi-d564c70.txt)
- [wasi-d6fe7b0.txt](validation/wasi-d6fe7b0.txt)
- [wasi-e5b414b.txt](validation/wasi-e5b414b.txt)
- [wasi-f100df2.txt](validation/wasi-f100df2.txt)
- [wasi-f248d6e.txt](validation/wasi-f248d6e.txt)
- [wasi-form.txt](validation/wasi-form.txt)
- [wasi-initial.txt](validation/wasi-initial.txt)
- [wasi-input-tree.txt](validation/wasi-input-tree.txt)
- [wasi-measurement-input-group.txt](validation/wasi-measurement-input-group.txt)
- [wasi-otp.txt](validation/wasi-otp.txt)
- [wasi-pickers.txt](validation/wasi-pickers.txt)
- [wasi-recovery.txt](validation/wasi-recovery.txt)
- [wasi-resumed-initial.txt](validation/wasi-resumed-initial.txt)
- [wasi-resumed.txt](validation/wasi-resumed.txt)
- [wasi-scroll-table.txt](validation/wasi-scroll-table.txt)
- [wasi-surfaces.txt](validation/wasi-surfaces.txt)
- [wasi-table-dropdown.txt](validation/wasi-table-dropdown.txt)
- [wasi.txt](validation/wasi.txt)
- [waveform-motion-sync-browser.txt](validation/waveform-motion-sync-browser.txt)
- [waveform-motion-sync-native-tests.txt](validation/waveform-motion-sync-native-tests.txt)
- [web-build-22061e5.txt](validation/web-build-22061e5.txt)
- [web-build-75ef24f.txt](validation/web-build-75ef24f.txt)
- [web-build-94503eb.txt](validation/web-build-94503eb.txt)
- [website-feedback-new.txt](validation/website-feedback-new.txt)

Commands: `cmake -S . -B build -G Ninja -DCUI_GPU_TESTS=ON`; `cmake --build build`; `ctest --test-dir build --output-on-failure`. ASan/UBSan config uses `-DCUI_SANITIZE=ON`; portable Clang 21.1.8 is supplied by Zig 0.16.0 with `-DCMAKE_LINK_DEPENDS_USE_LINKER=FALSE` to avoid a Zig/CMake linker dependency-file crash. This workaround is not required by ordinary GCC builds.

Linux Xorg actual runtime: SDL_Renderer OpenGL; SDL_GPU Vulkan reported Intel(R) Graphics (ARL), Intel open-source Mesa driver. Actual Vulkan and Sokol GLCORE pixel readback verifies nested clip/alpha/geometry; Sokol text cache/readback/eviction passed under ASan/UBSan. SDL dummy/software tests verify cache/clipboard/event service contracts; dummy is not OS platform evidence. C++ and Zig C-ABI consumer executables ran. Portable wasm32-wasi behavioral/embedding modules ran in pinned Wasmtime. Real Chromium browser tests separately exercise the WebAssembly gallery, including pointer/keyboard behavior, text input, popups and mobile-width bounds. These are separate validation paths. Native font/cache tests require a discovered real font.

## Captures and performance

Actual generated frames are in [screenshots](screenshots/): light gallery, dark text preview, narrow values, tiny-window resize fallback, SDL_GPU and Sokol. They are not borrowed upstream imagery. The browser gallery has also been captured from actual running CUI WebAssembly; no component mockups stand in for runtime evidence. No video captured.

CPU layout benchmark initial default unoptimized build on Intel Core Ultra 5 235U/Arch/GCC 16.2.1:500 frames × 100 boxes, 0.196 ms/frame CPU clock, 28000 visible commands after culling. This excludes shaping/rendering and is not a GPU frame or competitor benchmark. Keyed virtualization measurements/workload/memory are in VIRTUALIZATION.md. Reproduce with `make bench` and `./build/test_virtual --benchmark`.

## Platform matrix

Linux Xorg and headless: tested partial. Wayland: untested. Windows/macOS: hosted portable C suites run; results and coverage limits are in the platform matrix. Android/iOS: untested. Browser WASM: tested partial in Chromium with an optional Canvas/input host, independently of headless WASI. See [PLATFORM_MATRIX.md](PLATFORM_MATRIX.md) for current observed CI changes. SDL_GPU currently SPIR-V/Vulkan only; Metal/D3D shaders pending. Native screen-reader bridges absent.

## Risks and next tasks

1. Single-line Input now has caret/selection painting, scalar pointer placement, scrolling, host clipboard and bounded history. Complete multiline presentation, IME, graphemes, shaping clusters, paragraph bidi and font fallback before claiming Editor.
2. Nested/context menus, multiple selection, custom row delegates, pickers and disclosure/tabs have tested styled slices. Complete reference-specific behavior; dialogs, alerts and sheets now have tested styled composition and nested focus, with advanced surface behavior pending.
3. Tree, table and scroll widgets now use clipped virtual viewports and keyboard models. Complete reference-default audits, variable heights and advanced integration; table pinning/multiple selection and grouped headers now have tested implementations; Dock and Plot remain partial model stories.
4. Complete independent renderer/resource lifecycle/device-loss contracts, broader GPU shader formats, OS accessibility and native platform adapters.
5. Implement remaining catalog families against feature requirements, then add real stories/behavior/semantic/theme tests before verified status.

Licensing: original code MIT, Clay and Sokol zlib; optional renderer dependencies retain their own inventory/license. No GPUI code/runtime/branding copied. In-process capability checks would not be a security sandbox; no extension host yet.

## Browser gallery

[Open the GitHub Pages gallery](https://matteo.merola.co/cui/). All 78 entries remain navigable; only implemented stories have live previews. Models are marked as partial and no family is promoted to verified by a screenshot. Deployment is gated by Chromium behavior tests and preserves the existing account domain.

Build with `python scripts/build_web.py` using pinned Zig 0.16.0, then serve `build/web` over HTTP. See WEB_GALLERY.md for dependency and browser-test commands.

## Local commands

```sh
cmake -S . -B build -G Ninja
cmake --build build
ctest --test-dir build --output-on-failure
./build/cui_gallery
./build/cui_gpu_demo --font /path/to/font.ttf
./build/cui_sokol_demo --smoke 3
make lint format-check dist-check bench
python scripts/check_manifest.py
```

No dependency installation needed on the present host. Optional system CMake/Clang and Weston commands are in MAINTAINER_ACTIONS.md; no sudo or password prompts were used unattended. CI uses hosted-runner setup, not a personal token for builds.

## Every incomplete family

| Family | Current partial status | Remaining work |
|---|---|---|
| Accordion | tested | Controlled expansion, disabled/keyboard behavior, custom content, measured reversible reveal, dynamic content height, inert painted exits and reduced motion tested. Styled animation consumes theme response/damping/epsilon/travel, including under/overdamping and immediate snap. Complete inherited styling audit remains pending. |
| AlertDialog | tested | Partial surface behavior; see docs/SURFACES.md. Nested focus, validated deferred close, resize, measured wheel scrolling and pinned footer composition are tested. Automatic root overlay management, body scrollbar/focus reveal, custom part styles/backdrop, exact default margins and animated transitions remain pending. |
| Alert | tested | Severity text, title/message, banner sizing and controlled dismissal tested. Independent success/warning theme colors, rich content/action slots and native accessibility announcement pending. |
| Attachment | tested | Actual host-provided RGBA frames, five fits, grayscale/opacity, loading/failure/animation timing and Attachment lifecycle/actions are tested headlessly and in Chromium. Built-in encoded file/network decoding, shared resource cache, full inherited text styling and exact effects remain pending; not full parity. |
| Avatar | tested | Named UTF-8 initials fallback, circular themed surface and explicit size tested. Image loading/failure state, avatar groups, custom icon slots and exact typographic centering pending. |
| Badge | tested | Count, zero hiding, overflow, dot, configurable SVG icon, four sizes, colors and arbitrary content composition are headless tested; source corner geometry is covered on the first frame.; Generic inherited style refinement remains ordinary Core composition; browser-specific interaction and visual evidence is recorded separately. |
| Bubble | tested | Seven themed composable surface variants tested. Parent owns alignment/wrapping; built-in reactions, footer and complete message slot API pending. |
| Button | tested | Nine variants and source XS/S/M/L metrics, centered labels, SVG icon slots and icon-only buttons, controlled selected/loading/disabled states, text adornments and presentation callbacks are tested. Theme SVG providers can replace bundled artwork. Compact/custom variants, all upstream styling/tooltip interactions, exact loading presentation and native accessibility remain partial. |
| Checkbox | tested | Styled checked/unchecked geometry, four sizes, disabled colors, labels, custom content, and keyboard activation tested. Checkbox also exposes mixed state. Spring mark animation, wrapped inherited text styling, tooltip integration and exact pointer focus behavior remain pending; not full parity. |
| Clipboard | tested | SDL native/dummy roundtrip and optional capability adapter tested; core input paste/cut integration pending. |
| Collapsible | tested | Controlled expansion, disabled/keyboard behavior, custom content, measured reversible reveal, dynamic content height, inert painted exits and reduced motion tested. Styled animation consumes theme response/damping/epsilon/travel, including under/overdamping and immediate snap. Complete inherited styling audit remains pending. |
| DropdownButton | tested | Split actions, optional halves, tooltip, inner style inheritance, shared ghost state, corner anchors, joined geometry and eight semantic variants/four sizes tested. Remaining inherited Button style/metadata surface is under reference audit. |
| Empty | tested | Themed title/description layout and named semantic snapshot tested. Media/icon/action slots and complete upstream audit pending. |
| Focus Trap | tested | Core nested modal scopes and overlay state tested. No OS accessibility bridge or full modal relationship semantics. |
| GroupBox | tested | Composable title/content, themed border/layout and group semantic snapshot tested. Complete upstream audit and native accessibility bridge pending. |
| HoverCard | tested | Delayed hover/focus, interactive content retention, viewport portals/collision and Escape suppression tested. advanced focus ownership and complete native accessibility bridge pending. |
| Icon | tested | 106 default SVGs plus optional full1830 catalog raster-tested; named/path/raw source, size/transform, tint and immutable bounded caches implemented. NanoSVG supports a subset of resvg; automatic inherited text scopes and browser/native visual audit remain pending.; Icon is decorative by default; named image semantics must currently be supplied by the caller. Dedicated accessible/decorative API remains pending. |
| Image | tested | Actual host-provided RGBA frames, five fits, grayscale/opacity, loading/failure/animation timing and Attachment lifecycle/actions are tested headlessly and in Chromium. Built-in encoded file/network decoding, shared resource cache, full inherited text styling and exact effects remain pending; not full parity. |
| Kbd | tested | Platform modifier formatting and styled UTF-8-safe hint tested. Action/keybinding lookup and grouped shortcut composition pending. |
| Label | implemented | Text wrapper renders, but control association and standalone semantic labeling pending. |
| Marker | tested | Text wrapping, inherited font styling and arbitrary separator styling remain incomplete. Custom child widths/layout are application supplied. Default shimmer now uses alpha-aware Oklab and the source lightness fallback. |
| Message | tested | Built-in Image/TextView/Attachment content is pending; custom content callbacks are supported. Exact inherited font weight, rounded avatar clipping, arbitrary style refinements and jump-button tooltip/variants remain incomplete. Browser stories are pending. |
| MessageScroller | tested | Built-in Image/TextView/Attachment content is pending; custom content callbacks are supported. Exact inherited font weight, rounded avatar clipping, arbitrary style refinements and jump-button tooltip/variants remain incomplete. Browser stories are pending. |
| Notification | tested | Native OS delivery adapters remain absent; portable host callbacks are opt-in. Shadows approximate blur with bounded rounded layers; intent icons are text symbols and title font weight is not independently styled. Custom content and action descriptors remain borrowed through lifecycle callbacks. |
| Pagination | tested | Independent bounded one-based page model, keyboard/pointer first/last/next/previous, ellipsis window and SIZE_MAX bounds tested. Responsive wrapping, configurable label slots and arbitrary page budgets pending. |
| Progress | tested | ProgressCircle supports bounded caller-owned antialiased RGBA rendering; arbitrary vector/style refinement and the subpixel source path seam are not reproduced.; Arbitrary easing and theme motion-token overrides remain unsupported. |
| Radio | tested | Styled checked/unchecked geometry, four sizes, disabled colors, labels, custom content, and keyboard activation tested. Checkbox also exposes mixed state. Spring mark animation, wrapped inherited text styling, tooltip integration and exact pointer focus behavior remain pending; not full parity. |
| Rating | tested | Independent keyboard/pointer half-step values, controlled clearing, readonly/disabled and range semantics tested. SVG masks/custom icons/color and hover preview pending. |
| Shimmer | tested | Single-line text only; inherited styled runs, wrapping, ellipsis and font weight await rich text infrastructure. Caller supplies elapsed time and schedules base-plan deadlines. |
| Skeleton | tested | Placeholder dimensions/radii, secondary opacity, phase pulse and reduced motion tested. Loading owner semantics and richer placeholder layouts pending. |
| Spinner | tested | Themed eight-dot ring, explicit clock phase, redraw invalidation, reduced motion and status snapshot tested. Scheduler deadlines and alternative icon geometry pending. |
| Stepper | tested | Controlled current index, explicit pending/active/completed/error states, pointer/keyboard navigation and semantic snapshots tested. Validation callbacks, connectors/custom icons and exact density variants pending. |
| Switch | tested | Controlled activation tested; native switch geometry, theme variants and touch pending. |
| Tag | tested | Frozen Tag has no built-in close action; interactive children are application composition. Semantic secondary colors are independently configurable. Exact inherited typography remains incomplete. |
| Toggle | tested | Ghost/Outline variants, four sizes, controlled requests, disabled input, custom content, tooltip composition and segmented multi-select groups have headless coverage. Convenience groups retain exclusive/allow-empty/keyboard policies through the shared painter. Full inherited text/icon styling and source-level visual acceptance remain pending; not full parity. |
| Tooltip | tested | Delayed pointer/focus opening, collision placement, viewport portals, Escape suppression, disabled omission and trigger description tested. configurable layer policy and native accessibility bridge pending. |
| Calendar | tested | Actual one-to-three month Gregorian grid, ordered keyboard navigation, controlled date/range selection, disabled/min/max predicates and roving semantic days tested. Locale labels, week numbers, today markers, hover range previews and neighboring-day interaction remain pending. |
| ColorPicker | tested | Actual HSV plane, hue/alpha/RGBA sliders and palettes; pointer capture, keyboard edits, disabled/readonly and 20,000 conversion/hex round trips tested. Editable text channels, continuous gradients, eyedropper and popup presentation remain pending. |
| Combobox | tested | Editable UTF-8 query with measured caret, selection, pointer placement and horizontal scrolling; ASCII filtering; single/multiple selection; protected removable chips; and ordered text/activation/wheel commits are tested. Remote data, Unicode case folding, rich IME/grapheme editing and complete upstream behavior remain pending. |
| DatePicker | tested | Real collision-placed calendar portal, date/range selection, trapped Tab and same-frame focus restoration tested. Editable date masks, custom formats/locales, clear controls, presets and responsive scrolling remain pending. |
| Editor | tested | Styled multiline caret/selection/scroll, gutter/wrap, host syntax colors, ASCII-insensitive retained search and atomic replacement, visible match highlights, caller-owned folding and completion have headless/browser tests.; Unicode17 grapheme and word keyboard editing passes pinned conformance fixtures; explicit scalar APIs remain compatible.; Measured multicursor interaction supports cursor creation, rectangular Alt-drag selection, grapheme navigation, wraps/folds, readonly behavior, atomic typing, multirange indentation, synchronous host clipboard distribution and full-selection Undo/Redo. Grouped typing/backspace/forward-delete undo, the 1,000-change boundary, caller-owned preferred columns and history eviction have headless coverage. Multicursor IME, parsers/LSP, diagnostics and bidi remain incomplete. Folds require host refresh after edits; browser clipboard does not yet expose the synchronous native seam.; 200,000-line document correctness is tested; performance targets and viewport-only incremental layout are not yet validated in this release. |
| Form | tested | Validation, submission and responsive column counts are application-owned as in the frozen reference.; Feature-level reference audit remains in progress; inherited Input text/IME/native accessibility limitations remain. |
| Input | tested | Single-line caret/selection paint, pointer positioning, horizontal scrolling, disabled/readonly semantics, synchronous host clipboard and bounded history have tests. Single-selection IME preedit/commit/cancel and one-step Undo pass headless, injected SDL and native Chromium composition tests. Physical OS candidate windows, mobile keyboards, bidi caret geometry and asynchronous browser clipboard remain unverified.; Design sizes and centered field text are tested. Native shaping is separate from browser Canvas; complete cluster hit testing and native menu behavior remain partial. See docs/INPUT.md and docs/IME.md. |
| Input Group | tested | Reference audit remains in progress; actual gallery composition and state tests exist.; Uses retained single-line Input; multiline, IME, grapheme and native accessibility limitations remain. |
| NumberInput | tested | Independent headless and styled finite numeric editing, draft validation, CJK normalization, locale-aware ASCII parsing/formatting, bounds, steps, readonly/disabled and semantics tested. Thousands separators, full custom pattern/appearance, caret/selection painting, pointer positioning and IME pending.; Source XS/S/M/L/custom size metrics and centered field text are tested; native shaping/IME/menu and remaining detailed editing behavior are still partial. See docs/INPUT.md and docs/NUMERIC.md. |
| OtpInput | tested | Actual gallery integration and feature-level audit remain in progress.; Host schedules caret blink deadlines. OS OTP autofill and IME bridges are absent. Frozen source does not provide bulk paste or caret-arrow editing; CUI does not infer these features. |
| Questionnaire | implemented | Retained answers, validation, delayed confirmation and initial composable graphical parts have targeted tests. Explicit scoped letter/number shortcuts, composition guards, filled-answer focus restoration, card palette/size styling and custom indicator/content/shortcut renderers are tested. Automatic root keyboard capture, cross-item focus transfer, slot style refinements, remaining styling, localization and full accessibility remain unfinished. The real browser story covers validation, delayed navigation, freeform submission, reset, disabled state, four sizes, letter/number shortcuts and composition commits; see docs/QUESTIONNAIRE.md. |
| Select | tested | Controlled single/multiple selection, removable protected chips, ordered activations, keyboard/typeahead, collision placement and modal focus/restore tested. remote loading, full virtualization remain pending. |
| Slider | tested | Capture drag, keyboard step/clamp and disabled tests pass; tick/handle geometry and touch pending. |
| Speech | tested | Injected-provider lifecycle, bounded copied report queue, transcript handling, level meter and graphical controls are tested. Real microphone capture, format conversion, platform recognizers, fallback, event-loop integration and live-device verification remain unfinished; see docs/SPEECH.md. |
| Textarea | tested | Measured wrap, selection, clipboard, history, design sizes, auto-grow and atomic indentation have headless tests. Single-selection composition and precise touch panning have headless/browser regressions; default composition snapshots grow with the actual document. Bidi caret/selection geometry, automatic wrapping context and complete first-frame growth acceptance remain unfinished; see docs/TEXTAREA.md and docs/IME.md. |
| TimeField | tested | Controlled 12/24-hour segmented hour/minute/optional-second model, persistent digit drafts, AM/PM, strict ASCII parsing and readonly/disabled behavior tested. Min/max times, typed separators, localization and IME remain pending. |
| Carousel | tested | Controlled variable extents, circular repositioning, spring motion, pointer axis-lock, wheel/precise phases, nested scroll deferral and actual Input-child behavior tested. Caller-supplied outer extents, first growing-width estimate and host gesture/tooltip services remain explicit boundaries; see docs/CAROUSEL.md. |
| DescriptionList | tested | Headless layout and passive semantics tested across sizes/themes; browser/native visual audit pending. Outer clipping is rectangular; independent cell clipping, inherited rich typography and Text values remain partial. |
| Dialog | tested | Partial surface behavior; see docs/SURFACES.md. Nested focus, validated deferred close, resize, measured wheel scrolling and pinned footer composition are tested. Automatic root overlay management, body scrollbar/focus reveal, custom part styles/backdrop, exact default margins and animated transitions remain pending. |
| Dock | tested | Styled tabs, menus, bulk close, zoom placement, nested resize, independent edge regions and atomic cross-region transfers are headless tested.; Opt-in collapsed bottom tab strip and real resize reopening match the frozen bottom-edge behavior.; Canonical n-ary layouts, root policies, locks, visibility, title palettes, typed PanelState/DockAreaState atomic persistence and full-state factory reconstruction are tested subsets. Workspace dumps prefer matching rendered split sizes, then retained sizes, then source minimum. Owned descriptor aggregates, explicit reconciliation, coalesced lifecycle ticks, completed layout bounds, edge operations and atomic zoom rollback have tests. JSON parsing and automatic framework lifecycle/prepaint delivery remain incomplete. See DOCK.md frozen export audit. |
| List | tested | Stable keyed single/multiple selection, per-event activations, roving keyboard/typeahead, disabled items, semantic options and bounded visible window tested. remote data, full virtualization remain pending. |
| Menu | tested | Actual flat/nested menus, check/radio, separators, text adornments, custom row delegates, context opening with modal guard, bounded collision placement and immediate focus restoration tested. OS shortcuts/link activation, arbitrary nested viewport scrolling and complete upstream behavior remain pending. |
| Popover | tested | Partial surface behavior; see docs/SURFACES.md for exact upstream gaps. Automatic root overlay management, independent scrolling with pinned footer, frame-independent close requests, customizable part styles and transitions remain pending.; Current side-based flip/clamp differs from upstream anchor-only clamp. Complete anchor/offset/arrow, right-click trigger and accessible trigger relationships are pending. |
| Resizable | tested | Caller-bounded panels, constraints, nesting, persistence, captured drag and keyboard handles tested. Browser/native audit pending. Edge pill follows ancestor clipping; motion duration/easing supplied explicitly; no native GPUI drag payload adapter. |
| Root View | tested | Per-window plugin snapshots, ordered render stages, field-specific style overrides, factory rollback, automatic Sheet/Dialog/Notification layers and Sheet-aware notification placement are tested. Window creation, entity observations, global selection/copy service, tooltip/native-menu/touch-selection hosts and automatic chrome remain incomplete; see docs/ROOT.md. |
| Scrollable | tested | Actual clipped two-axis content, nested wheel routing and keyboard/pointer scrollbars tested. Shared contact-aware precise touch scrolling, threshold/cancel handling, nested ancestor handoff and drag-priority controls have headless and Chromium touch tests. Auto-measured/grow sizing, inertial fling, auto-hide, physical-device/native-system behavior and complete family touch coverage remain pending. |
| Settings | tested | Searchable pages/groups, atomic filtered reset, source-size Input/NumberInput, checked Menu dropdown composition and reset tooltip are tested. Unicode search, virtualization, rich descriptions, unconstrained oversized Menu layout and complete host styling remain partial; see docs/SETTINGS.md. |
| Sheet | tested | Partial surface behavior; see docs/SURFACES.md. Nested focus, validated deferred close, resize, measured wheel scrolling and pinned footer composition are tested. Automatic root overlay management, body scrollbar/focus reveal, custom part styles/backdrop, exact default margins and animated transitions remain pending. |
| Sidebar | tested | Variable-height bounded navigation, nested items, independent carets, callbacks and animated Icon/Offcanvas collapse are tested. Full font-weight styling, intrinsic virtual measurement and host popup placement remain partial; see docs/SIDEBAR.md. |
| StatusBar | tested | Pinned end regions, adaptive center alignment, themed spacing and callback composition tested headlessly. Independent region clipping, inherited rich font styles and full Styled refinement remain partial; browser/native audit pending. |
| Tabs | tested | Stable-ID selection, keyboard navigation, five variants, custom body slots/close composition, measured UTF-8 ellipsis, wheel scrolling, overflow menu, application-applied drag requests, explicit/selected reveal and interactive scrollbars tested. Custom body width caps preserve short content and clip oversized bodies from the first frame. Structured per-item slots and text shaping remain incomplete. Cross-strip drag/autoscroll are not APIs in the frozen source. |
| Theme | tested | 38 pinned named palettes, token derivation, themed button surfaces and CSS/WASM palette equality are tested. Website system appearance, persistence and live OS changes are tested. Native OS preference adapters, complete upstream theme tokens, gradient brushes, font and syntax themes remain partial; Aurora gradients use their first stop. See docs/THEME_PRESETS.md. |
| TitleBar | tested | Source geometry/gradient/window SVGs and mock-host pointer/keyboard routing tested. SDL decoration/query/close contracts tested on X11; minimize/maximize remain OS-asynchronous. Native drag/menu, macOS traffic-light placement/preferences and Windows control hit areas remain host integration gaps. Portable double-click thresholds substitute OS preferences; browser story pending. |
| Toolbar | tested | Source audit corrects initial left/center/right-slot assumption: Toolbar provides ordered sized/unsized children and labelled groups. Scoped wrapping arrow navigation, ordinary Tab, nested consumption and hosted Input caret tested. Callbacks receive explicit preparation metrics; custom-box replacement substitutes full Styled refinement; caller owns icon paint/tooltips. Browser/native visual audit pending. |
| Tree | tested | Actual bounded keyed hierarchy, controlled expand/select, ordered parent/child navigation, fractional virtual viewport and clipped wheel routing tested. Variable row heights, drag/drop, custom row slots, full context actions and native accessibility remain pending. |
| VirtualList | tested | Styled visible-window rendering, both orientations, variable extents, scrollbars/keyboard and Auto/Infer sizing are headless tested. Intrinsic measurement is an explicit host callback; row selection/controls remain application-owned and scroll positions use float coordinates. |
| Chart | tested | Basic rendering for seven chart families, bounded geometry, explicit Sankey topology, theme colors and pointer hover are tested. Source-complete axes, legends, tooltips, gradients, layout refinement and animation remain unfinished; see docs/CHART.md. |
| Command | tested | Grouped filtering, original item paths, wrapping keyboard selection, pointer confirmation, virtual rows, actual Input/Scroll and Dialog cancellation are tested. Unicode matching, arbitrary intrinsic wrapping, native Action/window lifecycle and deferred callbacks remain explicit host boundaries; see docs/COMMAND.md. |
| DataTable | tested | Actual keyed rows/columns, controlled sort, bounded multiple selection, pointer/keyboard resize/reorder, pinned left columns and row headers, virtual rows/columns, context requests, load-range signaling, multi-level group headers, whole-row/trailing callbacks and actual scrollbars tested. Complete reference-default, remote adapter and behavior audit remains in progress. |
| Plot | tested | Styled bitmap/axis composition and source-audited scales/paths are partial APIs, not complete GPUI Plot parity.; Software output is bounded and non-antialiased; applications own interaction, buffers and plot state.; Source scales/shapes, styled appearance/hover/tooltip composition and cached software fill/stroke with two-stop sRGB gradients are tested. Hosts explicitly provide current bounds, pointer occlusion, frame serials and mark callbacks. Oklab gradients, source-projected slash/checker patterns, floating-point paint caches, bounded concave fills and adaptive open butt/miter strokes have tests. Bounded multi-contour fills now support holes, even-odd/nonzero rules and crossings; extended strokes cover closed paths, caps, joins and dash splitting with native/sanitizer/WASI geometry tests. Exact Lyon topology, unbounded geometry, variable-width/automatic curved routing, broader scale/legend/selection behavior, automatic prepaint and styled native mesh integration remain incomplete. |
| Table | tested | Real themed rows/cells, static header/body/footer sections, captions and validated spans plus associated data-table behavior tested. Exhaustive alignment/sizing/style composition and complete upstream scope remain pending. |
| TextView | tested | Bounded Markdown/HTML subset, styled Unicode selection, links, tables, images, streaming fade and reveal are tested. Complete parser semantics, source-copy reconstruction, plugins, shared touch/window selection, BiDi and adaptive tables remain unfinished; see docs/TEXT_VIEW.md. |

## Publishing/CI checkpoint

Safe branch pushes are performed with ordinary Git; no force push/history rewrite. This report records the evidence commit above. Integrator updates publishing/CI observations in swarm-status.md and PLATFORM_MATRIX.md after each push.
