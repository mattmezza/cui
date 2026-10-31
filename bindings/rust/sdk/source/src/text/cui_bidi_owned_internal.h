/* SPDX-License-Identifier: MIT */
#ifndef CUI_BIDI_OWNED_INTERNAL_H
#define CUI_BIDI_OWNED_INTERNAL_H
#include "cui/cui_bidi.h"
#include "cui_text_alloc_internal.h"
cui_bidi_status cui_bidi_paragraph_create_owned_internal(const char *, size_t, cui_bidi_direction,
                                                         uint32_t *, size_t, cui_bidi_paragraph **,
                                                         cui_text_owner_internal *);
void cui_bidi_paragraph_destroy_owned_internal(cui_bidi_paragraph *, cui_text_owner_internal *);
#endif
