# Owned Icon resources and configured Badge: proposed bridge V1

Design only after `956a853c` / `NODE_CONFIGURED_BADGE.md`. No symbols, structs or
new node kinds below are implemented. Exact C authority remains
`cui_icon.h` and `cui_components.h:470–515`. All 78 manifest families stay intact;
this adds an internal bridge IR kind, not a component family. Root review precedes
production. Existing kind11 Badge/kind50 Icon and their default routes are frozen.

## Records: native C layout, never a network wire

Below U32/U64/F32 denote uint32_t/uint64_t/float; RGBA is U32 with R in bits0–7,
G8–15, B16–23, A24–31. Native scalar endianness applies; foreign LE encodings must
convert every scalar individually. No struct contains pointers, size_t, C enums
or bool. All records start exact `struct_size,schema_version=1`; getters require
those initialized headers. Unknown flags and reserved words reject. Proposed
sizes/offsets are derived, **not measured**; strict native/WASM32 assertions and
old-header ABI probes are required before approval to publish declarations.

```c
typedef struct { U32 slot, generation; } cui_bridge_icon_handle_v1; //8,align4
// style:128,align4; every cui_box field has a representation
struct cui_bridge_icon_style_v1 {
 U32 struct_size,schema_version,flags,reserved;              //0..15
 F32 width,height,padding,gap,radius;                       //16..35
 U32 background_rgba,border_rgba; F32 border_width;         //36,40,44
 U32 align_x,align_y; F32 offset_x,offset_y;                 //48..63
 F32 corners[4],border_edges[4];                           //64..95
 F32 max_width,max_height,opacity,padding_edges[4];         //96..123
 U32 reserved_tail;                                       //124
};
// flags bits0..11: row,clip,custom_corners,custom_border_edges,fixed_width,
// fixed_height,layout_only,inert,custom_opacity,width_percent,height_percent,
// custom_padding. Corners TL,TR,BL,BR; edges L,R,T,B. Align START0/CENTER1/END2.
struct cui_bridge_icon_constraints_v1 { //32,align4
 U32 struct_size,schema_version,flags,reserved;
 F32 min_width,min_height,max_width,max_height;
}; //flags bits0..3: explicit minW,minH,maxW,maxH, including maximum0
struct cui_bridge_icon_options_v1 { //256,align4
 U32 struct_size,schema_version,flags,size,design_size,reserved[3]; //0..31
 F32 custom_size,inherited_text_size,width,height,opacity,rem_size;//32..55
 U32 color_rgba,inherited_color_rgba;                             //56,60
 F32 transform[6];                                               //64..87
 struct cui_bridge_icon_style_v1 style;                           //88..215
 struct cui_bridge_icon_constraints_v1 constraints;               //216..247
 U32 reserved_tail[2];                                           //248..255
}; //flags bits0..8: customWidth,customHeight,customColor,customInheritedColor,
   //customOpacity,customTransform,decorative,hasStyle,hasConstraints
struct cui_bridge_icon_limits_v1 { //48,align8
 U32 struct_size,schema_version,reserved[2];
 U64 max_svg_bytes,max_pixels,max_cache_bytes,max_versions;
};
struct cui_bridge_icon_info_v1 { //80,align8
 U32 struct_size,schema_version,icon_error,reference_count;
 U64 parse_count,raster_count,cache_bytes,cache_versions,source_revision;
 U32 source_kind,flags; struct cui_bridge_icon_handle_v1 handle;
 U64 reserved;
}; //source_kind EMPTY0/NAME1/PATH2/DATA3; flags0; revision nonzero
struct cui_bridge_icon_bitmap_v1 { //64,align8; pixels copied separately
 U32 struct_size,schema_version,format,reserved; //format RGBA8=0
 U32 width,height,stride,reserved2;
 U64 required_bytes,revision; F32 paint_x,paint_y,paint_width,paint_height;
};
struct cui_bridge_badge_config_v1 { //64,align8
 U32 struct_size,schema_version,reserved[2];
 U32 variant,size,design_size,flags; U64 count,maximum;
 U32 color_rgba,reserved2; struct cui_bridge_icon_handle_v1 artwork;
}; //flags customColor bit0; artwork{0,0}=named legacy artwork, otherwise owned
struct cui_bridge_badge_result_v1 { //40,align8
 U32 struct_size,schema_version,submitted,icon_error;
 cui_bridge_node node; U64 frame_sequence; U32 frame_status,reserved;
};
```

