/* SPDX-License-Identifier: MIT */
#ifndef CUI_BASE_SLIDER_MODEL_INTERNAL_H
#define CUI_BASE_SLIDER_MODEL_INTERNAL_H
#include "cui/cui_base_slider_model.h"
/* Internal composition only. The public facade always passes NULL. Policy/root
 * are copied before reduction, then compose runs once inside ROOT after reduction,
 * before closing it. It must only compose paint/geometry from these snapshots:
 * no event claim, semantic registration, owner reread or application callback.
 * user is borrowed through return; all policy data must remain immutable.
 * A non-NULL policy owns root layout validation before calling this function. */
typedef void (*cui_slider_model_compose_internal)(cui_context *, const cui_id ids[5],
                                                  cui_slider_model_value_v1,
                                                  const cui_slider_model_options_v1 *,
                                                  const cui_base_slider_model_layout_v1 *, void *);
typedef struct {
    cui_box root;
    cui_slider_model_compose_internal compose;
    void *user;
} cui_slider_model_presentation_internal;
/* Shared admission only: no mutation, callback or box submission. */
cui_slider_model_result_v1 cui_base_slider_model_validate_internal(
    cui_context *, cui_id, cui_slider_model_value_v1, const cui_slider_model_state_v1 *,
    const cui_slider_model_options_v1 *, const cui_base_slider_model_layout_v1 *,
    cui_slider_model_handler_v1, cui_id ids[5]);
cui_slider_model_result_v1 cui_base_slider_model_present_internal(
    cui_context *, cui_id, cui_slider_model_value_v1, cui_slider_model_state_v1 *,
    const cui_slider_model_options_v1 *, const cui_base_slider_model_layout_v1 *,
    cui_slider_model_handler_v1, const cui_slider_model_presentation_internal *);
#endif
