# Live development checkpoint

## Current priorities — 13 October

Published source `fbbc40b` passes all six framework CI jobs and Pages; all339 HTTPS artifact files match. Actual published Attachment checks pass62/62 across all14 design sizes, both densities and orientations, mobile subtitle pixels and real touch access to offscreen actions. Next21 includes responsive Settings/Description List layouts, controlled RadioGroup, image avatar stacks, compact conversations, shared scrollbar appearance and optional relocatable Linux accessibility packaging. Its strict native/sanitizer suites pass103/103 and WASI83/83. The Message gate now accepts complete wrapped text while retaining visibility and interaction assertions.

Next22 is under integration: Checkbox springs/wrapping/tooltip, contextual native paragraph shaping and example callbacks, semantic Alert/Bubble slots, pointer-focus policy, Spinner/Skeleton scheduling, richer Button contracts and Table selection/metadata. Native112/112 passes after Table integration (4579dd4 plus generated headers and test registration); actual integrated Checkbox34 and Alert/Bubble checks pass, and Spinner/Skeleton finite wake/reduced-motion browser proof passes. Button missing bundled-icon choice is corrected; combined browser11/11 passes. Named Table sizing and selection-only DataTable action/sort browser proof also passes. The sanitizer111 sweep has one GPU driver leak failure; all110 other tests pass, and a targeted retry is being investigated. WASI90/90 passes at67afbff with portable SHA256897d6cd86368cd88c7948f8228fde26de1a453468b02b53a7af30a4aeab80318; later Button/Table changes need a fresh stable run. These pending checks are not success claims.

The feature ledger records271 tested subsets, four implemented subsets and46 not started features; full family verification remains0/78. Theme registry/configuration/motion continues in an isolated worktree. Only root integrates. No dependency installation is required from the user.

## Historical checkpoints

The entries below preserve earlier evidence and failures. Their story counts, status totals and pending work describe those historical revisions, not the current repository.

