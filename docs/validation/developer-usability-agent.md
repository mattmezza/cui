# Developer usability: documentation-led native app evaluation

Historical evidence recorded 2026-10-15. This is one Linux software-renderer
consumer run, not full component or platform parity.

## Scope and evidence boundary

The evaluator started with `docs/AI_AGENTS.md`, `docs/START_HERE.md` and
`docs/NATIVE_APP_STARTER.md`, then their linked layout, ID/lifetime, debugging,
icon and native-bundle guides and public headers. No library implementation was
read to build or repair the app. The same agent previously implemented some of
these APIs; this is a separate documentation-led application task, not a blind
independent-agent experiment.

Root supplied `build/cui-native.tar.gz` after source registration and distribution
integration, before the later callback-mutation follow-up. Archive SHA-256:
`edfaf198f70f7722092bb18a729d1f8d6af2c0c6261883f1efbc757d173ba256`.
The archive was extracted outside the checkout at
`/tmp/cui-builder-eval-2hR6Zh/cui-native`. Its host `main.c` was unchanged; only
`app.c` was replaced with application code below. No vendor source or build
metadata was changed. System pkg-config reported SDL3 3.4.18, FreeType 26.6.20
and HarfBuzz 14.6.0. Bundled Inter Regular/Bold fonts were used.

## Procedure and observed result

Extraction started at 13:17:35 UTC; the successful bounded capture completed by
13:19:15 UTC (about 100 seconds including writing the application and compilation).

```sh
make -j2
xvfb-run -a -s '-screen 0 1600x1000x24' \
  env SDL_VIDEODRIVER=x11 SDL_RENDER_DRIVER=software \
  ./build/native_starter --frames 3 --capture evaluation.bmp \
  --debug-layout json > evaluation-layout.json 2> evaluation-stderr.log
```

The 720×420 native screenshot visibly contained two long single-line subjects
ending in ellipses, two fixed trailing star buttons, repeated avatars/tags and
a missing-clock outline. Exported box JSON confirmed both star slots were
40 pixels wide at `[656,81,40,40]` and `[656,161,40,40]`, within the root's right
edge of 696. All 30 nonzero exported box IDs were unique. Three frames rendered
successfully without a reported error on the application context.

The separate deliberately unbalanced context produced the expected first
snapshot:

```text
EVAL deliberate unbalanced: error=3 detail=9 operation=cui_end_frame id=0 path=1
```

The app asserted `clock-3` was `CUI_ICON_CATALOG_NOT_EMBEDDED` in this default
bundle and that requesting its debug placeholder retained `CUI_ICON_NOT_FOUND`.
The screenshot showed its allocated outline beside an explanatory label.
A second unchanged `make -j2` reported nothing to build.

Local evidence files are `evaluation.bmp`, `evaluation.png` (format conversion
only), `evaluation-layout.json` and `evaluation-stderr.log` in the extraction
directory; build logs are `/tmp/cui-eval-build.log` and
`/tmp/cui-eval-incremental.log`. These temporary paths are not shipped artifacts.

## Failures, corrections and limits

1. The first application build failed because the evaluator passed a numeric
   record key to `cui_push_id`. The public header requires `const char *`.
   Changing to stable `"record-100"` / `"record-101"` keys fixed it. This was
   evaluator misuse, not a library defect. The ID guide's untyped `message_key`
   example could be clearer with an explicit string declaration.
2. The successful default optimized vendor build emitted six GCC
   `-Wmaybe-uninitialized` warnings for `view.width`/`view.height` in
   `vendor/src/text/cui_text.c` at lines 817–818, 1065 and 1105. Application code
   compiled without warnings. This run therefore does not establish a completely
   warning-clean native bundle; warnings were reported to the integrator.
3. START_HERE's C block, both LAYOUT_GUIDE C blocks and ICON's C example were
   compiled against extracted public headers with
   `-std=c11 -Wall -Wextra -Wpedantic -Werror -fsyntax-only`. Tiny wrappers supplied
   documented context variables. An initial wrapper-only unused-parameter warning
   was fixed in the harness; the documentation snippets needed no correction.
