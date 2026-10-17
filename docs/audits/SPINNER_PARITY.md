# Spinner frozen-source parity audit

Audit target: CUI `7b2b8f85905880adab3ac493a8c7103a582d6fb5`.
Reference: GPUI Kit 0.7.1 `87d10ae5e1299d1be18670a37570c5402f952e34`.
This is an evidence map, not a ledger promotion or a new component family.
**Recommendation: retain tested/partial; do not mark the entire family verified.**

## Source authority

The clean local reference checkout is `/tmp/cui-gpui-reference` at the exact frozen
commit. Relevant source:

- `crates/component/src/spinner.rs`: constructor at line 20, icon at 35, color at 41, easing at 47,
  Sizable at 53, rendering at 61 and reduced-motion regression at 91.
- `crates/component/src/icon.rs`: owned/shared SVG data at 136, replacement transform at 158,
  Icon resolution at 175, size resolution at 188 and Styled at 204.
- `crates/component/src/sizing.rs`: Size at 6 and Sizable at 190.
- `crates/story/src/stories/spinner_story.rs`: presets/custom 64, LoaderCircle,
  custom colors, linear/bounce/quint easing examples.
- `website/component/spinner.md`: frozen docs hash
  `a19b69831b06d9349ed8ed70b0f8bea0110bb03abebc46eea976fda7e7d8aacf`.

Spinner source SHA256:
`96d8c6f1b168fc2c01061c319f943349240497c7d03dc875c86bf370899991bc`.
Its runtime animation implementation is locked by upstream Cargo.lock line 4094 to
`gpui-pre 0.3.8`; the cached `/tmp/cui-gpui-pre-0.3.8.crate` SHA256 matches the lock
checksum `d7f0bc3ebba070f8981b3f8e905a12f47d27794a30d029e59cdb9edf7a4b3360`.
See its `src/elements/animation.rs`: unclamped easing at 60, reduced motion at 407,
callback invocation at 444 and quadratic easing at 516. This matters for the two concrete
behavior differences below; inspecting only Spinner's wrapper misses them.

## Contract and evidence map

| Frozen contract | Local implementation | Existing evidence and boundary |
| --- | --- | --- |
| Default Loader SVG, Medium, foreground, 0.8s quadratic repeating rotation | `include/cui/cui_content.h:229`, `src/base/cui_status.c:337`, `src/components/cui_content.c:735` | `test_status_motion.c` checks default options, sample angle, real mask/software pixels; `test_spinner_contract.c` checks default tint and decorative semantics. Default Loader and LoaderCircle embedded bytes exactly match frozen assets. |
| Alternate LoaderCircle and arbitrary compatible Icon | `cui_spinner_options.paint`, `cui_icon_named`, caller-owned `cui_icon_state` | `test_spinner_contract.c` rotates loader-circle/arrow-right and checks raster changes; `test_status_motion.c` checks custom callback arguments. Generic raw-data/path ownership is tested by `test_icon.c`, not by a combined Spinner raw-data fixture. |
| XS/S/M/L plus custom pixels | `options.size` gives explicit pixels; `cui_icon_spinner` separately supports `cui_icon_options` presets/rem | `test_spinner_contract.c` covers 12/14/16/24/64 at one root scale; gallery covers the same choices. Named rem identity is not represented by `cui_spinner_ex`; see gap below. |
| Explicit color, otherwise custom Icon color/ambient foreground | `options.custom_color/color`, theme foreground, arbitrary painter | Explicit/transparent colors and ancestor opacity pass software/gallery checks. Ambient text-scope inheritance remains unsupported by Core/Icon; custom Icon style precedence must be assembled by the painter. |
| User-provided easing | `cui_loading_motion_sample`, callback user data | Standard default/linear timing and invalid-result atomicity are tested. Finite overshoot is rejected; source permits it. |
| Reduced motion: static repeating animation, no requested frame | context/option reduced motion, inactive result and zero wake | Native/software and real gallery tests prove quiescence/static default output. Custom ease(0) differs from source. |
| Decorative/no native loading status or focus stop | `cui_spinner_ex` submits decorative Icon | `test_spinner_contract.c`, `test_status_motion.c`, `test_spinner_gallery.py`, `test_status_motion.py` inspect absent status/busy semantics and idle redraw. Completion/action callbacks are not upstream contracts. |
| Custom Icon box style and leaf layout | `cui_icon_options.style/constraints`, `src/base/cui_icon_layout.h`, `src/components/cui_icon.c` | New `test_icon_style_source.c` proves padding/border floors, minima/maxima, transforms and retained masks; Spinner fixture proves background/radius and ancestor opacity/clip. General inherited Icon styling remains partial. |
| Asset/callback lifetime | Icon owns SVG copy; source closures stored by Rust; C callbacks borrowed synchronously | `test_icon.c` tests source replacement, loader revisions, old masks; `test_icon_style_source.c` consumes three retained versions after replacement. Spinner motion/result rollback and resource exhaustion are in `test_status_motion.c`. A combined Spinner caller-owned-data/reload/destroy sequence is still absent. |

Local test paths above are under `tests/unit/` unless named `.py` under
`tests/web/`. These are inspected evidence, not a new run of the complete native
or gallery suites during this audit.

## Concrete remaining differences

1. **Finite easing overshoot is rejected.** Source `Spinner::ease` accepts a
   callback whose output is fed to rotation without clamping. Locked GPUI's
   Animation explicitly allows finite outputs outside 0..1. Local
   `src/base/cui_status.c:363` rejects them. A constant 1.25-turn callback succeeds
   upstream but makes `cui_spinner_ex` fail locally. Do not simply relax the shared
   sampler: Skeleton derives opacity from it and needs its separate bounded
   contract. A Spinner-specific finite rotation sampler/policy can preserve phase
   and reduce turns before multiplying by radians to avoid overflow.

