# Build CUI applications with an AI agent

Give an agent your application requirements and the repository's current API and tests. CUI is a portable C11 library with optional host/renderer adapters. The live gallery covers 78 entries, while the feature manifest still records incomplete reference behavior.

## Consumer setup prompt

```text
Build my UI using the CUI repository at https://github.com/mattmezza/cui.
My application requirements: [describe the screens, interactions and host].

Start by reading README.md, docs/START_HERE.md,
docs/IMMEDIATE_AND_STATE.md, docs/INTERFACES.md and docs/gpui-parity.json.
Use the current public headers as the API authority; do not invent symbols.
Check the feature status and limitations for every component you choose.

Use portable C11 for CUI integration. Keep platform and renderer code in host
adapters. Keep application values, buffers and explicit component models alive
across frames. Use stable IDs and balance every successful begin/end pair.
Render borrowed draw data before starting the next frame. Connect actions using
return-value events or the documented controlled callbacks.

For the native source bundle, read README.md and vendor/include/cui/cui.h.
Edit app.c for portable UI; main.c owns the native host. Run make with warnings.
Use APP_SOURCES for additional C files and TARGET for the executable name.
For Linux automation use an invisible Xvfb display; never DISPLAY=:0 or the
maintainer's Wayland session. Browser automation must be headless.

For browser work build the real WASM gallery and exercise actual interactions. Report what ran, what remains incomplete, and
which renderer/platform was tested. Do not claim full parity or platform support
from an API declaration, compilation or a screenshot alone.
```

Replace the application-requirements line before sending the prompt. The source repository's AGENTS.md and your current task instructions determine the engineering workflow.

## Repository maintainer workflow

Consumers need the bundle's compiler/Make/system-library prerequisites, not the
repository toolchain. Maintainers additionally read `AGENTS.md`, use isolated
worktrees for workers with explicit file ownership, and let only the integrator
merge. Preserve unrelated changes and all 78 manifest entries. Add meaningful
headless tests for behavior changes and compile with warnings:

```sh
cmake -S . -B build -G Ninja
cmake --build build
python scripts/test_local.py --build-dir build
```

The wrapper runs CTest under invisible Xvfb on Linux; never fall back to the
maintainer's desktop. GPU hardware checks unsupported there remain deferred.
For distribution changes run `python scripts/amalgamate.py` and the repository's
lint, format and distribution checks. Use current public headers as API authority,
then [Layout](LAYOUT_GUIDE.md), [IDs/lifetimes](IDS_AND_LIFETIMES.md) and
[Debugging](DEBUGGING.md). Respect Core → base → components; adapters depend on
Core. Do not introduce platform/Clay types into public APIs.

## Download documentation without browser rendering

The built website publishes:

| File | Contents |
| --- | --- |
| `llms.txt` | A concise overview and Markdown links to documentation |
| `llms-full.txt` | The documentation bundle with source attribution and current catalog data |
| `llm.txt` / `llm-full.txt` | Aliases using the filenames requested for this website |
| `markdown/…` | Plain Markdown documents generated from the same repository content |
| `catalog.json` | All 78 entries, feature statuses and limitations |

Start with the small index, then fetch relevant guides and public headers. Use the full bundle when your tool can accommodate it. Historical reports are labeled as historical; the current feature manifest and checkpoint take precedence.

The index follows the [llms.txt proposal](https://llmstxt.org/). Publishing these files supplies readable documentation; it does not establish a search-ranking result or guarantee that an agent understands every API.

## Ask for evidence

For a UI change, request the triggering interaction, expected result, meaningful test and actual running screenshot. For a host integration, request a buildable example, its memory/lifetime contract and the observed backend. For another language, distinguish a tested FFI or WASM example from a proposed binding.
