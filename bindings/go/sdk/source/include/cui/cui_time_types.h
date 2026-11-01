/* SPDX-License-Identifier: MIT */
#ifndef CUI_TIME_TYPES_H
#define CUI_TIME_TYPES_H
/* Portable time-of-day value; unchanged layout shared by legacy picker APIs. */
typedef struct {
    unsigned hour, minute, second;
} cui_time;
#endif
