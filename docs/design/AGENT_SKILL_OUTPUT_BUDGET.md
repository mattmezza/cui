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


## Current preparation checkpoint — 2026-10-18

**Still preparation only.** Implement only after the Scroll/API batch is complete
and root has reviewed the resulting CLI scope. The earlier user dependency on the
overall component-event API change remains in force; this checkpoint does not
claim that batch is frozen or authorize CLI/SKILL/exporter/generated edits.

Read the system skill-creator guidance and repository `skills/cui/SKILL.md` before
this refresh. Its relevant principles are cheap discovery and progressive detail,
while preserving non-obvious invariants and actual user intent. No need to invent
another reference directory or duplicate the C manuals inside the entrypoint.

### Repeated measurements and freshness

Measured clean isolated source HEAD
`c63b100f81539428c3beef8eaee3ddd23abc99dd`. Bundled resource-file SHA256:
`ff58ff9188a4ba3cc027a25f7a90c036bf8a6ff30d8cb2e4469aa6f78bdff1cc`.
Bundled `source_digest`:
`c65a2d1ab742de3bc47f5b71f2c918d4c3e58efaeef8546c7c1b395a1fba846a`.
Live `collect(source)` digest:
`0b73f7a749067e4502ccb0ef15f020143af679b5bf377a35159435688216d24e`.

The digests differ: the sole differing resource hash is `docs/TEXT_VIEW.md`.
This is an observed pending generation mismatch, not evidence of matching snapshots
or an instruction to hand-edit resources. All measured output bytes/lines below
happen to match between modes, but component stdout hashes differ because they
print the different digest. API/search/read stdout hashes match for these queries.
Regenerate only through the canonical generator after the final batch; remeasure
rather than treating equal byte counts as equal content.

Each row was run both without source (bundled) and with
`--source /home/matteo/.cache/cui-worktrees/skill-output-current`. Values include
the stdout final newline; token estimates are `round(UTF-8 bytes/4)`, not tokenizer
counts. Both-mode results:

| Arguments after helper | Bytes, each mode | Lines | Estimated tokens |
| --- | ---: | ---: | ---: |
| `component Select` | 88,935 | 1,468 | 22,234 |
| `component Input` | 80,985 | 1,389 | 20,246 |
| `component Select --show usage` | 597 | 18 | 149 |
| `component Select --show limitations` | 1,294 | 51 | 324 |
| `component Input --show limitations` | 7,588 | 167 | 1,897 |
| `component Select --show examples` | 59,082 | 868 | 14,770 |
| `component Input --show examples` | 45,952 | 647 | 11,488 |
| `component Select --show api` | 28,229 | 537 | 7,057 |
| `component Input --show api` | 26,897 | 564 | 6,724 |
| `api cui_begin_frame` | 48,354 | 985 | 12,088 |
| `api cui_select_events` | 27,713 | 521 | 6,928 |
| `search font fallback --kind guide` | 634 | 20 | 158 |
| `read docs/START_HERE.md` | 5,539 | 97 | 1,385 |

Raw stdout files and per-output hashes are preserved externally in
`/home/matteo/.cache/cui-validation/skill-output-current/`; `measurements.json`
SHA256 is `4f87d0deba106a0f2f2e574b1401292307aa05b05678153cb18ba7ce5c52d021`.
That evidence is machine-local, not a newly shipped skill resource. Commands use
only Python stdlib and offline bundled/live resources; no downloads, builds,
installation or native/browser windows were involved.

Compared with the historical table, Select default grew 10,681 bytes and its
examples grew 5,842; Input examples grew 2,387. The current default still prints all
sections. `examples` still means **every linked guide body**, not a targeted
runnable example: Select expands 53,015-byte `docs/CHOICE.md` plus 5,813-byte
`docs/SELECT_EVENTS.md`. Input expands seven guides totaling 44,719 body bytes,
plus headings/references. Input's complete limitations/feature/platform section
alone is 7,588 bytes. It cannot fit a 4 KiB default without either safe formatting
savings or a labelled budget exception; removing constraints to satisfy the target
is unacceptable.

