# Consumer skill output budget proposal

Status: audit and proposal only. **Implement after the overall component-event API
change lands**, as explicitly requested by the user. Rebase this audit against
that result before choosing event examples or editing skill scripts. This document
does not change CLI behavior, generated resources, or public component APIs.

## Evidence and reproducibility

Audited `integration/next39-components` at
`e9d2c1a561ced3254e606b0206f412af8b156317` (the isolated worktree's starting
commit), then repeated all eight measurements against the current integration
checkout at `84376caa17b93e56ed643599207d5ee818b74011`: every byte/line count
was identical. The skill instructions, CLI, canonical exporter and generator also
have no diff between these revisions. Commands below use
`python skills/cui/scripts/cui.py` with bundled references, without `--source`.
Measurements include stdout and its final newline, count UTF-8 bytes and
`splitlines()` lines. Estimates are **bytes / 4 rounded**, not tokenizer counts,
model costs, or guarantees; code and multilingual text tokenize differently.

| Helper arguments | Bytes | Lines | Estimated tokens |
| --- | ---: | ---: | ---: |
| `component Select` | 78,254 | 1,341 | 19,564 |
| `component Select --show usage` | 597 | 18 | 149 |
| `component Select --show limitations` | 1,294 | 51 | 324 |
| `component Select --show examples` | 53,240 | 822 | 13,310 |
| `component Input --show examples` | 43,565 | 601 | 10,891 |
| `search "font fallback" --kind guide` | 540 | 20 | 135 |
| `api cui_begin_frame` | 47,790 | 976 | 11,948 |
| `read docs/START_HERE.md` | 5,539 | 97 | 1,385 |

To repeat: invoke each command using `subprocess.run(..., capture_output=True,
check=True)` and report `len(result.stdout)`,
`len(result.stdout.splitlines())`, and `round(len(result.stdout)/4)`.
Measure both bundled and `--source` modes after the event API merge; record the
full commit SHA and resource digest with the implementation baseline.

The present helper's `component()` defaults to `all`; `examples` expands every
linked component document's complete body plus sample and test references. These
are often broad guides, not narrowly scoped component examples. `api` prints
complete matching headers, even when one function is requested. Search already
limits results to ten, ranks literal words, labels partial matches, and exposes
`--limit`; it is not currently an unbounded document dump. Limitations for Select
are comparatively small: reducing their completeness would save little and risk
misrepresenting supported behavior. The skill already recommends targeted
`--show` calls, but its default and examples expansion work against that guidance.

## Proposed discovery contract

Default component output should be a concise, useful summary: canonical name,
resource digest/revision, scope, state ownership, interaction/event contract,
public header path, selected declarations, and explicit support/limitation
statements. Provide precise follow-up commands for API, examples and full source.
Do not print a whole header, broad guide, test file, or all feature inventory just
because a component name was requested.

Proposed initial human-readable budgets (targets to validate, not truncation
rules): default component summary 4 KiB; example index 2 KiB; search at most five
results and 2 KiB normally. A limit can be exceeded by required safety/unsupported
information, with a labelled explanation. Avoid a global byte slicer: it can cut
a declaration, ownership condition, or limitation in half. Keep the source digest
available in JSON and print a compact digest once in text.

Suggested staged interface, to finalize after event API stabilization:

- `component Select`: summary plus all critical limitations and follow-up paths.
- `component Select --show examples`: indexed paths, language/host, purpose,
  component relevance, and build context; no automatic document bodies.
- `component Select --example PATH`: one complete selected example, if linked and
  verified. Until such mapping is available, `read PATH` is the exact retrieval
  mechanism. Do not guess component relevance from arbitrary substrings.
- `component Select --show limitations`: complete limitations, explicit feature
  support states and platform scope. Format concisely; do not hide unsupported
  behavior behind a count or an optional follow-up.
- `component Select --full`: current complete `all` behavior. Keep explicit
  `--show all` as its compatibility spelling.
- `api cui_begin_frame`: declaration, owning header path and required related
  type/ownership references; `--full` restores complete matching headers.
  Exact header reads remain available. An incomplete extracted declaration must
  not be presented as a complete callable contract.
- `search QUERY`: five compact results with match status and retrieval command.
  Retain `--limit N`, validate positive integers, and cap ordinary requests
  (proposed 50). Offer explicit `--full` for all matching result metadata.
  Report returned versus total matches and continuation guidance; never silently
  discard results. Keep ranking and complete-before-partial behavior unchanged.

