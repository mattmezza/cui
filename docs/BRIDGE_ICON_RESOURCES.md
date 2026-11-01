# Owned bridge Icon resources (unreleased source checkpoint)

This source-only slice provides reusable app-owned Icon state and copied resource
utilities. It has not been compiled or run. ABI sizes/offsets, constructor-byte
change, allocation behavior and native/WASM compatibility remain prepared tests,
not measured claims. Configured Badge/standalone configured Icon/Spinner
presentation and their five language facades remain pending. Legacy kind11 text
Badge and kind50 named Icon keep their render paths. All 78 families remain.

`cui_bridge_icon_versions()` bit0 advertises the complete resource group on the
existing supported owner-thread build. Default/shared WASM and NO_TLS are not
supported; explicit unshared WASM uses the existing Slider opt-in and needs final
module validation. `icon_record_size_v1` exposes only LIMITS3/INFO4/BITMAP5.
Future style/options/Badge record kinds return0; those declarations and Badge
capability await their real parser/render consumer, including debug-placeholder.

Create an app-scoped slot/generation handle with `icon_create_v1` and an initialized
limits record. Zero limits normalize to the C defaults **before** reservation:
256 KiB SVG, 1M pixels, 8 MiB cache,16 versions. The C ceilings remain1 MiB SVG,
16M pixels,64 MiB cache,128 versions. A lazy pool starts with8 slots and grows to128.
No pool allocation occurs on old routes; the opaque app gains one appended pointer
whose native/WASM byte cost still requires measurement. Native C callers must use
the matching app; coincident handles from different apps cannot be distinguished.
Foreign wrappers must strongly retain/check app identity before C calls.

Limits, info and copied-bitmap records are native-endian fixed-width records with
exact size/version1. No C struct is implicitly a little-endian network wire.
Getters require initialized headers and leave outputs unchanged on error. Strings
and SVG are copied strict UTF8 without embedded NUL; name/path <=4096 bytes, data
<=max_svg_bytes. EMPTY calls set_empty; NAME uses C catalog/alias lookup and its
canonical loader path; PATH retains its exact supplied key; DATA copies raw bytes.
Empty NAME reports EMPTY with exact NOT_FOUND in info and preserves old source;
empty PATH/DATA preserve C's empty behavior. No guessed name/path conversion.

The C-owned resource-local path map overrides embedded assets, with copied keys
and SVG, max128 entries. Put/remove/clear invalidate preparation and advance the
nonzero source revision. No foreign callback, file or network access occurs during
Core. Arbitrary deferred C loader callbacks are not exposed. Reload keeps the
source and marks it dirty. Prepare returns bridge OK plus the exact Icon error;
that includes NOT_FOUND/PARSE/LIMIT/ALLOCATION and does not promise a new bitmap.
After a failed prepare the C state can still expose its earlier active mask;
callers must check prepare's error before treating a bitmap as new artwork.

`icon_bitmap_copy_v1` copies packed straight RGBA8 rows and paint bounds; empty
state is canonical0. No returned pixel pointer borrows native storage. Catalog
name/path/default and embedded SVG utilities likewise copy. String required sizes
include NUL; bitmap required is pixels only. CAPACITY writes only required sizes
(both for catalog), never partial destinations. Known input/output ranges have
checked uintptr intervals and pairwise exclusions **before any writes**, including
capacity paths. Valid stable readable/writable storage and disjointness from the
entire opaque app/resource graph remain caller preconditions; this is not a full
validator for arbitrary native pointers. No aliasing model is inferred from JS.

Admission reserves sum cache quotas <=64 MiB. Source admission uses sum SVG quotas
plus retained copied source/map reservations <=8 MiB. It counts the bridge request
copy and the native C source separately; shrinking max_svg never releases a still
owned source reservation. PATH/DATA reserve exact native length+NUL; NAME reserves
4097 bytes conservatively for C's canonical path without duplicating alias rules.
Replacement checks old+new bridge and native source copies while both may be live;
map replacement likewise needs old+new payload headroom. This bounds admitted
payload reservations, not parser/document/rasterizer/allocator overhead or total
process memory. Native allocation failure preserves prior owned source/map.

All app calls serialize on the original owner thread and outside step/render;
resource operations hold an owner busy guard. Destroy rejects references; current
resource-only clients cannot attach node references yet. Generations advance on
slot reuse and retire on wrap. Collect frees inactive masks only after the caller
has consumed all prior draws. Source replacement keeps old masks alive. Close
makes resource calls CLOSED; final app destroy releases resources once after
synchronous bridge rendering has returned. No asynchronous borrowed-list consumer
contract is added. `cui_bridge_destroy` during a resource operation is a no-op and
must be retried, consistent with retaining the outer operation's storage.

Prepared source fixtures cover independent ordinary Icon pixels, copied-source
replacement/old masks, catalog authority, map fallback/reload/parse error, zero
normalization, actual retained-source shrink and exact8 MiB boundaries, temporary
replacement headroom, stale/refcount/phase/busy/close, allocation failures and
known-range aliases/overflow on success and capacity paths. Compile-only native/
WASM app layout compares every old field with frozen `dbfb7945`; C/C++ public
record fixtures assert all offsets. Root owns CMake registration and canonical
metadata/generation. Acceptance must include unchanged legacy ABI/pixels/allocation
counts; no acceptance or complete configured Badge claim follows from this draft.
