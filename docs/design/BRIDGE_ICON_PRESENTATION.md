# Retained Icon presentation and Badge composition

The [resource checkpoint](ICON_BRIDGE_CHECKPOINT.md) established a bounded
app-local Icon pool. The [Badge audit](BADGE_FAMILY_COMPLETION_AUDIT.md) covers
the direct C component and its independent software and gallery evidence.
This decision records how the retained bridge will use them. The names here
describe the approved work. Use the public header to check availability.

## Reuse the same descriptor for Icon and Badge artwork

Start with an opt-in presentation for an existing Icon node. Copy the full
Icon options, including both pointed descriptors and the accessible name. Use
the ordinary C Icon painter with a retained reference to an app-owned resource.
Keep the resource group and the original named Icon painter available. An
unconfigured node still reports legacy rendering failures as before.

Use the earlier fixed-width, native-endian, pointer-free records for the full
Core box (128 bytes), constraints (32 bytes), and Icon options (256 bytes).
Presence flags distinguish zero width, height, opacity, color and constraints.
The existing resource size query keeps its original results. Add a separate
presentation version and size query so callers can negotiate the entire group
and keep working with older libraries. A copied result records node identity,
frame sequence, exact Icon error and bridge frame status independently. The
result is available only if the node was invoked in the most recent admitted
step and its configuration has stayed the same. Result reads have no effects.

The configured setter validates the record and stages its allocation and UTF-8
copy before modifying the node or reference counts. A resource cannot be
destroyed while configured nodes reference it. Removal or reset releases those
references. Existing owner-thread and synchronous renderer lifetime rules apply.
Resource cache collection remains explicit. Several differently sized consumers
may share a resource, so collecting after each invocation would invalidate masks
borrowed earlier in the frame. No additional allocation is required for legacy
nodes. The lazy presentation table lives under the existing private Icon pool
pointer, preserving bridge app and node layouts.

Configured Badge then adds an explicit container factory and uses the same
copied Icon descriptor. Count, dot and named-artwork modes need no owned
resource. The container submits ordinary children in their existing order
through the C Badge content callback. It reports structural submission and
artwork errors separately and preserves shell and child output on artwork
failure. Arbitrary foreign callbacks remain outside C composition.

Alternatives considered were a Badge-only reduced SVG descriptor, publishing
unused record IDs through the existing resource query, and creating a separate
renderer or resource cache per binding. Sharing the full descriptor carries
the actual C options across both retained consumers and requires one bridge
renderer integration. The independent query preserves prior resource ABI
observations. Legacy scalar Badge child admission and rendering stay intact.

## Acceptance

- Standalone Icon. Compare software output to the ordinary C painter, including
  transforms, inherited defaults, alpha zero, box style, explicit constraint
  limits, and named or decorative semantics. Exercise copied records and text,
  stale handles, reference replacement, removal, failed frames, omission and
  renderer errors. Verify C11/C++ and WASM record layout and frozen legacy paths.
- Configured Badge. Verify all three variants, count/max widths and zero cases,
  exact artwork and shell lifetime, zero-flow placement, nested child action
  ordering, one marker semantic, and an independently reported frame status.
- Bindings. Probe complete symbol groups and record sizes. Preserve old
  providers and reject foreign-app resources in the wrapper before entering C.
  Run real composed examples and validate from appropriate package staging.
  Direct arbitrary loader callbacks are a documented binding omission.