4. The image viewer could not decode BMP directly; an attempted Pillow conversion
   found Pillow unavailable. Installed ImageMagick converted BMP to PNG, which was
   visually inspected. No screenshot content was edited.

No pointer interaction, reorder, resize, IME, other operating system or hardware
GPU behavior was exercised. Unique scoped identities and fixed-slot geometry were
checked in this static fixture; the library's own tests cover additional cases.
No dependency acquisition or desktop display access occurred.

## Application source

```c
#include "cui/cui_components.h"
#include <assert.h>
#include <stdio.h>
static void diagnose_unbalanced(void) {
    cui_context *probe = cui_create((cui_config){0});
    assert(probe && cui_begin_frame(probe, 100, 100, .016f));
    cui_begin_box(probe, 910, (cui_box){.width = 80, .height = 60});
    (void)cui_end_frame(probe); /* Deliberately omit end_box. */
    cui_diagnostic d;
    assert(cui_first_diagnostic(probe, &d));
    assert(d.error == CUI_ERROR_LIFECYCLE);
    assert(d.detail == CUI_DIAGNOSTIC_UNBALANCED_SCOPE);
    fprintf(stderr, "EVAL deliberate unbalanced: error=%d detail=%d operation=%s id=%llu path=%zu\n",
            d.error, d.detail, d.operation, (unsigned long long)d.id, d.path_count);
    cui_destroy(probe);
}
void app_frame(cui_context *ui, const cui_theme *theme, bool *clicked) {
    static bool initialized;
    (void)clicked;
    if (!initialized) {
        diagnose_unbalanced();
        assert(cui_icon_query(cui_string("clock-3")) == CUI_ICON_CATALOG_NOT_EMBEDDED);
        initialized = true;
    }
    cui_begin_box(ui, cui_id_from(ui, "root"),
        (cui_box){.width = -1, .height = -1, .padding = 24, .gap = 16,
                  .background = {245,247,250,255}});
    cui_text(ui, cui_string("Fresh native message app"), 24, theme->text);
    for (size_t row = 0; row < 2; ++row) {
        assert(cui_push_id(ui, row ? "record-101" : "record-100"));
        cui_begin_box(ui, cui_id_from(ui, "row"),
            (cui_box){.width = -1, .row = true, .gap = 12, .height = 64,
                      .align_y = CUI_ALIGN_CENTER});
        cui_avatar(ui, cui_id_from(ui, "avatar"), theme, cui_string("Alex Reader"), 40);
        cui_begin_box(ui, cui_id_from(ui, "body"), (cui_box){.width = -1, .gap = 4});
        assert(cui_text_ellipsis(ui, cui_id_from(ui, "subject"),
            cui_string("An unusually long message subject that should end with an ellipsis and preserve the full semantic name"),
            (cui_text_ellipsis_options){.width = -1, .font_size = 17, .color = theme->text}));
        assert(cui_tag(ui, cui_id_from(ui, "tag"), theme, cui_string("Inbox"),
                       (cui_tag_options){0}));
        cui_end_box(ui);
        cui_begin_box(ui, cui_id_from(ui, "star-slot"),
            (cui_box){.width = 40, .height = 40, .fixed_width = true});
        cui_button_ex(ui, cui_id_from(ui, "star"), theme, cui_string("Star message"),
            (cui_button_options){.width = 40, .icon = {"star",4}, .icon_only = true});
        cui_end_box(ui);
        cui_end_box(ui);
        cui_pop_id(ui);
    }
    cui_begin_box(ui, cui_id_from(ui, "missing-row"),
        (cui_box){.width = -1, .row = true, .gap = 12});
    assert(cui_icon_named(ui, cui_id_from(ui, "missing-clock"), theme, cui_string("clock-3"),
        (cui_icon_options){.size = CUI_ICON_LARGE, .debug_placeholder = true,
                          .accessible_name = {"Unavailable clock",17}}) == CUI_ICON_NOT_FOUND);
    cui_text(ui, cui_string("clock-3: catalog entry, SVG not embedded (debug outline)"), 16, theme->text);
    cui_end_box(ui);
    cui_end_box(ui);
}
```
