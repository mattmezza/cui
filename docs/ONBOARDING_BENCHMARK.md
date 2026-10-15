# Repeatable human and agent onboarding tasks

This is a task protocol, not a measured usability result. Run the same tasks for
a human and an agent against the same revision, extracted native bundle, machine
and installed prerequisites. Start timing after prerequisite verification; record
installation time separately. Keep the documentation available in both runs.

| Task | Completion evidence |
| --- | --- |
| Build unchanged bundle outside repository | `make` exit status and bounded capture |
| Change greeting in `app.c` | Capture shows new text; vendor object timestamps unchanged |
| Add a second portable `.c` file and rename target | Build using `APP_SOURCES`/`TARGET`, capture from renamed executable |
| Compose a mail row | Fixed avatar/trailing timestamp, growing text, constrained narrow-window content |
| Repeat and reorder three rows | Stable record-key scopes; correct row activates after reordering |
| Diagnose a missing icon and duplicate ID | Correct registry lookup, observed error and repaired submission |

Record elapsed time, build attempts, compiler errors, invented API names,
documentation pages opened, interventions, final test evidence and unresolved
limitations for each task. Do not count a screenshot alone as interaction proof.
For mail rows test long subjects and narrow widths; for identity test activation
before and after reorder. Keep benchmark application experiments outside vendor
sources. Automated Linux captures must run on invisible Xvfb; repository tests
use `python scripts/test_local.py --build-dir build`. Defer GPU-only observations.

Report raw observations before comparing human/agent outcomes. This protocol
makes no claim that the tasks have been run or that onboarding is universally
fast. The full catalog retains 78 entries and their recorded limitations.

## Automated observations for this milestone

The native-bundle consumer test builds the unchanged two-file app outside the
checkout, verifies UI-only incremental compilation, builds a source in a
subdirectory with a renamed executable, and captures both that app and the mail
fixture. The mail Core fixture drives Compose/Discard, selects and stars the
second repeated row, verifies its reader text, and scrolls the list while the
folder pane stays still. The exact four-argument sidebar content callback is
compiled with warnings treated as errors. Text/JSON layout export is exercised
through the native command line. These are automated correctness observations;
human elapsed time, agent elapsed time and independent usability outcomes have
not been measured.
