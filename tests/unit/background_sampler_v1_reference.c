/* SPDX-License-Identifier: MIT */
/* Test-only frozen source compiled with renamed public symbols. No installed API. */
#define cui_background_color_from_srgba8 background_v1_color_from_srgba8
#define cui_background_color_from_source_038_hsla background_v1_color_from_source_038_hsla
#define cui_background_valid background_v1_valid
#define cui_background_make_solid background_v1_make_solid
#define cui_background_make_linear background_v1_make_linear
#define cui_background_sample background_v1_sample
#define cui_background_sample_srgba8 background_v1_sample_srgba8
#include "../fixtures/background_sampler_v1.c"