Options cover every `cui_icon_options` scalar and both pointed descriptors;
accessible_name is a separately copied string. Style includes all cui_box fields,
even those Icon overrides; no second layout implementation. Size vocabulary is
INHERITED0/XSMALL1/SMALL2/MEDIUM3/LARGE4/CUSTOM5; errors are the explicit six Icon
values0..5, availability UNKNOWN0/CATALOG_NOT_EMBEDDED1/EMBEDDED2. Control/design
sizes use separately documented existing C vocabularies, not these Icon numbers.

## Exact proposed function family

All names in this section have prefix `cui_bridge_`; A=app*, H=icon_handle_v1,
N=existing cui_bridge_node. Except the U32 queries, functions return existing
cui_bridge_status. Pointer arguments are synchronous borrowed call arguments, not
wire members; all mutable source/config bytes are copied before returning.

- `icon_versions(void)->U32` bit0; `icon_record_size_v1(U32 record_kind)->U32`
  kinds STYLE0/CONSTRAINTS1/OPTIONS2/LIMITS3/INFO4/BITMAP5/BADGE_CONFIG6/BADGE_RESULT7,
  unknown=>0. Complete optional group required before any facade mount.
- `icon_create_v1(A,const limits*,H*)`; `icon_destroy_v1(A,H)`;
  `icon_get_info_v1(A,H,info*)`; `icon_set_limits_v1(A,H,const limits*)`;
  `icon_get_limits_v1(A,H,limits*)`; `icon_default_limits_v1(limits*)`.
- `icon_set_source_v1(A,H,U32 kind,const char*,U32 byte_length)`;
  `icon_get_source_v1(A,H,U32* kind,char*,U32 capacity,U32* required)`;
  `icon_reload_v1(A,H)`; `icon_collect_v1(A,H)`.
- `icon_asset_put_v1(A,H,const char* path,U32 path_bytes,const char* svg,U32 svg_bytes)`;
  `icon_asset_remove_v1(A,H,const char* path,U32 path_bytes)`;
  `icon_asset_clear_v1(A,H)`. These update a resource-local C-owned copied path map.
- `icon_catalog_count_v1(void)->U32`; `icon_catalog_entry_v1(U32 index,U32* is_default,
  char* name,U32 name_capacity,U32* name_required,char* path,U32 path_capacity,
  U32* path_required)`; `icon_query_v1(const char*,U32,U32* availability)`;
  `icon_asset_copy_v1(const char*,U32,char*,U32 capacity,U32* required)`.
- `icon_prepare_v1(A,H,F32 width,F32 height,const F32 transform[6],U32* icon_error)`;
  `icon_bitmap_copy_v1(A,H,bitmap*,unsigned char*,U32 capacity,U32* required)`.
  Prepare is outside frames only; bitmap is straight RGBA8, packed copied rows,
  no native pointer/stride borrow. Empty bitmap canonical width/height/stride/bytes0.
- `badge_configured_create_v1(A,N parent,const char* key,N*)`;
  `badge_configure_v1(A,N,const badge_config*,const options*,const char* name,U32,
  const char* named_icon,U32,const char* artwork_accessible_name,U32)`;
  `badge_get_config_v1(A,N,badge_config*,options*)`;
  `badge_copy_text_v1(A,N,U32 field,char*,U32,U32*)` (NAME0/NAMED_ICON1/ART_NAME2);
  `badge_get_result_v1(A,N,badge_result*)`.

Full Badge setter is atomic across records/strings/handle refcounts; config getter
returns native values, text getter copies. Count/maximum U64 reject >SIZE_MAX on
32-bit targets, never narrow. Existing make/remove hierarchy, generic child APIs,
design enum, node identity, ordinary event FIFO and poststep callback dispatch are
reused. Generic node text/visual setters reject this new kind when ambiguous;
explicit text setters above avoid accidentally changing the old scalar pill.
No new input routing or component manifest entry. New parent admission permits
ordinary children and nested configured Badges, preserving each child's rules.

## Validation, bounded owners and error semantics

All app operations serialize on owner thread, reject during step/render/provider
or teardown, and validate handle generation within the supplied app before state access.
The proposed8-byte handle is app-scoped: native C callers must retain the matching
app; equal slot/generation pairs from different apps are indistinguishable in C.
Foreign opaque resource wrappers retain their owner app and reject cross-app use
before C. A C-level guarantee against coincident foreign pairs would require an
additional owner nonce/registry and changed record sizes; that is not claimed or
smuggled into this proposal. Root may choose that alternative before freezing ABI.
Config floats/presence follow actual Icon active-field rules and native conversion
bounds, including opacity normalization and explicit maximum0; no NaN-as-presence.
Inactive descriptors are canonical all-zero except initialized descriptor headers;
inactive transforms/colors/scalars canonical zero. Strings use strict UTF8/no NUL:
name/path/accessible/key <=4096 bytes, SVG <=configured max_svg_bytes. Empty source
selects empty; unknown NAME returns operational NOT_FOUND, never silently PATH.

