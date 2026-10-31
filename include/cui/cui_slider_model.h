/* SPDX-License-Identifier: MIT */
#ifndef CUI_SLIDER_MODEL_H
#define CUI_SLIDER_MODEL_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
#define CUI_SLIDER_MODEL_V1 1u
enum { CUI_SLIDER_MODEL_SINGLE = 0, CUI_SLIDER_MODEL_RANGE = 1 };
enum { CUI_SLIDER_MODEL_LINEAR = 0, CUI_SLIDER_MODEL_LOGARITHMIC = 1 };
enum { CUI_SLIDER_MODEL_HORIZONTAL = 0, CUI_SLIDER_MODEL_VERTICAL = 1 };
enum { CUI_SLIDER_MODEL_START = 0, CUI_SLIDER_MODEL_END = 1 };
typedef struct {
    uint32_t kind, reserved;
    double start, end;
} cui_slider_model_value_v1;
typedef struct {
    double minimum, maximum, step;
    uint32_t scale, reserved;
} cui_slider_model_domain_v1;
enum { CUI_SLIDER_MODEL_DRAGGING = 1u };
/* Owned, pointer-free storage reserved for the future typed reducer. Init only
 * is implemented: no capture, UI submission or owner binding occurs here. */
typedef struct {
    uint32_t version, struct_size, flags, active_thumb;
    cui_id owner, captured_target;
    cui_slider_model_value_v1 start, preview;
    cui_slider_model_domain_v1 gesture_domain;
    uint32_t gesture_orientation, pointer_type;
    uint64_t pointer_id, pointer_device, source_generation;
    uint64_t last_painted_frame, request_sequence;
} cui_slider_model_state_v1;
/* Pure/allocation-free. Nested domain/value records use this v1 schema, not a
 * variable-sized prefix. Unknown tags/nonzero reserved fields reject. Domain:
 * finite min<=max, finite span, finite step>=0; log additionally 0<min<max.
 * SINGLE ignores inactive start (even NaN); RANGE requires ordered finite pair
 * inside domain. These validators neither snap nor alter caller storage. */
bool cui_slider_model_domain_valid_v1(cui_slider_model_domain_v1 domain);
bool cui_slider_model_value_valid_v1(cui_slider_model_domain_v1 domain,
                                     cui_slider_model_value_v1 value);
/* Propose one endpoint: finite request clamps to domain, snaps to min-origin
 * nearest step (halfway toward larger value), then clamps to its neighbor.
 * Exact domain endpoints remain reachable; off-grid neighbor may win. SINGLE
 * accepts END only, canonicalizes inactive start=0. No sorting/owner acceptance.
 * Invalid argument/null output leaves ALL output bytes unchanged; out may alias
 * caller value storage because inputs are passed by value. */
bool cui_slider_model_value_constrain_v1(cui_slider_model_domain_v1 domain,
                                         cui_slider_model_value_v1 current, uint32_t thumb,
                                         double requested, cui_slider_model_value_v1 *out);
/* Increasing min->max visual fraction, independent of axis/reverse-fill. Input
 * must lie in domain/[0,1]; no extrapolation. Linear zero span maps to0 and its
 * sole value; log uses stable close-ratio and extreme-ratio math. Endpoints are
 * exact. Invalid/null output leaves output unchanged. No step snapping here. */
bool cui_slider_model_to_fraction_v1(cui_slider_model_domain_v1 domain, double value, double *out);
bool cui_slider_model_from_fraction_v1(cui_slider_model_domain_v1 domain, double fraction,
                                       double *out);
/* Exact sizeof(v1), non-null output; invalid leaves output unchanged. Initializes
 * every byte, version/size, SINGLE zero values, END thumb, no owner/gesture.
 * Copies need no context/lifetime. No reducer/events/host API in this slice. */
bool cui_slider_model_state_init_v1(uint32_t state_size, cui_slider_model_state_v1 *out);
#ifdef __cplusplus
}
#endif
#endif
