/* SPDX-License-Identifier: MIT */
#ifndef CUI_TABLE_INTERNAL_H
#define CUI_TABLE_INTERNAL_H
#include <stddef.h>

/* Bytes requested by cui_table_create, excluding allocator overhead. */
size_t cui_table_model_allocation_size_internal(size_t capacity);
#endif
