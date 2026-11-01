# Pure source-range seam before SDL native query

Proposal only, against accepted owned-frame code. No common/SDL production code
is changed. This resolves the ordering issue in `SDL_V2_BASELINE_CHECKPOINT.md`.
The SDL renderer may need native size/property queries to construct NULL-option
defaults. Those calls may invoke allocator callbacks, so known input aliases to
its busy byte/owner object must reject before temporarily claiming that guard.

## One active pointer grammar, two traversal policies

Proposed private common records/functions (no public types/platform dependency):

```c
typedef enum {
    CUI_CPU_SOURCE_IMAGE, CUI_CPU_SOURCE_MESH,
    CUI_CPU_SOURCE_TEXT_BYTES, CUI_CPU_SOURCE_TEXT_SPANS,
    CUI_CPU_SOURCE_TEXT_STOPS
} cui_cpu_v2_source_part;
/* Read only tag/scalar/pointer fields of an already accessible command record.
 * Inactive selector -> canonical NULL/0; failure leaves out untouched. */
cui_error cui_cpu_v2_source_range(const cui_draw_command_v2 *,
    cui_cpu_v2_source_part, cui_cpu_v2_exclusion *out);
/* NULL options means no options record; not fabricated dimensions/defaults.
 * No source bytes, vertex values, stops or spans are dereferenced. */
cui_error cui_cpu_v2_sources_exclude(const cui_draw_list_v2 *,
    const cui_render_v2_options *, const cui_cpu_v2_exclusion *, size_t count);
```

Range extraction is the sole authority for these pointer/extent selections:

| Selector | Active condition | Pointer / byte extent |
| --- | --- | --- |
| IMAGE | IMAGE tag and matching legacy type | bitmap pixels / stride * height |
| MESH | MESH tag and matching legacy type | vertices / vertex_count * sizeof(vertex) |
| TEXT_BYTES | TEXT | non-NULL paragraph.data selects entire paragraph, including length 0; otherwise raw text |
| TEXT_SPANS | TEXT with paragraph.data | text_colors / count * sizeof(span) |
| TEXT_STOPS | TEXT with LINEAR paint | stops / count * sizeof(stop) |

Other selectors return NULL/0 without reading inactive payload fields. BACKGROUND
is entirely owned by value; it must not be interpreted as legacy payload. POP has
no active borrowed fields. SOLID/NONE text paint ignores poisoned stop fields;
contextual source ignores poisoned raw text. Unsupported command kind or legacy
tag mismatch fails without inspecting inactive unions. This is finite typed
extraction, not a callback visitor or a generic renderer registry.

Range multiplication is checked against SIZE_MAX before forming a byte extent.
The extractor checks arithmetic, not font availability, UTF8, geometry, sampler
limits, source contents or aggregate work policy. It never reads pointees or calls
an allocator/provider. Its output is a caller-local scalar record, not retained
metadata. Pointer validity/accessibility is a caller precondition; this is not a
memory probing facility. Known overlap and address-end overflow reject through
the existing private disjoint primitive; whole opaque font/SDL graphs remain an
explicit disjoint/stable caller precondition.

## SDL-only prequery traversal and bounds

`sources_exclude` first rejects NULL list and invalid exclusion count/pointer
(max 7), then checks list record and optional options record against every owner
range **before reading either record**. No options fields/dimensions are needed.
It checks exact V2 header/stride/tag/error/output policy and count <= 16384, then
checked count*sizeof(command), NULL/count consistency and command-array overlap
before reading commands. Source records are borrowed and stable throughout.

It visits all five selectors per command (at most 81,920 constant-time extractions),
checking returned extents against bounded owner ranges. The SDL initial call needs
only its enclosing object range. At max 7 exclusions the conservative upper bound
is 573,440 nested range comparisons plus fixed top-level checks; no iteration over
resource bytes/vertices/spans/stops is performed. O(1) local storage, no heap,
font capability/shape/layout calls, SDL calls, input dispatch or numeric geometry.

Before native callbacks the new SDL traversal also rejects source-metadata work
that cannot be admitted: LINEAR stops 2..8, spans <= 4096 (only contextual text),
text <= 65536 bytes per command and 256 KiB aggregate, image stride*height overflow,
mesh vertex byte overflow and32 MiB inspected-work envelope. For contextual text,
compute text_bytes*(1+span_count) with checked arithmetic for the existing 32 MiB
repeated-paragraph inspection envelope; do not scan span contents or perform 4096
iterations merely to compute this admission. NULL/nonzero source mismatches and
malformed absent-paragraph metadata reject. Counts/extents are bounded before
aliases are traversed. Numeric/style/resource validity still belongs to full
validation after query; prequery success is not render support or a valid frame.

Failures precede any owner flag mutation, SDL allocation/query callback or output
write. Structural/tag/NULL/count-shape/overlap errors are INVALID_ACTION; exceeded
count/byte/work ceilings and byte arithmetic overflow are CAPACITY. Multiple
invalid fields have this new entry's explicit top-level/command/selector order;
that policy must not be retroactively imposed on the existing validator.

## Preserve old validator precedence exactly

Do **not** call the new whole-list `sources_exclude` entry at the start of the old
validator. Keep every existing geometry/style/font/capacity predicate, sequence,
short-circuit and error return at its original location. Replace only pointer/
extent extraction at each existing alias-check site with `source_range` for the
corresponding selector. In particular:

- Bitmap validity/crop/opacity checks still precede IMAGE alias checks.
- Mesh multiply/aggregate CAPACITY checks still precede mesh alias and vertex reads.
- TEXT font/context/style/count checks keep their order; the early stop-owner
  check remains before paint validation can read stops.
- Text byte/work checks still precede contextual source validation; span overlap
  stays before span contents; the raw metadata branch retains its short-circuit.
- Final stops/output-plan alias checks and final budget/output publication retain
  their current positions. The extractor cannot turn a prior INVALID into CAPACITY:
  at these sites its checked arithmetic has already passed the original guards.

The top-level list/options/array checks retain their existing sizes/order, using
the same disjoint primitive and unchanged pre-count/multiply guards. This shares
one active pointer grammar without sharing/reordering aggregate validation policy.
No legacy caller sees a native/provider callback or a new mutation point.
If any replacement cannot retain that precise ordering, stop and review that
specific case rather than widening the old validator's accepted inputs/errors.

## Alternatives and required proof

An external stack lock cannot be consulted by reentrant backend calls without a
new registry/owner indirection; storing its pointer in the backend merely moves
the same alias problem. A separately allocated lock adds default allocation and
ownership and still needs graph-disjoint preconditions. Caller-only nonreentry
weakens the reviewed runtime contract. Requiring explicit options/full preflight
first drops approved NULL defaults and does not prove fresh target dimensions.
Caching dimensions is stale after resize. None is preferable to the bounded seam.

Freeze old validator TU independently (already available), compare exact error/
output bytes for multi-invalid combinations at every ordering boundary, including
bad geometry plus aliased sources, invalid style plus overflow counts and malformed
context plus poisoned inactive raw text. Test zero-length active context, every
selector inactive poison, max/exceeded counts, SIZE_MAX/uintptr wrap, options/list/
array/nested owner aliases, unchanged flags/control/output and zero callbacks.
With a fake native-query hook, prove every owner alias rejects before its first
invocation; valid inputs reach it exactly once, guard is active during callbacks,
and a distinct-frame/same-owner attempt refuses. Then test full SDL/private-Xvfb
output/Retina/cache/ABI gates. No production implementation before root review.