The first resumed published checkpoint `8d670694cb63d7074c1f59ec717bccf848d45ac1` passed all six framework CI jobs and the WebAssembly browser/build/deploy pipeline. [Framework CI](https://github.com/mattmezza/cui/actions/runs/38219053307); [GitHub Pages deployment](https://github.com/mattmezza/cui/actions/runs/38219053380). Ubuntu/macOS/Windows portable22/22 each; Linux GCC/Clang native sanitizer25/25 each plus Xvfb gallery and Sokol readback; style/manifest/license/distribution gates. Hosted Sokol uses llvmpipe software separately from the development-host Intel/Mesa GPU evidence.

The live gallery is https://matteo.merola.co/cui/ (existing account domain inherited by GitHub Pages). HTTPS is enforced. All78 entries appear in navigation; only actual implementations/models have live stories. Newer local stories and component changes are integrated and validated before the next ordinary push, with exact source evidence in validation/ and NIGHTLY_REPORT.md.

Current catalog:10 not_started,0 scaffolded,4 implemented,64 tested,0 verified. Tested means a documented subset, including partial base models. Full family parity remains unfinished and every gap stays in gpui-parity.json. Do not infer completion from rendered previews or tested API subsets.

Resumed work adds viewport portals, semantic relationships and roving focus, same-frame modal dismissal, per-event validated activation, content/hover surfaces, disclosure/tabs, flat menus/lists/selects, editable comboboxes and independently linked numeric/sequence behavior. Multiple selection, calendar/date/time and HSV/RGBA color controls are now integrated. Ongoing batches extend nested/context menus, tree virtualization and gallery stories.

Local validation includes native GPU/readback tests, ASan/UBSan, optimized portable Release, actual WASI runtimes and Chromium browser interactions. Numeric WASI testing exposed a locale-probe assumption: WASI can accept a locale name while retaining the C decimal radix. The initial failure log is preserved; the corrected probe now distinguishes locale availability from the requested radix, and WASI passed20/20 before pickers and22/22 with pickers. No production numerical checks are being removed.

No dependency installation or privileged setup is needed from the user on the current host. Native accessibility bridges, complete editing/IME/graphemes/bidi/fallback, images/compound families and broader GPU/platform adapters remain in the active scope.

Published source `3ef6c49` passed all six framework CI jobs, including portable34/34 on Windows/macOS, and the 42-story WebAssembly build/deploy. [CI](https://github.com/mattmezza/cui/actions/runs/38225532353), [Pages](https://github.com/mattmezza/cui/actions/runs/38225532495). An actual Chromium visit to the HTTPS public site found78 entries, instantiated CUI WASM and rendered DatePicker without browser errors; see deployed-42.txt. The preceding39-story deployment attempt failed a story-readiness race (preserved metadata); it was fixed with an explicit successful-frame readiness signal, rather than suppressing checks.

New local validation:39 native strict tests at input-painter integration,37 instrumented ASan/UBSan tests and30 actual WASI runtime suites at the preceding table/scroll source, and43 local browser stories. Ongoing isolated workers continue table multi-selection/pinning, compound editing/choice semantics, and full gallery coverage. The full parity goal remains active.

The current worker policy is GPT-6.1 Sol with low reasoning; GPT-6 Sol low is permitted only when 6.1 is at capacity. Earlier workers were interrupted before replacement, preserving their isolated worktrees and explicit ownership.

Latest published source `a6fc99b` passed all six framework CI jobs and the 46-story Pages build/deploy. Saved job metadata: validation/ci-a6fc99b.json and validation/pages-a6fc99b.json. Local surface integration passed41 native tests,39 ASan/UBSan tests and32 WASI runtime suites; tests cover nested modal focus, actual alert controls and all sheet edges. Surface gallery stories are being added. Subsequent Table group/row/trailing/scrollbar and DropdownButton work passes41 native tests, with runtime gates recorded separately. Full78-family parity is still unfinished.


Current integration: `ed10ae5` adds Form/Field composition and standalone base tests, the 51-story gallery, measured scrolling/fixed modal footers, bounded Sheet resizing, richer Table composition, DropdownButton and Tabs behavior, and renderer corner/border geometry. Native 43/43, ASan/UBSan 41/41 and Chromium 51-story tests pass locally; WASI 34/34 also passed. Manifest: 26 not started, 4 implemented partial, 48 tested partial, zero verified, all 78 preserved. Workers use GPT-6.1 Sol low; GPT-6 Sol low is only the capacity fallback. Active work remains Form/InputGroup/OTP, Choice/Disclosure reference behaviors and actual gallery coverage. Published deployment remains the earlier 46-story version until current release gates complete.


Deployment `afca033` is confirmed: all six framework CI jobs and Pages build/deploy succeeded. Direct HTTPS Chromium verification opened Dialog via its actual canvas button, observed 51 registered stories and no page errors; capture: `docs/screenshots/web-gallery-surfaces.png`. Subsequent local InputGroup and exact-zero/layout-only measurement updates pass native 43/43. Manifest now records 25 not started, 4 implemented partial, 49 tested partial, zero verified. No family is complete merely from the story count.


Integration `d7e0e18`: native45/45, ASan/UBSan43/43, WASI36/36 and Chromium53-story runtime gates passed. Form/InputGroup are real gallery stories; OTP APIs and tests integrated, with gallery work underway. Inert painted subtrees support safe exits, independent layout-only measurement supports natural body sizing, and Tabs have accessible scrollbars/reveal requests. Manifest: 24 not started, 4 implemented partial, 50 tested partial, zero verified. Active worker ownership: Choice/Disclosure; Gallery; new status module Tag/Marker/Notification (former forms worker). All workers remain Sol6.1low. Work continues toward full parity.


Integration `6657425`: native47/47, ASan/UBSan45/45, WASI38/38 and Chromium56-story gates passed. Tag/Marker and OTP have actual interactive gallery coverage. The expanded Choice suite required a finite30-billion instruction budget; the10-billion exhaustion log is preserved. Source daf43b9 passed six CI jobs but Pages failed same-frame InputGroup typing; focus proxies and a held-frame browser regression fix that route. The public site remains afca033/51 stories until deployment succeeds. Manifest:22 not started,4 implemented partial,52 tested partial,zero verified,all78 retained. Notification checkpoint and further Choice/Disclosure work continue on isolated Sol6.1low workers.


Deployment `0e61b2f` succeeded:56 live WASM stories and all78 catalog entries are publicly available. Direct HTTPS Chromium rendered OTP, Tag and Marker without page errors; screenshot `screenshots/web-gallery-marker.png`. Pages metadata and browser evidence are retained. Notification integration passes47 native tests; Message/MessageScroller, advanced gallery coverage and Choice/Disclosure refinements continue.


Integration `ef24642` passes47 native tests,45 ASan/UBSan tests,38 WASI suites and57 real Chromium stories. Notification is now a tested partial family with host delivery callbacks and real timer/stack browser tests. Core adds opt-in container focus, dominant-axis wheel routing and alpha-aware Oklab color interpolation; Notification and Marker consume corrected theme/color behavior. Manifest:21 not started,4 implemented partial,53 tested partial,zero verified. Public0e61b2f/56-story deployment and all six CI jobs passed;57-story deployment follows.


Integration `47dfd82`:49 native tests,47 ASan/UBSan tests,40 WASI suites and58 Chromium stories passed. Shimmer has a real story; Message/MessageScroller have measured headless behavior and pending browser stories. Disclosure gallery verifies animated reversal, inert exits, reduced motion and keyboard focus. Publishededea484/57 stories passed all six CI jobs, Pages, and actual HTTPS Canvas Notification creation. Manifest:18 not started,4 implemented partial,56 tested partial,zero verified. Root is adding real bitmap rendering; workers continue Image/Attachment, gallery coverage and exact Choice/Disclosure behaviors.

Current release candidate `b640c26`: native51/51, ASan/UBSan49/49, WASI42/42 and Chromium60-story checks pass. Checkbox/Radio paint real antialiased indicators through RGBA bitmap adapters; gallery defaults to system light/dark and follows changes unless manually overridden. Message/Scroller browser tests verify stable prepend anchors and Home remaining at zero after measurement. Image/Attachment are integrated and headlessly tested, with browser stories next. The previous a96fd34 Pages failure is retained; public site remains edea484 until new CI/deploy completes.16 not started,4 implemented partial,58 tested partial,zero verified. Workers use GPT-6.1 Sol low, with gallery switched to authorized GPT-6 Sol low after capacity failure. Full parity work continues.

Recovery `996e356`: native51/51, sanitizers49/49, WASI42/42 and Chromium60-story checks pass, with graphical Switch and Choice selection marks added. Pages157961a failed a stale-frame Dropdown browser assertion; repaired tests wait for naturally scheduled frames and preserve behavior assertions. Public still edea484 pending recovery deployment. Full parity work continues.

Published59a0988 confirmed: all six framework CI jobs and Pages build/deploy passed. Direct HTTPS Chromium verifies system-theme default/live changes,78 catalog entries and graphical Checkbox/Radio/Switch with no page errors. Screenshots and workflow metadata retained.62-story Image/Attachment work and Toggle improvements integrated locally; remaining parity work continues.

62-story release2a188c5 passes51native,49sanitizer,42WASI and62-story Chromium gates. Real Media, richer graphical controls, source Menu/Dropdown behavior, fractional typography, cursor and paint-order hit resolution are integrated. Public59a0988 remains60 stories until deployment. Resizable is under composition validation; Choice worker moves to new Icon module with pinned portable SVG/assets.16 not started,62 partial; no full parity claim.

Recovery71a64d5 passes56 native,54 sanitizer,47 WASI and62 real browser stories. DescriptionList, StatusBar, Resizable and SVG Icon are tested partial additions; Icon embeds106 licensed defaults with optional1830 catalog. Core adds proportional dimensions, consumed-event/scoped-focus composition and bounded resources. Browser recovery verifies actual Image readiness and physically unobscured notification clicks. Public59a0988 remains60 stories until Pages deploy. Toolbar checkpoint is ready to integrate; Sidebar and66-story gallery work continue; remaining text icon substitutes are being replaced through context-owned resources.12 not started,4 implemented partial,62 tested partial,zero verified.

Published e49dc95 confirmed: all six CI jobs and Pages build/deploy succeeded. Actual HTTPS Chromium verifies62 stories,78 catalog entries, system dark default/live light changes and Radio/Checkbox/Image/Attachment without page errors; Radio screenshot retained. Local67-story gallery adds DescriptionList, StatusBar, Resizable, Icon and Toolbar. TitleBar portable host/rendering and SDL action contracts now pass58 native tests,56 sanitizer suites and X11 service checks; its browser story is next. No full parity claim.


Release candidate `d6aa5bd`: root59/59 native,57/57 ASAN/UBSAN,49/49 WASI and68-story Chromium pass. Source button metrics/centering, Canvas baseline, expanded four-size selectors, theme SVG providers and icon-only dialog/attachment controls are integrated. Native/WASI source7970367, browserd6aa5bd; exact logs in validation/latest-checkpoint.json. Public62-story deployment remains e49dc95 until next release verification. Sidebar2247631 and Textarea0622a2d await root integration; workers continue Carousel,Settings and homepage/docs/search/copy-code.

### Isolated next integration: 75ef24f

Chart source plot extents/dashed hover guides and marks, TextView atomic inline Markdown/HTML images and linked-image selection/copy, and Questionnaire semantic containers plus badge background corrections are combined on `integration/next-parity`. Native 77/77, ASan/UBSan 75/75, and quality gates pass. WASI 67/67 and the strict browser WASM build also pass; browser runtime and publication remain pending. This branch is not the published site and does not establish full feature parity. Main remains held for the separate physical-click helper release repair.

### Validated integration bc295be

The Chart/TextView/Questionnaire batch from portable source75ef24f is now integrated on main together with the browser physical-click repair. Native77, sanitizers75, WASI67, quality, strict web build and full Chromium78 pass. The merged portable source, adapters, examples, build scripts and browser files match the tested integration branch exactly. Publication is pending; public04e044e remains the last verified HTTPS78 release. All78 entries remain in scope and full feature parity is not claimed.

### Next isolated batch: 2af3f39

Automatic Chart axis-label measurement, Bar/point-axis source geometry, atomic standalone Markdown/HTML image sizing and baseline flow, and Questionnaire answer description/enabled-position semantics pass native77, ASan/UBSan75, WASI67, full Chromium78, quality and strict web build. This batch remains isolated until release5608e51 HTTPS validation completes; it does not establish full feature parity.

### Verified publication5608e51 and next release2af3f39

Pages38295715735 and CI38295715752 succeeded. Full HTTPS78 behavior passes for5608e51; WASM, JS, CSS, HTML, catalog, themes and docs match its Pages artifact byte-for-byte. The next automatic-axis/atomic-image/Questionnaire-semantics batch2af3f39 is now integrated after native77, sanitizers75, WASI67, browser78, quality and web-build gates passed. Its publication remains pending; full78-entry feature parity is still incomplete.

### Verified public releaseff475e6

Pages38296393361 and CI38296393398 succeeded. Full HTTPS78 behavior passes; seven deployed assets including WASM match the artifact byte-for-byte. This is now the latest verified release. Isolatedfb4464b fixes unpainted-control semantic ancestry and Attachment hidden scrollbar semantics, with two failing-before regressions, native75 and targeted sanitizer2 passes. The gallery hierarchy update and further Chart/TextView parity work remain active; all78 entries remain in scope and no full feature parity is claimed.

### Validated docs/navigation integration 3c08749

Combined native77, Chromium78, targeted visual checks, docs3 and quality gates pass. New getting-started, state/callback and AI-agent guides include Copy buttons and downloadable llms indexes, full bundles and Markdown exports. Cropped images use isolated source bitmaps before smooth scaling, preserving crop/tint/clip pixel assertions. Portable source matches navigation a679582 with sanitizer75/WASI67 evidence. Publication remains pending; full parity and the feedback audit remain open.

### Validated component feedback batch cf11f39

Native77, sanitizer75, WASI67, Chromium78, targeted visuals and quality pass. Portable source matches593bbb1. Stepper, calendar/date picker, choice/Command width, InputGroup, Message fixture and AlertDialog fixes are integrated. Public e6a5395 remains HTTPS78/fifteen-file verified until this batch is deployed and checked. Full parity remains incomplete; DataTable ordering/header polish is isolated work in progress.


### Verified publication0b667d0 and active next21

All six framework CI jobs and Pages pass. Direct HTTPS checks match all325 artifact files; public alignment, wheel scrolling, route position, all14 legacy design sizes, Sidebar clipping, HoverCard/Toolbar/TitleBar and Python/Node clipboard recipes pass. DatePicker locale/year selection passes28/28. Next21 warning native101/101 includes optional installed accessibility packaging, photo Avatar stacks and compact Message examples. Control audit promotes13 narrowly tested features; ledger is263 tested,4 implemented,54 not started, with0/78 verified families. Shared scrollbar appearance and RadioGroup are active isolated workers; contextual international text editing remains pending.
