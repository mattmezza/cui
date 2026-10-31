# Software V2 counted text session — architectural checkpoint

Design only, following `SOFTWARE_BACKGROUND_V2.md`, approved glyph policy
`bd97374e` and validator `f4e07861`. No text implementation/build/dependency change.
Recommendation: bound adapter-owned transactions and CUI text temporaries exactly;
explicitly separate the existing font/dependency runtime. Do not claim a joint
32MiB text-process limit. This changes the proposed budget wording and needs review.
Full text transforms, fallback/shaping/colored/gradient text remain required.

## Actual ownership and allocation evidence

| Existing path | Ownership and issue |
| --- | --- |
| `src/text/cui_text.c:119–151` | Font owns FT library/face and referenced HB shaper. File and borrowed-memory constructors use `FT_Init_FreeType`; public font is opaque. |
| `cui_text.c:44–83` | Lazy128-slot cache, <=16KiB/bitmap (~2MiB/face plus metadata). Miss allocates pixels/cache; allocation failure silently uses FT slot. Rendering can grow cache even if later painting fails. |
| `cui_text.c:293–305,375–407` | Non-Bidi creates HB buffer per shape; FT size/load/raster can allocate internally. This path does not check HB buffer allocation success after shaping. |
| `cui_text.c:475–810` | Bidi path allocates scalar/level/byte/mapping/mirror/script/run/segment arrays, layouts, face references, glyphs/clusters; retains shaped buffers for segments. Fallback attempts may allocate repeatedly. |
| `cui_text.c:831–885,1263–1344` | Raster uses selected/fallback faces and transient callback bitmaps. Contextual lines create/release geometry; colored rendering shapes before cluster-color selection. |
| `src/text/cui_bidi.c:44–107,130–225` | Caller workspace does NOT remove all allocations: paragraph/scalar ownership and SB algorithm/paragraph/line objects remain. |
| Sheen `Source/Core/Memory.c`, `API/SBAllocator.c` | Allocations AND frees dispatch through NULL/current global allocator; objects do not bind a per-session allocator. Temporarily swapping default can mismatch frees and races unrelated hosts. |
| `CMakeLists.txt:77–86` | Native FT/HB are host pkg-config dependencies, not a project-pinned private text runtime. This audit's installed HB15.0.0/FT2.15.0 are evidence, not new requirements. |