### Exact presentation proposal for review

Keep existing JSON schemas and meaning unchanged initially. Only human-readable
defaults change; new explicit flags are opt-in, not implemented by this checkpoint.

| Command | Proposed exact semantic output |
| --- | --- |
| `component NAME` | Summary: name/digest; complete canonical scope/composition/state ownership/interaction/usage strings; state types/callback names; indexed public declarations and exact header paths; **all** canonical limitation/platform/status/feature facts; explicit incomplete-contract label and required header/linked-guide read commands. No automatic guide/header/example/test bodies; no claim to reproduce header contracts. |
| `component NAME --show summary` | Explicit spelling for that same human summary. If JSON is requested, reject this new section initially rather than accidentally returning the legacy all schema under a new meaning. A concise JSON format needs a separately named/versioned contract. |
| `component NAME --full` | Exact existing all-section human output; alias of existing explicit `--show all`. With `--json`, exact existing `--show all --json` dictionary/values. Reject conflicting `--full --show usage/limitations/...`; do not guess precedence. |
| `component NAME --show usage` / `--show limitations` | Preserve existing section meaning. Limitations prints all current status/platforms/limitations/features, without count-only replacement, keyword filtering or suppressed unsupported states. |
| `component NAME --show examples` | Human **index**, preserving every linked document, sample and test reference. Each entry has canonical path, actual title when available, source role (guide/sample/test reference), and exact `read PATH` command. Broad guide must be labelled guide, not claimed component-only runnable example. No bodies. |
| `component NAME --show api` | Human declaration/index view using existing `symbols`, exact canonical header/linked-guide read commands and explicit incomplete-contract warning. JSON keeps its full existing API/header section. `--show all` / `--full` restores the old full human content. |
| `api SYMBOL` / `api SYMBOL --declaration` | Human default and explicit alias: exact indexed declaration(s), owning header path(s), digest, known linked guide paths and executable `read PATH` commands, plus `Declaration/index only: incomplete contract; read authoritative headers and linked guides before calling.` No inferred ownership/type/macro code or contract extraction. If no indexed function exists, print only exact matching resource paths and retrieval guidance, with `No indexed function declaration; this lookup does not prove a type or macro definition.` |
| `api SYMBOL --full` | Restore the exact old human text: indexed function declarations followed by every complete matching header. No clipping or implicit limit. Reject conflicting `--full --declaration`. JSON remains the existing full object by default and with `--full`. |
| `read PATH` | Preserve exact existing content plus CLI's existing final print newline; no silent size limit, section slicing or JSON-shape change. Deliberate exact read remains available for offline consumers. |

For legacy machine users, all existing `component --json` invocations (including
omitted `--show`) retain name/digest and all usage/api/header/status/examples/
sample/tests fields and complete strings. Existing `--show examples --json`
retains `{examples:[{path,text}],sample,tests,...}`; the new human index is not a
JSON truncation. Existing `api --json` retains `{functions,matching_headers}`.
`--declaration --json` should be rejected initially unless its distinct schema is
explicitly reviewed; do not remove matching_headers from old JSON. Existing search
JSON remains the same list with current fields/ranking/default 10; do not wrap it
in a new envelope to report totals. Human search may default 5 with a footer giving
returned/total/complete-or-partial status and `--limit` retrieval guidance; explicit
limits and full-mode details require validated bounded semantics. These are
presentation differences, not missing data in the bundle.

### Concrete minimal outputs and safe exceptions

A Select summary starts with the actual authoritative facts:

```text
Select | digest <current source_digest>
State: Caller retains cui_choice_popup, selected cui_id and item array.
Interaction: <complete canonical interaction/usage/scope/composition>
API: cui_select; cui_select_events
Header: include/cui/cui_choice.h
Limitations/platforms/features: <ALL canonical facts, not an omitted count>
Contract: INCOMPLETE summary; read these authoritative resources before calling.
  read include/cui/cui_choice.h
  read docs/SELECT_EVENTS.md
  read docs/CHOICE.md
Examples: component Select --show examples
Full: component Select --full
```

