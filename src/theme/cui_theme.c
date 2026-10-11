/* SPDX-License-Identifier: MIT */
#include "cui/cui_theme.h"
#define RGB(r,g,b) {r,g,b,255}
cui_theme cui_theme_light(void) { return (cui_theme){RGB(246,248,250),RGB(255,255,255),RGB(24,32,42),RGB(87,99,112),RGB(23,91,169),RGB(255,255,255),RGB(181,191,204),RGB(179,35,39),RGB(23,91,169),16,10,8,6,42}; }
cui_theme cui_theme_dark(void) { return (cui_theme){RGB(18,23,30),RGB(30,38,48),RGB(235,240,246),RGB(164,176,190),RGB(116,178,250),RGB(14,24,38),RGB(102,118,137),RGB(255,137,142),RGB(164,209,255),16,10,8,6,42}; }
cui_theme cui_theme_high_contrast(void) { return (cui_theme){RGB(0,0,0),RGB(0,0,0),RGB(255,255,255),RGB(255,255,255),RGB(255,255,0),RGB(0,0,0),RGB(255,255,255),RGB(255,120,120),RGB(0,255,255),18,10,10,0,46}; }
cui_theme cui_theme_terminal(void) { return (cui_theme){RGB(31,29,23),RGB(45,42,33),RGB(255,227,164),RGB(205,182,140),RGB(255,192,77),RGB(31,29,23),RGB(148,126,84),RGB(255,137,103),RGB(255,227,164),16,12,6,0,44}; }
