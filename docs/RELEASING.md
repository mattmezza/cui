# Next candidate: v0.1.0-alpha.4 (unpublished)

Version metadata is prepared as `0.1.0-alpha.4` (Python `0.1.0a4`); CMake's base
remains `0.1.0`. No alpha.4 tag, registry release, GitHub release or website
publication is implied. The last published release below remains alpha.3.

The candidate batches typed Slider and localized TimeField language authoring,
Dialog composition, configured C Badge artwork, software/SDL_Renderer V2,
Skeleton palettes/responsive roots and owned bridge Icon utilities. See
[the changelog](../CHANGELOG.md) for supported subsets and remaining omissions.

Before publication, freeze the reconciled clean `main` commit, rerun exact-source
hosted CI/native-binding/Pages browser gates, build matching release assets and
validate installed consumers. Prepare/download/verify the draft assets before
publishing. Keep both old repository and nested Go tags immutable. Registry
ownership and actual physical-platform acceptance remain separate gates.

Use `python scripts/test_release_metadata.py --tag v0.1.0-alpha.4` for this
candidate. The alpha.3 records and examples below describe the immutable previous
release; substitute alpha.4 only when executing the next release checklist.

# Published prerelease: v0.1.0-alpha.3

[**v0.1.0-alpha.3 is published**](https://github.com/mattmezza/cui/releases/tag/v0.1.0-alpha.3)
from frozen source `a7dee36090cbe9c1ca1db1c7bed0cd804c69985f` on 2026-10-31.
The website was deployed and verified at that revision. The release adds
civil DateTime selection, date-only picker actions, Questionnaire observations,
snapshots, keyboard policies, progress, presentation and checked geometry, plus
explicit Popover anchors/arrows. SDL3 and the browser gallery gain bounded
analytic mesh consumers with pixel and failure-path evidence. The gallery has
actual desktop/touch controls and compiled usage examples for the new options.

The batch also includes Avatar metrics, identity palettes, SVG placeholders and
typed activation observers; reusable VideoPlayer defaults, seek icons,
player-only browser fullscreen and an audible sample; and optional bounded
native video/audio and macOS Speech host integrations. Injected Apple fixtures
are separate from physical microphone, permission and recognizer acceptance.
Linux/Windows speech recognizers and broader native media support remain gaps.

The release also adds opt-in Stepper connectors, bitmap indicators and
shared design sizes, with a controlled interactive gallery; nested Resizable
examples and repeated desktop/touch drag regression coverage; split-button styling
and metadata; and growing InputGroup containment. Stepper visual builders are
available in Python, JS/TS, Go and Rust; centered/flexible geometry and interaction
presentation also have additive C APIs. Those newer C presentation options remain
documented binding omissions. Input caret/selection/IME projection after viewport
changes still has a documented one-frame limitation. These additions do not
establish complete Stepper, Input or Resizable parity or native GPU acceptance.

The release additionally includes Spinner finite/reduced-motion
browser contracts, opt-in HoverCard raw outside-release policy, and initially
hidden SDL3 window creation with explicit visibility APIs. Hidden-window authoring
has focused Linux private-Xvfb tests in all five language paths; other native
platforms, Wayland and physical GPU behavior remain separately unverified.
Initial named-element shadow setters and CSS blur conversion reuse existing
renderers. Six builder root kinds have typed overrides across Python, JS/TS,
Go and Rust. Eight additive C Checkbox/Radio/group facades can name indicators;
legacy routes preserve anonymous identities. Indicator bindings, compound parts
and portals remain open. Read `ELEMENT_SHADOWS.md`, `BRIDGE_SHADOWS.md` and
`NAMED_SELECTION_INDICATORS.md` for ownership and limitations. These additions passed integrated CI, release artifact and deployment checks;
that evidence does not close their documented broader parity gaps.

CI preparation reuses checksum-verified raw optional-regex downloads and shares a
private umbrella implementation across feature distribution tests. Independent
self-contained distribution and standalone-module checks remain. Local and Windows CI timing
samples are documented in `DISTRIBUTION_TEST_BUILD.md`; they are observational
comparisons rather than controlled speedup measurements. Describe final CI results from the frozen revision,
not preparation runs from an earlier candidate.

Rebuild consumers with matching headers and libraries: the Questionnaire state
and several by-value options changed since alpha.2. Read the current binding
coverage for C-only options; source packaging does not imply equivalent foreign
APIs or registry publication. All 78 baseline families remain present, with
feature-level omissions documented. Full parity is unfinished. The published notes and asset manifest record the
actual frozen revision, tested subset, host dependencies and remaining limitations.

Published alpha.2 (`v0.1.0-alpha.2`, Python `0.1.0a2`) remains immutable. Alpha.3 does not move or replace the alpha.2 tag or assets.

Keep alpha.1 (`v0.1.0-alpha.1`, Python `0.1.0a1`) and any associated published
assets immutable. This version update does not retag or replace alpha.1. No
package-name ownership, registry authentication, trusted publisher configuration,
or cross-platform native compatibility is established by changing metadata.
Publish a coherent release after each major completed body of work; batch related
commits together. Do not turn each implementation commit into a release.

## Publication evidence

All exact-main gates passed: [CI](https://github.com/mattmezza/cui/actions/runs/41226410425),
[native binding callbacks](https://github.com/mattmezza/cui/actions/runs/41226410434)
and [Pages with three browser shards](https://github.com/mattmezza/cui/actions/runs/41226410372).
The public site verifier checked the same revision, docs, all 78 entries, API
headers and native starter archive.

Ten release assets were downloaded from the draft and compared byte-for-byte with
local staging before publication; nine payload SHA-256 values and both annotated
tag targets matched. Assets contain the source archive, Linux x86_64 static C
install bundle, native Python wheel/npm tarball, Go source proxy bundle, Rust
crate, notes, manifest, source inspection receipt and checksums. Installed C,
native Python/Node, separate strict TypeScript 5.8.3, real-font packaged text and
native Go/Rust consumer checks passed. Binary artifacts were built/tested with
GNU libc 2.42 and compatible system SDL/FreeType/HarfBuzz; they are not certified
for every Linux distribution. GPU and physical-platform acceptance remain deferred.
The manifest records the earlier offline TypeScript metadata miss and the separate
successful cached-compiler check; the combined command is not falsely reported green.

The public Go tag `bindings/go/v0.1.0-alpha.3` points to the same source commit;
the public Go proxy version-info endpoint was also verified against that commit.
This does not establish PyPI/npm/crates.io ownership or publication. Use the asset
manifest and current binding coverage when choosing a downloadable package.

The procedures below remain the checklist for future releases; replace the tag,
version and frozen source together rather than republishing immutable alpha.3.

## Version and source freeze

| Surface | Alpha.3 version |
| --- | --- |
| Repository/GitHub release | `v0.1.0-alpha.3` |
| CMake ABI/base project version | `0.1.0` |
| Public C `CUI_VERSION` | `0.1.0-alpha.3` |
| Python / wheel metadata | `0.1.0a3` |
| npm / Cargo | `0.1.0-alpha.3` |
| Go proxy version | `v0.1.0-alpha.3` |
| Public nested Go module tag | `bindings/go/v0.1.0-alpha.3` |

The Go module is `github.com/mattmezza/cui/bindings/go`; its public Git tag needs
the subdirectory prefix. A repository release tag alone does not publish this
module version. See the [Go module tag rules](https://go.dev/ref/mod#vcs-version).
CMake's base version does not promise stable alpha ABI or full binding parity.

Choose one reviewed commit, finish metadata changes, regenerate SDK copies, and
commit them **before** building final assets. Require a clean packaging checkout and verified
source content/provenance. Record the full commit SHA, compiler/toolchain versions,
architecture, build options, dependency receipts, and validation logs. Do not
publish packages staged from a dirty checkout just because packaging allows it.

SDK `SOURCE_PROVENANCE.json` records the historical generator step; its HEAD and
dirty flag are not a packaging-checkout receipt. Preserve it rather than rewriting
history. Verify the complete generated SDK map against the frozen checkout and
record the packaging commit plus final archive hashes externally. Alpha.3 includes
that complete comparison in `source-artifact-inspection.json`.

The audited tree repeats versions in Python project metadata, wheel backend NAME
and METADATA, npm, Cargo, the Go packaging function, and CI artifact filenames.
The Go `.info` timestamp is derived from the packaged Git revision. Verify it
against the frozen revision before publishing a proxy bundle.
Future version bumps must audit these literals or centralize their generation.
The checker below requires Python 3.11 or newer and deliberately checks the
existing format rather than rewriting binding manifests:

```sh
python scripts/test_release_metadata.py --tag v0.1.0-alpha.3
python scripts/test_release_metadata.py --self-test
```

## Build and review the assets

Use a frozen checkout and an external build/output directory. The commands here
are preparation examples; substitute approved absolute paths. Final artifact
builds must wait until the integrator declares the source frozen.

```sh
cmake -S . -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release -DCUI_BUILD_NATIVE=ON
cmake --build "$build_dir"
python scripts/test_local.py --build-dir "$build_dir"
CUI_PACKAGE_NATIVE=1 python scripts/package_bindings.py python --output "$assets/native-python"
CUI_PACKAGE_NATIVE=1 python scripts/package_bindings.py node --prebuilt --output "$assets/native-node"
python scripts/package_bindings.py go --source-only --output "$assets/go-source"
python scripts/package_bindings.py rust --output "$assets/rust-source"
```

Run SDK generator freshness checks and the installed-package, installed-text, and
binding-run validators against these **exact** outputs. Consult
[PACKAGING.md](PACKAGING.md), [INSTALLED_TEXT_VALIDATION.md](INSTALLED_TEXT_VALIDATION.md),
and [BINDING_NATIVE_RUN.md](BINDING_NATIVE_RUN.md) for commands and prerequisites.
Actual-font checks must pass rather than silently skip. Native Linux tests use
the private Xvfb wrapper; never the maintainer's DISPLAY or Wayland session.
Hardware GPU validation remains explicitly deferred where unsupported by Xvfb.
Source-module validation alone does not validate exported archives.

Review this asset inventory before creating a release:

- A source archive from the frozen tag, with the full modular `include/cui`
  headers, sources, dependency manifest, licenses and build instructions. If
  shipping generated distribution headers, check their freshness separately.
- An installed C library/header bundle labelled with OS, architecture, build
  configuration and shared/static flavor; include runtime dependency information
  and the license/notices for every included component. Test its consumer path.
- The native Linux Python wheel and prebuilt npm tarball, with embedded source
  receipts and build information. Preserve valid original package filenames.
- The Go source proxy `.zip`, `.mod`, `.info` as one labelled archive; the verified
  Rust `.crate`. Source packages can require local native development libraries.
- Release notes, an asset manifest mapping every file to commit/build/flavor,
  SHA256SUMS, dependency/runtime reports, and validation logs or linked CI runs.

Portable and native wheel/npm builds currently have identical basenames and
versions. Keep separate output directories; for GitHub, put them inside clearly
labelled outer archives if publishing both. Never overwrite one flavor with the
other, or rename a wheel's compatibility tag to imply broader portability.
The alpha wheel is host Linux tagged, **not manylinux certified**. Native assets
currently have Linux validation only; label the actual tested architecture,
not “all Linux” or “cross-platform”. Document SDL >= 3.2, FreeType, HarfBuzz and
actual dynamic dependencies from the produced binary. Bundling a bridge does not
bundle all its runtime dependencies. Keep the CUI MIT license and applicable
third-party licenses/notices from `third_party/dependencies.json`; do not reduce
third-party licensing to the top-level MIT license.

## GitHub prerelease publication

The integrator executes these steps after checking the final manifest and tests.
Set `release_sha` to the full frozen commit and `assets` to the reviewed output.
Generate checksums over the final uploaded files, excluding SHA256SUMS itself;
verify them after downloading the draft assets. Avoid uploading duplicate asset
basenames. Prepare release notes describing the tested subset, explicit platform
and runtime requirements, known limitations, and source revision.

```sh
tag=v0.1.0-alpha.3
git tag -a "$tag" "$release_sha" -m "CUI $tag"
git push origin "refs/tags/$tag"
gh release create "$tag" --repo mattmezza/cui --verify-tag --draft --prerelease \
  --title "CUI $tag" --notes-file "$assets/RELEASE_NOTES.md" "$assets/upload/"*
# Inspect/download the draft and verify its manifest and checksums first.
gh release edit "$tag" --repo mattmezza/cui --draft=false --prerelease
```

The source archive served by GitHub and any custom source bundle must resolve to
the same tag SHA. Deploy the website from the corresponding reviewed source and
verify its revision; a successful Pages deployment does not publish any registry
package. Do not move a published tag to accommodate later website corrections.

## Registry gates (separate publication decisions)

At the audited revision, npm has `private: true` and Cargo has `publish = false`.
These intentionally prevent publication. The existing CI packages and tests
local artifacts; there is no registry release workflow. Pages' `id-token: write`
permission is for Pages deployment and proves nothing about registry trust.
`python scripts/test_release_metadata.py --registries` rejects these observable
manifest blockers; passing it would still not prove the gates below.

**PyPI:** establish ownership/availability of `cui-bridge`, approve the distribution
policy and Linux compatibility, and configure the exact GitHub repository,
workflow filename and environment as a trusted publisher. Give only the publish
job `id-token: write`, with environment protections and pinned actions; test a
separate TestPyPI publisher first if desired. Review metadata, license contents,
wheel RECORD, native dependencies and install tests. Current packaging does not
establish manylinux policy compliance. See
[PyPI publisher setup](https://docs.pypi.org/trusted-publishers/adding-a-publisher/)
and [publishing permissions](https://docs.pypi.org/trusted-publishers/using-a-publisher/).

**npm:** verify control of `@cui`, explicitly approve removing `private`, choose
public scoped access, and configure a supported trusted publisher for the exact
workflow/environment. Publish the reviewed native flavor only after deciding its
runtime policy; an alpha should use an explicit `alpha` dist-tag rather than
silently replacing `latest`, e.g. `npm publish reviewed.tgz --access public --tag alpha`.
See [npm trusted publishing](https://docs.npmjs.com/trusted-publishers/) and
[public scoped packages](https://docs.npmjs.com/creating-and-publishing-scoped-public-packages/).
No npm login or ownership check was performed in this audit.

**crates.io:** verify `cui-builders` ownership/availability and publisher
credentials, explicitly approve enabling publication, inspect `cargo package
--list`, and run `cargo publish --dry-run` with the intended features and clean
consumer builds. The current packaging helper uses `--allow-dirty --no-verify
--offline`; a generated crate is not evidence that registry verification passed.
See [Cargo manifest publication controls](https://doc.rust-lang.org/cargo/reference/manifest.html#the-publish-field).

**Go:** validate a clean external consumer without local `replace` paths, then
push the separate `bindings/go/v0.1.0-alpha.3` tag at the same release SHA if
publishing that module is intended. Check public source retrieval and module
checksums. A local proxy archive is not proof that public proxy ingestion works.

## Corrections and rollback

Keep published versions and tags immutable. Before publishing a draft, replace
incorrect draft assets and regenerate checksums. After publication, document the
problem, use registry-supported deprecation/yank mechanisms when appropriate,
and issue a new version (for example, `v0.1.0-alpha.3`, Python `0.1.0a3`)
with corrected assets. Never retag,
force-push, or upload different bytes under an already published version. A
website rollback can deploy a known-good revision independently while clearly
identifying which library release its instructions describe.
