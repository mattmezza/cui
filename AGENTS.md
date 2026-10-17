# CUI engineering workflow
Native automated tests must never open windows on the maintainer's desktop. On Linux use `python scripts/test_local.py --build-dir build` (invisible Xvfb); never fall back to DISPLAY=:0 or the user's Wayland session. GPU hardware checks unsupported by the virtual display remain explicitly deferred. Browser tests run headless. This local display rule overrides the direct CTest invocation below; the wrapper still runs CTest and preserves its result.
Use isolated worktrees/branches for workers; only integrator merges. Portable C11, public APIs contain no platform/Clay types. Core -> base -> components; adapters depend on core. Worker ownership is assigned explicitly. Do not modify others' files. Every behavior change needs meaningful headless tests. Compile with warnings and run CTest before submitting commits. Never claim full parity from partial APIs. Keep all 78 manifest entries. No sudo, force push, destructive cleanup or secret output. Dependencies pinned. Build: cmake -S . -B build -G Ninja; cmake --build build; ctest --test-dir build --output-on-failure.

# CUI agent skill maintenance
The distributable consumer skill lives in `skills/cui`. When changing public APIs,
component behavior/limitations, bindings, integration prerequisites, consumer
examples or build workflows, update the corresponding source metadata and skill
guidance. Run `python scripts/generate_agent_skill.py` and
`python tests/test_agent_skill.py`; CI and CTest enforce generated-resource
freshness and installed-skill behavior. Do not edit generated
`skills/cui/data/resources.json` or `skills/cui/scripts/agent_resources.py` directly.
The exporter in `scripts/agent_resources.py` is shared with the website's API index.
Keep all 78 manifest entries, exact header authority, binding limitations and
revision matching. Changes to setup/search helpers need meaningful offline tests;
never install dependencies or open native windows from those tests.
