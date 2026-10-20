# Changelog

## 0.1.0-alpha.3 — unreleased

- Adds opt-in date-only DatePicker presets, clear/Delete actions and an interactive
  desktop/touch gallery mode. Legacy date-only APIs remain available.
- Adds bounded Questionnaire copy-out snapshots, keyboard shortcut policies and
  persistent typed observation with explicit payload, nesting and lifetime rules.
- Adds an optional native FFmpeg/SDL video-audio example with a local bounded
  fixture; this does not imply general codec, device or platform acceptance.
- Fixes source-package configuration when documentation examples are absent, and
  checks DatePicker semantic values independently of C struct padding.
- Rebuild C consumers and bindings with matching headers and libraries:
  `cui_questionnaire_state` gained an observer pointer. Language builders do not
  expose the new C observer/payload arenas or DatePicker action options.
- Release artifacts and registry publication are not yet available for alpha.3.
  All 78 catalog families remain present; full feature parity is unfinished.

## 0.1.0-alpha.2 — 2026-10-20

- Adds implemented component event facades while retaining explicit typed and
  host-service exceptions; this does not imply uniform APIs or full binding parity.
- Fixes secure Styled InputGroup masking and owned IME composition lifetime, and
  same-frame IME caret ownership during Find-to-Editor focus transfer.
- Adds focused callback-contract and distribution compiler regressions.
- Adds call-local VirtualList final-offset and MessageScroller double-offset/tail
  observers, preserving legacy reduction and deferred-window semantics.
- Adds bounded Questionnaire box-slot styles and six translated labels; rebuild
  consumers for the extended by-value options structure.
- Makes human skill discovery concise, preserves full/JSON output and adds explicit
  partial Markdown section reads without clipping canonical limitations.
- Adds a real browser-decoded Video gallery extension with bounded WebM/MP4
  assets, CUI controls and explicit decoder/audio limitations; baseline remains 78.
- Published as GitHub release artifacts with validation receipts. PyPI, npm,
  crates.io publication and a public Go module tag remain separate work.
- Alpha.1 versions, tags and published assets remain immutable.

## 0.1.0-dev

Initial experimental C11/Clay foundation. No stable ABI or production-readiness commitment.

### Application-building and debugging

- Bounded first/latest diagnostics report operation, ID, frame, message, subsystem,
  capacity counters and open-box paths. Optional callbacks leave logging to the host.
- Opt-in text/JSON layout exports include box ancestry, requested sizing and resolved
  bounds/clips. Native bundle hosts expose `--debug-layout text|json`.
- Identity-bearing avatar/tag calls and relevant Core operations reject zero IDs
  explicitly; anonymous layout boxes remain supported. See `docs/IDS_AND_LIFETIMES.md`.
- `cui_text_ellipsis` resolves constrained single-line text in the current frame,
  preserves grapheme boundaries and keeps the full semantic label.
- `cui_icon_query` distinguishes unknown, nonembedded and embedded icons; an
  opt-in missing-icon outline preserves the original failure result.
- Native bundles support multiple application sources, custom targets, a two-file
  starter and a mail acceptance fixture. Guides cover layout, identity and debugging.
- Website documentation adds symptom search, matching public-header declarations,
  versioned agent exports and checksummed native source downloads.
