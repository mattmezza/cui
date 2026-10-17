/* SPDX-License-Identifier: MIT */
#ifndef CUI_RESPONSIVE_DASHBOARD_H
#define CUI_RESPONSIVE_DASHBOARD_H
#include "cui/cui.h"
typedef enum { DASH_NAVIGATED, DASH_ACCOUNT_CHANGED } dashboard_event;
typedef void (*dashboard_callback)(void *, dashboard_event, unsigned page, bool signed_in);
typedef struct {
    unsigned page;
    bool drawer_open, signed_in;
    dashboard_callback callback;
    void *user;
} dashboard_app;
enum {
    DASH_ROOT = 8100,
    DASH_MENU,
    DASH_NAV,
    DASH_HOME,
    DASH_REPORTS,
    DASH_ACCOUNT,
    DASH_MAIN,
    DASH_SUMMARY,
    DASH_ACTIVITY
};
/* No allocation. Application state survives container and breakpoint changes. */
void dashboard_init(dashboard_app *, dashboard_callback, void *);
/* Submit within an active frame. width/height describe this container, which
 * may be smaller than the host window. Caller supplies its placement. */
void dashboard_scene(cui_context *, dashboard_app *, float width, float height);
#endif