Keep `read PATH` exact and unbounded by default: the caller deliberately selected
a resource and existing users rely on verbatim retrieval. Add heading-based reads
only if section boundaries can be resolved reliably, with document path, heading,
and explicit omitted-section markers. Never return a clipped code example that
looks runnable. `info` is a metadata inventory; consider a small default overview
and explicit full hashes separately, rather than conflating it with search.

## Examples and limitations need source structure

Prefer one authoritative minimal runnable example for each applicable language and
host, then references to full applications. Include event setup, state lifetime,
error handling, build entry point, and headless execution guidance when required;
a shorter disconnected fragment is not necessarily cheaper to use correctly.
The event API change may replace current polling examples, so mappings and tests
must be selected after that merge. Do not promise all 78 components have a complete
binding example or parity.

Use existing component-specific limitation and feature records as authority.
Summaries must preserve unsupported states, host/backend qualifications, required
providers, and ownership/lifetime conditions. If records are too ambiguous to
summarize safely, emit them completely and exceed the target. A future curated
critical/noncritical classification requires explicit reviewed source metadata;
keyword filtering is insufficient. Native display isolation, pinned setup,
matching `--source` revision, dependency authorization, compiler/build verification,
and incomplete platform/binding parity remain visible instructions in SKILL.md.

## Source and generated ownership

- Hand-maintained entry instructions: `skills/cui/SKILL.md`.
- Hand-maintained CLI and presentation: `skills/cui/scripts/cui.py`.
- Canonical exporter: `scripts/agent_resources.py`; shared website/skill data model.
- Canonical component usage/lifetime/example references:
  `docs/component-usage.json`; status/features/platforms/limitations come from
  `docs/gpui-parity.json` (confirm collector inputs after the event merge).
- Original documents, public headers and examples remain authoritative. Generated
  summaries should reference them, never invent independent API declarations.
- `scripts/generate_agent_skill.py` generates
  `skills/cui/data/resources.json` and copies the canonical exporter to
  `skills/cui/scripts/agent_resources.py`. **Never hand-edit these outputs.** Run
  the generator after source changes and verify `--check`.
- Regression owner: `tests/test_agent_skill.py`, alongside shared exporter tests
  discovered in the updated tree. Preserve all 78 catalog entries and lifecycle
  metadata; lower output volume does not justify removing indexed resources.

Prefer presentation-only changes first; add structured example metadata to the
canonical exporter only when current references cannot distinguish examples from
broad guides. Do not prune the offline resource bundle merely to reduce stdout:
shipping size, discovery output, and runtime context consumption are separate costs.

## Compatibility and implementation sequence

1. Wait for component-event API merge; pin its SHA, refresh this measurement table,
   and verify exact entry-point/build instructions against it.
2. Add explicit summary/full modes and example indexing with meaningful tests.
   Preserve current structured JSON defaults initially; opt into concise JSON
   with a versioned schema or explicit mode. Avoid silently changing lists into
   envelopes or removing header/document fields from existing automation.
3. Switch human-readable defaults with documented migration to `--full` /
   `--show all`; update SKILL.md to start with summaries and fetch only chosen
   references. Do not require a second call to discover critical limitations.
4. Measure representative before/after bytes and exercise actual consumer flows:
   choose component, inspect contract, find one host-appropriate example, build
   and trigger an event. If multiple follow-up calls erase the savings, revise
   the defaults rather than claiming success from a single smaller response.
5. Regenerate resources and run existing skill/export checks. Implementation
   remains a separately assigned change; this audit does not authorize it.

Regression coverage should assert semantic content plus generous byte ceilings,
not brittle exact whitespace snapshots. Test Select/Input and a component with
unsupported behavior; test short versus exceptionally long critical limitations,
non-ASCII content, missing resources, exact/partial/no-match searches, positive and
excessive limits, deterministic ranking, displayed omission counts, full-mode
completeness, and legacy JSON shape. Check that default example output never
expands unrelated document bodies and selected exact reads preserve all bytes.
Compare bundled, live `--source`, and packaged-resource project modes; retain
source digests, stale-generation detection, offline operation, archive guards and
no-launch behavior. Compile chosen event examples with warnings and execute their
meaningful headless tests through `scripts/test_local.py` for native cases.
Budget assertions must allow a safety-completeness exception with an explicit
reason. No benchmark should pass by deleting unsupported/API/build warnings.
