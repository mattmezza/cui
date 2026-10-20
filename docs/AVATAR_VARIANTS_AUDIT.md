# Avatar palette, placeholder and metrics audit

Frozen reference: GPUI Kit 0.7.1 commit
`87d10ae5e1299d1be18670a37570c5402f952e34`.

Sources: `crates/component/src/avatar/avatar.rs`, `avatar/mod.rs`,
`website/component/avatar.md` and `website/base/primitives/avatar.md`.
The base guide describes parent-supplied loading/error fallback content; it does
not establish an automatic Avatar error transition.

The styled Avatar chooses an image when supplied, initials when a name exists,
and a configurable `Icon` placeholder otherwise. Its default icon is User.
Initials carry a 12-hue identity palette; photo/anonymous borders remain neutral.
Each palette hue is 30 degrees apart in OkLCH. Light palette triples
(background, foreground, border) are (0.97,0.032), (0.50,0.145), (0.89,0.05);
dark triples are (0.30,0.05), (0.82,0.11), (0.36,0.06). The hue index is selected
by private GPUI hashing of the short name. CUI must disclose a different stable
hash assignment if it does not port that implementation.

Frozen metrics are XS=16px with 0.65rem text; Small=24px with 0.75rem text;
Medium=48px with 0.875rem text; Large=80px with 1.875rem semibold text.
Custom `Size` sets the initials child dimensions to half the avatar dimensions;
it does not clearly specify half-size text. Do not claim such typography parity.
Additional CUI design-scale sizes must be identified as extensions.

Remaining broader differences include upstream Unicode uppercasing/initial
selection versus CUI first/last UTF-8 scalar initials, arbitrary GPUI style
refinements, and arbitrary base fallback children. This audit does not establish
complete Avatar or AvatarGroup parity.

Implemented opt-in mapping: `cui_avatar_options.design_size` reuses the shared
CUI scale. INHERIT retains existing `.size` behavior. Explicit XS/SM/BASE=MD/LG
use the frozen preset dimensions above. XL..9XL use twice shared control height,
with the overall scale capped at 512 divided by raw variant diameter; typography
uses shared font size times 30/18,
with semibold weight. The cap scales diameter and text together to preserve
their ratio, including 9XL at rem=16. These higher steps are CUI extensions. A positive rem
parameter scales presets; zero in options selects 16. Metrics helper rejects
invalid inputs without modifying output, while Avatar falls back to legacy
metrics for invalid options.

Identity colors use the frozen OkLCH triples and theme.dark. CUI deliberately
uses FNV-1a over its UTF-8 initials bytes, not an unverifiable GPUI hash clone.
An explicit 0..11 override supports host-controlled assignment. Explicit legacy
background/foreground options still win. Invalid hue overrides retain legacy
colors. The identity border applies only to named initials frames.

Anonymous SVG rendering is opt-in, with borrowed typed state, then named
context-cache lookup, then User as the default. The Icon contract determines
lookup/parse failures. A photo or supplied name wins; decorative SVGs add no
semantic duplication. Typed-mask lifetime and named-cache bounds stay those of
the existing Icon API. No network source acquisition is added.

Headless tests in `tests/unit/test_content.c`, `avatar_variants`, check all
light/dark palette hues for AA contrast after 8-bit encoding and non-clipped
border channels, hash stability across identical initials, rejected hue overrides,
all design sizes, explicit metrics precedence, rem scaling, typography weights,
legacy metrics/colors, typed/named/default icons, initials precedence, image
precedence, absence of duplicate semantics, and software pixels. The targeted
content CTest uses the isolated-display wrapper; no GPU or screen-reader claim
is implied. Generated SDK/skill metadata remains an integrator refresh step.

The scale-cap regression exercises every variant at rem 1, 16, 64 and 512,
checks the font-size/diameter ratio against uncapped small-rem metrics, and
verifies actual Avatar bounds and emitted text sizes. Large rem values cannot
leave oversized initials after the diameter reaches 512.
