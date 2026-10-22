# Questionnaire localized progress and bounded text presentation

Approved checkpoint with a worker implementation. The additive header and
progress-only common submission seam now implement the contract below; canonical
registration and gallery integration remain separately owned. Baseline CUI is `9e5ec407`; frozen upstream is
`87d10ae5e1299d1be18670a37570c5402f952e34` (v0.7.1), verified in the local
`/tmp/cui-gpui-reference` checkout. This is one bounded family slice, not complete
Questionnaire parity. Core Form-role support is explicitly outside this slice.

## Source evidence and actual gap

All upstream paths below are relative to that exact frozen checkout.

| Evidence | Frozen contract | Current CUI / remaining scope |
| --- | --- | --- |
| `crates/base/src/questionnaire/state.rs:179-190`; `types.rs:327-342` | Current is the one-based position among enabled items, or zero without a current item; total counts enabled items. This is question position, not answered/completed fraction. | `src/base/cui_questionnaire.c:51-70` already supplies enabled total/position. Preserve these helpers and float projection into existing semantics, including large-count precision limitations. |
| `crates/component/src/questionnaire/components.rs:367-431` | Progress owns optional size, style and children. Localized `Questionnaire.progress` is always the accessible label. Custom children replace the default visible label, not the numeric semantics. Root role is ProgressIndicator, min zero, max total, value current. Muted foreground is the default. | `src/components/cui_questionnaire.c:573-599` already projects role/min/max/value and muted medium text. One English stack-formatted sentence serves both paint and accessible name. No progress-specific translated label, visible override or custom child API exists. |
| `components.rs:203-211,248-252` | Progress uses typography xs except Large uses sm; custom source size uses size times .75. Token size, line height and weight are applied, then progress forces medium weight. Explicit Styled refinement follows, so callers may override it. | Legacy CUI uses 12px (14px Large), or design-size font times .75, medium weight and natural line height zero. The numeric similarities do not prove token line-box equivalence. Add opt-in explicit text metrics; leave legacy paint unchanged. |
| `components.rs:181-226,232-252` | Answer, secondary/description and title have distinct token mappings; answer adornments align on the first answer line box. Title forces medium. Custom source size multipliers are 1, .875, 1.125 and answer line height 1.5. | These are neighboring typography obligations, not all solved by progress typography. Current title/description/choice/error defaults and first-line alignment must stay untouched in the proposed progress slice. A later family text slice can reuse the bounded refinement vocabulary after separate audit/tests. |
| `components.rs:1584-1612` | Actual source test verifies progress numeric values and separately the root Form role. | `tests/unit/test_questionnaire_styled.c:418-444` verifies actual CUI progress semantics across three current positions. It does not verify translated names, replacement child text or explicit typography. The Form-role difference remains separate. |

Existing `cui_questionnaire_options` has choice-only child callbacks, three
box-only slot refinements and six masked action/error labels. It explicitly does
not implement a typography cascade. Descriptor values are copied at submission;
string bytes are borrowed through completed-frame consumption. Existing masked
invalid values reject the submission before interaction. Root publishes size,
not translations or slot styles. Those contracts must not silently expand.

## Chosen additive surface

The worker adds `include/cui/cui_questionnaire_progress.h`, leaving the existing public options
record and state layout unchanged. It includes the legacy Questionnaire header.
One new Styled entry point takes the independent record by value:

```c
/* Implemented additive declarations, shown here as the contract. */
enum {
    CUI_QUESTIONNAIRE_TEXT_SIZE = 1u << 0,
    CUI_QUESTIONNAIRE_TEXT_LINE_HEIGHT = 1u << 1,
    CUI_QUESTIONNAIRE_TEXT_COLOR = 1u << 2,
    CUI_QUESTIONNAIRE_TEXT_STYLE = 1u << 3
};
typedef struct {
    uint32_t fields;
    float font_size, line_height;
    cui_color color;
    cui_text_style style;
} cui_questionnaire_text_refinement;

typedef struct {
    size_t current, total;
    float font_size, line_height;
    cui_color color;
    cui_text_style style;
} cui_questionnaire_progress_view;
typedef void (*cui_questionnaire_progress_render_fn)(
    cui_context *, cui_id, const cui_questionnaire_progress_view *, void *);

enum {
    CUI_QUESTIONNAIRE_PROGRESS_LABEL = 1u << 0,
    CUI_QUESTIONNAIRE_PROGRESS_VISIBLE = 1u << 1
};
typedef struct {
    uint32_t fields;
    cui_str label, visible;
    cui_questionnaire_text_refinement text;
    cui_questionnaire_progress_render_fn render_child;
    void *render_user;
} cui_questionnaire_progress_options;

bool cui_questionnaire_progress_ex(
    cui_context *, cui_id, const cui_theme *, const cui_questionnaire_state *,
    cui_questionnaire_options, cui_questionnaire_progress_options);
```

