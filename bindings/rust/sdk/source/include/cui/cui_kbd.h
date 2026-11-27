/* SPDX-License-Identifier: MIT */
#ifndef CUI_KBD_H
#define CUI_KBD_H
#include "cui_content.h"
#include "cui_typography.h"
#ifdef __cplusplus
extern "C" {
#endif
enum { CUI_KBD_KEY_MAX_BYTES = 128, CUI_KBD_TEXT_CAPACITY = 160 };
/* One stroke, not a sequence. Modifiers are CTRL/ALT/SHIFT/META bits. The host
 * maps parser/platform aliases; key names use frozen lower-case spellings. */
typedef struct {
    unsigned modifiers;
    cui_str key;
} cui_kbd_stroke;
/* Returns required bytes excluding NUL, or SIZE_MAX for invalid input. Nonempty
 * key must be strict single-line UTF-8 <=128 bytes, without ASCII controls.
 * Invalid input leaves output untouched. NULL output queries required bytes.
 * Partial output ends on a complete scalar and is NUL terminated when capacity>0.
 * Common special names use platform labels. Other keys apply Unicode 17.0
 * default uppercase to the first scalar and preserve the remaining bytes.
 * This can expand a scalar to several. Required count is at most 151 bytes.
 * Legacy cui_kbd_format preserves the key bytes. Output may overlap the input. */
size_t cui_kbd_format_stroke(cui_kbd_stroke, bool mac_symbols, char *output, size_t capacity);

typedef enum {
    CUI_KBD_BINDING_CURRENT, /* Host's active key context. */
    CUI_KBD_BINDING_CONTEXT, /* target identifies an explicit host context. */
    CUI_KBD_BINDING_FOCUS,   /* target identifies a focus target known to the host. */
    CUI_KBD_BINDING_GLOBAL   /* Host lookup with an empty key context. */
} cui_kbd_binding_scope;
typedef struct {
    cui_id action;
    cui_kbd_binding_scope scope;
    cui_id target; /* Required for CONTEXT/FOCUS; zero for CURRENT/GLOBAL. */
} cui_kbd_binding_query;
/* Return the stroke count of the winning binding and set *strokes to the
 * host-owned sequence. Return zero for an unbound action. The host determines
 * precedence and focus-to-context mapping. Query and output pointer are borrowed
 * for this synchronous call. Sequence and key storage must outlive the call. */
typedef size_t (*cui_kbd_lookup_fn)(void *user, const cui_kbd_binding_query *,
                                    const cui_kbd_stroke **strokes);
typedef struct {
    cui_kbd_lookup_fn lookup;
    void *user;
} cui_kbd_binding_source;
/* Calls lookup at most once. On success copies the first stroke; its key bytes
 * remain borrowed from the host until the host next invalidates them. Submit or
 * copy immediately. Invalid arguments, no match, or an invalid first stroke
 * return false and leave *out unchanged. No allocation, registration or dispatch. */
bool cui_kbd_binding_stroke(cui_kbd_binding_source, cui_kbd_binding_query, cui_kbd_stroke *out);

typedef struct {
    bool plain, outline, mac_symbols; /* plain ignores outline; zero is normal pill. */
    /* Optional strict single-line UTF-8, max1024 bytes; no NUL/ASCII controls. */
    cui_str accessible_name;
} cui_kbd_options;
/* Decorative, noninteractive shortcut label. All
 * invalid arguments fail before submission. One copied label semantic defaults
 * to formatted text. Source-shaped pill uses theme font_size/16 rem scaling:
 * X4/Y2 padding, min width20, text12, half radius; outline adds 1-unit border.
 * Theme font must be finite(0,512], radius finite>=0.
 * Plain uses scoped size and solid color or theme defaults, without pill geometry.
 * Inputs are copied into Core
 * frame storage on submission; no retained state or resources. Compose several
 * calls in a normal parent row for grouped hints. */
bool cui_kbd_ex(cui_context *, cui_id, const cui_theme *, cui_kbd_stroke, cui_kbd_options);
typedef struct {
    const cui_box *box;                     /* Optional complete pill box. */
    const cui_typography_token *typography; /* Optional font, line layout and wrapping. */
    const cui_text_paint *text_paint;       /* Optional solid or supported linear text paint. */
} cui_kbd_style;
/* NULL fields keep the normal or outline defaults. With no box override, the
 * pill fits its typography. To wrap at the current parent width, supply a
 * fitting-height box with a bounded width and set typography.wrap. Plain mode
 * ignores all style overrides. The inputs are read during submission and any
 * gradient stops are copied into the frame. Invalid typography/paint returns
 * false before submission. Use Core's named box API to add shadows after it. */
bool cui_kbd_styled(cui_context *, cui_id, const cui_theme *, cui_kbd_stroke, cui_kbd_options,
                    const cui_kbd_style *);
/* Style a passive literal hint using the same box, typography and paint as a
 * typed hint. Text must be valid UTF-8 and is used as the accessible name.
 * NULL fields keep the literal pill's themed border, fill and typography. */
bool cui_kbd_label_styled(cui_context *, cui_id, const cui_theme *, cui_str label,
                          const cui_kbd_style *);
#ifdef __cplusplus
}
#endif
#endif
