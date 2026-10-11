# Live checkpoint

Repository https://github.com/mattmezza/cui. Default branch `main`, promoted from the safe integration branch after all build gates passed. Source/evidence commit `01865d5b99bb7908ac838bdf4efe642c31ce6e9e` pushed successfully. Existing public visibility preserved; no force pushes.

Catalog:59 not_started,0 scaffolded,4 implemented,15 tested,0 verified. All78 families remain in scope. Tested means a documented subset, including base-only models; no full family is verified. See gpui-parity.json and NIGHTLY_REPORT.md for every gap.

Local evidence: GCC/native24/24, ASan/UBSan22/22, portable Clang19/19, optimized Release19/19, actual headless WASI15/15. Independent Sokol Xorg pixel/cache tests also passed ASan/UBSan with leak detection. C++/Zig consumers ran; installed relocated pkg-config consumer ran. Exact logs are in validation/.

[CI run38186085456](https://github.com/mattmezza/cui/actions/runs/38186085456) passed all six jobs: Ubuntu/macOS/Windows portable19/19 each; Linux GCC/Clang sanitizer/native22/22 each, Xvfb gallery smoke and independent Sokol pixel/cache readbacks; style/manifest/license/distribution. CI GL used llvmpipe software rendering, separately from development-host Intel/Mesa GPU evidence. Early Windows encoding/hash, Mac dock decimal roundtrip and Ubuntu Plot libm endpoint failures were corrected and retested. Superseded runs with Windows assertion dialogs were canceled; test executions now have60-second timeouts. Native Windows/macOS GUI, Wayland, mobile and browser integration remain untested.

Worker source changes are integrated. Native accessibility, IME/graphemes/bidi/fallback, complete compound widgets, images and broader GPU/platform contracts remain pending. No privileged installs or user actions are needed on the current host. This is a committed development checkpoint, not an unattended process promised to continue after the session ends.