FreeType provides a custom allocator when constructing a NEW library; it cannot
retrofit the existing library's ownership. See its [memory record](https://freetype.org/freetype2/docs/reference/ft2-system_interface.html#ft_memoryrec)
and [library constructor](https://freetype.org/freetype2/docs/reference/ft2-module_management.html#ft_new_library).
HarfBuzz15 has [compile-time allocator hooks](https://github.com/harfbuzz/harfbuzz/blob/15.0.0/src/hb.hh#L506-L524),
not a per-buffer setter; [public allocation helpers](https://github.com/harfbuzz/harfbuzz/blob/15.0.0/src/hb-common.cc#L1115-L1161)
use that build choice. Preallocating buffers is not a bound on all shaping/font/cache
allocations. Vendored Sheen3.0.0 is pinned by `third_party/sheenbidi/manifest.json`
to cfe430e7375a7845b679adae9d51dac6deaa8858; its allocator is global.

## Recommended contract: three explicit accounting classes

1. **Existing font runtime:** caller-owned `cui_font`, families/fallbacks, FT/HB
   faces/shapers, existing glyph caches and dependency-owned reusable state.
   Kept alive/stable through synchronous render; not copied into the transaction.
   Existing cache payload ceiling is measurable, but FT/HB resident bytes are
   opaque. It is not legitimate to report their sum as zero or exactly bounded.
2. **Counted transaction:** scratch target, background/shadow storage, glyph
   coverage copies, CUI shaping/layout/Bidi wrapper arrays and descriptors.
   All requested payload/alignment bytes admitted BEFORE allocation/write, jointly
   within options.cpu_bytes<=32MiB; descriptor metadata separately<=1MiB. No new
   CUI allocation bypasses the owner. Fixed control/stack and allocator bookkeeping
   are separately documented, not a total-process/RSS guarantee.
3. **Dependency/provider work during preparation:** new FT load/raster storage,
   HB buffers/shaping/cache growth and SB internals use their existing allocators.
   Object lifetimes/failure paths are audited and released, but their byte peaks
   cannot be measured/enforced by the current CUI API. This exception is explicit,
   including newly allocated temporaries; it is NOT merely pre-existing memory.
   Bounded input/work admission below does not prove an opaque allocator bound.

An application requiring a hard joint dependency-memory ceiling cannot rely on
this renderer contract. Its support query must not advertise that guarantee.
`cpu_bytes` means class2 only; update `BACKGROUND_V2_RECORDS.md` and software guide
before implementation, rather than silently weakening the existing wording.
A successful render still requires every dependency operation to succeed and the
whole frame to fit class2. Failed preparation leaves published pixels unchanged;
font/cache/provider side effects need not be rolled back. No V1 fallback.

## Smallest private seam (proposed signatures, not existing APIs)

```c
/* Private to text/backend code; no public font/layout ABI or global allocator hook. */
typedef struct cui_text_prepare_session cui_text_prepare_session;
bool cui_text_prepare_line(cui_text_prepare_session *, cui_font *,
                           const cui_draw_command *, float source_density);
void cui_text_prepare_session_end(cui_text_prepare_session *);
```

Session borrows the counted workspace and glyph recorder; it never emits to the
published target. Refactor internal layout/line allocation and destruction helpers
to accept an explicit optional owner: NULL retains original V1 malloc/free/cache
behavior; V2 routes every CUI array/node/copy through the owner. No process-wide
malloc interception, SBAllocatorSetDefault, mutable font allocator or TLS allocator
swap. Public entry points continue calling the legacy route unchanged.

Use a transaction persistent region for target/BG/glyph copies and a per-command
scratch region for CUI layout arrays. Destroy all command layouts/dependency
objects/references before rewinding scratch. Allocation failure aborts that frame;
never reuse scratch while HB/SB or face references can still point into it.
Private glyph lookup may reuse already owned cache entries read-only; cold V2
raster bypasses cache INSERTION and copies the FT slot immediately. Thus no new
CUI glyph-cache allocation escapes the transaction; V1's fallback-on-cache-failure
remains unchanged. FT size/slot/HB reusable state may still mutate (class1/3).

Audit all HB create/add/shape/get paths for allocation failure, including the
non-Bidi path; check allocation status before recording glyphs. Destroy every
attempt/selected buffer on every exit. A private counted CUI path is not permission
to replace complex shaping/fallback with a basic approximation. Keep selection,
cluster offsets, paragraph context, metrics and paint order identical.

## Proposed bounded work admission (explicit rejection, not memory proof)

Before text preparation: <=64KiB per full paragraph, <=256KiB aggregate paragraph
input counted per invocation (including repeated contextual lines), <=64 distinct
selected/fallback faces per frame. Before each attempt: <=8192 shape attempts and
<=4096 script/Bidi segments per frame. After shaping and before copying: <=65536
glyph records and <=8M copied coverage pixels per frame, each bitmap side<=4096
and <=4M pixels. Count actual typed/aligned CUI arrays against class2 regardless of
these maxima. These proposed constants need review and independent boundary tests.

Some limits are necessarily detected AFTER an opaque operation: shaping can
expand before its glyph count is known; FT_Load_Glyph/Render_Glyph may allocate
before bitmap dimensions are returned. Preserve existing FT flags/hinting and
shaping semantics; do not pretend an output-size check bounded those allocations.
No worst-case wall-time guarantee for arbitrary fonts/providers. Fail before
publication, release dependencies, and permit a valid next call without replay.

Glyph callbacks copy validated rows before return (including checked signed pitch,
overflow, zero-sized glyphs and coordinates); they cannot keep slot pointers.
Apply the approved q/bilinear policy later from owned coverage. Native software
uses `cui_font` directly, not Core application measure/geometry callbacks. The
session does not replay input or application callbacks. Any future provider API
must state its own lifetime/side-effect contract; it is not implicitly supported.
Font graph mutations/concurrent close are forbidden through render completion,
as required by existing borrowed resource lifetime; do not claim new thread safety.
Reject reentrant session preparation/teardown; unwind acquired resources once.

## Alternative if a hard joint cap is mandatory

An optional isolated custom-allocator text stack could count FT via a new library,
HB via a privately built/namespaced custom-allocator dependency, and SB via an
explicit owner-propagating/private instance. It also needs font-source/face cloning,
state/cache identity parity, global/interned allocation lifetime proof, packaging
and symbol-isolation tests. Source FT streams and existing faces cannot simply
be rebound to a new allocator. No replacing/pinning host libraries, duplicate
runtime build or vendor patch is authorized by this checkpoint. This is materially
larger than the recommended private CUI session and needs a separate decision.

## Verification before full software support

Independent V1 unit/integer-phase font pixels; all approved density/coverage tests;
Bidi/fallback/contextual color/gradient ink and tracking; poisoned borrowed data,
work and byte admission boundaries. Inject every CUI session allocation failure
and HB/FT/SB operation failure with deterministic stubs, including late failures
and successful retry; no partial publication/leaks/double release. Instrument
cache misses to prove no V2 CUI cache growth and unchanged V1 behavior. Counters
must separately report class2 exact peaks and class3 unknown bytes, not a fake
joint32MiB figure. Actual bundled-font dependency runs supplement stubs. All five
adapters remain required; no text/renderer production is implemented here.