The existing void helper delegates with a zero record and discards the new bool.
A zero record must be draw-command, pixels, semantics and callback equivalent to
the existing helper for every legacy/design size and inherited-root size. No new
state field, retained callback, locale registry or dispatch route is needed.

### Content and semantics precedence

1. Snapshot enabled current/total once using the existing model helpers. Resolve
   inherited size exactly once on the existing path. No validator/editor flush,
   model mutation, event emission or keyboard capture occurs.
2. Build the existing English fallback only when needed. A masked `label` replaces
   the accessible name; otherwise keep the exact existing sentence. Localization
   and pluralization belong to the host, which has the current/total helpers.
   No English-format parser or interpolation language is added.
3. Semantic role, min zero, max total and current remain model-derived even if
   supplied text says something different. Preserve the same ID, parentage,
   nonfocusable/noninteractive status and absence of activation handlers.
4. A non-NULL custom child callback replaces visible default content. Otherwise
   masked `visible` wins; otherwise use the resolved accessible label. Thus a
   translated accessible label alone translates the default visible sentence too.
   A visible-only override intentionally keeps the legacy accessible sentence.
5. Masked empty strings are valid explicit empty values, matching existing label
   options. Empty visible text can hide the sentence while leaving the accessible
   name/numbers. Empty accessible text is deliberate and does not remove numeric
   semantics; applications should normally supply a meaningful translated name.
   Unmasked strings are ignored, including their invalid/NULL bytes.

The callback runs once synchronously inside the existing full-width progress
wrapper after progress semantics are submitted. Its namespace ID is derived from
root ID, distinct from the progress semantic node. It must balance its own boxes,
not close the wrapper, mutate Questionnaire state or change event dispatch. It
receives a borrowed scalar view; copy it if retained. The callback/user are
borrowed only for that call and never retained by the model. Its render primitives
may borrow strings/resources through normal completed-frame draw consumption.
Custom text should normally remain decorative to avoid a duplicate progress node;
applications adding interactive descendants own those independent semantics.

The callback receives resolved text values but there is no implicit text cascade.
It chooses whether to use those values with ordinary `cui_text_ex` or compose
other content. This is the explicit C composition equivalent of source children,
not arbitrary source Styled inheritance or a reusable arbitrary element tree.

### Typography and validation

Apply the explicit mask after legacy metrics, muted color and medium weight.
`TEXT_STYLE` replaces the complete portable `cui_text_style` atomically, allowing
explicit normal weight, false italic/monospace, family zero and letter spacing
zero. There are no per-field zero-sentinel ambiguities. Font size and line height
are separate masks: finite font size in (0,512], finite line height in [0,4096].
Explicit zero line height requests existing natural Core spacing; positive values
supply the text line box directly, rather than an outer fixed-height approximation.
Color accepts the complete existing color value, including transparent.

Validate active style weight against the four existing weights and finite letter
spacing; retain Core's shaping-capability and cluster-progression rules rather
than promising unsupported host shaping. Unknown presence bits, invalid active
values and invalid UTF-8/embedded NUL in masked strings reject the entire
submission, with no semantic node, draw or callback. `{NULL,0}` is valid empty;
nonzero length requires readable bytes. No descriptor or text-arena allocation.
Font-family IDs remain application-defined, with normal host fallback behavior.

The new bool reports accepted submission versus invalid arguments/options. It is
not a transaction over renderer/layout failures caused by custom content; those
continue through ordinary frame error reporting. Legacy void failure behavior is
preserved. A custom renderer does not excuse invalid active fields: validate the
whole descriptor before any user callback. Copy descriptors and resolved values
at entry; strings remain borrowed through draw/semantic consumption. Do not copy
string bytes into hidden retained state.

No source size enum is introduced. Existing design sizes XS through 9XL and root
inheritance continue unchanged. A caller can explicitly choose font/line metrics
from its theme to reproduce the source token relationships; this slice does not
change CUI theme defaults or falsely equate every source custom size to a design
step. A later token-mapping helper would need independently verified theme data.

