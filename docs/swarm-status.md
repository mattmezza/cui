# Live development checkpoint

Repository https://github.com/mattmezza/cui, default branch `main`. Existing public visibility preserved; ordinary pushes only. The full 78-family parity objective is active, with isolated compound, tree and browser-gallery workers; only the root integrator merges. Impeccable metadata was removed at the user's request.

The first resumed published checkpoint `8d670694cb63d7074c1f59ec717bccf848d45ac1` passed all six framework CI jobs and the WebAssembly browser/build/deploy pipeline. [Framework CI](https://github.com/mattmezza/cui/actions/runs/38219053307); [GitHub Pages deployment](https://github.com/mattmezza/cui/actions/runs/38219053380). Ubuntu/macOS/Windows portable22/22 each; Linux GCC/Clang native sanitizer25/25 each plus Xvfb gallery and Sokol readback; style/manifest/license/distribution gates. Hosted Sokol uses llvmpipe software separately from the development-host Intel/Mesa GPU evidence.

The live gallery is https://matteo.merola.co/cui/ (existing account domain inherited by GitHub Pages). HTTPS is enforced. All78 entries appear in navigation; only actual implementations/models have live stories. Newer local stories and component changes are integrated and validated before the next ordinary push, with exact source evidence in validation/ and NIGHTLY_REPORT.md.

Current catalog:35 not_started,0 scaffolded,4 implemented,39 tested,0 verified. Tested means a documented subset, including partial base models. Full family parity remains unfinished and every gap stays in gpui-parity.json. Do not infer completion from rendered previews or tested API subsets.

Resumed work adds viewport portals, semantic relationships and roving focus, same-frame modal dismissal, per-event validated activation, content/hover surfaces, disclosure/tabs, flat menus/lists/selects, editable comboboxes and independently linked numeric/sequence behavior. Multiple selection, calendar/date/time and HSV/RGBA color controls are now integrated. Ongoing batches extend nested/context menus, tree virtualization and gallery stories.

Local validation includes native GPU/readback tests, ASan/UBSan, optimized portable Release, actual WASI runtimes and Chromium browser interactions. Numeric WASI testing exposed a locale-probe assumption: WASI can accept a locale name while retaining the C decimal radix. The initial failure log is preserved; the corrected probe now distinguishes locale availability from the requested radix, and WASI passed20/20 before pickers and22/22 with pickers. No production numerical checks are being removed.

No dependency installation or privileged setup is needed from the user on the current host. Native accessibility bridges, complete editing/IME/graphemes/bidi/fallback, images/compound families and broader GPU/platform adapters remain in the active scope.