Angle-bracket fields above are proposal placeholders, not shipped output or
permission to omit content. The implementation must populate complete source
facts and distinguish current legacy usage text from the added native observer
contract; current Select interaction metadata still says only `Returns selection
change...`, while the event guide carries multiplicity/lifetimes. Merely using
that short usage sentence does not establish the event contract. The summary must
say the contract is incomplete and give those exact required reads; it must not
claim all header/guide conditions were preserved inline.

A Select example index can accurately say:

```text
Guide: docs/CHOICE.md — Tabs and toggle groups (shared multi-family guide)
  read docs/CHOICE.md
Guide: docs/SELECT_EVENTS.md — Select selection notifications
  read docs/SELECT_EVENTS.md
Sample reference: web/gallery.c (shared source, not a standalone Select app)
Tests: tests/unit/test_choice.c; tests/web/test_browser.py (references only)
```

Do not guess language/host/relevance/build commands from a filename alone. The
current metadata contains paths, not reviewed per-component runnable-example
mappings. Choose existing complete examples only after their build context is
explicitly captured; `read` of a linked full guide remains the honest fallback.
Optional future heading reads must preserve a complete uniquely identified section
and explicitly name omitted sections. `docs/CHOICE.md` has a `List, Select and Menu`
section plus separate multiple-selection/context/dropdown sections; its first title
is not a safe automatic Select slice. Input guides often have only one top-level
heading, so heading slicing would not solve their output cost by itself.

**Critical contracts cannot be extracted with the current regex index.** The
first implementation is a cohesive **presentation-only** change: use existing
`data.symbols` declarations and canonical component metadata, label the default
view incomplete, and route explicitly to complete authoritative header/guide
reads. Do not infer ownership from declaration syntax, classify safety by keywords,
clip a header, or claim that a concise summary contains all header contracts.
All actual canonical limitation/status/platform/feature facts remain inline;
required build/lifetime/security instructions already in SKILL.md remain visible.
Targets are 4 KiB summary / 2 KiB example index with no hard cut-off: complete Input
limitations and other oversized canonical facts may exceed them. No new source
metadata is a prerequisite for this presentation slice.

If a future reviewed iteration wants **complete inline contract excerpts**, it
needs explicit canonical section references and validated boundaries, with full
fallback on ambiguity. That is separate from default declaration/index discovery;
do not delay the useful presentation-only step or silently invent such metadata.
Do not claim an 80% complete-building-flow reduction from a small initial response:
include all mandatory contract and chosen-example/build reads in end-to-end totals.

Essential SKILL.md rules remain visible: actual stack and matching `--source`,
pinned setup/offline provenance, explicit optional dependencies, stable IDs and
borrowed lifetimes, balanced begin/end, all 78 ≠ binding/platform parity, real
font/provider and measurement/paint configuration, warnings/behavior checks, and
private Xvfb/headless browser/no desktop launch. Example indexing does not delete
build context; selected exact examples must retain it. Unsupported APIs and plain
versus secure text/queued versus synchronous event boundaries stay explicit.

### Implementation boundaries and acceptance after the batch

Hand-maintained ownership remains CLI `skills/cui/scripts/cui.py`, entrypoint
`skills/cui/SKILL.md`, canonical exporter `scripts/agent_resources.py`, metadata
`docs/component-usage.json` and actual docs/headers/examples. Generated
`skills/cui/scripts/agent_resources.py` and `skills/cui/data/resources.json` are
never hand-edited. The frozen batch must be regenerated with
`python scripts/generate_agent_skill.py`, then checked with `--check`. Keep the
shared website model, all 78 component records, hashes/lifecycle information and
source-vs-packaged project selection intact.