## Intended usage and alternatives

Proposed use: before frame composition, the application computes current and total
with `cui_questionnaire_position`/`cui_questionnaire_total`, formats a localized
sentence into its own bounded UTF-8 buffer, and passes LABEL plus an optional
shorter VISIBLE sentence. Keep that storage alive until completed-frame draw and
semantic consumption. For rich content, provide `render_child`, use view.current
and view.total, and render a compact count plus ordinary decorative graphics.
The callback cannot return a borrowed temporary string to outlive its invocation.

A borrowed formatter callback returning `cui_str` was considered and rejected for
this first slice: it adds a second callback/lifetime boundary merely to implement
host localization already possible with public count queries. Root-global locale
or typography pointers were rejected because they would introduce retained
ownership, size-cache coupling and new propagation rules. Extending the existing
by-value options record was rejected to avoid another unrelated ABI enlargement.
A begin/end child API is possible later, but a bounded callback matches existing
choice composition and makes wrapper balancing and one-shot resolved metrics clear.

## Implementation ownership and proof before promotion

Production requires the new header, a progress-only private common submission
seam in `src/components/cui_questionnaire.c`, a dedicated test and guide. No Base
reducer, core role, public legacy options, observer, DatePicker or bindings change
is needed. Canonical module/amalgamation/SDK/skill/docs registrations follow only
after production review. Bindings initially omit this record, callback and text
refinements explicitly; packaged C headers alone are not a binding facade.

Required strict C11 Release tests through the private headless wrapper:

- Differential old/zero path draw records, software pixels and complete semantics
  across all legacy sizes, XS..9XL, density/root inheritance, disabled items,
  no current/all disabled, completion/reset and repeated submissions.
- Localized UTF-8 accessible label with one progress node; visible replacement
  differs without changing numbers; explicit empties; ignored unmasked invalid
  strings; active embedded NUL/invalid UTF-8, unknown masks, nonfinite/out-of-range
  text values reject before callback/semantics. Caller buffers retained across
  frame consumption; no hidden allocation or state mutation.
- Actual text command font/weight/style/color/line height plus software pixel
  and measured-height changes with a deterministic font/metrics fixture. Verify
  explicit normal weight and natural-height zero, transparent color, wrapping
  and long localized text. Missing host font capability is an explicit limit.
- Custom child runs once, replaces default visible text, uses stable separate
  namespace and reports unchanged model counts; default progress semantics stay
  intact. Descriptor copies are unaffected by callback-side descriptor edits.
- Queued pointer/key/AT input plus attached legacy/typed observers demonstrate
  this passive helper consumes no activation or emits model events. Existing
  keyboard/observer/Styled suites remain regressions; no fake dispatch assertions.
- Run the focused suite under the canonical pinned WASI runner, compile the real
  guide and C++ header, and prove standalone/new-header plus umbrella distribution
  consumers when canonical registration is assigned.
- A separately owned gallery follow-up should expose localized and shortened
  progress, custom decorative count and typography examples. Headless actual-WASM
  tests must inspect real semantics, number changes during navigation/disable,
  text draw geometry and desktop/touch containment; no browser proof is claimed
  by this audit or by a native-only implementation.

This closes only progress content/localization and explicit progress typography.
Full family token mappings, root Form semantics, broad localization, arbitrary
Styled behavior and language presentation/event facades remain separate work.
All 78 manifest entries and honest partial-platform statements must remain.

Implementation clarification: the derived nonzero child namespace is distinct
from the progress ID but is not globally collision-free. Callers avoid their
other application IDs as with existing choice child composition. The focused
software test uses deterministic rectangular glyph coverage, proving transport
and actual software compositing rather than production font shaping.

Worker verification: strict Release native progress/Styled/keyboard/observer
suites 4/4; pinned WASI equivalents 4/4; ASan/UBSan progress 1/1; real guide
and C++ header consumer compiled; pinned clang-format 23.1.2 check passed.
Zero comparison uses an independent pre-change rendering body, not two calls
through the new forwarding seam. Tests compare semantic/draw members and pixel
bytes without comparing ABI padding. Completed progress is checked before reset
and reset separately. No browser, production font shaping or physical AT proof
is claimed; canonical generators and gallery are still pending integration.
