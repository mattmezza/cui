# Private counted text preparation

`src/text/cui_text_prepare_internal.h` is a maintainer-only recording seam, not a
public font API or renderer. No adapter advertises V2 background support. Full
physical glyph painting and all five adapters remain pending.

Initialize a zero `cui_text_owner_internal` with malloc-compatible aligned,
distinct-block allocation/free callbacks and the **remaining** transaction CPU and
metadata envelope; create `cui_text_prepare_session` with that owner. Call
`cui_text_prepare_line(session,font,TEXT_command,source_density)` synchronously.
Density is explicit1..16; future renderer chooses the approved q from target axes.
This records original source-grid positions, color/paint and tightly packed owned
glyph coverage. It does not perform bilinear resampling or target blending.

CUI layout/Bidi wrapper arrays, allocation prefixes and glyph coverage charge CPU;
glyph descriptors charge separate metadata. Every request is checked before its
allocator callback. No new CUI glyph-cache insertion occurs on cold V2 raster;
existing hot entries may be read. NULL-owner internal paths retain legacy public
allocation, cache insertion and fallback behavior. Font and Bidi public headers,
opaque object ABI and V1 renderer routes are unchanged.

Existing font state and newly allocated opaque FT/HB/Sheen/provider temporaries
are explicitly separate: no joint32MiB/process/RSS or arbitrary-provider time
promise. See [accounting decision](design/SOFTWARE_V2_FONT_SESSION.md) and the
public options comment. Every selected face/reference, layout, buffer and paragraph
is released through its original or counted owner on failure. No global allocator
swap, dependency replacement or application input/measure callback is involved.

Frame work limits:64KiB/full paragraph,256KiB aggregate repeated paragraph input,
64 faces,8192 shaping attempts,4096 segments (an empty line consumes one),65536 shaped glyphs,8M copied coverage
pixels; each bitmap <=4096/side and4M pixels. Native font size retains its existing
4096 logical limit. Opaque operations may allocate before output-dependent limits
can be checked. CUI budget failures and admission are sticky in the owner; use a
fresh owner for a new transaction after releasing the failed one.

Failure appends no partial glyphs for that command; earlier records remain only
for teardown after a sticky error. End releases owned records, not borrowed
paint-stop arrays. Borrowed font graphs and source resources must stay stable
through session end/render completion. Glyph callback rows are copied before
return, including checked negative pitch. Allocation/release callback reentrancy
cannot prepare or end the active session. Provider/dependency state side effects
are not rolled back; only future published target pixels are transactional.

Tests compile frozen original CUI text/Bidi/software source into separately
prefixed types/functions with their own fonts. Same-runtime old/new legacy output
and recorded glyph pixels compare exact active fields, coverage and RGBA bytes;
no pointer/padding/hash-only oracle. Bundled Inter tests include multiline/soft
breaks, tracking, contextual colors/gradient paint, Bold selection and16x source
raster; legacy cache/density/shaping tests supplement these. Every CUI allocation
and explicit dependency failure step is swept; budgets, alias/lifetime and retry
checks run headlessly. Bidi-disabled source is tested separately.

Frozen pre-refactor byte files additionally pin the captured toolchain, archive,
source, flags and font hashes. They are not universal FT raster goldens. Set
`CUI_TEXT_FROZEN_EXACT=1` only for that compatible dependency build; version numbers
alone do not prove matching hinting configuration. Other hosts must still pass the
independently compiled original-source comparison. The Python reference contract
always verifies immutable source/font/fixture hashes; never refresh expected bytes
from candidate code. Optional `--compare-reject` runs a real mutated-byte negative
check under the private-display test wrapper on the captured toolchain.

Local verification: strict C11/O2 with all warnings as errors compiled both Bidi
and no-Bidi candidate/original-source test pairs. Private-Xvfb focused tests passed
6/6 (recording pairs, legacy cache/density/shaping, real mutated-byte rejection);
`CUI_TEXT_FROZEN_EXACT=1` used the pinned bundled Inter files. An initial legacy
density invocation lacked the font environment and skipped; the complete rerun
with the explicit bundled font passed. The reference hash tests passed2/2 and
installed-skill tests passed18/18. Full current native CMake integration and other
host dependency builds remain integrator/CI gates. Failure injection covers every
CUI request and explicit dependency-call boundary, not opaque internal malloc.
