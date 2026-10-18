# Changelog

## 0.1.0-alpha.2 — preparation (not published)

- Adds implemented component event facades while retaining explicit typed and
  host-service exceptions; this does not imply uniform APIs or full binding parity.
- Fixes secure Styled InputGroup masking and owned IME composition lifetime, and
  same-frame IME caret ownership during Find-to-Editor focus transfer.
- Adds focused callback-contract and distribution compiler regressions.
- Makes human skill discovery concise, preserves full/JSON output and adds explicit
  partial Markdown section reads without clipping canonical limitations.
- Final source freeze, integrated checks and release artifact validation remain
  pending. No registry publication is implied.
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
