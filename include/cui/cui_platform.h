#ifndef CUI_PLATFORM_H
#define CUI_PLATFORM_H
#include "cui.h"
enum { CUI_PLATFORM_CLIPBOARD=1,CUI_PLATFORM_IME=2,CUI_PLATFORM_ACCESSIBILITY=4 };
typedef struct { void *user; unsigned capabilities; bool (*poll)(void *user,cui_event *event); bool (*clipboard_set)(void *user,cui_str text); cui_str (*clipboard_get)(void *user); void (*text_input)(void *user,bool enabled,cui_rect bounds); } cui_platform;
#endif
