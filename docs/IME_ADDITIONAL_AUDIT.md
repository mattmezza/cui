# Additional text consumer IME audit

Audited source: `b6fbb70`, isolated branch `audit/ime-additional`. The initial audit
added no implementation changes; the follow-up below records the scoped fix. The existing four-consumer IME test does not
establish IME support for Numeric or OTP.

| Consumer | Browser native composition | Commit/cancel | Candidate caret | Disabled/read-only |
| --- | --- | --- | --- | --- |
| Numeric (`NumberInput`) | Focus reaches the bridge; preedit is missing | Composing `57` leaves initial value `42`; cancel preserves it | Available from input painting | Disabling releases the bridge and preserves value; read-only source policy only |
| OTP (`OtpInput`) | Focus reaches the bridge; preedit is missing | Composing `7` leaves digit count zero; cancel preserves it | Missing (`gallery_ime_rect(0) == -1`) | Disabling releases the bridge and preserves value; no read-only option |
| Combobox | Preedit `ap` appears without changing query | Commit produces two query bytes exactly once; cancel preserves query; one Undo restores empty query | Available | Disabling active composition preserves query and releases the bridge; no read-only option |

These results use Chromium's `Input.imeSetComposition` and `Input.insertText`,
which dispatch real DOM composition events to the focused host textarea. The
script records native start/update/end events, including nonempty commit and
empty cancellation. It does not fabricate CUI mode flags or DOM composition
objects. Tests select Numeric's existing draft before committing a different
value, avoiding a false pass when initial and committed values match. OTP uses
one ASCII digit so the failure cannot be explained by unsupported nonnumeric
characters. The Combobox check exercises the single-selection gallery story;
multiselect and large-query capacity are not established by this audit.

Run the strict regression with an existing gallery build and installed Chromium:

```sh
uv run tests/web/test_ime_additional.py --directory /tmp/cui-next16-integration/build/web \
  --chromium /home/matteo/.cache/ms-playwright/chromium-1228/chrome-linux64/chrome
```

The observed result is **five failed requirements**, collected before exit status
1: Numeric preedit and commit; OTP preedit, caret and commit. Combobox's checked
requirements pass. Cancel and disabled-state checks pass, but do not prove a
working composition session for consumers that ignored its creation. Browser
runtime errors were absent. The failures intentionally remain strict assertions
for future fixes; this script is not registered as a passing CTest check.

## Source findings and native implications

* `src/base/cui_numeric.c`, `cui_base_number_input`, handles `CUI_EVENT_TEXT`
  and keyboard editing directly. It neither creates a composition session nor
  reduces composition events. It reads the legacy inline text array rather than
  the variable payload. Its existing numeric grammar, draft/controlled-value
  transition and Enter/blur commit behavior need an explicit composition policy.
  Numeric's `readonly` option blocks its editing loop and stepping; the gallery
  does not expose that option, so this audit makes no browser read-only claim.
* `src/base/cui_otp.c`, `cui_base_otp`, handles only ordinary text and Backspace.
  The ordinary-text branch converts one accepted digit per event from the inline
  text array. Composition events are ignored. OTP also lacks a model composition
  session and candidate caret publication. A future commit path must define how
  multiple digits, capacity and completion callbacks are processed atomically;
  this audit does not infer those policies from ordinary typing.
* `src/base/cui_choice.c`, `cui_choice_reduce_combobox`, delegates query editing
  to `cui_base_input_ex` with retained `query_behavior`. The default composition
  session is therefore available. `src/components/cui_choice.c`,
  `cui_choice_paint_query`, passes that behavior to `cui_input_paint`, which renders
  preedit and publishes the caret. Filtering uses the committed query, preserving
  document/query semantics during preedit. The base-created default retains its
  fixed rendered buffer; it does not opt into the new styled Input default's
  dynamic snapshot sizing. This audit does not claim large-query support.
* `backends/sdl3/cui_sdl3.c` converts active SDL text editing into composition
  update/cancel, and the following SDL text input into composition commit. Numeric
  and OTP's shared reducer omissions therefore also affect native SDL IME input.
  Native SDL injection was not added in this two-new-file audit scope; this is a
  source-supported implication, not a measured platform IME result.
* `web/gallery.js`, `syncImeTarget`, enables the native bridge for every focused,
  enabled semantic textbox. Bridge focus alone does not imply that its CUI
  consumer handles composition. Numeric and OTP are concrete examples.

OTP and Combobox public options have no read-only field. Disabled controls were
checked through the existing gallery control. Numeric read-only coverage requires
a dedicated host fixture or an authorized gallery option addition.

## Validation

Fresh portable warning build succeeded without warnings. Full native CTest ran
90 checks: 89 passed; only `distribution` failed because the base's generated
headers are stale. Distribution regeneration belongs to the integrator and was
not performed in this audit. The strict browser audit fails for the five product
gaps above. Only this document and `tests/web/test_ime_additional.py` were changed.

## Scoped Numeric/OTP follow-up

The isolated implementation branch starts at `46d7010`. Numeric now retains an
edit composition session, renders it through existing input painting, normalizes
commit through existing Numeric filtering and groups it into one Undo transaction.
OTP now retains bounded preedit, paints the first eligible proposed scalar using
its existing mask, publishes the focused cell caret and consumes composition
commit. Source/focus/disabled changes cancel; Numeric also cancels for read-only.
OTP retains the pinned reference's first-scalar policy: trailing scalars ignored,
invalid first scalar ignored, full target ignored, actual buffer exhaustion
atomic. This does not promise whole-string or multi-digit OTP composition.

The strict CDP audit now passes all original requirements and additionally checks
Numeric one-step Undo plus masked fullwidth OTP commit (only its first digit).
The mask control is an HTML checkbox: the test explicitly refocuses the canvas OTP
before invoking native composition, so it exercises the intended host textbox.
The original four-consumer native CDP regression also passes. Headless tests cover
ASCII/fullwidth/trailing/invalid-first filtering, callback counts, capacity and
UTF-8 boundaries, masking, composing shortcuts, stale focus and source/model
replacement. Four focused ASAN/UBSan tests pass. Warning build is clean; full
CTest passes 90/91, with only expected stale generated distribution failing.
No shared Core/SDL/browser implementation or distribution files were changed.
The Combobox large-query and Numeric browser read-only fixture limitations above
remain unverified. Native platform UI behavior remains source-supported rather
than tested through an OS IME in this follow-up.