No new status enum: malformed/layout/overflow=>INVALID_ARGUMENT, invalid UTF8=>
INVALID_UTF8, stale=>STALE_HANDLE, phase/refcount=>INVALID_STATE,
allocation=>OUT_OF_MEMORY, resource budget=>CAPACITY, closed=>CLOSED. Catalog
out-of-range=>INVALID_ARGUMENT; unembedded/unknown asset copy=>EMPTY. Operational
prepare returns OK plus exact Icon error (including NOT_FOUND/PARSE/LIMIT/ALLOCATION);
BADGE likewise exposes icon_error separately. Set-source native failure uses the
same status mapping but retains old source; get-info preserves exact last error.
Output records remain byte-unchanged on failure. Copy CAPACITY changes only required
sizes (both required sizes for catalog); no partial destination writes. Required
string size includes terminating NUL; bitmap required is pixels only. Preflight
all output/input aliases and capacity arithmetic before copying or native mutation.

App-local Icon slots: default8,max128; lazily allocated only after explicit create.
Each resource uses C default limits (256KiB SVG/1M pixels/8MiB cache/16 versions),
C ceilings1MiB/16M/64MiB/128. Reserve sum configured cache quotas <=64MiB per app,
and max_svg quotas plus actual path-map bytes <=8MiB; max128 map entries/resource,
max1MiB SVG/entry. Reject overflow/cap before copying; replacement checks peak old+
new source/map copies, not just final bytes. This bounds owned source/mask payloads,
not total parser/document/allocator/dependency memory; no joint-process cap claim.
Limits/set-source/map replacement leave previous ownership intact on failure.
A resource map loader runs only C lookup: copied override first, false falls back
to embedded. Put/remove/clear increments source_revision and calls reload. No JS
callback or implicit file/network access. Arbitrary deferred C loader callbacks
remain unsupported; pre-frame service is a future separately reviewed extension.

Each configured node retains one handle ref, source/state reusable across nodes
within its app. Foreign cross-app wrappers reject before native calls.
Successful config replacement transfers refs once; node removal drops once;
destroy rejects reference_count>0 without invalidating handle. App closes release
nodes then resources after renderer consumption; a destroyed slot increments its
nonzero generation and retires permanently on wrap (never stale-handle reuse).
Masks survive all in-flight lists. Auto-collect only after synchronous renderer
returns; explicit collect/prepare/destroy rejects any bridge frame in progress.
Preparation failure does not authorize early collection. This assumes current
synchronous bridge render completion, not arbitrary external async list consumers.

Configured render calls cui_badge_ex or cui_badge_with_icon exactly once; the C
content callback recurses children once, no foreign call. Structural invalidity
has submitted=false and no content; artwork errors preserve submitted shell/content.
Store latest frame/result on invocation and finalize frame_status after submission;
getter requires invocation in the last attempted frame and unchanged config,
otherwise INVALID_STATE. submitted never promises a successful Core frame. No
result FIFO or accept/replay operation. Configured artwork stays decorative even
if its copied accessible name is nonempty; sole STATUS name is the Badge's name.

## Scope and gates before implementation acceptance

This family covers Badge full options plus reusable Icon resources/utilities;
standalone configured Icon node and animated spinner presentation remain future
facades, explicitly not satisfied by old kind50's name/visual_size. Identity and
rotation helpers can be pure host math, validated against C. All five language
facades remain individually pending; no source-family completion claim.

Required gates: native/WASM record offsets/size/alignment, strict C11/C++ and old
header consumers; immutable old Badge/Icon bridge outputs/allocation counts;
complete/each-missing optional symbols; full native size_t/UTF8/alias/reserved bounds;
resource limits/copy failure/collect/reload/reference/stale/close tests and foreign
wrapper cross-app refusal (including coincident numerical handles);
old-mask render after source replacement; independent ordinary Icon tree RGBA for
all Badge variants/sizes/styles/constraints/transforms/alpha; count0/max0/overflow,
nested interactive content once, resize/omission and separate artwork/frame errors;
actual Node/TS app and strict declarations later. Headless private wrapper only.
No compile, tests, generation, public declarations or production ran for this doc.