Existing `tests/test_agent_skill.py` checks complete component fields and JSON
fixtures, rank/partial behavior, source freshness/digests, archive pinning and
no-launch setup. Extend it rather than replacing those contracts with byte-only
assertions. Add bundled/live/packaged cases for unchanged legacy JSON dictionaries
and complete guide/header bodies; human summary/index/full and conflict errors;
exact limitation/feature/platform retention including Input; explicit declaration
absence for types/macros; broad-guide labels and correct read commands; explicit incomplete-contract labels and authoritative full-read commands;
no inferred contract/typedef extraction or byte clipping;
non-ASCII and metadata larger than target; 5-vs-10 human/JSON search semantics and
honest counts. Exact `read` stays unchanged. Verify changed-digest source mode
actually changes identity, not just byte counts.

Measure real flows (summary → selected declaration → mandatory contract → selected
example/build context) with both output modes after implementation. Report bytes/
lines and labelled estimates per call and total. No dependency installs, automatic
UI launch, production CLI changes or generated edits occurred in this preparation.


### API-default refinement for final review (still no implementation)

This refinement supersedes the earlier suggestion to keep human `api` full until
curated contract blocks exist. Human default discovery is concise and explicitly
incomplete; full contract reads are required next, not claimed to have vanished.
No CLI, SKILL, exporter, canonical metadata or generated files change here.

For a known indexed function the proposed human default is structurally:

```text
API: cui_select_events | digest <actual selected source digest>
<exact declaration from data.symbols, without guessed companion definitions>
Declaration/index only: INCOMPLETE CONTRACT.
Required authoritative reads before calling:
  python "$HELPER" read include/cui/cui_choice.h [same --source context]
  python "$HELPER" read docs/CHOICE.md [same --source context]
  python "$HELPER" read docs/SELECT_EVENTS.md [same --source context]
Full matching-header output:
  python "$HELPER" api cui_select_events --full [same --source context]
```

Placeholders above describe formatting, not runnable emitted commands. Actual
output uses exact paths/source selection and shell-safe quoting for the helper
and source path. Known owning headers come from exact indexed-function matches;
related component guides come only from canonical `api_symbols` membership and
its `docs` paths. Do not claim this list is a complete automatically derived type
or ownership dependency graph. Other matching headers may be listed separately as
`matching resource`, never incorrectly labelled the owning declaration.

For `api TYPE_OR_MACRO` without an indexed function, print `No indexed function
declaration; exact matching resources only.` and all exact matching header paths
with full read commands. Do not reconstruct struct bodies, macros, enum constants
or typedefs with the function regex. If no indexed declaration or matching header
exists, preserve the existing unknown-symbol error/nonzero status; no invented
code, broad fuzzy replacement or successful empty-contract response. `--full`
provides the old complete matching-header text even for such resource-only hits.

All existing `api --json` defaults remain `{functions, matching_headers}` with
complete header strings, including resource-only hits. The default change is
human presentation only; `--declaration --json` remains an explicit error until a
separate new schema is reviewed. Component JSON and examples JSON remain unchanged.
Human component summaries preserve all canonical limitations and prominently mark
missing complete header contracts; human example indexes retain every path.
Human search defaults 5 versus JSON 10 as above, without changing legacy JSON list
shape or ranking. Exact `read` and full escape hatches remain unbounded and offline.

Acceptance measures a **complete building flow**, not just the first lookup:
inspect existing integration and pinned source, component summary (all limitations),
API declaration/index, each required authoritative contract, one selected example
with build context, compile and meaningful headless behavior check. Track bytes/
lines per discovery/read response and their sum in bundled, live-source and
packaged modes. A 59 KB shared Choice guide remains a 59 KB required read until a
separate reviewed section-read/curated example feature exists; example indexing
avoids automatic unrelated guide dumps but does not justify hiding required
contracts or asserting every consumer flow saves the same amount. Targeted users
who only need discovery benefit immediately; implementation users must still read
the applicable contracts. Report observed savings and remaining large reads honestly.
Production stays blocked on Scroll/API batch completion and root's subsequent
implementation authorization, as requested by the user.
