# CUI v1.0 readiness

CUI is a working alpha. A live gallery and passing tests demonstrate useful functionality; they do not establish production readiness or complete component parity. This assessment is dated 13 October 2026.

The catalog contains all 78 families: 76 tested subsets, two implemented families and zero fully verified families. The feature ledger contains 321 entries: 265 tested, four implemented and 52 not started. Features differ greatly in complexity, so these counts are not a completion percentage or a release-date estimate. The machine-readable component manifest remains the acceptance ledger.

## Release gates

| Gate | Current evidence | Required before v1.0 |
| --- | --- | --- |
| Component behavior | All 78 families have live examples and substantive APIs; tested subsets remain explicitly partial. | Close every feature gap, verify realistic usage, controlled state, keyboard, pointer, relevant touch, semantics and visual states for each family. |
| Touch and pointers | Shared controls expose hover, pressed, focus and activation. The published integration tests contact/device routing, precise touch scrolling, opt-in pan/pinch, cancellation, drag priority and broader cursor overrides. Physical-device/family coverage remains incomplete. | Test tap versus drag cancellation, nested scrolling, capture loss, overlays, multi-contact pan/pinch, disabled controls and device-specific cursor fallbacks. Zoom must require opt-in. |
| Text and editing | Unicode grapheme/word conformance, selection, multicursor edits, bounded grouped undo and cached large-document viewport work have tests. Single-selection composition across four text consumers plus Numeric, OTP and Combobox has integrated tests. Optional native bidi/hard-line shaping and explicit font fallback have tests. | Finish IME across remaining editable consumers, physical OS candidate windows/mobile keyboards, bidi caret/wrap integration and automatic font discovery, language-service integration and large-document end-to-end performance. |
| Rendering | Software, browser Canvas, Linux Sokol GL and SDL_GPU Vulkan have runtime evidence for supported capabilities. Native/software analytic path meshes have integrated runtime and pixel-test evidence for uniform paints; broader source paint parity remains unfinished. | Verify capability coverage, clipping, DPI, resource lifetime, recovery, visual consistency and native renderers on the advertised platforms. |
| Accessibility | Roles, state, names, actions and focus have headless semantic tests. The optional Linux AccessKit package passes native DBus and relocated CMake/pkg-config consumer tests; assistive-technology and broader platform acceptance remain open. | Implement and validate platform accessibility bridges with real assistive technologies; cover contrast, text scaling, reduced motion and touch targets. |
| Platforms | Portable library CI includes Linux, Windows and macOS; browser and Linux native runtime checks exist. | Native Windows/macOS, Linux display-server variants and Android/iOS runtime evidence. Compile-only evidence must remain labeled separately. |
| API and ownership | Portable C11 APIs, layered implementation, generated embedding headers and explicit memory contracts exist. | Review ABI/version policy, lifetimes, failure behavior, capacity limits and upgrade compatibility; finish the optional ecosystem promised by the scope. |
| Performance and safety | Warning builds, sanitizers, WASI and browser tests; selected measured workloads. | Reproducible startup, memory and frame-budget baselines, broad stress/fuzz coverage and regression thresholds on supported targets. |
| Documentation | Browsable guides, compiled C snippets, callback/state explanations and host-language examples. | Every shipped capability has accurate usage, ownership, limitations and reproduction instructions; examples must match released APIs. |

## How progress is reported

A tested feature is narrower than a verified family. A compile check is narrower than a native runtime check. A completed worker branch is narrower than an integrated and deployed release. Reports and the platform matrix preserve these distinctions.

The next milestones are consistent input across touch and pointer devices, complete heavyweight component behavior, international text and native accessibility, followed by platform acceptance and API stabilization. There is no defensible v1.0 date yet. Full catalog and ecosystem scope remains active rather than being reduced to reach a release label.

See [the component ledger](gpui-parity.json), [platform evidence](PLATFORM_MATRIX.md), [current report](NIGHTLY_REPORT.md) and [interaction feedback](website-feedback.md).
