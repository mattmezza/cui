# Questionnaire geometry: final additive contract

Approved bounded implementation contract, 2026-10-22. Builds on the measured
wrapper design in `bda38642`; frozen source is `87d10ae5`.

A new independent `cui_questionnaire_geometry.h` exposes five masked box patches
(root, item, choices, actions, card), checked complete metrics resolution, three
choice text refinements and extended optional child renderers. Legacy public
options, choice view and state layouts do not change. Input, Button, arbitrary
style inheritance and asymmetric editor padding remain required separate work.

The extended callback receives an immutable borrowed snapshot containing the old
choice view, the fully resolved metric value record, and answer, secondary and
shortcut text views. Each new renderer wins over the corresponding old renderer;
otherwise the old renderer runs unchanged. The same child namespaces and ordering
apply. Snapshots/functions/userdata are borrowed for synchronous submission only;
strings drawn by children survive completed-frame consumption. Children balance
boxes, do not mutate the model or dispatch input, and cannot retain the snapshot.
There is no callback/style cache or numeric inheritance. Root publishes only its
existing size/density. Caller explicitly passes geometry to each part.

Resolution order is existing size/density, custom scalar only with shared size
INHERIT, masked metric overrides, text refinements, then box instance patches.
Shared XS through9XL wins over custom scalar. Legacy numeric role sizes and source
custom formulas are separately documented. Custom scalar is finite (0,256];
lengths are finite [0,4096], fonts (0,512]. Unknown masks/invalid active values
reject before bindings, input reduction, model mutation or callbacks. Checked
metric helper failure leaves output unchanged. Inactive values are ignored.

A box patch offers width (grow -1, FIT0, positive fixed, explicit fixed-zero),
physical padding, gap, radius, background, border, alignment and opacity. Card
outer gap stays zero; its gap patch applies to the actual answer row. Explicit
minimum and fixed card heights reject fixed below minimum before interaction.
The minimum uses a zero-width spacer inside the interactive outer card; there is
one paint/hit/focus/semantic owner. Taller content grows FIT cards; ancestor clip
and fixed-height overflow retain ordinary Core behavior. No max-height promise.
Post-state card opacity changes paint only, never disabled eligibility.

NULL and all-zero geometry take the exact existing tree/text path. Legacy entry
points delegate without active geometry. Existing keyboard capture, callback
mutation/timing, observer ordering and bool meanings remain unchanged. Text
refinement changes default paint in the existing reducer, never creates a second
control. Natural line height uses font metrics for first-line alignment only
when active; zero preserves legacy offsets exactly. No observer/event facade or
full-family parity claim is introduced.
