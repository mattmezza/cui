/* SPDX-License-Identifier: MIT */
#ifndef CUI_THEME_H
#define CUI_THEME_H
#include "cui.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
 cui_color background,surface,text,muted,primary,on_primary,border,danger,focus;
 float font_size,padding,gap,radius,control_height;
} cui_theme;
cui_theme cui_theme_light(void);
cui_theme cui_theme_dark(void);
cui_theme cui_theme_high_contrast(void);
/* Independent, square-cornered amber/ink design system using the same base. */
cui_theme cui_theme_terminal(void);
#ifdef __cplusplus
}
#endif
#endif
