# Avatar failure and naming audit

Reference: GPUI Kit 0.7.1, commit
`87d10ae5e1299d1be18670a37570c5402f952e34`, inspected from the local
reference checkout recorded in `docs/upstream-baseline.md`.

`crates/base/src/avatar.rs`, `Avatar::render`, chooses its optional image slot
before its fallback slot. `AvatarImage::render` delegates to `gpui::Img`.
There is no Avatar load-state callback, error transition or retry mechanism.
The base tests assert image-slot precedence and fallback when the slot is absent;
they do not assert fallback after decoding failure.

`crates/component/src/avatar/avatar.rs`, `Avatar::render`, always constructs a
fallback and adds an image slot when `src` exists. Without `src`, a supplied name
selects initials and identity colors; without a name it selects the configurable
placeholder icon. This does not establish automatic failed-image fallback or
accessible-name behavior. GPUI image internals are outside this bounded audit.

CUI accepts host-decoded RGBA, validates it synchronously and paints initials
when the current source/cache cannot prepare. Preparation failure leaves a prior
cache intact; `cui_avatar_ex` gates rendering on the current return value rather
than the cache's `ready` flag. A valid source on a later frame recovers. No
production behavior change or speculative lifecycle API is needed.

`tests/unit/test_content.c`, `image_avatars`, exercises successful rendering,
missing and invalid-stride sources after success, preserved cache, initials draw
commands, software fallback pixels, full-name image/button and anonymous image semantics, and recovery of
the photo without unnecessary cache revision changes. Fallback text commands are asserted independently of the software fill checks,
which omit text to avoid a host-font dependency. CUI uses Anonymous for unnamed
extended avatars. Semantic names are checked before the next frame starts.

This slice does not establish complete Avatar parity. Configurable upstream icon
placeholders, identity colors and typography/size variants remain outside this
failure-and-naming evidence. There is no host image acquisition, decoder, network
retry, screen-reader integration or GPU coverage in these tests.