2. **Reduced motion skips the easing callback.** Locked GPUI line 407 chooses raw phase 0
   for repeating animations, then invokes its easing at line 444. Local
   `src/base/cui_status.c:359` bypasses easing and forces the result to 0. For
   `ease(t)=0.25+t`, upstream freezes at a quarter turn; local freezes unrotated.
   Standard ease(0)=0 tests cannot detect this. Preserve no-wake behavior while
   deciding explicitly whether to match this source contract. Existing local
   tests deliberately expect a failing callback to be bypassed when reduced;
   changing this requires updating that documented contract and targeted tests.

3. **Named versus custom size identity is flattened in the source-style API.**
   Upstream always applies Spinner's Size to its Icon; named sizes resolve in rem
   units and explicit Pixels stay unscaled. Local `cui_spinner_ex` always passes
   CUI_ICON_CUSTOM, so 12/14/16/24 stay fixed as the application rem changes.
   `cui_icon_spinner` can express named/rem sizing, but cannot simultaneously
   express the source Spinner's custom icon/easing. Applications can manually
   resolve pixels/custom painters; that is compositional capability, not automatic
   named-size parity. Recommend an additive preset/custom/rem descriptor reusing
   `cui_icon_size`, without breaking existing explicit-pixel options.

4. **Ambient/custom Icon style support is not complete.** New numeric box-style
   tests resolve important gaps; they do not establish general flex/percentage
   layout, automatic ambient text inheritance, or the full SVG renderer feature
   set inherited by source `.icon(Icon)`. Current ICON/SPINNER guides correctly
   retain this limitation. Parent composition covers many layouts, but does not
   prove identical custom Icon style precedence. Track the exact supported style
   subset and accept it explicitly before any scoped verification claim.

5. **The bridge Spinner remains the legacy primitive.**
   `src/bridge/cui_bridge.c:4387` calls eight-dot `cui_spinner`, with Loading status
   semantics and `reduced=false`, rather than decorative `cui_spinner_ex`.
   This is outside the current ledger's headless/WASM platform claim, but it blocks
   any unqualified claim covering language builders. Preserve legacy C API behavior;
   add an explicit source-style bridge contract and reduced-motion path if bindings
   are included in the eventual verification boundary.

## Documentation/source distinctions and deliberate CUI policies

Upstream docs describe configurable speed, CSS transforms and shared timing.
The frozen Rust Spinner has no public speed setter; speed is private 0.8s. It uses
GPUI transforms, not a CSS engine. Its `.repeat()` has `synced=false`, unlike the
separate `.repeat_synced()` facility. Do not invent these as missing Spinner APIs.
CUI's bounded duration option is an extension, not a source requirement.

The source has no completion callback, loading-status announcement, focus handler,
click action or pointer/touch state machine. Their absence from the decorative
Spinner is correct. A custom painter is the portable C composition mechanism,
not an upstream on-paint/action callback with matching Rust ownership semantics.

CUI schedules explicit caller-clock deadlines at 60 Hz; source requests available
animation frames with no maximum-fps policy on Spinner. This is a documented
scheduling-policy difference, not evidence of hardware refresh-rate parity.
Hidden/clipped spinners can still return active deadlines locally; the guide
requires the application to omit them or stop scheduling. No hidden-work/power
optimization is established by current evidence.

## Meaningful proof before reconsidering verification

- Add callback fixtures producing finite negative/>1 turns, shifted ease(0),
  nonfinite results, and reduced-motion transitions. Check both callback count
  and actual asymmetric SVG pixels, with unchanged-state failure semantics.
- Check complete deterministic 0/.2/.4/.6/.8/.2-next-cycle angles and no trailing
  wake after reduction/removal. Add source bounce/quint curves, independent
  instances mounted at different times, and omission/remount policy.
- Exercise named sizes at at least two rem scales independently of text font
  size, with custom 64 remaining fixed. Repeat in real gallery WASM.
- Combine a caller-owned raw-data Icon, style padding/border/min/max, explicit
  color precedence, rotation and source replacement in one Spinner frame; consume
  all returned draws before collect/destroy. Existing separate Icon tests are
  necessary but do not prove this composition end-to-end.
- If builders are in scope, test actual bridge SVG/default semantics, reduced
  quiescence, custom icons/sizes/color/easing; legacy eight-dot smoke is insufficient.
- Retain desktop/390px and DPR1/2 browser checks. GPU/platform acceptance remains
  separate; do not infer it from software or Canvas output.

The ledger's current three features remain `tested`, both listed platforms remain
`tested_partial`, and its explicit inherited-style limitation remains justified.
It omits newer `test_spinner_contract.c`/`test_spinner_gallery.py` from its evidence
lists; an integrator can refresh evidence after review without promoting status.

## Isolated probe executed in this audit

A temporary strict ISO C11 probe links only the audited `cui_status.c` motion
sections (unused sections garbage-collected). It confirms finite 1.25 output is
rejected atomically and reduced motion never calls shifted easing. This is a
passing characterization of a source difference, **not** a parity acceptance test.

```sh
cmake -S /tmp/cui-spinner-audit-probe -B /tmp/cui-spinner-audit-build -G Ninja
cmake --build /tmp/cui-spinner-audit-build
python scripts/test_local.py --build-dir /tmp/cui-spinner-audit-build -- -V
```

Result: 1/1 passed, `-std=c11 -Wall -Wextra -Wpedantic -Werror`, private wrapper.
No production/header/ledger/manifest changes were made.
