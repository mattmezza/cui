# Prerelease preparation: v0.1.0-alpha.2

This document prepares **v0.1.0-alpha.2**; it is not evidence that publication
has occurred or that the release source is frozen. The next batch includes the
implemented component event slices, documented typed/host-service exceptions,
secure InputGroup rendering and IME lifetime/focus fixes. Concise skill discovery
and explicit partial Markdown reads are implemented. Final integrated validation,
source freeze and artifact checks remain pending; implementation is not publication.
Final release notes must describe the actual frozen commit.

Keep alpha.1 (`v0.1.0-alpha.1`, Python `0.1.0a1`) and any associated published
assets immutable. This version update does not retag or replace alpha.1. No
package-name ownership, registry authentication, trusted publisher configuration,
or cross-platform native compatibility is established by changing metadata.
Publish a coherent release after each major completed body of work; batch related
commits together. Do not turn each implementation commit into a release.

## Version and source freeze

| Surface | Prepared version |
| --- | --- |
| Repository/GitHub release | `v0.1.0-alpha.2` |
| CMake ABI/base project version | `0.1.0` |
| Public C `CUI_VERSION` | `0.1.0-alpha.2` |
| Python / wheel metadata | `0.1.0a2` |
| npm / Cargo | `0.1.0-alpha.2` |
| Go proxy version | `v0.1.0-alpha.2` |
| Public nested Go module tag | `bindings/go/v0.1.0-alpha.2` |

The Go module is `github.com/mattmezza/cui/bindings/go`; its public Git tag needs
the subdirectory prefix. A repository release tag alone does not publish this
module version. See the [Go module tag rules](https://go.dev/ref/mod#vcs-version).
CMake's base version does not promise stable alpha ABI or full binding parity.

Choose one reviewed commit, finish metadata changes, regenerate SDK copies, and
commit them **before** building final assets. Require a clean checkout and clean
source provenance. Record the full commit SHA, compiler/toolchain versions,
architecture, build options, dependency receipts, and validation logs. Do not
publish packages staged from a dirty checkout just because packaging allows it.

The audited tree repeats versions in Python project metadata, wheel backend NAME
and METADATA, npm, Cargo, the Go packaging function, and CI artifact filenames.
The Go `.info` timestamp is derived from the packaged Git revision. Verify it
against the frozen revision before publishing a proxy bundle.
Future version bumps must audit these literals or centralize their generation.
The checker below requires Python 3.11 or newer and deliberately checks the
existing format rather than rewriting binding manifests:

```sh
python scripts/test_release_metadata.py --tag v0.1.0-alpha.2
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
tag=v0.1.0-alpha.2
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
push the separate `bindings/go/v0.1.0-alpha.2` tag at the same release SHA if
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
