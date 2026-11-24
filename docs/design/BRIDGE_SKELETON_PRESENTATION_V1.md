# Skeleton presentation in the retained bridge

The bridge's existing Skeleton kind is 35. Its fixed-size renderer uses
`cui_skeleton`, a two-second phase derived from Core's clock, and its own legacy
colors. Existing constructors and sizing calls keep this behavior. Configuring
a node explicitly selects the responsive presentation on that instance.

## Wire and ownership

Use two pointer-free V1 records, each with a 40-byte size and leading
`uint32_t struct_size, schema_version` fields. Both must match exactly. Query
version bits and both record sizes before wiring an optional language facade.

- Configuration has flags and width mode at offsets 8 and 12, followed by
  `float width, height, max_width, radius, duration` at offsets 16 through 32
  and numeric `0xRRGGBBAA` at offset 36. Fixed width accepts (0, 16384],
  fractional width accepts (0, 1], and grow uses a zero width field. Height
  accepts (0, 16384], max width and radius accept [0, 16384], and duration
  accepts [0, 60] seconds, where zero requests the source default of two.
  Width and height are logical pixels except for the fractional width. Zero
  max width leaves sizing unconstrained. Fractional mode requires zero max
  width because Core gives fractional sizing precedence over caps. Inactive
  color must be zero. Flags select secondary, custom color, and reduced motion.
- Presentation has the actual Core frame sequence at offset 8, Core frame time
  and absolute wake deadline at offsets 16 and 24, then sampled phase and
  opacity at offsets 32 and 36. Zero deadline means no wake requested by this
  Skeleton. Time values share the clock advanced by `cui_bridge_step`; the
  deadline is not wall time or a promise about native runner scheduling.

Calls use the existing app owner-thread and outside-step rules. Configuration
copies the record and owns an independent motion state. It uses one lazily
allocated table of at most 1,025 pointers and one record per configured live
node. Allocation and validation failures leave previous configuration intact.
Removing a node frees its record; clearing all nodes does not free the pointer
table until app destruction. Configuring a reused slot starts a new animation.

A successful configure invalidates the previous presentation query. Changing
period or reduced-motion policy restarts the pulse; restyling preserves the
clock. An omitted frame restarts the next submitted pulse. A failed frame
makes presentation unavailable, even if composition had sampled motion before
a later host rendering failure. A read copies the latest successfully stepped
and submitted sample; it never advances time or runs application code. Omitted
nodes have no sample. A failed read leaves its output untouched.

The renderer uses the active preset's owned Skeleton token or adapts the
bridge theme as a fallback. Secondary halves that token's alpha. Custom color
takes precedence, including transparent black. Core sizes the root and handles
clipping, border radii, and inherited opacity. Fixed height keeps the existing
registered-scroll constraints. Configured nodes use this configuration's sizing
fields. The legacy visual-size setter continues to serve unconfigured nodes and
reports invalid state on a configured node instead of storing an unused size.

No callbacks, semantic nodes or event queue entries are added. Applications
supply busy state and switch from placeholders to meaningful content. V1 keeps
built-in easing; direct synchronous C presentation remains the route for custom
easing and box refinements. Manual hosts may inspect the passive wake hint. The
built-in native bridge run loop keeps its existing cadence.

## Implementation order and evidence

1. Add the C ABI, copy boundary, node lifecycle and responsive rendering. Verify
   actual bridge software pixels, resize, reduced-motion timing, omission and
   retry after a failed render, slot reuse and legacy output. Probe exported
   record sizes through the public shared library.
2. Add Python and JS/TS optional-symbol authoring with real mounted app examples.
   Reject unsupported providers before creating UI, including partially
   implemented providers.
3. Add Go and Rust static/dynamic authoring with the same examples and
   compatibility checks. Update coverage as each facade receives runtime proof.

This extension has no dispatch changes, architectural migration, or new catalog
family. All 78 manifest entries stay intact.
