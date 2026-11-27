package cui

/*
#cgo CFLAGS: -std=c11 -Wall -Wextra -Werror -Wno-error=unused-parameter -I${SRCDIR}/../sdk/source/include
// Go 1.23 generates an empty export stub with an unused parameter in _cgo_main.c.
// Keep that diagnostic visible without failing generated code; canonical C sources
// are compiled independently with all warnings fatal by the CMake checks.
#cgo linux LDFLAGS: -ldl -pthread
#include "cui/cui_bridge.h"
#if defined(_WIN32)
#include <windows.h>
typedef DWORD bridge_thread_token;
#else
#include <dlfcn.h>
#include <pthread.h>
typedef pthread_t bridge_thread_token;
#endif
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>

static void *bridge_lib;
#if defined(_WIN32)
static DWORD bridge_last_error;
static char bridge_error_buf[80];
#endif
#define API(X, V) X(abi_version, uint32_t, (void), ()) \
 X(options_size, uint32_t, (void), ()) \
 X(status_message, const char *, (cui_bridge_status s), (s)) \
 X(create, cui_bridge_app *, (const cui_bridge_options *o, cui_bridge_status *s), (o,s)) \
 V(destroy, void, (cui_bridge_app *a), (a)) \
 X(node_create, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node p, cui_bridge_kind k, const char *key, cui_bridge_node *out), (a,p,k,key,out)) \
 X(node_remove, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n), (a,n)) \
 X(set_text, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const char *v), (a,n,v)) \
 X(set_checked, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_disabled, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_value, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, double v), (a,n,v)) \
 X(set_range, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, double min, double max, double step), (a,n,min,max,step)) \
 X(get_checked, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t *out), (a,n,out)) \
 X(get_value, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, double *out), (a,n,out)) \
 X(get_text, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, char *dst, uint32_t cap, uint32_t *required), (a,n,dst,cap,required)) \
 X(queue_text, cui_bridge_status, (cui_bridge_app *a, const char *text), (a,text)) \
 X(poll_event_ex, cui_bridge_status, (cui_bridge_app *a, cui_bridge_event_ex *e, char *dst, uint32_t cap, uint32_t *required), (a,e,dst,cap,required)) \
 X(set_detail_text, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const char *v), (a,n,v)) \
 X(get_open, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t *out), (a,n,out)) \
 X(set_anchor, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_node anchor), (a,n,anchor)) \
 X(set_open, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_selected, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node group, cui_bridge_node item), (a,group,item)) \
 X(get_selected, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node group, cui_bridge_node *out), (a,group,out)) \
 X(set_scroll_extent, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, float w, float h), (a,n,w,h)) \
 X(queue_scroll, cui_bridge_status, (cui_bridge_app *a, float x, float y, float dx, float dy), (a,x,y,dx,dy)) \
 X(set_image_rgba, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const uint8_t *pixels, uint32_t w, uint32_t h, uint32_t stride, uint32_t count), (a,n,pixels,w,h,stride,count)) \
 X(set_time, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_time v), (a,n,v)) \
 X(get_time, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_time *out), (a,n,out)) \
 X(set_color_rgba, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(get_color_rgba, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t *out), (a,n,out)) \
 X(color_part_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t part, cui_bridge_rect *r), (a,n,part,r)) \
 X(set_date, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_date v), (a,n,v)) \
 X(get_date, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_date *out), (a,n,out)) \
 X(calendar_day_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_date day, cui_bridge_rect *r), (a,n,day,r)) \
 X(set_attachment_status, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_attachment_status v), (a,n,v)) \
 X(set_total, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_step_status, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(numeric_part_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t part, cui_bridge_rect *r), (a,n,part,r)) \
 X(set_visual_size, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, float w, float h), (a,n,w,h)) \
 X(set_gap, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, float v), (a,n,v)) \
 X(set_button_variant, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_variant v), (a,n,v)) \
 X(set_stepper_visual, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t enabled, uint32_t vertical, uint32_t density), (a,n,enabled,vertical,density)) \
 X(set_design_size, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_size v), (a,n,v)) \
 X(set_avatar_identity, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t enabled, uint32_t custom, uint32_t hue), (a,n,enabled,custom,hue)) \
 X(set_avatar_placeholder, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t enabled, const char *name), (a,n,enabled,name)) \
 X(set_button_size, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_size v), (a,n,v)) \
 X(set_button_rounding, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_rounding v, float radius), (a,n,v,radius)) \
 X(set_button_disabled, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_button_hover_rgba, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha), (a,n,r,g,b,alpha)) \
 X(queue_pointer, cui_bridge_status, (cui_bridge_app *a, cui_bridge_pointer_phase p, float x, float y), (a,p,x,y)) \
 X(queue_key, cui_bridge_status, (cui_bridge_app *a, cui_bridge_key_phase p, cui_bridge_key k, uint32_t m, uint32_t r), (a,p,k,m,r)) \
 X(set_font_fallbacks, cui_bridge_status, (cui_bridge_app *a, const char *const *paths, uint32_t count), (a,paths,count)) \
 X(resizable_handle_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node group, cui_bridge_node before, cui_bridge_rect *r), (a,group,before,r)) \
 X(set_text_view_format, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t format), (a,n,format)) \
 X(text_view_select, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t anchor, uint32_t cursor), (a,n,anchor,cursor)) \
 X(text_view_copy_selection, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, char *dst, uint32_t cap, uint32_t *required), (a,n,dst,cap,required)) \
 X(chart_set_data, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const cui_bridge_chart_datum *data, uint32_t count), (a,n,data,count)) \
 X(chart_set_kind, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t kind), (a,n,kind)) \
 X(chart_get_datum, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t index, char *label, uint32_t cap, uint32_t *required, double *value), (a,n,index,label,cap,required,value)) \
 X(data_table_set, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const char *const *headers, uint32_t columns, const char *const *cells, uint32_t rows), (a,n,headers,columns,cells,rows)) \
 X(data_table_get_cell, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t row, uint32_t col, char *dst, uint32_t cap, uint32_t *required), (a,n,row,col,dst,cap,required)) \
 X(data_table_get_state, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t *row, uint32_t *col, uint32_t *sort_col, uint32_t *sort_dir), (a,n,row,col,sort_col,sort_dir)) \
 X(data_table_cell_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t row, uint32_t col, cui_bridge_rect *r), (a,n,row,col,r)) \
 X(data_table_header_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t col, cui_bridge_rect *r), (a,n,col,r)) \
 X(plot_set_points, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const cui_bridge_plot_point *points, uint32_t count), (a,n,points,count)) \
 X(plot_get_point, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t index, cui_bridge_plot_point *out), (a,n,index,out)) \
 X(command_item_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node command, cui_bridge_node item, cui_bridge_rect *r), (a,command,item,r)) \
 X(virtual_list_item_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node list, cui_bridge_node item, cui_bridge_rect *r), (a,list,item,r)) \
 X(data_table_set_selection_mode, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t mode), (a,n,mode)) \
 X(set_root_padding, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, float padding), (a,n,padding)) \
 X(clipboard_set, cui_bridge_status, (cui_bridge_app *a, const char *text), (a,text)) \
 X(clipboard_get, cui_bridge_status, (cui_bridge_app *a, char *dst, uint32_t capacity, uint32_t *length), (a,dst,capacity,length)) \
 X(speech_start, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n), (a,n)) \
 X(speech_report, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, const char *text, uint32_t final_phrase), (a,n,text,final_phrase)) \
 X(speech_finish, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n), (a,n)) \
 X(speech_transcript, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, char *dst, uint32_t capacity, uint32_t *length), (a,n,dst,capacity,length)) \
 X(questionnaire_set_question, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t required, uint32_t multiple), (a,n,required,multiple)) \
 X(questionnaire_get_state, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_node *current, uint32_t *complete), (a,n,current,complete)) \
 X(questionnaire_choice_selected, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t *selected), (a,n,selected)) \
 X(questionnaire_next, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n), (a,n)) \
 X(questionnaire_previous, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n), (a,n)) \
 X(questionnaire_submit, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n), (a,n)) \
 X(dock_activate, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node dock, cui_bridge_node panel), (a,dock,panel)) \
 X(dock_move_split, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node dock, cui_bridge_node panel, cui_bridge_node reference, uint32_t axis, double ratio, uint32_t before), (a,dock,panel,reference,axis,ratio,before)) \
 X(dock_get_active, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node dock, cui_bridge_node *panel), (a,dock,panel)) \
 X(dock_panel_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node dock, cui_bridge_node panel, cui_bridge_rect *bounds), (a,dock,panel,bounds)) \
 X(dock_serialize, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node dock, char *dst, uint32_t capacity, uint32_t *required), (a,dock,dst,capacity,required)) \
 X(settings_set_query, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node settings, const char *query), (a,settings,query)) \
 X(settings_get_page, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node settings, cui_bridge_node *page), (a,settings,page)) \
 X(settings_set_page, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node settings, cui_bridge_node page), (a,settings,page)) \
 X(settings_reset_page, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node settings, cui_bridge_node page), (a,settings,page)) \
 X(settings_field_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node settings, cui_bridge_node field, cui_bridge_rect *bounds), (a,settings,field,bounds)) \
 X(theme_preset_count, uint32_t, (void), ()) \
 X(theme_preset_name, cui_bridge_status, (uint32_t index, char *dst, uint32_t capacity, uint32_t *length), (index,dst,capacity,length)) \
 X(set_theme_preset, cui_bridge_status, (cui_bridge_app *a, uint32_t index), (a,index)) \
 X(get_theme_preset, cui_bridge_status, (const cui_bridge_app *a, uint32_t *index), (a,index)) \
 X(step, cui_bridge_status, (cui_bridge_app *a, float dt), (a,dt)) \
 X(dropdown_part_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t menu, cui_bridge_rect *r), (a,n,menu,r)) \
 X(node_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_rect *r), (a,n,r)) \
 X(request_close, cui_bridge_status, (cui_bridge_app *a), (a))
#define DECL(name, ret, params, args) static ret (*fp_##name) params; static ret go_##name params { return fp_##name args; }
#define DECL_VOID(name, ret, params, args) static void (*fp_##name) params; static void go_##name params { fp_##name args; }
API(DECL, DECL_VOID)
#undef DECL
typedef cui_bridge_app *(*window_create_fn)(const cui_bridge_options *, const cui_bridge_window_options *, cui_bridge_status *);
typedef cui_bridge_status (*window_set_fn)(cui_bridge_app *, uint32_t);
typedef cui_bridge_status (*window_get_fn)(cui_bridge_app *, uint32_t *);
#if defined(CUI_GO_STATIC)
extern cui_bridge_app *cui_bridge_create_with_window_options(const cui_bridge_options *, const cui_bridge_window_options *, cui_bridge_status *) __attribute__((weak));
extern cui_bridge_status cui_bridge_set_window_visible(cui_bridge_app *, uint32_t) __attribute__((weak));
extern cui_bridge_status cui_bridge_get_window_visible(cui_bridge_app *, uint32_t *) __attribute__((weak));
#endif
static cui_bridge_app *go_create_window(const cui_bridge_options *o, uint32_t hidden, cui_bridge_status *status) {
 window_create_fn fn = NULL;
#if defined(CUI_GO_STATIC)
 if (!bridge_lib) fn = cui_bridge_create_with_window_options;
#endif
 if (bridge_lib) {
#if defined(_WIN32)
  FARPROC sym = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_create_with_window_options");
#else
  void *sym = dlsym(bridge_lib, "cui_bridge_create_with_window_options");
#endif
  if (sym) memcpy(&fn, &sym, sizeof fn);
 }
 if (!fn) { *status = CUI_BRIDGE_BACKEND_UNAVAILABLE; return NULL; }
 cui_bridge_window_options w = {sizeof(w), 1, hidden};
 return fn(o, &w, status);
}
static cui_bridge_status go_window_visible(cui_bridge_app *app, uint32_t value, uint32_t *out) {
 window_set_fn set = NULL; window_get_fn get = NULL;
#if defined(CUI_GO_STATIC)
 if (!bridge_lib) { set = cui_bridge_set_window_visible; get = cui_bridge_get_window_visible; }
#endif
 if (bridge_lib) {
#if defined(_WIN32)
  FARPROC ss = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_set_window_visible");
  FARPROC gs = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_get_window_visible");
#else
  void *ss = dlsym(bridge_lib, "cui_bridge_set_window_visible");
  void *gs = dlsym(bridge_lib, "cui_bridge_get_window_visible");
#endif
  if (ss) memcpy(&set, &ss, sizeof set);
  if (gs) memcpy(&get, &gs, sizeof get);
 }
 if (out) return get ? get(app, out) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
 return set ? set(app, value) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
typedef cui_bridge_status (*run_after_fn)(void *);
typedef cui_bridge_status (*run_fn)(cui_bridge_app *, run_after_fn, void *);
extern int goBridgeAfterFrame(uintptr_t user);
#if defined(CUI_GO_STATIC)
extern uint32_t cui_bridge_notification_v1_size(void) __attribute__((weak));
extern uint32_t cui_bridge_notification_versions(void) __attribute__((weak));
extern cui_bridge_status cui_bridge_poll_notification_v1(cui_bridge_app *, uint32_t, cui_bridge_notification_v1 *, char *, uint32_t, uint32_t *) __attribute__((weak));
extern uint32_t cui_bridge_notification_value_payloads_v1(void) __attribute__((weak));
extern cui_bridge_status cui_bridge_poll_value_notification_v1(cui_bridge_app *, uint32_t, uint32_t, cui_bridge_notification_v1 *, char *, uint32_t, uint32_t *) __attribute__((weak));
extern cui_bridge_status cui_bridge_run(cui_bridge_app *, run_after_fn, void *) __attribute__((weak));
#endif
static cui_bridge_status go_after_frame(void *user) {
 return (cui_bridge_status)goBridgeAfterFrame((uintptr_t)user);
}
static cui_bridge_status go_run(cui_bridge_app *app, uintptr_t user) {
 run_fn runner = NULL;
#if defined(CUI_GO_STATIC)
 if (!bridge_lib) runner = cui_bridge_run;
#endif
 if (bridge_lib) {
#if defined(_WIN32)
  FARPROC symbol = GetProcAddress((HMODULE)bridge_lib, "cui_bridge_run");
#else
  void *symbol = dlsym(bridge_lib, "cui_bridge_run");
#endif
  if (symbol) memcpy(&runner, &symbol, sizeof runner);
 }
 if (!runner) return CUI_BRIDGE_BACKEND_UNAVAILABLE;
 return runner(app, go_after_frame, (void *)user);
}
typedef uint32_t (*notification_query_fn)(void);
typedef cui_bridge_status (*notification_poll_fn)(cui_bridge_app *, uint32_t,
    cui_bridge_notification_v1 *, char *, uint32_t, uint32_t *);
static cui_bridge_status go_poll_notification_v1(cui_bridge_app *app,
    cui_bridge_notification_v1 *out, char *text, uint32_t capacity, uint32_t *required) {
 notification_query_fn size = NULL, versions = NULL;
 notification_poll_fn poll = NULL;
#if defined(CUI_GO_STATIC)
 if (!bridge_lib) {
  size = cui_bridge_notification_v1_size;
  versions = cui_bridge_notification_versions;
  poll = cui_bridge_poll_notification_v1;
 }
#endif
 if (bridge_lib) {
#if defined(_WIN32)
#define NOTIFY_LOAD(dst, name) do { FARPROC sym = GetProcAddress((HMODULE)bridge_lib, name); if (sym) memcpy(&dst, &sym, sizeof dst); } while (0)
#else
#define NOTIFY_LOAD(dst, name) do { void *sym = dlsym(bridge_lib, name); if (sym) memcpy(&dst, &sym, sizeof dst); } while (0)
#endif
  NOTIFY_LOAD(size, "cui_bridge_notification_v1_size");
  NOTIFY_LOAD(versions, "cui_bridge_notification_versions");
  NOTIFY_LOAD(poll, "cui_bridge_poll_notification_v1");
#undef NOTIFY_LOAD
 }
 if (!size || !versions || !poll || !(versions() & 1u))
  return CUI_BRIDGE_BACKEND_UNAVAILABLE;
 if (size() != sizeof(*out)) return CUI_BRIDGE_INVALID_ARGUMENT;
 return poll(app, (uint32_t)sizeof(*out), out, text, capacity, required);
}
typedef cui_bridge_status (*notification_value_poll_fn)(cui_bridge_app *, uint32_t, uint32_t,
    cui_bridge_notification_v1 *, char *, uint32_t, uint32_t *);
static cui_bridge_status go_value_notification_setup(uint32_t wanted, uint32_t required_mask,
    notification_value_poll_fn *out_poll, uint32_t *enabled_out) {
 notification_query_fn size = NULL, versions = NULL, payloads = NULL;
 notification_value_poll_fn poll = NULL;
#if defined(CUI_GO_STATIC)
 if (!bridge_lib) {
  size = cui_bridge_notification_v1_size;
  versions = cui_bridge_notification_versions;
  poll = cui_bridge_poll_value_notification_v1;
  payloads = cui_bridge_notification_value_payloads_v1;
 }
#endif
 if (bridge_lib) {
#if defined(_WIN32)
#define NOTIFY_LOAD(dst, name) do { FARPROC sym = GetProcAddress((HMODULE)bridge_lib, name); if (sym) memcpy(&dst, &sym, sizeof dst); } while (0)
#else
#define NOTIFY_LOAD(dst, name) do { void *sym = dlsym(bridge_lib, name); if (sym) memcpy(&dst, &sym, sizeof dst); } while (0)
#endif
  NOTIFY_LOAD(size, "cui_bridge_notification_v1_size");
  NOTIFY_LOAD(versions, "cui_bridge_notification_versions");
  NOTIFY_LOAD(payloads, "cui_bridge_notification_value_payloads_v1");
  NOTIFY_LOAD(poll, "cui_bridge_poll_value_notification_v1");
#undef NOTIFY_LOAD
 }
 if (!size || !versions || !payloads || !poll || !(versions() & 1u))
  return CUI_BRIDGE_BACKEND_UNAVAILABLE;
 if (size() != sizeof(cui_bridge_notification_v1)) return CUI_BRIDGE_INVALID_ARGUMENT;
 uint32_t enabled = payloads() & wanted;
 if (!enabled || (enabled & required_mask) != required_mask) return CUI_BRIDGE_BACKEND_UNAVAILABLE;
 *out_poll = poll;
 *enabled_out = enabled;
 return CUI_BRIDGE_OK;
}
static cui_bridge_status go_poll_value_notification_v1(cui_bridge_app *app,
    cui_bridge_notification_v1 *out, char *text, uint32_t capacity, uint32_t *required,
    uint32_t required_mask, uint32_t *enabled_out) {
 notification_value_poll_fn poll;
 uint32_t wanted = CUI_BRIDGE_VALUE_SCALAR_V1 | CUI_BRIDGE_VALUE_TEXT_V1 |
                   CUI_BRIDGE_VALUE_SELECTION_V1 | required_mask;
 cui_bridge_status status = go_value_notification_setup(wanted, required_mask, &poll, enabled_out);
 if (status != CUI_BRIDGE_OK) return status;
 return poll(app, (uint32_t)sizeof(*out), *enabled_out, out, text, capacity, required);
}
static double go_notification_scalar(const cui_bridge_notification_v1 *n) { return n->payload.scalar_value; }
static cui_bridge_node go_notification_selected(const cui_bridge_notification_v1 *n) { return n->payload.selected; }
static uint32_t go_notification_boolean(const cui_bridge_notification_v1 *n) { return n->payload.boolean_value; }
static cui_bridge_table_notification_v1 go_notification_table(const cui_bridge_notification_v1 *n) { return n->payload.table; }
static uint32_t go_notification_surface(const cui_bridge_notification_v1 *n) { return n->payload.surface_result; }
static cui_bridge_attachment_action_v1 go_notification_attachment(const cui_bridge_notification_v1 *n) {
 return n->payload.attachment_action;
}
static int bridge_initialized;
_Static_assert(sizeof(cui_bridge_box_shadow_v1)==24, "shadow V1 layer size");
_Static_assert(sizeof(cui_bridge_box_shadows_v1)==224, "shadow V1 style size");
_Static_assert(offsetof(cui_bridge_box_shadows_v1,layers)==32, "shadow V1 layers offset");
_Static_assert(offsetof(cui_bridge_box_shadow_v1,rgba)==20, "shadow V1 color offset");
typedef uint32_t (*shadow_size_fn)(void);
typedef cui_bridge_status (*shadow_set_fn)(cui_bridge_app *, cui_bridge_node, uint32_t, const cui_bridge_box_shadows_v1 *);
typedef cui_bridge_status (*shadow_reset_fn)(cui_bridge_app *, cui_bridge_node);
typedef cui_bridge_status (*shadow_css_fn)(float,float,float,float,uint32_t,uint32_t,uint32_t,cui_bridge_box_shadow_v1 *);
typedef cui_bridge_status (*shadow_preset_fn)(uint32_t,float,float,float,float,uint32_t,cui_bridge_box_shadows_v1 *);
#if defined(CUI_GO_STATIC)
extern uint32_t cui_bridge_box_shadows_v1_size(void) __attribute__((weak));
extern cui_bridge_status cui_bridge_set_box_shadows_v1(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_box_shadows_v1 *) __attribute__((weak));
extern cui_bridge_status cui_bridge_reset_box_shadows(cui_bridge_app *,cui_bridge_node) __attribute__((weak));
extern cui_bridge_status cui_bridge_box_shadow_from_css_v1(float,float,float,float,uint32_t,uint32_t,uint32_t,cui_bridge_box_shadow_v1 *) __attribute__((weak));
extern cui_bridge_status cui_bridge_shadow_preset_v1(uint32_t,float,float,float,float,uint32_t,cui_bridge_box_shadows_v1 *) __attribute__((weak));
#define SHADOW_STATIC(dst,name) if (!bridge_lib) dst=cui_bridge_##name
#else
#define SHADOW_STATIC(dst,name)
#endif
#if defined(_WIN32)
#define SHADOW_DYNAMIC(dst,name) do { if (bridge_lib) { FARPROC sym=GetProcAddress((HMODULE)bridge_lib,"cui_bridge_" #name); if(sym)memcpy(&dst,&sym,sizeof dst); } } while(0)
#else
#define SHADOW_DYNAMIC(dst,name) do { if (bridge_lib) { void *sym=dlsym(bridge_lib,"cui_bridge_" #name); if(sym)memcpy(&dst,&sym,sizeof dst); } } while(0)
#endif
#define SHADOW_RESOLVE(dst,name) do { SHADOW_STATIC(dst,name); SHADOW_DYNAMIC(dst,name); } while(0)
static cui_bridge_status go_shadow_ready(void) {
 shadow_size_fn size=NULL;SHADOW_RESOLVE(size,box_shadows_v1_size);
 return size && size()==sizeof(cui_bridge_box_shadows_v1) ? CUI_BRIDGE_OK : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_shadow_set(cui_bridge_app *a,cui_bridge_node n,const cui_bridge_box_shadows_v1 *v) {
 shadow_set_fn fn=NULL;SHADOW_RESOLVE(fn,set_box_shadows_v1);
 return fn && go_shadow_ready()==CUI_BRIDGE_OK ? fn(a,n,sizeof(*v),v) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_shadow_reset(cui_bridge_app *a,cui_bridge_node n) {
 shadow_reset_fn fn=NULL;SHADOW_RESOLVE(fn,reset_box_shadows);
 return fn && go_shadow_ready()==CUI_BRIDGE_OK ? fn(a,n) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_shadow_css(float x,float y,float radius,float spread,uint32_t rgba,uint32_t kind,cui_bridge_box_shadow_v1 *v) {
 shadow_css_fn fn=NULL;SHADOW_RESOLVE(fn,box_shadow_from_css_v1);
 return fn && go_shadow_ready()==CUI_BRIDGE_OK ? fn(x,y,radius,spread,rgba,kind,sizeof(*v),v) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_shadow_preset(uint32_t p,float tl,float tr,float bl,float br,cui_bridge_box_shadows_v1 *v) {
 shadow_preset_fn fn=NULL;SHADOW_RESOLVE(fn,shadow_preset_v1);
 return fn && go_shadow_ready()==CUI_BRIDGE_OK ? fn(p,tl,tr,bl,br,sizeof(*v),v) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
_Static_assert(sizeof(cui_bridge_shadow_target_v1)==16 && offsetof(cui_bridge_shadow_target_v1,role)==8,"part target V1");
_Static_assert(sizeof(cui_bridge_shadow_target_info_v1)==16 && offsetof(cui_bridge_shadow_target_info_v1,flags)==12,"part info V1");
typedef cui_bridge_status (*part_set_fn)(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_shadow_target_v1 *,uint32_t,const cui_bridge_box_shadows_v1 *);
typedef cui_bridge_status (*part_reset_fn)(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_shadow_target_v1 *);
typedef cui_bridge_status (*part_query_fn)(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_shadow_target_v1 *,uint32_t,cui_bridge_shadow_target_info_v1 *);
#if defined(CUI_GO_STATIC)
extern cui_bridge_status cui_bridge_set_target_box_shadows_v1(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_shadow_target_v1 *,uint32_t,const cui_bridge_box_shadows_v1 *) __attribute__((weak));
extern cui_bridge_status cui_bridge_reset_target_box_shadows_v1(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_shadow_target_v1 *) __attribute__((weak));
extern cui_bridge_status cui_bridge_shadow_target_query_v1(cui_bridge_app *,cui_bridge_node,uint32_t,const cui_bridge_shadow_target_v1 *,uint32_t,cui_bridge_shadow_target_info_v1 *) __attribute__((weak));
#endif
static cui_bridge_status go_part_set(cui_bridge_app *a,cui_bridge_node n,uint32_t role,const cui_bridge_box_shadows_v1 *style) {
 part_set_fn fn=NULL;SHADOW_RESOLVE(fn,set_target_box_shadows_v1);cui_bridge_shadow_target_v1 target={16,1,role,0};
 return fn?fn(a,n,16,&target,224,style):CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_part_reset(cui_bridge_app *a,cui_bridge_node n,uint32_t role) {
 part_reset_fn fn=NULL;SHADOW_RESOLVE(fn,reset_target_box_shadows_v1);cui_bridge_shadow_target_v1 target={16,1,role,0};
 return fn?fn(a,n,16,&target):CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
static cui_bridge_status go_part_query(cui_bridge_app *a,cui_bridge_node n,uint32_t role,cui_bridge_shadow_target_info_v1 *out) {
 part_query_fn fn=NULL;SHADOW_RESOLVE(fn,shadow_target_query_v1);cui_bridge_shadow_target_v1 target={16,1,role,0};
 return fn?fn(a,n,16,&target,16,out):CUI_BRIDGE_BACKEND_UNAVAILABLE;
}
#undef SHADOW_RESOLVE
#undef SHADOW_DYNAMIC
#undef SHADOW_STATIC

#include "skeleton_presentation_ffi.h"
#include "spinner_presentation_ffi.h"
#include "icon_resources_ffi.h"
#include "icon_badge_ffi.h"

typedef cui_bridge_status (*data_table_keyed_fn)(cui_bridge_app *, cui_bridge_node,
 const char *const *, uint32_t, const char *const *, const uint32_t *, uint32_t);
static data_table_keyed_fn table_keyed;
static struct {
 uint32_t (*maximum)(void);
 cui_bridge_status (*set)(cui_bridge_app *, cui_bridge_node, const char *const *,
                          uint32_t, const char *const *, const uint32_t *, uint32_t, uint32_t);
} table_capacity;

_Static_assert(sizeof(cui_bridge_data_table_load_more_v1) == 32, "DataTable paging V1 wire size");
static struct {
 uint32_t (*size)(void);
 cui_bridge_status (*configure)(cui_bridge_app *, cui_bridge_node, uint32_t, uint32_t);
 cui_bridge_status (*poll)(cui_bridge_app *, cui_bridge_node, uint32_t,
                          cui_bridge_data_table_load_more_v1 *);
} table_paging;
static int table_paging_supported;

static void go_load_optional_symbol(void *destination, size_t size, const char *name) {
#if defined(_WIN32)
 FARPROC symbol = GetProcAddress((HMODULE)bridge_lib, name);
#else
 void *symbol = dlsym(bridge_lib, name);
#endif
 if (symbol && sizeof symbol == size)
  memcpy(destination, &symbol, size);
}
#include "spinner_icon_ffi.h"

#include "pagination_ffi.h"
#include "rating_ffi.h"

// Optional symbol: admission of older providers remains unchanged.
#if defined(CUI_GO_STATIC)
extern cui_bridge_status cui_bridge_set_stepper_fill(cui_bridge_app *, cui_bridge_node, uint32_t, uint32_t, float, float);
#pragma weak cui_bridge_set_stepper_fill
#endif
static cui_bridge_status (*stepper_fill_set)(cui_bridge_app *, cui_bridge_node, uint32_t, uint32_t, float, float);
static void go_stepper_fill_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&stepper_fill_set, sizeof stepper_fill_set, "cui_bridge_set_stepper_fill");
 }
#if defined(CUI_GO_STATIC)
 else { stepper_fill_set = cui_bridge_set_stepper_fill; }
#endif
}
static int go_stepper_fill_supported(void) { return stepper_fill_set != NULL; }
static cui_bridge_status go_stepper_fill(cui_bridge_app *a, cui_bridge_node n, uint32_t vertical, uint32_t density, float width, float height) {
 return stepper_fill_set ? stepper_fill_set(a,n,vertical,density,width,height) : CUI_BRIDGE_INVALID_ARGUMENT;
}

// Optional symbol: admission of older providers remains unchanged.
#if defined(CUI_GO_STATIC)
extern cui_bridge_status cui_bridge_set_stepper_wrapped_fill_v1(cui_bridge_app *, cui_bridge_node, uint32_t, uint32_t, float, float);
#pragma weak cui_bridge_set_stepper_wrapped_fill_v1
#endif
static cui_bridge_status (*stepper_wrapped_fill_set)(cui_bridge_app *, cui_bridge_node, uint32_t, uint32_t, float, float);
static void go_stepper_wrapped_fill_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&stepper_wrapped_fill_set, sizeof stepper_wrapped_fill_set, "cui_bridge_set_stepper_wrapped_fill_v1");
 }
#if defined(CUI_GO_STATIC)
 else { stepper_wrapped_fill_set = cui_bridge_set_stepper_wrapped_fill_v1; }
#endif
}
static int go_stepper_wrapped_fill_supported(void) { return stepper_wrapped_fill_set != NULL; }
static cui_bridge_status go_stepper_wrapped_fill(cui_bridge_app *a, cui_bridge_node n, uint32_t vertical, uint32_t density, float width, float height) {
 return stepper_wrapped_fill_set ? stepper_wrapped_fill_set(a,n,vertical,density,width,height) : CUI_BRIDGE_INVALID_ARGUMENT;
}


static cui_bridge_status (*radio_tooltip_set)(cui_bridge_app *, cui_bridge_node, const char *, uint32_t);
static void go_radio_tooltip_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&radio_tooltip_set, sizeof radio_tooltip_set,
                          "cui_bridge_radio_set_tooltip_v1");
 }
#if defined(CUI_GO_STATIC)
 else { radio_tooltip_set = cui_bridge_radio_set_tooltip_v1; }
#endif
}
static int go_radio_tooltip_supported(void) { return radio_tooltip_set != NULL; }
static cui_bridge_status go_set_radio_tooltip(cui_bridge_app *app, cui_bridge_node node,
                                               const char *text, uint32_t delay) {
 return radio_tooltip_set ? radio_tooltip_set(app, node, text, delay)
                         : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status (*radio_paint_set)(cui_bridge_app *, cui_bridge_node, uint32_t, const cui_bridge_radio_paint_v1 *);
static void go_radio_paint_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&radio_paint_set, sizeof radio_paint_set, "cui_bridge_radio_set_paint_v1");
 }
#if defined(CUI_GO_STATIC)
 else { radio_paint_set = cui_bridge_radio_set_paint_v1; }
#endif
}
static int go_radio_paint_supported(void) { return radio_paint_set != NULL; }
static cui_bridge_status go_set_radio_paint(cui_bridge_app *app, cui_bridge_node node,
                                           uint32_t layer, const cui_bridge_radio_paint_v1 *paint) {
 return radio_paint_set ? radio_paint_set(app, node, layer, paint) : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status (*responsive_notifications_set)(cui_bridge_app *, uint32_t);
static void go_responsive_notifications_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&responsive_notifications_set, sizeof responsive_notifications_set,
                          "cui_bridge_set_responsive_notifications_v1");
 }
#if defined(CUI_GO_STATIC)
 else { responsive_notifications_set = cui_bridge_set_responsive_notifications_v1; }
#endif
}
static int go_responsive_notifications_supported(void) { return responsive_notifications_set != NULL; }
static cui_bridge_status go_set_responsive_notifications(cui_bridge_app *app, uint32_t enabled) {
 return responsive_notifications_set ? responsive_notifications_set(app, enabled)
                                     : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status (*radio_motion_set)(cui_bridge_app *, cui_bridge_node, uint32_t);

static cui_bridge_status (*radio_content_set)(cui_bridge_app *, cui_bridge_node, uint32_t);
static void go_radio_content_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&radio_content_set, sizeof radio_content_set,
                          "cui_bridge_radio_item_set_content_v1");
 }
#if defined(CUI_GO_STATIC)
 else { radio_content_set = cui_bridge_radio_item_set_content_v1; }
#endif
}
static int go_radio_content_supported(void) { return radio_content_set != NULL; }
static cui_bridge_status go_set_radio_content(cui_bridge_app *app, cui_bridge_node node,
                                              uint32_t enabled) {
 return radio_content_set ? radio_content_set(app, node, enabled)
                          : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status (*radio_tab_index_set)(cui_bridge_app *, cui_bridge_node, uint32_t, int32_t);
static void go_radio_tab_index_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&radio_tab_index_set, sizeof radio_tab_index_set,
                         "cui_bridge_radio_set_tab_index_v1");
 }
#if defined(CUI_GO_STATIC)
 else { radio_tab_index_set = cui_bridge_radio_set_tab_index_v1; }
#endif
}
static int go_radio_tab_index_supported(void) { return radio_tab_index_set != NULL; }
static cui_bridge_status go_set_radio_tab_index(cui_bridge_app *app, cui_bridge_node node,
                                                uint32_t enabled, int32_t index) {
 return radio_tab_index_set ? radio_tab_index_set(app, node, enabled, index)
                            : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static void go_radio_motion_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&radio_motion_set, sizeof radio_motion_set,
                          "cui_bridge_radio_set_motion_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  radio_motion_set = cui_bridge_radio_set_motion_v1;
 }
#endif
}

static int go_radio_motion_supported(void) {
 return radio_motion_set != NULL;
}

static cui_bridge_status go_set_radio_motion(cui_bridge_app *app, cui_bridge_node node,
                                             uint32_t enabled) {
 return radio_motion_set ? radio_motion_set(app, node, enabled)
                         : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static void go_data_table_initialize(void) {
 if (!bridge_lib) {
#if defined(CUI_GO_STATIC)
  table_keyed = cui_bridge_data_table_set_keyed;
  table_capacity.maximum = cui_bridge_data_table_max_row_capacity_v1;
  table_capacity.set = cui_bridge_data_table_set_keyed_capacity_v1;
  table_paging.size = cui_bridge_data_table_load_more_size_v1;
  table_paging.configure = cui_bridge_data_table_configure_load_more_v1;
  table_paging.poll = cui_bridge_data_table_poll_load_more_v1;
#endif
 } else {
  go_load_optional_symbol(&table_keyed, sizeof table_keyed, "cui_bridge_data_table_set_keyed");
  go_load_optional_symbol(&table_capacity.maximum, sizeof table_capacity.maximum,
                            "cui_bridge_data_table_max_row_capacity_v1");
  go_load_optional_symbol(&table_capacity.set, sizeof table_capacity.set,
                            "cui_bridge_data_table_set_keyed_capacity_v1");
  go_load_optional_symbol(&table_paging.size, sizeof table_paging.size,
                            "cui_bridge_data_table_load_more_size_v1");
  go_load_optional_symbol(&table_paging.configure, sizeof table_paging.configure,
                            "cui_bridge_data_table_configure_load_more_v1");
  go_load_optional_symbol(&table_paging.poll, sizeof table_paging.poll,
                            "cui_bridge_data_table_poll_load_more_v1");
 }
 table_paging_supported = table_paging.size && table_paging.configure && table_paging.poll &&
                          table_paging.size() == sizeof(cui_bridge_data_table_load_more_v1);
}

static int go_data_table_keyed_supported(void) {
 return table_keyed != NULL;
}

static cui_bridge_status go_data_table_set_keyed(cui_bridge_app *app, cui_bridge_node node,
 const char *const *headers, uint32_t columns, const char *const *cells,
 const uint32_t *row_keys, uint32_t rows) {
 return table_keyed ? table_keyed(app, node, headers, columns, cells, row_keys, rows)
                    : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static uint32_t go_data_table_max_row_capacity(void) {
 return table_capacity.maximum && table_capacity.set ? table_capacity.maximum() : 0;
}

static cui_bridge_status go_data_table_set_keyed_capacity(cui_bridge_app *app,
 cui_bridge_node node, const char *const *headers, uint32_t columns, const char *const *cells,
 const uint32_t *row_keys, uint32_t rows, uint32_t capacity) {
 return table_capacity.maximum && table_capacity.set
        ? table_capacity.set(app, node, headers, columns, cells, row_keys, rows, capacity)
        : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static int go_data_table_paging_supported(void) {
 return table_paging_supported;
}

static cui_bridge_status go_data_table_configure_load_more(cui_bridge_app *app,
 cui_bridge_node node, uint32_t enabled, uint32_t threshold) {
 return table_paging_supported ? table_paging.configure(app, node, enabled, threshold)
                               : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static cui_bridge_status go_data_table_poll_load_more(cui_bridge_app *app,
 cui_bridge_node node, cui_bridge_data_table_load_more_v1 *out) {
 return table_paging_supported ? table_paging.poll(app, node, sizeof(*out), out)
                               : CUI_BRIDGE_BACKEND_UNAVAILABLE;
}

static struct {
 cui_bridge_status (*set_rows)(cui_bridge_app *, cui_bridge_node, uint32_t,
                               const cui_bridge_virtual_list_row_v1 *, uint32_t);
 cui_bridge_status (*preserve_anchor)(cui_bridge_app *, cui_bridge_node, uint32_t);
 cui_bridge_status (*set_axis)(cui_bridge_app *, cui_bridge_node, uint32_t);
} virtual_list_api;

static void go_virtual_list_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&virtual_list_api.set_axis, sizeof virtual_list_api.set_axis,
                          "cui_bridge_virtual_list_set_axis_v1");
  go_load_optional_symbol(&virtual_list_api.set_rows, sizeof virtual_list_api.set_rows,
                          "cui_bridge_virtual_list_set_rows_v1");
  go_load_optional_symbol(&virtual_list_api.preserve_anchor, sizeof virtual_list_api.preserve_anchor,
                          "cui_bridge_virtual_list_preserve_anchor_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  virtual_list_api.set_axis = cui_bridge_virtual_list_set_axis_v1;
  virtual_list_api.set_rows = cui_bridge_virtual_list_set_rows_v1;
  virtual_list_api.preserve_anchor = cui_bridge_virtual_list_preserve_anchor_v1;
 }
#endif
}

static int go_virtual_list_axis_supported(void) { return virtual_list_api.set_axis != NULL; }
static cui_bridge_status go_virtual_list_set_axis(cui_bridge_app *app, cui_bridge_node list, uint32_t axis) {
 return virtual_list_api.set_axis(app, list, axis);
}

static int go_virtual_list_rows_supported(void) {
 return virtual_list_api.set_rows && virtual_list_api.preserve_anchor;
}

static cui_bridge_status go_virtual_list_set_rows(cui_bridge_app *app, cui_bridge_node list,
 const cui_bridge_virtual_list_row_v1 *rows, uint32_t count) {
 return virtual_list_api.set_rows(app, list, sizeof *rows, rows, count);
}

static cui_bridge_status go_virtual_list_preserve_anchor(cui_bridge_app *app,
 cui_bridge_node list, uint32_t enabled) {
 return virtual_list_api.preserve_anchor(app, list, enabled);
}

static cui_bridge_status (*disclosure_configure)(cui_bridge_app *, cui_bridge_node,
 const cui_bridge_disclosure_config_v1 *);
static cui_bridge_status (*disclosure_style)(cui_bridge_app *, cui_bridge_node,
 const cui_bridge_disclosure_trigger_style_v1 *);
static cui_bridge_status (*disclosure_heading)(cui_bridge_app *, cui_bridge_node, uint32_t,
 const cui_bridge_disclosure_heading_v1 *);
static cui_bridge_status (*disclosure_title)(cui_bridge_app *, cui_bridge_node, uint32_t,
 const cui_bridge_disclosure_heading_v1 *);
static cui_bridge_status (*disclosure_divider)(cui_bridge_app *, cui_bridge_node,
 const cui_bridge_disclosure_divider_v1 *);
static void go_disclosure_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&disclosure_configure, sizeof disclosure_configure,
                          "cui_bridge_disclosure_configure_v1");
  go_load_optional_symbol(&disclosure_style, sizeof disclosure_style,
                          "cui_bridge_disclosure_set_trigger_style_v1");
  go_load_optional_symbol(&disclosure_heading, sizeof disclosure_heading,
                          "cui_bridge_disclosure_set_heading_v1");
  go_load_optional_symbol(&disclosure_title, sizeof disclosure_title,
                          "cui_bridge_disclosure_set_title_state_v1");
  go_load_optional_symbol(&disclosure_divider, sizeof disclosure_divider,
                          "cui_bridge_disclosure_set_divider_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  disclosure_configure = cui_bridge_disclosure_configure_v1;
  disclosure_style = cui_bridge_disclosure_set_trigger_style_v1;
  disclosure_heading = cui_bridge_disclosure_set_heading_v1;
  disclosure_title = cui_bridge_disclosure_set_title_state_v1;
  disclosure_divider = cui_bridge_disclosure_set_divider_v1;
 }
#endif
}
static int go_disclosure_supported(void) { return disclosure_configure && disclosure_style; }
static int go_disclosure_heading_supported(void) { return disclosure_heading != NULL; }
static int go_disclosure_title_supported(void) { return disclosure_title != NULL; }
static int go_disclosure_divider_supported(void) { return disclosure_divider != NULL; }
static cui_bridge_status go_disclosure_configure(cui_bridge_app *app, cui_bridge_node node,
 const cui_bridge_disclosure_config_v1 *config) { return disclosure_configure(app, node, config); }
static cui_bridge_status go_disclosure_style(cui_bridge_app *app, cui_bridge_node node,
 const cui_bridge_disclosure_trigger_style_v1 *style) { return disclosure_style(app, node, style); }
static cui_bridge_status go_disclosure_heading(cui_bridge_app *app, cui_bridge_node node,
 uint32_t layer, const cui_bridge_disclosure_heading_v1 *style) {
 return disclosure_heading(app, node, layer, style);
}
static cui_bridge_status go_disclosure_title(cui_bridge_app *app, cui_bridge_node node,
 uint32_t state, const cui_bridge_disclosure_heading_v1 *style) {
 return disclosure_title(app, node, state, style);
}
static cui_bridge_status go_disclosure_divider(cui_bridge_app *app, cui_bridge_node node,
 const cui_bridge_disclosure_divider_v1 *style) { return disclosure_divider(app, node, style); }

static cui_bridge_status (*attachment_configure)(cui_bridge_app *, cui_bridge_node,
 const cui_bridge_attachment_config_v1 *);
static cui_bridge_status (*attachment_preview)(cui_bridge_app *, cui_bridge_node,
 const uint8_t *, uint32_t, uint32_t, uint32_t, uint32_t);
static cui_bridge_status (*attachment_slot_create)(cui_bridge_app *, cui_bridge_node, uint32_t, const char *, float, cui_bridge_node *);
static cui_bridge_status (*notification_action_create)(cui_bridge_app *, cui_bridge_node, const char *, cui_bridge_node *);
static cui_bridge_status (*notification_auto_hide)(cui_bridge_app *, cui_bridge_node, uint32_t);
static int go_notification_auto_hide_supported(void) { return notification_auto_hide != NULL; }
static cui_bridge_status go_notification_auto_hide(cui_bridge_app *a, cui_bridge_node n, uint32_t enabled) { return notification_auto_hide(a,n,enabled); }
static int go_notification_action_supported(void) { return notification_action_create != NULL; }
static cui_bridge_status go_notification_action_create(cui_bridge_app *a, cui_bridge_node n, const char *key, cui_bridge_node *out) { return notification_action_create(a,n,key,out); }
static int go_attachment_slots_supported(void) { return attachment_slot_create != NULL; }
static cui_bridge_status go_attachment_slot_create(cui_bridge_app *a, cui_bridge_node n, uint32_t role, const char *key, float width, cui_bridge_node *out) { return attachment_slot_create(a,n,role,key,width,out); }
static cui_bridge_status (*attachment_set_actions)(cui_bridge_app *, cui_bridge_node, uint32_t);

static void go_attachment_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&notification_auto_hide, sizeof notification_auto_hide, "cui_bridge_notification_set_auto_hide_v1");
  go_load_optional_symbol(&notification_action_create, sizeof notification_action_create, "cui_bridge_notification_action_create_v1");
  go_load_optional_symbol(&attachment_slot_create, sizeof attachment_slot_create, "cui_bridge_attachment_slot_create_v1");
  go_load_optional_symbol(&attachment_configure, sizeof attachment_configure,
                          "cui_bridge_attachment_configure_v1");
  go_load_optional_symbol(&attachment_preview, sizeof attachment_preview,
                          "cui_bridge_attachment_set_preview_rgba_v1");
  go_load_optional_symbol(&attachment_set_actions, sizeof attachment_set_actions,
                          "cui_bridge_attachment_set_actions_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  notification_auto_hide = cui_bridge_notification_set_auto_hide_v1;
  notification_action_create = cui_bridge_notification_action_create_v1;
  attachment_slot_create = cui_bridge_attachment_slot_create_v1;
  attachment_configure = cui_bridge_attachment_configure_v1;
  attachment_preview = cui_bridge_attachment_set_preview_rgba_v1;
  attachment_set_actions = cui_bridge_attachment_set_actions_v1;
 }
#endif
}

static int go_attachment_configure_supported(void) { return attachment_configure != NULL; }
static int go_attachment_preview_supported(void) { return attachment_preview != NULL; }
static int go_attachment_actions_supported(void) {
 notification_value_poll_fn poll;
 uint32_t enabled;
 return attachment_set_actions && go_value_notification_setup(CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1,
  CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1, &poll, &enabled) == CUI_BRIDGE_OK;
}
static cui_bridge_status go_attachment_set_actions(cui_bridge_app *app, cui_bridge_node node, uint32_t mask) {
 return attachment_set_actions(app, node, mask);
}
static cui_bridge_status go_attachment_configure(cui_bridge_app *a, cui_bridge_node n,
 const cui_bridge_attachment_config_v1 *options) {
 return attachment_configure(a, n, options);
}
static cui_bridge_status go_attachment_preview(cui_bridge_app *a, cui_bridge_node n,
 const uint8_t *pixels, uint32_t w, uint32_t h, uint32_t stride, uint32_t length) {
 return attachment_preview(a, n, pixels, w, h, stride, length);
}

typedef cui_bridge_status (*carousel_navigate_fn)(cui_bridge_app *, cui_bridge_node,
                                                   uint32_t, cui_bridge_node *);
static carousel_navigate_fn carousel_adjacent, carousel_step;
static cui_bridge_status (*carousel_set_basis)(cui_bridge_app *, cui_bridge_node, float);
static cui_bridge_status (*carousel_set_fit)(cui_bridge_app *, cui_bridge_node, uint32_t);
static cui_bridge_status (*carousel_set_axis)(cui_bridge_app *, cui_bridge_node, uint32_t);
static cui_bridge_status (*carousel_set_looping)(cui_bridge_app *, cui_bridge_node, uint32_t);
static cui_bridge_status (*carousel_set_controls)(cui_bridge_app *, cui_bridge_node, uint32_t);
static cui_bridge_status (*carousel_set_control_sizes)(cui_bridge_app *, cui_bridge_node,
                                                       uint32_t, uint32_t);
static cui_bridge_status (*carousel_create_slot)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                                 const char *, cui_bridge_node *);
static cui_bridge_status (*kbd_stroke_set)(cui_bridge_app *, cui_bridge_node, uint32_t,
                                           uint32_t, const char *, size_t, const char *, size_t);
static int go_kbd_stroke_supported(void) { return kbd_stroke_set != NULL; }
static cui_bridge_status go_kbd_stroke_set(cui_bridge_app *a, cui_bridge_node n, uint32_t m,
                                           uint32_t f, const char *k, size_t kb,
                                           const char *name, size_t nb) {
 return kbd_stroke_set(a, n, m, f, k, kb, name, nb);
}
static cui_bridge_status (*kbd_style_set)(cui_bridge_app *, cui_bridge_node, const cui_bridge_kbd_style_v1 *);
static int go_kbd_style_supported(void) { return kbd_style_set != NULL; }
static cui_bridge_status go_kbd_style_set(cui_bridge_app *a, cui_bridge_node n, const cui_bridge_kbd_style_v1 *s) { return kbd_style_set(a, n, s); }
static void go_kbd_stroke_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&kbd_style_set, sizeof kbd_style_set, "cui_bridge_set_kbd_style_v1");
  go_load_optional_symbol(&kbd_stroke_set, sizeof kbd_stroke_set,
                          "cui_bridge_set_kbd_stroke_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  kbd_style_set = cui_bridge_set_kbd_style_v1;
  kbd_stroke_set = cui_bridge_set_kbd_stroke_v1;
 }
#endif
}
static void go_carousel_initialize(void) {
 if (bridge_lib) {
  go_load_optional_symbol(&carousel_adjacent, sizeof carousel_adjacent,
                          "cui_bridge_carousel_adjacent_v1");
  go_load_optional_symbol(&carousel_step, sizeof carousel_step,
                          "cui_bridge_carousel_step_v1");
  go_load_optional_symbol(&carousel_set_basis, sizeof carousel_set_basis,
                          "cui_bridge_set_carousel_slide_basis_v1");
  go_load_optional_symbol(&carousel_set_fit, sizeof carousel_set_fit,
                          "cui_bridge_set_carousel_slide_fit_v1");
  go_load_optional_symbol(&carousel_set_axis, sizeof carousel_set_axis,
                          "cui_bridge_set_carousel_axis_v1");
  go_load_optional_symbol(&carousel_set_looping, sizeof carousel_set_looping,
                          "cui_bridge_set_carousel_looping_v1");
  go_load_optional_symbol(&carousel_set_controls, sizeof carousel_set_controls,
                          "cui_bridge_set_carousel_controls_v1");
  go_load_optional_symbol(&carousel_set_control_sizes, sizeof carousel_set_control_sizes,
                          "cui_bridge_set_carousel_control_sizes_v1");
  go_load_optional_symbol(&carousel_create_slot, sizeof carousel_create_slot,
                          "cui_bridge_carousel_slot_create_v1");
 }
#if defined(CUI_GO_STATIC)
 else {
  carousel_adjacent = cui_bridge_carousel_adjacent_v1;
  carousel_step = cui_bridge_carousel_step_v1;
  carousel_set_basis = cui_bridge_set_carousel_slide_basis_v1;
  carousel_set_fit = cui_bridge_set_carousel_slide_fit_v1;
  carousel_set_axis = cui_bridge_set_carousel_axis_v1;
  carousel_set_looping = cui_bridge_set_carousel_looping_v1;
  carousel_set_controls = cui_bridge_set_carousel_controls_v1;
  carousel_set_control_sizes = cui_bridge_set_carousel_control_sizes_v1;
  carousel_create_slot = cui_bridge_carousel_slot_create_v1;
 }
#endif
}
static int go_carousel_navigation_supported(void) { return carousel_adjacent && carousel_step; }
static cui_bridge_status go_carousel_navigate(cui_bridge_app *a, cui_bridge_node carousel,
                                               uint32_t next, int commit, cui_bridge_node *out) {
 return (commit ? carousel_step : carousel_adjacent)(a, carousel, next, out);
}
static int go_carousel_basis_supported(void) { return carousel_set_basis != NULL; }
static cui_bridge_status go_carousel_set_basis(cui_bridge_app *a, cui_bridge_node slide, float basis) {
 return carousel_set_basis(a, slide, basis);
}
static int go_carousel_fit_supported(void) { return carousel_set_fit != NULL; }
static cui_bridge_status go_carousel_set_fit(cui_bridge_app *a, cui_bridge_node slide, uint32_t enabled) {
 return carousel_set_fit(a, slide, enabled);
}
static int go_carousel_axis_supported(void) { return carousel_set_axis != NULL; }
static cui_bridge_status go_carousel_set_axis(cui_bridge_app *a, cui_bridge_node carousel, uint32_t axis) {
 return carousel_set_axis(a, carousel, axis);
}

static int go_carousel_looping_supported(void) { return carousel_set_looping != NULL; }
static cui_bridge_status go_carousel_set_looping(cui_bridge_app *a, cui_bridge_node carousel, uint32_t enabled) {
 return carousel_set_looping(a, carousel, enabled);
}

static int go_carousel_controls_supported(void) { return carousel_set_controls != NULL; }
static cui_bridge_status go_carousel_set_controls(cui_bridge_app *a, cui_bridge_node carousel, uint32_t mask) {
 return carousel_set_controls(a, carousel, mask);
}
static int go_carousel_control_sizes_supported(void) { return carousel_set_control_sizes != NULL; }
static cui_bridge_status go_carousel_set_control_sizes(cui_bridge_app *a, cui_bridge_node carousel,
                                                       uint32_t arrows, uint32_t pagination) {
 return carousel_set_control_sizes(a, carousel, arrows, pagination);
}
static int go_carousel_slots_supported(void) { return carousel_create_slot != NULL; }
static cui_bridge_status go_carousel_create_slot(cui_bridge_app *a, cui_bridge_node owner,
 uint32_t role, const char *key, cui_bridge_node *out) {
 return carousel_create_slot(a, owner, role, key, out);
}

static int bridge_open(const char *path) {
 if (bridge_initialized) return 0;
#if defined(CUI_GO_STATIC)
 if (!path[0]) {
#define STATIC_LOAD(name, ret, params, args) fp_##name = cui_bridge_##name;
 API(STATIC_LOAD, STATIC_LOAD)
#undef STATIC_LOAD
 go_data_table_initialize();
 go_skeleton_initialize();
 go_spinner_initialize();
 go_spinner_icon_initialize();
 go_pagination_initialize();
 go_rating_initialize();
 go_responsive_notifications_initialize();
 go_radio_motion_initialize();
 go_radio_paint_initialize();
 go_radio_tab_index_initialize();
 go_radio_content_initialize();
 go_radio_tooltip_initialize();
 go_stepper_fill_initialize();
 go_stepper_wrapped_fill_initialize();
 go_virtual_list_initialize();
 go_disclosure_initialize();
 go_attachment_initialize();
 go_carousel_initialize();
 go_kbd_stroke_initialize();
 cui_go_icon_resources_init(bridge_lib);
 cui_go_icon_badge_init(bridge_lib);
 bridge_initialized = 1;
 return 0;
 }
#endif
#if defined(_WIN32)
 int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, NULL, 0);
 if (count <= 0) { bridge_last_error = GetLastError(); return -3; }
 wchar_t *wide = malloc((size_t)count * sizeof *wide);
 if (!wide) { bridge_last_error = ERROR_OUTOFMEMORY; return -4; }
 if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, path, -1, wide, count) != count) {
   bridge_last_error = GetLastError(); free(wide); return -3;
 }
 HMODULE lib = LoadLibraryW(wide);
 free(wide);
 if (!lib) { bridge_last_error = GetLastError(); return -1; }
#define LOAD(name, ret, params, args) do { FARPROC sym = GetProcAddress(lib, "cui_bridge_" #name); if (!sym) { bridge_last_error = GetLastError(); FreeLibrary(lib); return -2; } memcpy(&fp_##name, &sym, sizeof sym); } while (0);
#else
 void *lib = dlopen(path, RTLD_NOW | RTLD_LOCAL);
 if (!lib) return -1;
#define LOAD(name, ret, params, args) do { void *sym = dlsym(lib, "cui_bridge_" #name); if (!sym) { dlclose(lib); return -2; } memcpy(&fp_##name, &sym, sizeof sym); } while (0);
#endif
 API(LOAD, LOAD)
#undef LOAD
 bridge_lib = lib;
 go_data_table_initialize();
 go_skeleton_initialize();
 go_spinner_initialize();
 go_spinner_icon_initialize();
 go_pagination_initialize();
 go_rating_initialize();
 go_responsive_notifications_initialize();
 go_radio_motion_initialize();
 go_radio_paint_initialize();
 go_radio_tab_index_initialize();
 go_radio_content_initialize();
 go_radio_tooltip_initialize();
 go_stepper_fill_initialize();
 go_stepper_wrapped_fill_initialize();
 go_virtual_list_initialize();
 go_disclosure_initialize();
 go_attachment_initialize();
 go_carousel_initialize();
 go_kbd_stroke_initialize();
 cui_go_icon_resources_init(bridge_lib);
 cui_go_icon_badge_init(bridge_lib);
 bridge_initialized = 1;
 return 0;
}
static const char *bridge_dlerror(void) {
#if defined(_WIN32)
 snprintf(bridge_error_buf, sizeof bridge_error_buf, "Windows loader error %lu", (unsigned long)bridge_last_error);
 return bridge_error_buf;
#else
 const char *e = dlerror(); return e ? e : "missing bridge symbol";
#endif
}
static bridge_thread_token bridge_thread(void) {
#if defined(_WIN32)
 return GetCurrentThreadId();
#else
 return pthread_self();
#endif
}
static int bridge_same_thread(bridge_thread_token t) {
#if defined(_WIN32)
 return t == GetCurrentThreadId();
#else
 return pthread_equal(t, pthread_self());
#endif
}
#include "slider_model_ffi.h"
#include "time_localized_ffi.h"
*/
import "C"

import (
	"bytes"
	"errors"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"runtime"
	"runtime/cgo"
	"slices"
	"strings"
	"sync"
	"unicode/utf8"
	"unsafe"
)

type App struct {
	timeEnabled, timeDispatching     bool
	closeRequested                   bool
	timeCapacity                     uint32
	sliderNamespace                  *sliderNamespace
	sliderDispatching, sliderClosing bool
	ptr                              *C.cui_bridge_app
	thread                           C.bridge_thread_token
	root                             *Node
	nodes                            map[uint64]*Node
	closed                           bool
	running                          bool
}
type Options struct {
	InitiallyHidden          bool
	Native                   bool
	Width, Height            float32
	Title, FontPath, Library string
	FontFallbacks            []string
	MaxNodes, MaxEvents      uint32
}
type Node struct {
	notificationAction                   bool
	notificationAutoHide                 *bool
	timeLocalized                        *TimeFieldConfig
	spinnerIconResource                  *IconResource
	spinnerConfig                        *SpinnerConfig
	skeletonConfig                       *SkeletonConfig
	paginationConfig                     *PaginationConfig
	ratingConfig                         *RatingConfig
	disclosureConfig                     *disclosureConfigWire
	disclosureStyle                      *disclosureStyleWire
	disclosureHeadings                   disclosureHeadingLayers
	disclosureTitles                     disclosureTitleStyles
	disclosureDivider                    *disclosureDividerWire
	paginationCustomSize                 float32
	iconConfig                           *IconConfig
	badgeConfig                          *BadgeConfig
	timeChange                           func(TimeFieldChange)
	slider                               *sliderNode
	kind                                 int
	key, text                            string
	children                             []*Node
	app                                  *App
	handle                               C.cui_bridge_node
	gap                                  float32
	size, variant, rounding              int
	radius                               float32
	disabled                             bool
	hover                                [4]uint8
	hasHover                             bool
	click                                func(Event)
	change                               func(Event)
	radioPaints                          map[RadioPaintLayer]RadioPaint
	radioTooltipText                     string
	radioTooltipDelay                    int
	radioTabIndex                        *int32
	checked, radioMotion, radioContent   bool
	value, min, max, step                float64
	hasRange, hasValue, hasChecked       bool
	detail                               string
	opened, hasOpen                      bool
	selected                             *Node
	anchor                               *Node
	visualWidth, visualHeight            float32
	hasVisualSize                        bool
	total                                uint32
	hasTotal                             bool
	stepStatus                           uint32
	hasStepStatus                        bool
	scrollWidth, scrollHeight            float32
	hasScrollExtent                      bool
	attachmentStatus                     int
	hasAttachmentStatus                  bool
	attachmentOptions                    *attachmentOptionsWire
	attachmentPreview                    *attachmentPreview
	attachmentActions                    AttachmentActions
	attachmentSlot                       AttachmentSlotRole
	carouselSlot                         CarouselSlotRole
	attachmentSlotWidth                  float32
	carouselBasis                        float32
	carouselFit                          bool
	kbdStyle                             *C.cui_bridge_kbd_style_v1
	kbdStroke                            *kbdStrokeConfig
	virtualListAxis                      VirtualListAxis
	carouselAxis                         CarouselAxis
	carouselLooping                      bool
	carouselControls                     C.uint32_t
	carouselControlSizes                 CarouselControlSizes
	imagePixels                          []byte
	imageWidth, imageHeight, imageStride uint32
	timeValue                            Time
	hasTime                              bool
	colorRGBA                            uint32
	hasColor                             bool
	textViewFormat                       uint32
	hasTextViewFormat                    bool
	chartKind                            ChartKind
	hasChartKind                         bool
	chartData                            []ChartDatum
	hasChartData                         bool
	tableHeaders                         []string
	tableRows                            [][]string
	tableRowKeys                         []uint32
	tableRowCapacity                     uint32
	hasTableData                         bool
	tableLoadMore                        bool
	tableLoadMoreThreshold               uint32
	tableSelectionMode                   uint32
	hasTableSelectionMode                bool
	plotPoints                           []PlotPoint
	hasPlotPoints                        bool
	rootPadding                          float32
	hasRootPadding                       bool
	questionRequired, questionMultiple   bool
	hasQuestionConfig                    bool
	settingsQuery                        string
	hasSettingsQuery                     bool
	dateValue                            Date
	hasDate                              bool
	onClose                              func(Event)
	err                                  error
	removed                              bool

	stepperVisual, stepperVertical, hasStepperVisual   bool
	stepperFill                                        bool
	stepperWrappedFill                                 bool
	stepperFillWidth, stepperFillHeight                float32
	stepperDensity                                     StepperDensity
	designSize                                         int
	hasDesignSize                                      bool
	avatarIdentity, avatarCustomHue, hasAvatarIdentity bool
	shadowStyle                                        *ShadowStyle
	partShadows                                        map[ShadowPart]ShadowStyle
	avatarHue                                          uint8
	avatarAnonymous, hasAvatarPlaceholder              bool
	avatarPlaceholder                                  string
}
type Rect struct{ X, Y, Width, Height float32 }
type Handle struct{ Slot, Generation uint32 }
type Time struct{ Hour, Minute, Second uint32 }
type Date struct{ Year, Month, Day int32 }
type Event struct {
	Node           *Node
	Frame          uint64
	Type           int
	Checked        bool
	Value          float64
	Text           string
	Selected       *Node
	SelectedHandle Handle
	CloseResult    int
}

// DesignSize is the shared scale for Button, Avatar, Pagination and Stepper.
type DesignSize = Size

type Size int

const (
	Inherit Size = iota
	XS
	SM
	Base
	MD
	LG
	XL
	Size2XL
	Size3XL
	Size4XL
	Size5XL
	Size6XL
	Size7XL
	Size8XL
	Size9XL
)

type Variant int

const (
	Primary Variant = iota
	Secondary
	Outline
	Ghost
)

type Rounding int

const (
	Theme Rounding = iota
	Square
	Small
	Large
	Custom
)

type PointerPhase int

const (
	PointerMove PointerPhase = iota
	PointerDown
	PointerUp
)

type KeyPhase int

const (
	KeyDown KeyPhase = iota
	KeyUp
)

type Key int

const (
	Tab Key = iota + 1
	Enter
	Space
	Escape
	Left
	Right
	Up
	Down
)
const (
	ModShift = 1
	ModCtrl  = 2
	ModAlt   = 4
	ModMeta  = 8
)

func valid(s string) error {
	if !utf8.ValidString(s) || strings.IndexByte(s, 0) >= 0 || len(s) > 4096 {
		return errors.New("string must be UTF-8 without NUL and <=4096 bytes")
	}
	return nil
}
func finite(v float32) bool { return !math.IsNaN(float64(v)) && !math.IsInf(float64(v), 0) }

var ErrClosed = errors.New("cui bridge closed")
var loaderMu sync.Mutex
var loadedLibrary string

func result(s C.cui_bridge_status) error {
	if s == C.CUI_BRIDGE_CLOSED {
		return ErrClosed
	}
	if s == C.CUI_BRIDGE_OK {
		return nil
	}
	return fmt.Errorf("cui bridge: %s", C.GoString(C.go_status_message(s)))
}
func cs(s string) *C.char { return C.CString(s) }
func New(o Options) (*App, error) {
	if o.Library == "" {
		o.Library = os.Getenv("CUI_BRIDGE_LIBRARY")
	}
	if o.Library == "" {
		var err error
		o.Library, err = packagedLibrary(o.Native)
		if err != nil {
			return nil, err
		}
	}
	if o.Width == 0 {
		o.Width = 480
	}
	if o.Height == 0 {
		o.Height = 240
	}
	if !finite(o.Width) || !finite(o.Height) || o.Width <= 0 || o.Height <= 0 || o.Width > 32768 || o.Height > 32768 {
		return nil, errors.New("invalid viewport")
	}
	if err := valid(o.Title); err != nil {
		return nil, err
	}
	if err := valid(o.FontPath); err != nil {
		return nil, err
	}
	if o.Native && o.FontPath == "" {
		return nil, errors.New("native backend requires FontPath")
	}
	if err := valid(o.Library); err != nil {
		return nil, fmt.Errorf("invalid bridge library path: %w", err)
	}
	if o.Library != "" && !filepath.IsAbs(o.Library) {
		return nil, errors.New("bridge library path must be absolute")
	}
	identity := o.Library
	if identity == "" {
		identity = "<statically linked CUI>"
	}
	loaderMu.Lock()
	if loadedLibrary != "" && loadedLibrary != identity {
		loaderMu.Unlock()
		return nil, errors.New("different bridge library already loaded")
	}
	path := cs(o.Library)
	codeLoad := C.bridge_open(path)
	C.free(unsafe.Pointer(path))
	if codeLoad != 0 {
		loaderMu.Unlock()
		return nil, fmt.Errorf("bridge load failed: %s", C.GoString(C.bridge_dlerror()))
	}
	loadedLibrary = identity
	loaderMu.Unlock()
	if C.go_abi_version() != C.CUI_BRIDGE_ABI_VERSION || C.go_options_size() != C.uint32_t(C.sizeof_cui_bridge_options) {
		return nil, errors.New("bridge ABI mismatch")
	}
	title, font := cs(o.Title), cs(o.FontPath)
	defer C.free(unsafe.Pointer(title))
	defer C.free(unsafe.Pointer(font))
	backend := C.CUI_BRIDGE_BACKEND_NONE
	if o.Native {
		backend = C.CUI_BRIDGE_BACKEND_SDL
	}
	options := C.cui_bridge_options{struct_size: C.uint32_t(C.sizeof_cui_bridge_options), abi_version: C.CUI_BRIDGE_ABI_VERSION, backend: C.cui_bridge_backend(backend), max_nodes: C.uint32_t(o.MaxNodes), max_events: C.uint32_t(o.MaxEvents), width: C.float(o.Width), height: C.float(o.Height), title_utf8: title, font_path_utf8: font}
	runtime.LockOSThread()
	var code C.cui_bridge_status
	var ptr *C.cui_bridge_app
	if o.InitiallyHidden {
		ptr = C.go_create_window(&options, 1, &code)
	} else {
		ptr = C.go_create(&options, &code)
	}
	if ptr == nil {
		runtime.UnlockOSThread()
		return nil, result(code)
	}
	a := &App{ptr: ptr, thread: C.bridge_thread(), nodes: map[uint64]*Node{}}
	if len(o.FontFallbacks) > 0 {
		if e := a.SetFontFallbacks(o.FontFallbacks); e != nil {
			_ = a.Close()
			return nil, e
		}
	}
	return a, nil
}
func (a *App) check() error {
	if a == nil || a.closed || a.ptr == nil {
		return ErrClosed
	}
	if C.bridge_same_thread(a.thread) == 0 {
		return errors.New("app called on another OS thread")
	}
	return nil
}
func (a *App) ThemePresets() ([]string, error) {
	if err := a.check(); err != nil {
		return nil, err
	}
	count := uint32(C.go_theme_preset_count())
	if count > 1024 {
		return nil, errors.New("theme preset count exceeds limit")
	}
	names := make([]string, 0, count)
	buf := (*C.char)(C.malloc(4096))
	if buf == nil {
		return nil, errors.New("theme preset allocation failed")
	}
	defer C.free(unsafe.Pointer(buf))
	for i := uint32(0); i < count; i++ {
		var length C.uint32_t
		if err := result(C.go_theme_preset_name(C.uint32_t(i), buf, 4096, &length)); err != nil {
			return nil, err
		}
		names = append(names, C.GoStringN(buf, C.int(length)))
	}
	return names, nil
}
func (a *App) SetThemePreset(index uint32) error {
	if err := a.check(); err != nil {
		return err
	}
	return result(C.go_set_theme_preset(a.ptr, C.uint32_t(index)))
}
func (a *App) ThemePreset() (uint32, bool, error) {
	if err := a.check(); err != nil {
		return 0, false, err
	}
	var index C.uint32_t
	code := C.go_get_theme_preset(a.ptr, &index)
	if code == C.CUI_BRIDGE_EMPTY {
		return 0, false, nil
	}
	if err := result(code); err != nil {
		return 0, false, err
	}
	return uint32(index), true, nil
}

// SetWindowVisible changes the native window on its owning UI thread, outside stepping.
func (a *App) SetWindowVisible(visible bool) error {
	if err := a.check(); err != nil {
		return err
	}
	var value C.uint32_t
	if visible {
		value = 1
	}
	return result(C.go_window_visible(a.ptr, value, nil))
}
func (a *App) WindowVisible() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	var value C.uint32_t
	if err := result(C.go_window_visible(a.ptr, 0, &value)); err != nil {
		return false, err
	}
	return value != 0, nil
}

func (a *App) SetClipboard(text string) error {
	if err := a.check(); err != nil {
		return err
	}
	if !utf8.ValidString(text) || strings.IndexByte(text, 0) >= 0 || len(text) > 65536 {
		return errors.New("clipboard text must be UTF-8 without NUL and <=65536 bytes")
	}
	p := cs(text)
	defer C.free(unsafe.Pointer(p))
	return result(C.go_clipboard_set(a.ptr, p))
}
func (a *App) Clipboard() (string, error) {
	if err := a.check(); err != nil {
		return "", err
	}
	p := (*C.char)(C.malloc(65537))
	if p == nil {
		return "", errors.New("clipboard allocation failed")
	}
	defer C.free(unsafe.Pointer(p))
	var length C.uint32_t
	if err := result(C.go_clipboard_get(a.ptr, p, 65537, &length)); err != nil {
		return "", err
	}
	return C.GoStringN(p, C.int(length)), nil
}
func Speech(key string) *Node        { return &Node{kind: 77, key: key} }
func Questionnaire(key string) *Node { return &Node{kind: 78, key: key} }
func Dock(key string) *Node          { return &Node{kind: 79, key: key} }
func Settings(key string) *Node      { return &Node{kind: 80, key: key} }
func (n *Node) Query(query string) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 80 || n.removed || !utf8.ValidString(query) || strings.IndexByte(query, 0) >= 0 || len(query) > 256 {
		n.err = errors.New("invalid Settings query")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		p := cs(query)
		defer C.free(unsafe.Pointer(p))
		if e := result(C.go_settings_set_query(n.app.ptr, n.handle, p)); e != nil {
			n.err = e
			return n
		}
	}
	n.settingsQuery = query
	n.hasSettingsQuery = true
	return n
}
func (a *App) settingsNode(n *Node) error {
	if e := a.check(); e != nil {
		return e
	}
	if n == nil || n.app != a || n.removed || n.kind != 80 {
		return errors.New("invalid Settings node")
	}
	return nil
}
func (a *App) settingsPage(settings, page *Node) error {
	if e := a.settingsNode(settings); e != nil {
		return e
	}
	if page == nil || page.app != a || page.removed || page.kind != 20 {
		return errors.New("invalid Settings page")
	}
	return nil
}
func (a *App) SettingsPage(settings *Node) (*Node, error) {
	if e := a.settingsNode(settings); e != nil {
		return nil, e
	}
	var page C.cui_bridge_node
	if e := result(C.go_settings_get_page(a.ptr, settings.handle, &page)); e != nil {
		return nil, e
	}
	return a.nodes[keyID(page)], nil
}
func (a *App) SetSettingsPage(settings, page *Node) error {
	if e := a.settingsPage(settings, page); e != nil {
		return e
	}
	return result(C.go_settings_set_page(a.ptr, settings.handle, page.handle))
}
func (a *App) ResetSettingsPage(settings, page *Node) error {
	if e := a.settingsPage(settings, page); e != nil {
		return e
	}
	return result(C.go_settings_reset_page(a.ptr, settings.handle, page.handle))
}
func (a *App) SettingsFieldBounds(settings, field *Node) (Rect, error) {
	if e := a.settingsNode(settings); e != nil {
		return Rect{}, e
	}
	if field == nil || field.app != a || field.removed {
		return Rect{}, errors.New("invalid Settings field")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_settings_field_bounds(a.ptr, settings.handle, field.handle, &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}

type DockAxis uint32

const (
	DockHorizontal DockAxis = iota
	DockVertical
)

func (a *App) dockPanel(dock, panel *Node) error {
	if err := a.check(); err != nil {
		return err
	}
	if dock == nil || dock.app != a || dock.removed || dock.kind != 79 || panel == nil || panel.app != a || panel.removed || panel.kind != 20 {
		return errors.New("invalid Dock panel")
	}
	return nil
}
func (a *App) ActivateDockPanel(dock, panel *Node) error {
	if err := a.dockPanel(dock, panel); err != nil {
		return err
	}
	return result(C.go_dock_activate(a.ptr, dock.handle, panel.handle))
}
func (a *App) MoveDockSplit(dock, panel, reference *Node, axis DockAxis, ratio float64, before bool) error {
	if err := a.dockPanel(dock, panel); err != nil {
		return err
	}
	if err := a.dockPanel(dock, reference); err != nil {
		return err
	}
	if axis > DockVertical || math.IsNaN(ratio) || math.IsInf(ratio, 0) || ratio <= 0 || ratio >= 1 {
		return errors.New("invalid Dock split")
	}
	return result(C.go_dock_move_split(a.ptr, dock.handle, panel.handle, reference.handle, C.uint32_t(axis), C.double(ratio), C.uint32_t(btoi(before))))
}
func (a *App) ActiveDockPanel(dock *Node) (*Node, error) {
	if err := a.check(); err != nil {
		return nil, err
	}
	if dock == nil || dock.app != a || dock.removed || dock.kind != 79 {
		return nil, errors.New("invalid Dock")
	}
	var panel C.cui_bridge_node
	if err := result(C.go_dock_get_active(a.ptr, dock.handle, &panel)); err != nil {
		return nil, err
	}
	return a.nodes[keyID(panel)], nil
}
func (a *App) DockPanelBounds(dock, panel *Node) (Rect, error) {
	if err := a.dockPanel(dock, panel); err != nil {
		return Rect{}, err
	}
	var r C.cui_bridge_rect
	if err := result(C.go_dock_panel_bounds(a.ptr, dock.handle, panel.handle, &r)); err != nil {
		return Rect{}, err
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (a *App) SerializeDock(dock *Node) (string, error) {
	if err := a.check(); err != nil {
		return "", err
	}
	if dock == nil || dock.app != a || dock.removed || dock.kind != 79 {
		return "", errors.New("invalid Dock")
	}
	var required C.uint32_t
	code := C.go_dock_serialize(a.ptr, dock.handle, nil, 0, &required)
	if code != C.CUI_BRIDGE_CAPACITY {
		return "", result(code)
	}
	if required == 0 || required > 1048576 {
		return "", errors.New("Dock model exceeds limit")
	}
	p := (*C.char)(C.malloc(C.size_t(required)))
	if p == nil {
		return "", errors.New("Dock model allocation failed")
	}
	defer C.free(unsafe.Pointer(p))
	if err := result(C.go_dock_serialize(a.ptr, dock.handle, p, required, &required)); err != nil {
		return "", err
	}
	return C.GoStringN(p, C.int(required-1)), nil
}
func (n *Node) Question(required, multiple bool) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 20 || n.removed {
		n.err = errors.New("question requires Item")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if e := result(C.go_questionnaire_set_question(n.app.ptr, n.handle, C.uint32_t(btoi(required)), C.uint32_t(btoi(multiple)))); e != nil {
			n.err = e
			return n
		}
	}
	n.questionRequired, n.questionMultiple, n.hasQuestionConfig = required, multiple, true
	return n
}
func (a *App) questionnaireNode(n *Node) error {
	if err := a.check(); err != nil {
		return err
	}
	if n == nil || n.app != a || n.removed || n.kind != 78 {
		return errors.New("invalid Questionnaire node")
	}
	return nil
}
func (a *App) QuestionnaireState(n *Node) (*Node, bool, error) {
	if err := a.questionnaireNode(n); err != nil {
		return nil, false, err
	}
	var current C.cui_bridge_node
	var complete C.uint32_t
	if err := result(C.go_questionnaire_get_state(a.ptr, n.handle, &current, &complete)); err != nil {
		return nil, false, err
	}
	return a.nodes[keyID(current)], complete != 0, nil
}
func (a *App) ChoiceSelected(choice *Node) (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	if choice == nil || choice.app != a || choice.removed || choice.kind != 20 {
		return false, errors.New("invalid questionnaire choice")
	}
	var selected C.uint32_t
	if err := result(C.go_questionnaire_choice_selected(a.ptr, choice.handle, &selected)); err != nil {
		return false, err
	}
	return selected != 0, nil
}
func (a *App) NextQuestion(n *Node) error {
	if err := a.questionnaireNode(n); err != nil {
		return err
	}
	return result(C.go_questionnaire_next(a.ptr, n.handle))
}
func (a *App) PreviousQuestion(n *Node) error {
	if err := a.questionnaireNode(n); err != nil {
		return err
	}
	return result(C.go_questionnaire_previous(a.ptr, n.handle))
}
func (a *App) SubmitQuestionnaire(n *Node) error {
	if err := a.questionnaireNode(n); err != nil {
		return err
	}
	return result(C.go_questionnaire_submit(a.ptr, n.handle))
}
func (a *App) speechNode(n *Node) error {
	if err := a.check(); err != nil {
		return err
	}
	if n == nil || n.app != a || n.removed || n.kind != 77 {
		return errors.New("invalid Speech node")
	}
	return nil
}
func (a *App) StartSpeech(n *Node) error {
	if err := a.speechNode(n); err != nil {
		return err
	}
	return result(C.go_speech_start(a.ptr, n.handle))
}
func (a *App) ReportSpeech(n *Node, text string, finalPhrase bool) error {
	if err := a.speechNode(n); err != nil {
		return err
	}
	if err := valid(text); err != nil {
		return err
	}
	p := cs(text)
	defer C.free(unsafe.Pointer(p))
	return result(C.go_speech_report(a.ptr, n.handle, p, C.uint32_t(btoi(finalPhrase))))
}
func (a *App) FinishSpeech(n *Node) error {
	if err := a.speechNode(n); err != nil {
		return err
	}
	return result(C.go_speech_finish(a.ptr, n.handle))
}
func (a *App) SpeechTranscript(n *Node) (string, error) {
	if err := a.speechNode(n); err != nil {
		return "", err
	}
	p := (*C.char)(C.malloc(4097))
	if p == nil {
		return "", errors.New("transcript allocation failed")
	}
	defer C.free(unsafe.Pointer(p))
	var length C.uint32_t
	if err := result(C.go_speech_transcript(a.ptr, n.handle, p, 4097, &length)); err != nil {
		return "", err
	}
	return C.GoStringN(p, C.int(length)), nil
}
func (a *App) Close() error {
	if err := a.check(); err != nil {
		return err
	}
	C.go_destroy(a.ptr)
	for _, n := range a.nodes {
		n.timeChange = nil
		n.iconConfig, n.badgeConfig, n.paginationConfig, n.ratingConfig = nil, nil, nil, nil
		n.disclosureConfig, n.disclosureStyle = nil, nil
		n.disclosureHeadings, n.disclosureDivider = disclosureHeadingLayers{}, nil
		n.disclosureTitles = disclosureTitleStyles{}
		if n.slider != nil {
			n.slider.callback = nil
		}
	}
	a.sliderClosing = true
	a.closeRequested = true
	a.ptr = nil
	a.closed = true
	if !a.running {
		runtime.UnlockOSThread()
	}
	return nil
}
func (a *App) RequestClose() error {
	if err := a.check(); err != nil {
		return err
	}
	e := result(C.go_request_close(a.ptr))
	if e == nil {
		a.closeRequested = true
	}
	if e == nil && a.sliderNamespace != nil {
		a.sliderClosing = true
	}
	return e
}

// SetFontFallbacks replaces the SDL font chain; an empty list clears it.
func (a *App) SetFontFallbacks(paths []string) error {
	if e := a.check(); e != nil {
		return e
	}
	if len(paths) > 32 {
		return errors.New("too many font fallbacks")
	}
	if len(paths) == 0 {
		return result(C.go_set_font_fallbacks(a.ptr, nil, 0))
	}
	csPaths := make([]*C.char, len(paths))
	defer func() {
		for _, p := range csPaths {
			if p != nil {
				C.free(unsafe.Pointer(p))
			}
		}
	}()
	for i, p := range paths {
		if p == "" || !filepath.IsAbs(p) {
			return errors.New("font fallback path must be absolute")
		}
		if e := valid(p); e != nil {
			return e
		}
		csPaths[i] = cs(p)
	}
	return result(C.go_set_font_fallbacks(a.ptr, (**C.char)(unsafe.Pointer(&csPaths[0])), C.uint32_t(len(csPaths))))
}
func keyID(h C.cui_bridge_node) uint64 { return uint64(h.slot)<<32 | uint64(h.generation) }
func Row(key string) *Node             { return &Node{kind: 1, key: key, size: int(Base)} }
func Column(key string) *Node          { return &Node{kind: 2, key: key, size: int(Base)} }
func RootView(key string) *Node        { return &Node{kind: 76, key: key} }
func (n *Node) RootPadding(padding float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 76 || n.removed || !finite(padding) || padding < 0 || padding > 256 {
		n.err = errors.New("invalid root padding")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if e := result(C.go_set_root_padding(n.app.ptr, n.handle, C.float(padding))); e != nil {
			n.err = e
			return n
		}
	}
	n.rootPadding, n.hasRootPadding = padding, true
	return n
}
func Label(text string) *Node { return &Node{kind: 3, text: text, size: int(Base)} }
func Button(key string) *Node { return &Node{kind: 4, key: key, size: int(Base)} }
func (n *Node) Key(key string) *Node {
	if n.err != nil {
		return n
	}
	if n.app != nil {
		n.err = errors.New("cannot change key after mount")
		return n
	}
	n.key = key
	return n
}
func (n *Node) Children(v ...*Node) *Node {
	if n.err != nil {
		return n
	}
	if n.app != nil {
		n.err = errors.New("cannot add children after mount")
	} else {
		n.children = append(n.children, v...)
	}
	return n
}
func (n *Node) Err() error { return n.err }
func (n *Node) Text(v string) *Node {
	if n.err != nil {
		return n
	}
	n.err = n.SetText(v)
	return n
}
func (n *Node) SetText(v string) error {
	if err := valid(v); err != nil {
		return err
	}
	if n.removed {
		return errors.New("removed node")
	}
	if err := n.validateAttachmentLabel(v); err != nil {
		return err
	}
	if n.iconConfig != nil || n.kind == configuredBadgeKind {
		return errors.New("use configuration to set Icon or Badge text")
	}
	if n.app != nil {
		if err := n.app.check(); err != nil {
			return err
		}
		if err := n.setNativeText(v); err != nil {
			return err
		}
	}
	n.text = v
	return nil
}
func (n *Node) setNativeText(v string) error {
	s := cs(v)
	defer C.free(unsafe.Pointer(s))
	return result(C.go_set_text(n.app.ptr, n.handle, s))
}
func (n *Node) GapPx(v float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 1 && n.kind != 2 || !finite(v) || v < 0 {
		n.err = errors.New("invalid gap")
		return n
	}
	n.gap = v
	if n.app != nil {
		if err := n.app.check(); err != nil {
			n.err = err
		} else {
			n.err = result(C.go_set_gap(n.app.ptr, n.handle, C.float(v)))
		}
	}
	return n
}
func (n *Node) GapUnits(v float32) *Node { return n.GapPx(4 * v) }
func (n *Node) Variant(v Variant) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 4 || v < Primary || v > Ghost {
		n.err = errors.New("invalid variant")
		return n
	}
	n.variant = int(v)
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_button_variant(n.app.ptr, n.handle, C.cui_bridge_button_variant(v)))
		}
	}
	return n
}
func (n *Node) Size(v Size) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 4 || v < Inherit || v > Size9XL {
		n.err = errors.New("invalid size")
		return n
	}
	n.size = int(v)
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_button_size(n.app.ptr, n.handle, C.cui_bridge_button_size(v)))
		}
	}
	return n
}

// StepperDensity selects the source visual proportions.
type StepperDensity uint32

const (
	StepperMD StepperDensity = iota
	StepperXS
	StepperSM
	StepperLG
)

// StepperVisual opts into connectors/icons; selection still dispatches aggregate CHANGE.
func (n *Node) StepperVisual(enabled, vertical bool, density StepperDensity) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 44 || density > StepperLG {
		n.err = errors.New("invalid Stepper visual options")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		var on, axis C.uint32_t
		if enabled {
			on = 1
		}
		if vertical {
			axis = 1
		}
		if e := result(C.go_set_stepper_visual(n.app.ptr, n.handle, on, axis, C.uint32_t(density))); e != nil {
			n.err = e
			return n
		}
	}
	n.stepperVisual, n.stepperVertical, n.stepperDensity, n.hasStepperVisual = enabled, vertical, density, true
	n.stepperFill, n.stepperWrappedFill = false, false
	return n
}

// ErrStepperFillUnavailable indicates that the selected provider lacks filled steppers.
var ErrStepperFillUnavailable = errors.New("Stepper fill unavailable from bridge provider")

// SupportsStepperFill reports whether the loaded provider offers filled Stepper presentation.
func (a *App) SupportsStepperFill() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_stepper_fill_supported() != 0, nil
}

// StepperFill fills the available width when width is zero; zero height is intrinsic.
// A subsequent StepperVisual call restores ordinary presentation.
func (n *Node) StepperFill(vertical bool, density StepperDensity, width, height float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 44 || density > StepperLG || !(0 <= width && width <= 4096 && 0 <= height && height <= 4096) {
		n.err = errors.New("invalid Stepper fill options")
		return n
	}
	if n.app != nil {
		supported, err := n.app.SupportsStepperFill()
		if err != nil {
			n.err = err
			return n
		}
		if !supported {
			n.err = ErrStepperFillUnavailable
			return n
		}
		var axis C.uint32_t
		if vertical {
			axis = 1
		}
		if err := result(C.go_stepper_fill(n.app.ptr, n.handle, axis, C.uint32_t(density), C.float(width), C.float(height))); err != nil {
			n.err = err
			return n
		}
	}
	n.stepperFill, n.stepperWrappedFill, n.hasStepperVisual = true, false, false
	n.stepperVertical, n.stepperDensity = vertical, density
	n.stepperFillWidth, n.stepperFillHeight = width, height
	return n
}

// ErrStepperWrappedFillUnavailable indicates that the selected provider lacks wrapped filled steppers.
var ErrStepperWrappedFillUnavailable = errors.New("Stepper wrapped fill unavailable from bridge provider")

// SupportsStepperWrappedFill independently probes wrapped Fill without affecting ordinary Fill.
func (a *App) SupportsStepperWrappedFill() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_stepper_wrapped_fill_supported() != 0, nil
}

// StepperWrappedFill wraps Title and description in equal horizontal lanes or beside vertical indicators.
// Zero width grows with the parent; zero height fits the wrapped rows.
// A subsequent StepperVisual call restores ordinary presentation.
func (n *Node) StepperWrappedFill(vertical bool, density StepperDensity, width, height float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 44 || density > StepperLG || !(0 <= width && width <= 4096 && 0 <= height && height <= 4096) {
		n.err = errors.New("invalid Stepper wrapped fill options")
		return n
	}
	if n.app != nil {
		supported, err := n.app.SupportsStepperWrappedFill()
		if err != nil {
			n.err = err
			return n
		}
		if !supported {
			n.err = ErrStepperWrappedFillUnavailable
			return n
		}
		var axis C.uint32_t
		if vertical {
			axis = 1
		}
		if err := result(C.go_stepper_wrapped_fill(n.app.ptr, n.handle, axis, C.uint32_t(density), C.float(width), C.float(height))); err != nil {
			n.err = err
			return n
		}
	}
	n.stepperWrappedFill, n.stepperFill, n.hasStepperVisual = true, false, false
	n.stepperVertical, n.stepperDensity = vertical, density
	n.stepperFillWidth, n.stepperFillHeight = width, height
	return n
}

// DesignSize applies the shared scale to Button, Avatar, Pagination, Stepper and Attachment.
func (n *Node) DesignSize(v DesignSize) *Node {
	if n.kind == 4 {
		return n.Size(v)
	}
	if n.err != nil {
		return n
	}
	if (n.kind != 30 && n.kind != 42 && n.kind != 44 && n.kind != C.CUI_BRIDGE_ATTACHMENT) || v < Inherit || v > Size9XL {
		n.err = errors.New("design size requires Button, Avatar, Pagination, Stepper or Attachment and a supported size")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if n.kind == 42 && C.go_pagination_supported() == 0 {
			n.err = ErrPaginationOptionsUnavailable
			return n
		}
		if e := result(C.go_set_design_size(n.app.ptr, n.handle, C.cui_bridge_button_size(v))); e != nil {
			n.err = e
			return n
		}
	}
	n.designSize = int(v)
	n.hasDesignSize = true
	return n
}

// IdentityColors enables Avatar identity colors with zero or one hue (0..11).
func (n *Node) IdentityColors(enabled bool, hue ...uint8) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 30 || len(hue) > 1 || (len(hue) == 1 && hue[0] > 11) {
		n.err = errors.New("identity colors requires Avatar and optional hue 0..11")
		return n
	}
	var h uint8
	if len(hue) == 1 {
		h = hue[0]
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if e := result(C.go_set_avatar_identity(n.app.ptr, n.handle, C.uint32_t(btoi(enabled)), C.uint32_t(btoi(len(hue) == 1)), C.uint32_t(h))); e != nil {
			n.err = e
			return n
		}
	}
	n.avatarIdentity = enabled
	n.avatarCustomHue = len(hue) == 1
	n.avatarHue = h
	n.hasAvatarIdentity = true
	return n
}

// AnonymousIcon supplies an owned named SVG fallback for an empty Avatar name.
func (n *Node) AnonymousIcon(enabled bool, name string) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 30 {
		n.err = errors.New("anonymous icon requires Avatar")
		return n
	}
	if e := valid(name); e != nil {
		n.err = e
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		p := cs(name)
		e := result(C.go_set_avatar_placeholder(n.app.ptr, n.handle, C.uint32_t(btoi(enabled)), p))
		C.free(unsafe.Pointer(p))
		if e != nil {
			n.err = e
			return n
		}
	}
	n.avatarAnonymous = enabled
	n.avatarPlaceholder = name
	n.hasAvatarPlaceholder = true
	return n
}
func (n *Node) Rounding(v Rounding, r float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 4 || v < Theme || v > Custom || !finite(r) || r < 0 {
		n.err = errors.New("invalid rounding")
		return n
	}
	n.rounding = int(v)
	n.radius = r
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_button_rounding(n.app.ptr, n.handle, C.cui_bridge_button_rounding(v), C.float(r)))
		}
	}
	return n
}
func (n *Node) RoundedMD() *Node { return n.Rounding(Theme, 0) }
func (n *Node) Disabled(v bool) *Node {
	if n.kind == 81 {
		if e := n.sliderCheck(); e != nil {
			n.err = e
			return n
		}
		if n.app != nil {
			if e := result(C.go_set_disabled(n.app.ptr, n.handle, C.uint32_t(btoi(v)))); e != nil {
				n.err = e
				return n
			}
		}
		n.disabled = v
		return n
	}
	if n.err != nil {
		return n
	}
	if !(n.attachmentSlot != 0 || n.kind == 52 || n.kind == 4 || n.kind == configuredBadgeKind || (n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || (n.kind >= 17 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 48 || (n.kind >= 54 && n.kind <= 61) || (n.kind >= 65 && n.kind <= 69) || n.kind == 71 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 77 || n.kind == 78 || n.kind == 79) {
		n.err = errors.New("disabled requires control")
		return n
	}
	n.disabled = v
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else if n.kind == 4 {
			n.err = result(C.go_set_button_disabled(n.app.ptr, n.handle, C.uint32_t(btoi(v))))
		} else {
			n.err = result(C.go_set_disabled(n.app.ptr, n.handle, C.uint32_t(btoi(v))))
		}
	}
	return n
}
func btoi(v bool) int {
	if v {
		return 1
	}
	return 0
}
func (n *Node) HoverRGBA(r, g, b, a uint8) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 4 {
		n.err = errors.New("hover requires button")
		return n
	}
	n.hover = [4]uint8{r, g, b, a}
	n.hasHover = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_button_hover_rgba(n.app.ptr, n.handle, C.uint8_t(r), C.uint8_t(g), C.uint8_t(b), C.uint8_t(a)))
		}
	}
	return n
}
func (n *Node) OnClick(fn func(Event)) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 4 && n.kind != 23 && n.kind != 20 && n.kind != 62 {
		n.err = errors.New("click requires button")
		return n
	}
	n.click = fn
	return n
}
func (a *App) Mount(root *Node) error {
	if err := a.check(); err != nil {
		return err
	}
	if a.root != nil {
		return errors.New("root already mounted")
	}
	seen := map[*Node]bool{}
	keys := map[string]bool{}
	usesSpinner := false
	usesResponsiveSkeleton := false
	var visit func(*Node, *Node, int, bool, bool) error
	visit = func(n, parent *Node, d int, content, slotContent bool) error {
		if n == nil || seen[n] || n.app != nil || n.removed {
			return errors.New("invalid or reused node")
		}
		if slotContent && n.kind == int(C.CUI_BRIDGE_NOTIFICATION) {
			return errors.New("Notification cannot be nested inside an Attachment slot")
		}
		seen[n] = true
		if n.notificationAutoHide != nil && C.go_notification_auto_hide_supported() == 0 {
			return ErrNotificationAutoHideUnavailable
		}
		if err := a.validateNotificationAction(n, parent); err != nil {
			return err
		}
		if err := a.validateAttachmentSlots(n, parent); err != nil {
			return err
		}
		if err := a.validateCarouselSlots(n, parent); err != nil {
			return err
		}
		if n.kbdStyle != nil && C.go_kbd_style_supported() == 0 {
			return ErrKbdStyleUnavailable
		}
		if n.kbdStroke != nil && C.go_kbd_stroke_supported() == 0 {
			return ErrKbdStrokeUnavailable
		}
		if n.virtualListAxis == VirtualListHorizontal && C.go_virtual_list_axis_supported() == 0 {
			return ErrVirtualListAxisUnavailable
		}
		if n.carouselAxis == CarouselVertical && C.go_carousel_axis_supported() == 0 {
			return ErrCarouselAxisUnavailable
		}
		if n.carouselLooping && C.go_carousel_looping_supported() == 0 {
			return ErrCarouselLoopingUnavailable
		}
		if n.carouselControls != 0 && C.go_carousel_controls_supported() == 0 {
			return ErrCarouselControlsUnavailable
		}
		if n.carouselControlSizes != (CarouselControlSizes{}) && C.go_carousel_control_sizes_supported() == 0 {
			return ErrCarouselControlSizesUnavailable
		}
		if n.carouselBasis > 0 {
			if parent == nil || parent.kind != int(C.CUI_BRIDGE_CAROUSEL) {
				return errors.New("CarouselBasis requires a direct Carousel Item")
			}
			if C.go_carousel_basis_supported() == 0 {
				return ErrCarouselBasisUnavailable
			}
		}
		if n.carouselFit {
			if parent == nil || parent.kind != int(C.CUI_BRIDGE_CAROUSEL) {
				return errors.New("CarouselFit requires a direct Carousel Item")
			}
			if C.go_carousel_fit_supported() == 0 {
				return ErrCarouselFitUnavailable
			}
		}
		if content && !radioContentKind(n.kind) {
			return errors.New("Control decorations require passive content")
		}
		if len(n.radioPaints) != 0 {
			if n.kind == int(C.CUI_BRIDGE_ITEM) && (parent == nil || parent.kind != int(C.CUI_BRIDGE_RADIO_GROUP)) {
				return errors.New("Radio paint requires a direct RadioGroup Item")
			}
			if C.go_radio_paint_supported() == 0 {
				return ErrRadioPaintUnavailable
			}
		}
		if n.radioTooltipText != "" {
			if n.kind == int(C.CUI_BRIDGE_ITEM) && (parent == nil || parent.kind != int(C.CUI_BRIDGE_RADIO_GROUP)) {
				return errors.New("Radio tooltip requires a direct RadioGroup Item")
			}
			if C.go_radio_tooltip_supported() == 0 {
				return ErrRadioTooltipUnavailable
			}
		}
		if n.radioContent {
			if parent == nil || parent.kind != int(C.CUI_BRIDGE_RADIO_GROUP) {
				return errors.New("decorative content requires a direct RadioGroup Item")
			}
			if C.go_radio_content_supported() == 0 {
				return ErrRadioItemContentUnavailable
			}
		}
		if n.tableRowCapacity != 0 {
			if err := a.requireDataTableCapacity(n.tableRowCapacity); err != nil {
				return err
			}
		} else if n.tableRowKeys != nil && C.go_data_table_keyed_supported() == 0 {
			return ErrKeyedDataTableUnavailable
		}
		if n.tableLoadMore {
			if C.go_data_table_paging_supported() == 0 {
				return ErrDataTablePagingUnavailable
			}
			if !n.hasTableData {
				return errors.New("DataTable paging requires column headers")
			}
		}
		if d > 128 {
			return errors.New("tree too deep")
		}
		if n.kind == 76 && d != 1 {
			return errors.New("RootView must be the root")
		}
		if n.kind == 78 {
			if len(n.children) == 0 || len(n.children) > 16 {
				return errors.New("Questionnaire needs 1..16 questions")
			}
			total := 0
			for _, q := range n.children {
				if q == nil || q.kind != 20 || len(q.children) == 0 {
					return errors.New("Questionnaire questions require Item choices")
				}
				total += len(q.children)
				for _, c := range q.children {
					if c == nil || c.kind != 20 || len(c.children) > 0 {
						return errors.New("Questionnaire choices must be leaf Items")
					}
				}
			}
			if total > 256 {
				return errors.New("Questionnaire exceeds 256 choices")
			}
		}
		if n.kind == 80 {
			if len(n.children) == 0 || len(n.children) > 8 {
				return errors.New("Settings needs 1..8 pages")
			}
			groups, fields := 0, 0
			for _, page := range n.children {
				if page == nil || page.kind != 20 || len(page.children) == 0 || len(page.children) > 8 {
					return errors.New("Settings pages need 1..8 Item groups")
				}
				groups += len(page.children)
				for _, group := range page.children {
					if group == nil || group.kind != 20 || len(group.children) == 0 || len(group.children) > 16 {
						return errors.New("Settings groups need 1..16 fields")
					}
					fields += len(group.children)
					for _, field := range group.children {
						if field == nil || (field.kind != 5 && field.kind != 7 && field.kind != 12 && field.kind != 14) || len(field.children) > 0 {
							return errors.New("invalid Settings field")
						}
					}
				}
			}
			if groups > 32 || fields > 128 {
				return errors.New("Settings exceeds group or field limit")
			}
		}
		if n.err != nil {
			return n.err
		}
		if n.iconConfig != nil {
			if _, err := a.iconPresentationHandle(*n.iconConfig); err != nil {
				return err
			}
		}
		if n.badgeConfig != nil {
			if err := a.requireConfiguredBadge(); err != nil {
				return err
			}
			if _, _, err := n.badgeConfig.wire(a); err != nil {
				return err
			}
		}
		if n.radioTabIndex != nil && C.go_radio_tab_index_supported() == 0 {
			return ErrRadioTabIndexUnavailable
		}
		if n.radioMotion && C.go_radio_motion_supported() == 0 {
			return ErrRadioMotionUnavailable
		}
		if n.stepperWrappedFill && C.go_stepper_wrapped_fill_supported() == 0 {
			return ErrStepperWrappedFillUnavailable
		}
		if n.stepperFill && C.go_stepper_fill_supported() == 0 {
			return ErrStepperFillUnavailable
		}
		usesSpinner = usesSpinner || n.spinnerConfig != nil
		usesResponsiveSkeleton = usesResponsiveSkeleton || n.skeletonConfig != nil
		if n.paginationCustomSize != 0 && C.go_pagination_custom_supported() == 0 {
			return ErrPaginationCustomSizeUnavailable
		}
		if n.kind == 42 && (n.paginationConfig != nil || n.hasDesignSize) && C.go_pagination_supported() == 0 {
			return ErrPaginationOptionsUnavailable
		}
		if n.ratingConfig != nil {
			if C.go_rating_supported() == 0 {
				return ErrRatingOptionsUnavailable
			}
			if _, err := n.ratingConfig.wire(a); err != nil {
				return err
			}
		}
		if (n.disclosureConfig != nil || n.disclosureStyle != nil) && C.go_disclosure_supported() == 0 {
			return ErrDisclosureOptionsUnavailable
		}
		if n.disclosureStyle != nil && n.kind == C.CUI_BRIDGE_ITEM &&
			(parent == nil || parent.kind != C.CUI_BRIDGE_ACCORDION) {
			return errors.New("disclosure trigger style requires a direct Accordion Item")
		}
		if n.disclosureHeadings != (disclosureHeadingLayers{}) {
			if C.go_disclosure_heading_supported() == 0 {
				return ErrDisclosureOptionsUnavailable
			}
			if n.kind == C.CUI_BRIDGE_ITEM && (parent == nil || parent.kind != C.CUI_BRIDGE_ACCORDION) {
				return errors.New("disclosure heading requires a direct Accordion Item")
			}
		}
		if n.disclosureTitles != (disclosureTitleStyles{}) {
			if C.go_disclosure_title_supported() == 0 {
				return ErrDisclosureOptionsUnavailable
			}
			if n.kind == C.CUI_BRIDGE_ITEM && (parent == nil || parent.kind != C.CUI_BRIDGE_ACCORDION) {
				return errors.New("disclosure title style requires a direct Accordion Item")
			}
		}
		if n.disclosureDivider != nil && C.go_disclosure_divider_supported() == 0 {
			return ErrDisclosureOptionsUnavailable
		}
		if n.attachmentActions != 0 && C.go_attachment_actions_supported() == 0 {
			return ErrAttachmentActionsUnavailable
		}
		if n.key == "" {
			n.key = fmt.Sprintf("go-%p", n)
		}
		if err := valid(n.key); err != nil {
			return err
		}
		if err := valid(n.text); err != nil {
			return err
		}
		if keys[n.key] {
			return errors.New("duplicate key")
		}
		keys[n.key] = true
		if n.kind >= 3 && n.kind != 52 && n.kind != 15 && n.kind != 17 && n.kind != 18 && n.kind != 19 && n.kind != 20 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 24 && n.kind != 26 && n.kind != 27 && n.kind != 29 && n.kind != 33 && n.kind != 40 && n.kind != 41 && n.kind != 44 && n.kind != 45 && n.kind != 46 && n.kind != 47 && n.kind != 48 && n.kind != 49 && n.kind != 54 && n.kind != 60 && n.kind != 61 && n.kind != 62 && n.kind != 63 && n.kind != 64 && n.kind != 65 && n.kind != 66 && n.kind != 70 && n.kind != 73 && n.kind != 74 && n.kind != 75 && n.kind != 76 && n.kind != 78 && n.kind != 79 && n.kind != 80 && n.kind != configuredBadgeKind && len(n.children) > 0 {
			return errors.New("leaf has children")
		}
		if (n.kind == 54 || n.kind == 60 || n.kind == 64 || n.kind == 66 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 79) && len(n.children) > 64 {
			return errors.New("tree exceeds 64 Items")
		}
		if n.kind == 17 || n.kind == 18 || n.kind == 19 || n.kind == 21 || n.kind == 22 || n.kind == 23 || n.kind == 40 || n.kind == 41 || n.kind == 44 || n.kind == 45 || n.kind == 47 || n.kind == 48 || n.kind == 49 || n.kind == 54 || n.kind == 60 || n.kind == 61 || n.kind == 64 || n.kind == 66 || n.kind == 70 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 79 {
			for _, ch := range n.children {
				if ch == nil || ch.kind != 20 {
					return errors.New("group children must be Item")
				}
			}
		}
		for _, ch := range n.children {
			if err := visit(ch, n, d+1, content || n.radioContent || n.carouselSlot != 0, slotContent || n.attachmentSlot != 0); err != nil {
				return err
			}
		}
		return nil
	}
	if err := visit(root, nil, 1, false, false); err != nil {
		return err
	}
	if usesSpinner && C.go_spinner_supported() == 0 {
		return ErrSpinnerPresentationUnavailable
	}
	if usesResponsiveSkeleton && C.go_skeleton_supported() == 0 {
		return ErrSkeletonPresentationUnavailable
	}
	if e := a.timePreflight(root); e != nil {
		return e
	}
	if e := a.sliderPreflight(root); e != nil {
		return e
	}
	var mountedAttachmentPreviews []*Node
	var add func(*Node, C.cui_bridge_node) error
	add = func(n *Node, p C.cui_bridge_node) error {
		key := cs(n.key)
		defer C.free(unsafe.Pointer(key))
		var h C.cui_bridge_node
		if n.notificationAction {
			if e := result(C.go_notification_action_create(a.ptr, p, key, &h)); e != nil {
				return e
			}
		} else if n.attachmentSlot != 0 {
			if e := result(C.go_attachment_slot_create(a.ptr, p, C.uint32_t(n.attachmentSlot), key, C.float(n.attachmentSlotWidth), &h)); e != nil {
				return e
			}
		} else if n.carouselSlot != 0 {
			if e := result(C.go_carousel_create_slot(a.ptr, p, C.uint32_t(n.carouselSlot), key, &h)); e != nil {
				return e
			}
		} else if n.kind == 81 {
			c, v := sliderWireConfig(n.slider.config), sliderWireValue(n.slider.confirmed)
			if e := result(C.go_slider_create(a.ptr, p, key, &c, &v, &h)); e != nil {
				return e
			}
		} else if n.kind == configuredBadgeKind {
			badge, err := a.createNativeBadge(n, p, key)
			if err != nil {
				return err
			}
			h = badge
		} else {
			if err := result(C.go_node_create(a.ptr, p, C.cui_bridge_kind(n.kind), key, &h)); err != nil {
				return err
			}
		}
		n.app = a
		n.handle = h
		a.nodes[keyID(h)] = n
		for layer, paint := range n.radioPaints {
			if err := n.SetRadioPaint(layer, paint); err != nil {
				return err
			}
		}
		if n.radioTooltipText != "" {
			if err := n.SetRadioTooltip(n.radioTooltipText, n.radioTooltipDelay); err != nil {
				return err
			}
		}
		if n.radioContent {
			if err := n.SetRadioItemContent(true); err != nil {
				return err
			}
		}
		if n.radioTabIndex != nil {
			if err := n.SetRadioTabIndex(*n.radioTabIndex); err != nil {
				return err
			}
		}
		if n.radioMotion {
			if err := n.SetRadioMotion(true); err != nil {
				return err
			}
		}
		if n.spinnerConfig != nil {
			if e := n.configureMountedSpinner(*n.spinnerConfig); e != nil {
				return e
			}
		}
		if n.skeletonConfig != nil {
			if e := n.configureMountedSkeleton(*n.skeletonConfig); e != nil {
				return e
			}
		}
		if n.paginationCustomSize != 0 {
			if e := n.setMountedPaginationCustomSize(n.paginationCustomSize); e != nil {
				return e
			}
		}
		if n.paginationConfig != nil {
			if e := n.setPaginationConfig(n.paginationConfig); e != nil {
				return e
			}
		}
		if n.ratingConfig != nil {
			wire, err := n.ratingConfig.wire(a)
			if err != nil {
				return err
			}
			if err := n.setRatingWire(&wire); err != nil {
				return err
			}
		}
		if n.disclosureConfig != nil {
			if err := n.setDisclosureConfig(n.disclosureConfig); err != nil {
				return err
			}
		}
		if n.disclosureStyle != nil {
			if err := n.setDisclosureStyle(n.disclosureStyle); err != nil {
				return err
			}
		}
		for layer, heading := range n.disclosureHeadings {
			if heading != nil {
				if err := n.setDisclosureHeading(DisclosureHeadingLayer(layer), heading); err != nil {
					return err
				}
			}
		}
		for state, style := range n.disclosureTitles {
			if style != nil {
				if err := n.setDisclosureTitleState(DisclosureTitleState(state), style); err != nil {
					return err
				}
			}
		}
		if n.disclosureDivider != nil {
			if err := n.setDisclosureDivider(n.disclosureDivider); err != nil {
				return err
			}
		}
		if n.hasSettingsQuery {
			p := cs(n.settingsQuery)
			e := result(C.go_settings_set_query(a.ptr, h, p))
			C.free(unsafe.Pointer(p))
			if e != nil {
				return e
			}
		}
		if n.hasQuestionConfig {
			if e := result(C.go_questionnaire_set_question(a.ptr, h, C.uint32_t(btoi(n.questionRequired)), C.uint32_t(btoi(n.questionMultiple)))); e != nil {
				return e
			}
		}
		if n.hasRootPadding {
			if e := result(C.go_set_root_padding(a.ptr, h, C.float(n.rootPadding))); e != nil {
				return e
			}
		}
		if n.kind == 59 && n.hasTotal {
			if e := result(C.go_set_total(a.ptr, h, C.uint32_t(n.total))); e != nil {
				return e
			}
		}
		if n.text != "" {
			if err := n.setNativeText(n.text); err != nil {
				return err
			}
		}
		if n.hasTextViewFormat {
			if e := result(C.go_set_text_view_format(a.ptr, h, C.uint32_t(n.textViewFormat))); e != nil {
				return e
			}
		}
		if n.hasChartKind {
			if e := result(C.go_chart_set_kind(a.ptr, h, C.uint32_t(n.chartKind))); e != nil {
				return e
			}
		}
		if n.hasChartData {
			if e := n.SetData(n.chartData); e != nil {
				return e
			}
		}
		if n.hasTableData {
			if e := n.applyTable(n.tableHeaders, n.tableRows, n.tableRowKeys, n.tableRowCapacity); e != nil {
				return e
			}
		}
		if n.tableLoadMore {
			if e := n.ConfigureLoadMore(true, n.tableLoadMoreThreshold); e != nil {
				return e
			}
		}
		if n.hasTableSelectionMode {
			if e := result(C.go_data_table_set_selection_mode(a.ptr, h, C.uint32_t(n.tableSelectionMode))); e != nil {
				return e
			}
		}
		if n.hasPlotPoints {
			if e := n.SetPoints(n.plotPoints); e != nil {
				return e
			}
		}
		if len(n.imagePixels) > 0 {
			e := result(C.go_set_image_rgba(a.ptr, h, (*C.uint8_t)(unsafe.Pointer(&n.imagePixels[0])), C.uint32_t(n.imageWidth), C.uint32_t(n.imageHeight), C.uint32_t(n.imageStride), C.uint32_t(len(n.imagePixels))))
			runtime.KeepAlive(n.imagePixels)
			if e != nil {
				return e
			}
		}
		if n.hasTime {
			if e := result(C.go_set_time(a.ptr, h, C.cui_bridge_time{hour: C.uint32_t(n.timeValue.Hour), minute: C.uint32_t(n.timeValue.Minute), second: C.uint32_t(n.timeValue.Second)})); e != nil {
				return e
			}
		}
		if n.timeLocalized != nil {
			if e := n.SetLocalizedTime(*n.timeLocalized); e != nil {
				return e
			}
		}
		if n.hasColor {
			if e := result(C.go_set_color_rgba(a.ptr, h, C.uint32_t(n.colorRGBA))); e != nil {
				return e
			}
		}
		if n.hasDate {
			if e := result(C.go_set_date(a.ptr, h, C.cui_bridge_date{year: C.int32_t(n.dateValue.Year), month: C.int32_t(n.dateValue.Month), day: C.int32_t(n.dateValue.Day)})); e != nil {
				return e
			}
		}
		if n.hasAttachmentStatus {
			if e := result(C.go_set_attachment_status(a.ptr, h, C.cui_bridge_attachment_status(n.attachmentStatus))); e != nil {
				return e
			}
		}
		if n.hasScrollExtent {
			if e := result(C.go_set_scroll_extent(a.ptr, h, C.float(n.scrollWidth), C.float(n.scrollHeight))); e != nil {
				return e
			}
		}
		if n.hasTotal && n.kind != 59 {
			if e := result(C.go_set_total(a.ptr, h, C.uint32_t(n.total))); e != nil {
				return e
			}
		}
		if n.hasStepStatus {
			if e := result(C.go_set_step_status(a.ptr, h, C.uint32_t(n.stepStatus))); e != nil {
				return e
			}
		}
		if n.stepperWrappedFill {
			if e := n.StepperWrappedFill(n.stepperVertical, n.stepperDensity, n.stepperFillWidth, n.stepperFillHeight).err; e != nil {
				return e
			}
		}
		if n.stepperFill {
			if e := n.StepperFill(n.stepperVertical, n.stepperDensity, n.stepperFillWidth, n.stepperFillHeight).err; e != nil {
				return e
			}
		}
		if n.hasStepperVisual {
			if e := n.StepperVisual(n.stepperVisual, n.stepperVertical, n.stepperDensity).err; e != nil {
				return e
			}
		}
		if n.hasDesignSize {
			if e := result(C.go_set_design_size(a.ptr, h, C.cui_bridge_button_size(n.designSize))); e != nil {
				return e
			}
		}
		if n.shadowStyle != nil {
			wire, e := shadowWire(*n.shadowStyle)
			if e != nil {
				return e
			}
			if e := shadowResult(C.go_shadow_set(a.ptr, h, &wire)); e != nil {
				return e
			}
		}
		for part, style := range n.partShadows {
			if e := n.setPartNative(part, style); e != nil {
				return e
			}
		}
		if n.hasAvatarIdentity {
			if e := result(C.go_set_avatar_identity(a.ptr, h, C.uint32_t(btoi(n.avatarIdentity)), C.uint32_t(btoi(n.avatarCustomHue)), C.uint32_t(n.avatarHue))); e != nil {
				return e
			}
		}
		if n.hasAvatarPlaceholder {
			p := cs(n.avatarPlaceholder)
			e := result(C.go_set_avatar_placeholder(a.ptr, h, C.uint32_t(btoi(n.avatarAnonymous)), p))
			C.free(unsafe.Pointer(p))
			if e != nil {
				return e
			}
		}
		if n.hasVisualSize {
			if e := result(C.go_set_visual_size(a.ptr, h, C.float(n.visualWidth), C.float(n.visualHeight))); e != nil {
				return e
			}
		}
		if n.carouselBasis > 0 {
			if err := result(C.go_carousel_set_basis(a.ptr, h, C.float(n.carouselBasis))); err != nil {
				return err
			}
		}
		if n.carouselFit {
			if err := result(C.go_carousel_set_fit(a.ptr, h, 1)); err != nil {
				return err
			}
		}
		if n.carouselLooping {
			if err := result(C.go_carousel_set_looping(a.ptr, h, 1)); err != nil {
				return err
			}
		}
		if n.carouselControls != 0 {
			if err := result(C.go_carousel_set_controls(a.ptr, h, n.carouselControls)); err != nil {
				return err
			}
		}
		if sizes := n.carouselControlSizes; sizes != (CarouselControlSizes{}) {
			if err := n.setCarouselControlSizes(sizes); err != nil {
				return err
			}
		}
		if n.virtualListAxis == VirtualListHorizontal {
			if err := result(C.go_virtual_list_set_axis(a.ptr, h, C.uint32_t(n.virtualListAxis))); err != nil {
				return err
			}
		}
		if n.carouselAxis == CarouselVertical {
			if err := result(C.go_carousel_set_axis(a.ptr, h, C.uint32_t(n.carouselAxis))); err != nil {
				return err
			}
		}
		if n.kbdStyle != nil {
			if err := n.setKbdStyle(n.kbdStyle); err != nil {
				return err
			}
		}
		if n.kbdStroke != nil {
			if err := n.setKbdStroke(*n.kbdStroke); err != nil {
				return err
			}
		}
		if n.iconConfig != nil {
			if err := n.setNativeIcon(*n.iconConfig); err != nil {
				return err
			}
		}
		if n.detail != "" {
			p := cs(n.detail)
			e := result(C.go_set_detail_text(a.ptr, h, p))
			C.free(unsafe.Pointer(p))
			if e != nil {
				return e
			}
		}
		if n.attachmentOptions != nil {
			if err := n.applyAttachmentOptions(n.attachmentOptions); err != nil {
				return err
			}
		}
		if n.attachmentActions != 0 {
			if err := n.applyAttachmentActions(n.attachmentActions); err != nil {
				return err
			}
		}
		if n.attachmentPreview != nil {
			if err := n.applyAttachmentPreview(n.attachmentPreview); err != nil {
				return err
			}
			mountedAttachmentPreviews = append(mountedAttachmentPreviews, n)
		}
		if n.notificationAutoHide != nil {
			if e := result(C.go_notification_auto_hide(a.ptr, h, C.uint32_t(btoi(*n.notificationAutoHide)))); e != nil {
				return e
			}
		}
		if n.hasOpen && (n.kind < 24 || n.kind == 58 || n.kind == 62 || n.kind == 63 || n.kind == 75) {
			if e := result(C.go_set_open(a.ptr, h, C.uint32_t(btoi(n.opened)))); e != nil {
				return e
			}
		}
		if n.gap != 0 {
			if err := result(C.go_set_gap(a.ptr, h, C.float(n.gap))); err != nil {
				return err
			}
		}
		if ((n.kind >= 5 && n.kind <= 8) || (n.kind == 20 && p.slot != 0 && n.hasChecked)) && n.hasChecked {
			if err := result(C.go_set_checked(a.ptr, h, C.uint32_t(btoi(n.checked)))); err != nil {
				return err
			}
		}
		if (n.attachmentSlot != 0 || n.kind == 52 || n.kind == 81 || n.kind == configuredBadgeKind || (n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || ((n.kind >= 17 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 48 || (n.kind >= 54 && n.kind <= 61) || (n.kind >= 65 && n.kind <= 69) || n.kind == 71 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 77 || n.kind == 78 || n.kind == 79)) && n.disabled {
			if err := result(C.go_set_disabled(a.ptr, h, 1)); err != nil {
				return err
			}
		}
		if (n.kind == 9 || n.kind == 14) && n.hasRange {
			if err := result(C.go_set_range(a.ptr, h, C.double(n.min), C.double(n.max), C.double(n.step))); err != nil {
				return err
			}
		}
		if (n.kind == 9 || n.kind == 10 || n.kind == 14 || n.kind == 42 || n.kind == 43 || n.kind == 52 || n.kind == 20 || n.kind == 74) && n.hasValue {
			if err := result(C.go_set_value(a.ptr, h, C.double(n.value))); err != nil {
				return err
			}
		}
		if n.kind == 4 {
			if err := result(C.go_set_button_variant(a.ptr, h, C.cui_bridge_button_variant(n.variant))); err != nil {
				return err
			}
			if err := result(C.go_set_button_size(a.ptr, h, C.cui_bridge_button_size(n.size))); err != nil {
				return err
			}
			if err := result(C.go_set_button_rounding(a.ptr, h, C.cui_bridge_button_rounding(n.rounding), C.float(n.radius))); err != nil {
				return err
			}
			if n.disabled {
				if err := result(C.go_set_button_disabled(a.ptr, h, 1)); err != nil {
					return err
				}
			}
			if n.hasHover {
				v := n.hover
				if err := result(C.go_set_button_hover_rgba(a.ptr, h, C.uint8_t(v[0]), C.uint8_t(v[1]), C.uint8_t(v[2]), C.uint8_t(v[3]))); err != nil {
					return err
				}
			}
		}
		for _, ch := range n.children {
			if err := add(ch, h); err != nil {
				return err
			}
		}
		if n.selected != nil {
			member := false
			for _, ch := range n.children {
				if ch == n.selected {
					member = true
				}
			}
			if !member {
				return errors.New("selected Item must be a direct child")
			}
			if n.selected.app != a {
				return errors.New("selected Item must belong to group")
			}
			if e := result(C.go_set_selected(a.ptr, h, n.selected.handle)); e != nil {
				return e
			}
		}
		return nil
	}
	rollback := func() {
		if root.app == a {
			_ = result(C.go_node_remove(a.ptr, root.handle))
		}
		var clear func(*Node)
		clear = func(n *Node) {
			if n == nil {
				return
			}
			if n.app == a {
				delete(a.nodes, keyID(n.handle))
				n.app = nil
				n.handle = C.cui_bridge_node{}
			}
			for _, ch := range n.children {
				clear(ch)
			}
		}
		clear(root)
	}
	if err := add(root, C.cui_bridge_node{}); err != nil {
		rollback()
		return err
	}
	var surfaces func(*Node) error
	surfaces = func(n *Node) error {
		for _, ch := range n.children {
			if e := surfaces(ch); e != nil {
				return e
			}
		}
		if n.anchor != nil {
			if n.anchor.app != a || n.anchor.removed {
				return errors.New("popover anchor must belong to app")
			}
			if e := result(C.go_set_anchor(a.ptr, n.handle, n.anchor.handle)); e != nil {
				return e
			}
		}
		if n.kind >= 24 && n.kind <= 27 && n.hasOpen {
			if e := result(C.go_set_open(a.ptr, n.handle, C.uint32_t(btoi(n.opened)))); e != nil {
				return e
			}
		}
		return nil
	}
	if e := surfaces(root); e != nil {
		rollback()
		return e
	}
	for _, node := range mountedAttachmentPreviews {
		node.attachmentPreview = nil
	}
	a.root = root
	return nil
}
func (a *App) Step(dt float32) error {
	if a != nil && (a.sliderDispatching || a.timeDispatching) {
		return errors.New("Step unavailable during controlled Slider dispatch")
	}
	if a != nil && a.running {
		return errors.New("Step is unavailable during Run")
	}
	if err := a.check(); err != nil {
		return err
	}
	if !finite(dt) || dt < 0 {
		return errors.New("invalid dt")
	}
	return result(C.go_step(a.ptr, C.float(dt)))
}
func (a *App) QueuePointer(p PointerPhase, x, y float32) error {
	if err := a.check(); err != nil {
		return err
	}
	if p < PointerMove || p > PointerUp || !finite(x) || !finite(y) {
		return errors.New("invalid pointer")
	}
	return result(C.go_queue_pointer(a.ptr, C.cui_bridge_pointer_phase(p), C.float(x), C.float(y)))
}
func (a *App) QueueKey(p KeyPhase, k Key, mods uint32, repeat bool) error {
	if err := a.check(); err != nil {
		return err
	}
	if p < KeyDown || p > KeyUp || k < Tab || k > Down {
		return errors.New("invalid key")
	}
	return result(C.go_queue_key(a.ptr, C.cui_bridge_key_phase(p), C.cui_bridge_key(k), C.uint32_t(mods), C.uint32_t(btoi(repeat))))
}
func (a *App) Bounds(n *Node) (Rect, error) {
	if err := a.check(); err != nil {
		return Rect{}, err
	}
	if n == nil || n.app != a || n.removed {
		return Rect{}, errors.New("node belongs to another app or was removed")
	}
	var r C.cui_bridge_rect
	if err := result(C.go_node_bounds(a.ptr, n.handle, &r)); err != nil {
		return Rect{}, err
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (a *App) PollEvent() (Event, bool, error) {
	if err := a.check(); err != nil {
		return Event{}, false, err
	}
	var e C.cui_bridge_event_ex
	var required C.uint32_t
	dst := (*C.char)(C.malloc(4097))
	defer C.free(unsafe.Pointer(dst))
	code := C.go_poll_event_ex(a.ptr, &e, dst, 4097, &required)
	if code == C.CUI_BRIDGE_EMPTY {
		return Event{}, false, nil
	}
	if err := result(code); err != nil {
		return Event{}, false, err
	}
	n := a.nodes[keyID(e.node)]
	if n == nil {
		return Event{}, false, errors.New("unknown event node")
	}
	selected := a.nodes[keyID(e.selected)]
	return Event{Node: n, Frame: uint64(e.frame_sequence), Type: int(e._type), Checked: e.checked != 0, Value: float64(e.value), Text: C.GoStringN(dst, C.int(e.text_length)), Selected: selected, SelectedHandle: Handle{uint32(e.selected.slot), uint32(e.selected.generation)}, CloseResult: int(e.value)}, true, nil
}

// NotificationKindV1 identifies the owning payload view. Initial V1 deliberately
// leaves scalar/text/selection producers in NotificationLegacyV1.
type NotificationKindV1 uint32

const (
	NotificationActivationV1 NotificationKindV1 = iota + 1
	NotificationBooleanChangeV1
	NotificationTableChangeV1
	NotificationSurfaceCloseV1
	NotificationLegacyV1
)

type LegacySnapshotV1 struct {
	Type           uint32
	Handle         Handle
	Frame          uint64
	CheckedRaw     uint32
	Value          float64
	Text           string
	SelectedHandle Handle
}
type BooleanChangeV1 struct{ Value bool }
type TableChangeV1 struct{ Reason, Row, Column, SortColumn, SortDirection uint32 }
type SurfaceCloseV1 struct{ Result uint32 }

// Exactly the Kind-selected payload is nonnil; Activation/Legacy have none.
// Handle/frame are captured identities, not live references or global ordering.
type NotificationV1 struct {
	Kind     NotificationKindV1
	Handle   Handle
	Node     *Node
	NodeKind uint32
	Frame    uint64
	HasFrame bool
	Legacy   LegacySnapshotV1
	Boolean  *BooleanChangeV1
	Table    *TableChangeV1
	Surface  *SurfaceCloseV1
}

var ErrNotificationPayloadLimit = errors.New("notification V1 text exceeds 1 MiB including NUL; event remains queued")

const notificationTextLimitV1 = 1 << 20

// readNotificationTextV1 retries once and never asks C to consume an event whose
// reported capacity exceeds the policy. Only successful native reads dequeue.
func readNotificationTextV1(read func([]byte) (uint32, uint32, error)) ([]byte, error) {
	buffer := make([]byte, 4097)
	for attempt := 0; attempt < 2; attempt++ {
		status, required, err := read(buffer)
		if err != nil {
			return nil, err
		}
		if status == 0 {
			return buffer, nil
		}
		if status == 1 {
			return nil, nil
		}
		if status != 5 {
			return nil, fmt.Errorf("notification V1 status %d", status)
		}
		if required > notificationTextLimitV1 {
			return nil, ErrNotificationPayloadLimit
		}
		if attempt != 0 || required <= uint32(len(buffer)) {
			return nil, errors.New("inconsistent notification V1 capacity")
		}
		buffer = make([]byte, int(required))
	}
	panic("unreachable")
}

// PollNotificationV1 manually drains the same FIFO as PollEvent/Dispatch. It
// invokes no handlers. Unsupported old libraries fail without legacy fallback.
func (a *App) PollNotificationV1() (NotificationV1, bool, error) {
	if err := a.check(); err != nil {
		return NotificationV1{}, false, err
	}
	var raw C.cui_bridge_notification_v1
	buffer, err := readNotificationTextV1(func(buffer []byte) (uint32, uint32, error) {
		var required C.uint32_t
		code := C.go_poll_notification_v1(a.ptr, &raw, (*C.char)(unsafe.Pointer(&buffer[0])), C.uint32_t(len(buffer)), &required)
		if code != C.CUI_BRIDGE_OK && code != C.CUI_BRIDGE_EMPTY && code != C.CUI_BRIDGE_CAPACITY {
			return 0, 0, result(code)
		}
		return uint32(code), uint32(required), nil
	})
	if err != nil || buffer == nil {
		return NotificationV1{}, false, err
	}
	if raw.struct_size != C.sizeof_cui_bridge_notification_v1 || raw.schema_version != 1 || uint64(raw.text_length) >= uint64(len(buffer)) || buffer[int(raw.text_length)] != 0 || raw.flags & ^C.uint32_t(1) != 0 {
		return NotificationV1{}, false, errors.New("invalid notification V1 layout/text/flags")
	}
	text := string(buffer[:int(raw.text_length)])
	if !utf8.ValidString(text) {
		return NotificationV1{}, false, errors.New("invalid notification V1 UTF-8")
	}
	old := raw.legacy
	out := NotificationV1{Handle: Handle{uint32(raw.node.slot), uint32(raw.node.generation)}, Node: a.nodes[keyID(raw.node)], NodeKind: uint32(raw.node_kind), Frame: uint64(raw.frame_sequence), HasFrame: raw.flags&1 != 0,
		Legacy: LegacySnapshotV1{Type: uint32(old._type), Handle: Handle{uint32(old.node.slot), uint32(old.node.generation)}, Frame: uint64(old.frame_sequence), CheckedRaw: uint32(old.checked), Value: float64(old.value), Text: text, SelectedHandle: Handle{uint32(old.selected.slot), uint32(old.selected.generation)}}}
	switch raw.payload_kind {
	case C.CUI_BRIDGE_PAYLOAD_NONE:
		if raw.kind != C.CUI_BRIDGE_NOTIFY_ACTIVATE {
			return NotificationV1{}, false, errors.New("invalid activation V1 kind")
		}
		out.Kind = NotificationActivationV1
	case C.CUI_BRIDGE_PAYLOAD_BOOLEAN:
		value := C.go_notification_boolean(&raw)
		if value > 1 || raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE {
			return NotificationV1{}, false, errors.New("invalid boolean V1 payload")
		}
		out.Kind, out.Boolean = NotificationBooleanChangeV1, &BooleanChangeV1{Value: value != 0}
	case C.CUI_BRIDGE_PAYLOAD_TABLE:
		table := C.go_notification_table(&raw)
		if raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE {
			return NotificationV1{}, false, errors.New("invalid table V1 kind")
		}
		out.Kind, out.Table = NotificationTableChangeV1, &TableChangeV1{uint32(table.reason), uint32(table.row), uint32(table.column), uint32(table.sort_column), uint32(table.sort_direction)}
	case C.CUI_BRIDGE_PAYLOAD_SURFACE:
		value := uint32(C.go_notification_surface(&raw))
		if value < 1 || value > 3 || raw.kind != C.CUI_BRIDGE_NOTIFY_LIFECYCLE {
			return NotificationV1{}, false, errors.New("invalid surface V1 payload")
		}
		out.Kind, out.Surface = NotificationSurfaceCloseV1, &SurfaceCloseV1{value}
	case C.CUI_BRIDGE_PAYLOAD_LEGACY:
		out.Kind = NotificationLegacyV1
	default:
		return NotificationV1{}, false, errors.New("unsupported notification V1 payload")
	}
	return out, true, nil
}

// ValueNotificationKindV1 is separate from the unchanged initial V1 enum.
type ValueNotificationKindV1 uint32

const (
	ValueNotificationActivationV1 ValueNotificationKindV1 = iota + 1
	ValueNotificationBooleanChangeV1
	ValueNotificationTableChangeV1
	ValueNotificationSurfaceCloseV1
	ValueNotificationLegacyV1
	ValueNotificationScalarChangeV1
	ValueNotificationTextChangeV1
	ValueNotificationSelectionChangeV1
)

type ScalarChangeV1 struct{ Value float64 }
type TextChangeV1 struct{ Text string }
type SelectionChangeV1 struct {
	Handle Handle
	Node   *Node
}

// ValueNotificationV1 owns copied values. Node references are optional live objects.
// Only the Kind-selected payload is nonnil; activation/legacy have none.
type ValueNotificationV1 struct {
	Kind      ValueNotificationKindV1
	Handle    Handle
	Node      *Node
	NodeKind  uint32
	Frame     uint64
	HasFrame  bool
	Legacy    LegacySnapshotV1
	Boolean   *BooleanChangeV1
	Table     *TableChangeV1
	Surface   *SurfaceCloseV1
	Scalar    *ScalarChangeV1
	Text      *TextChangeV1
	Selection *SelectionChangeV1
}

// PollValueNotificationV1 explicitly drains the shared queue without dispatching.
// It negotiates known value categories; unavailable libraries never fall back.
func (a *App) PollValueNotificationV1() (ValueNotificationV1, bool, error) {
	raw, text, ok, err := a.readValueNotificationV1(0)
	if !ok || err != nil {
		return ValueNotificationV1{}, false, err
	}
	return a.decodeValueNotificationV1(raw, text)
}

func (a *App) readValueNotificationV1(requiredMask C.uint32_t) (C.cui_bridge_notification_v1, string, bool, error) {
	var raw C.cui_bridge_notification_v1
	if err := a.check(); err != nil {
		return raw, "", false, err
	}
	var requiredCopy uint32
	var enabled C.uint32_t
	buffer, err := readNotificationTextV1(func(buffer []byte) (uint32, uint32, error) {
		var required C.uint32_t
		code := C.go_poll_value_notification_v1(a.ptr, &raw, (*C.char)(unsafe.Pointer(&buffer[0])), C.uint32_t(len(buffer)), &required, requiredMask, &enabled)
		if code != C.CUI_BRIDGE_OK && code != C.CUI_BRIDGE_EMPTY && code != C.CUI_BRIDGE_CAPACITY {
			return 0, 0, result(code)
		}
		requiredCopy = uint32(required)
		return uint32(code), uint32(required), nil
	})
	if err != nil || buffer == nil {
		return raw, "", false, err
	}
	if raw.struct_size != C.sizeof_cui_bridge_notification_v1 || raw.schema_version != 1 || uint64(raw.text_length) >= uint64(len(buffer)) || buffer[int(raw.text_length)] != 0 || raw.flags & ^C.uint32_t(1) != 0 {
		return raw, "", false, errors.New("invalid notification V1 layout/text/flags")
	}
	if requiredCopy != uint32(raw.text_length)+1 || raw.legacy.text_length != raw.text_length ||
		raw.legacy.frame_sequence != raw.frame_sequence || raw.legacy.node != raw.node || C.uint32_t(raw.legacy._type) != raw.kind ||
		raw.kind < 1 || raw.kind > 3 || ((raw.flags&1 != 0) != (raw.frame_sequence != 0)) {
		return raw, "", false, errors.New("invalid value notification V1 metadata")
	}
	if ((raw.payload_kind >= 2 && raw.payload_kind <= 4) || raw.payload_kind == C.CUI_BRIDGE_PAYLOAD_ATTACHMENT_ACTION) && uint32(enabled)&(1<<uint32(raw.payload_kind)) == 0 {
		return raw, "", false, errors.New("unnegotiated value notification V1 payload")
	}
	text := string(buffer[:int(raw.text_length)])
	if !utf8.ValidString(text) {
		return raw, "", false, errors.New("invalid notification V1 UTF-8")
	}
	return raw, text, true, nil
}

func (a *App) decodeValueNotificationV1(raw C.cui_bridge_notification_v1, text string) (ValueNotificationV1, bool, error) {
	old := raw.legacy
	out := ValueNotificationV1{Handle: Handle{uint32(raw.node.slot), uint32(raw.node.generation)}, Node: a.nodes[keyID(raw.node)], NodeKind: uint32(raw.node_kind), Frame: uint64(raw.frame_sequence), HasFrame: raw.flags&1 != 0,
		Legacy: LegacySnapshotV1{Type: uint32(old._type), Handle: Handle{uint32(old.node.slot), uint32(old.node.generation)}, Frame: uint64(old.frame_sequence), CheckedRaw: uint32(old.checked), Value: float64(old.value), Text: text, SelectedHandle: Handle{uint32(old.selected.slot), uint32(old.selected.generation)}}}
	switch raw.payload_kind {
	case C.CUI_BRIDGE_PAYLOAD_NONE:
		if raw.kind != C.CUI_BRIDGE_NOTIFY_ACTIVATE {
			return ValueNotificationV1{}, false, errors.New("invalid activation V1 kind")
		}
		out.Kind = ValueNotificationActivationV1
	case C.CUI_BRIDGE_PAYLOAD_BOOLEAN:
		value := C.go_notification_boolean(&raw)
		if value > 1 || raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE {
			return ValueNotificationV1{}, false, errors.New("invalid boolean V1 payload")
		}
		out.Kind, out.Boolean = ValueNotificationBooleanChangeV1, &BooleanChangeV1{Value: value != 0}
	case C.CUI_BRIDGE_PAYLOAD_SCALAR:
		value := float64(C.go_notification_scalar(&raw))
		if raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE || math.IsNaN(value) || math.IsInf(value, 0) || value != float64(old.value) {
			return ValueNotificationV1{}, false, errors.New("invalid scalar V1 payload")
		}
		out.Kind, out.Scalar = ValueNotificationScalarChangeV1, &ScalarChangeV1{Value: value}
	case C.CUI_BRIDGE_PAYLOAD_TEXT:
		if raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE {
			return ValueNotificationV1{}, false, errors.New("invalid text V1 kind")
		}
		out.Kind, out.Text = ValueNotificationTextChangeV1, &TextChangeV1{Text: text}
	case C.CUI_BRIDGE_PAYLOAD_SELECTION:
		selected := C.go_notification_selected(&raw)
		if raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE || selected != old.selected || ((selected.slot == 0) != (selected.generation == 0)) {
			return ValueNotificationV1{}, false, errors.New("invalid selection V1 payload")
		}
		out.Kind, out.Selection = ValueNotificationSelectionChangeV1, &SelectionChangeV1{Handle: Handle{uint32(selected.slot), uint32(selected.generation)}, Node: a.nodes[keyID(selected)]}
	case C.CUI_BRIDGE_PAYLOAD_TABLE:
		table := C.go_notification_table(&raw)
		if raw.kind != C.CUI_BRIDGE_NOTIFY_CHANGE || (table.reason != 1 && table.reason != 2) {
			return ValueNotificationV1{}, false, errors.New("invalid table V1 kind")
		}
		out.Kind, out.Table = ValueNotificationTableChangeV1, &TableChangeV1{uint32(table.reason), uint32(table.row), uint32(table.column), uint32(table.sort_column), uint32(table.sort_direction)}
	case C.CUI_BRIDGE_PAYLOAD_SURFACE:
		value := uint32(C.go_notification_surface(&raw))
		if value < 1 || value > 3 || raw.kind != C.CUI_BRIDGE_NOTIFY_LIFECYCLE {
			return ValueNotificationV1{}, false, errors.New("invalid surface V1 payload")
		}
		out.Kind, out.Surface = ValueNotificationSurfaceCloseV1, &SurfaceCloseV1{value}
	case C.CUI_BRIDGE_PAYLOAD_LEGACY:
		out.Kind = ValueNotificationLegacyV1
	default:
		return ValueNotificationV1{}, false, errors.New("unsupported notification V1 payload")
	}
	return out, true, nil
}

func (a *App) Dispatch() (int, error) {
	if a != nil && (a.sliderDispatching || a.timeDispatching) {
		return 0, errors.New("nested typed dispatch")
	}
	count := 0
	for {
		e, ok, err := a.PollEvent()
		if err != nil {
			return count, err
		}
		if !ok {
			if a.sliderNamespace != nil && !a.closed && !a.sliderClosing {
				n, e := a.DispatchSliderRequests()
				if e != nil {
					return count + n, e
				}
				count += n
			}
			if a.timeEnabled && !a.closed && !a.closeRequested {
				n, e := a.DispatchTimeFieldChanges()
				return count + n, e
			}
			return count, nil
		}
		count++
		if e.Type == 1 && e.Node.click != nil {
			e.Node.click(e)
		}
		if e.Type == 2 && e.Node.change != nil {
			e.Node.change(e)
		}
		if e.Type == 3 && e.Node.onClose != nil {
			e.Node.onClose(e)
		}
		if a.closed {
			return count, nil
		}
	}
}

// Run owns native event pumping, including live resize. dt is retained for
// compatibility; native scheduling supplies actual frame timing.
func (a *App) Run(dt float32) error {
	return a.RunWith(dt, nil)
}

// RunWith calls afterFrame on the owner thread after each successful native
// frame and regular dispatch. Closing during dispatch skips the hook.
// Returning an error stops the runner and returns that error to the caller.
func (a *App) RunWith(dt float32, afterFrame func(*App) error) error {
	if a != nil && (a.sliderDispatching || a.timeDispatching) {
		return errors.New("Run unavailable during typed dispatch")
	}
	if err := a.check(); err != nil {
		return err
	}
	if a.root == nil {
		return errors.New("mount a tree first")
	}
	if !finite(dt) || dt <= 0 {
		return errors.New("Run requires positive finite dt")
	}
	if a.running {
		return errors.New("app is already running")
	}
	state := &runState{app: a, afterFrame: afterFrame}
	handle := cgo.NewHandle(state)
	defer handle.Delete()
	a.running = true
	defer func() {
		a.running = false
		if a.closed {
			runtime.UnlockOSThread()
		}
	}()
	code := C.go_run(a.ptr, C.uintptr_t(handle))
	if state.panicked {
		panic(state.panicValue)
	}
	if state.err != nil {
		return state.err
	}
	if code == C.CUI_BRIDGE_CLOSED {
		return nil
	}
	return result(code)
}
func (n *Node) Remove() error {
	if n == nil || n.app == nil {
		return errors.New("node not mounted")
	}
	if err := n.app.check(); err != nil {
		return err
	}
	if err := result(C.go_node_remove(n.app.ptr, n.handle)); err != nil {
		return err
	}
	var mark func(*Node)
	mark = func(x *Node) {
		delete(x.app.nodes, keyID(x.handle))
		x.removed = true
		if x.notificationAction {
			x.click = nil
		}
		x.timeChange = nil
		x.iconConfig, x.badgeConfig, x.paginationConfig, x.ratingConfig = nil, nil, nil, nil
		x.disclosureConfig, x.disclosureStyle = nil, nil
		x.disclosureHeadings, x.disclosureDivider = disclosureHeadingLayers{}, nil
		x.disclosureTitles = disclosureTitleStyles{}
		if x.slider != nil {
			x.slider.callback = nil
		}
		for _, ch := range x.children {
			mark(ch)
		}
	}
	mark(n)
	return nil
}
func Checkbox(key string) *Node { return &Node{kind: 5, key: key} }
func Radio(key string) *Node    { return &Node{kind: 6, key: key} }
func Switch(key string) *Node   { return &Node{kind: 7, key: key} }
func Toggle(key string) *Node   { return &Node{kind: 8, key: key} }
func Slider(key string) *Node   { return &Node{kind: 9, key: key, min: 0, max: 1, step: 0.01} }
func Progress(key string) *Node { return &Node{kind: 10, key: key} }
func Badge(text string) *Node   { return &Node{kind: 11, text: text} }

// ErrRadioItemContentUnavailable indicates the provider lacks passive RadioGroup Item content.
var ErrRadioItemContentUnavailable = errors.New("RadioGroup Item content is unavailable in this bridge")

// SupportsRadioItemContent reports whether passive Item decorations are available.
func (a *App) SupportsRadioItemContent() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_radio_content_supported() != 0, nil
}

// DecorateContent opts a direct RadioGroup Item into passive child rendering.
// Use SetRadioItemContent to handle mounted update errors directly.
func (n *Node) DecorateContent(enabled bool) *Node {
	if n != nil && n.err == nil {
		n.err = n.SetRadioItemContent(enabled)
	}
	return n
}

// SetRadioItemContent enables or hides retained passive children without changing selection.
// Only direct RadioGroup Items may enable it. Children accept Row, Column, Label,
// plain Badge, Avatar, Kbd, Tag, Icon and Image; the radio row owns their input.
// Disabling remains usable with older providers.
func (n *Node) SetRadioItemContent(enabled bool) error {
	if n == nil || n.removed || n.kind != int(C.CUI_BRIDGE_ITEM) {
		return errors.New("decorative content requires a live RadioGroup Item")
	}
	if n.app != nil {
		supported, err := n.app.SupportsRadioItemContent()
		if err != nil {
			return err
		}
		if supported {
			if err := result(C.go_set_radio_content(n.app.ptr, n.handle, C.uint32_t(btoi(enabled)))); err != nil {
				return err
			}
		} else if enabled {
			return ErrRadioItemContentUnavailable
		}
	}
	n.radioContent = enabled
	return nil
}

func radioContentKind(kind int) bool {
	switch kind {
	case int(C.CUI_BRIDGE_ROW), int(C.CUI_BRIDGE_COLUMN), int(C.CUI_BRIDGE_LABEL),
		int(C.CUI_BRIDGE_BADGE), int(C.CUI_BRIDGE_AVATAR), int(C.CUI_BRIDGE_KBD),
		int(C.CUI_BRIDGE_TAG), int(C.CUI_BRIDGE_ICON), int(C.CUI_BRIDGE_IMAGE):
		return true
	}
	return false
}

// ErrRadioTooltipUnavailable indicates the selected bridge lacks retained Radio hints.
var ErrRadioTooltipUnavailable = errors.New("Radio tooltip is unavailable in this bridge")

// SupportsRadioTooltip reports support for retained standalone Radio and group Item hints.
func (a *App) SupportsRadioTooltip() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_radio_tooltip_supported() != 0, nil
}

// RadioTooltip configures a copied hover/focus hint. Delay defaults to 350 milliseconds.
// Use SetRadioTooltip to handle mounted update errors directly.
func (n *Node) RadioTooltip(text string, delayMS ...int) *Node {
	if n != nil && n.err == nil {
		n.err = n.SetRadioTooltip(text, delayMS...)
	}
	return n
}

// SetRadioTooltip replaces a hint atomically. Text is at most 256 valid UTF-8 bytes,
// without NUL; delay is 0..10000 milliseconds (default 350). Empty text clears
// and ignores the numeric delay.
// Only standalone Radio and direct RadioGroup Item nodes support hints. Detached
// Item ownership is checked at Mount. Clearing works with older providers.
func (n *Node) SetRadioTooltip(text string, delayMS ...int) error {
	if n == nil || n.removed || (n.kind != int(C.CUI_BRIDGE_RADIO) && n.kind != int(C.CUI_BRIDGE_ITEM)) {
		return errors.New("Radio tooltip requires a live Radio or RadioGroup Item")
	}
	delay := 350
	if len(delayMS) > 1 {
		return errors.New("Radio tooltip accepts one delay")
	}
	if len(delayMS) == 1 {
		delay = delayMS[0]
	}
	if !utf8.ValidString(text) || strings.IndexByte(text, 0) >= 0 || len(text) > 256 || (text != "" && (delay < 0 || delay > 10000)) {
		return errors.New("invalid Radio tooltip text or delay")
	}
	if text == "" {
		delay = 350
	}
	if n.app != nil {
		supported, err := n.app.SupportsRadioTooltip()
		if err != nil {
			return err
		}
		if supported {
			value := C.CString(text)
			defer C.free(unsafe.Pointer(value))
			if err := result(C.go_set_radio_tooltip(n.app.ptr, n.handle, value, C.uint32_t(delay))); err != nil {
				return err
			}
		} else if text != "" {
			return ErrRadioTooltipUnavailable
		}
	}
	n.radioTooltipText, n.radioTooltipDelay = text, delay
	return nil
}

// ErrRadioTabIndexUnavailable indicates the bridge lacks optional Radio tab order.
var ErrRadioTabIndexUnavailable = errors.New("Radio tab index is unavailable in this bridge")

// SupportsRadioTabIndex reports optional signed Radio and RadioGroup tab order support.
func (a *App) SupportsRadioTabIndex() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_radio_tab_index_supported() != 0, nil
}

// TabIndex opts a Radio or RadioGroup into explicit signed tab order, including zero.
// Lower values come first. RadioGroup shares one stop across its Items.
func (n *Node) TabIndex(index int32) *Node {
	if n.err == nil {
		n.err = n.SetRadioTabIndex(index)
	}
	return n
}

// SetRadioTabIndex sets explicit tab order before or after Mount without selecting.
func (n *Node) SetRadioTabIndex(index int32) error { return n.setRadioTabIndex(&index) }

// ClearRadioTabIndex restores automatic tab order. Clearing also works on older bridges.
func (n *Node) ClearRadioTabIndex() error { return n.setRadioTabIndex(nil) }

func (n *Node) setRadioTabIndex(index *int32) error {
	if n == nil || n.removed || (n.kind != int(C.CUI_BRIDGE_RADIO) && n.kind != int(C.CUI_BRIDGE_RADIO_GROUP)) {
		return errors.New("tab index requires a live Radio or RadioGroup")
	}
	if n.app != nil {
		supported, err := n.app.SupportsRadioTabIndex()
		if err != nil {
			return err
		}
		if supported {
			var value int32
			if index != nil {
				value = *index
			}
			if err := result(C.go_set_radio_tab_index(n.app.ptr, n.handle, C.uint32_t(btoi(index != nil)), C.int32_t(value))); err != nil {
				return err
			}
		} else if index != nil {
			return ErrRadioTabIndexUnavailable
		}
	}
	n.radioTabIndex = index
	return nil
}

// ErrRadioMotionUnavailable indicates the selected bridge lacks retained Radio motion.
var ErrRadioMotionUnavailable = errors.New("Radio motion is unavailable in this bridge")

// SupportsRadioMotion reports whether the active bridge can animate retained Radio selection.
func (a *App) SupportsRadioMotion() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_radio_motion_supported() != 0, nil
}

// AnimateSelection is the fluent Radio and RadioGroup motion setting.
// Use SetRadioMotion to handle errors directly when changing a mounted control.
func (n *Node) AnimateSelection(enabled bool) *Node {
	if n.err == nil {
		n.err = n.SetRadioMotion(enabled)
	}
	return n
}

// SetRadioMotion sets the optional theme animation before or after Mount.
// It preserves selection and change events. Disabling works on older bridges.
func (n *Node) SetRadioMotion(enabled bool) error {
	if n == nil || n.removed || (n.kind != int(C.CUI_BRIDGE_RADIO) && n.kind != int(C.CUI_BRIDGE_RADIO_GROUP)) {
		return errors.New("selection motion requires a live Radio or RadioGroup")
	}
	if n.app != nil {
		supported, err := n.app.SupportsRadioMotion()
		if err != nil {
			return err
		}
		if supported {
			if err := result(C.go_set_radio_motion(n.app.ptr, n.handle, C.uint32_t(btoi(enabled)))); err != nil {
				return err
			}
		} else if enabled {
			return ErrRadioMotionUnavailable
		}
	}
	n.radioMotion = enabled
	return nil
}

func (n *Node) Checked(v bool) *Node {
	if n.err != nil {
		return n
	}
	if (n.kind < 5 || n.kind > 8) && n.kind != 20 {
		n.err = errors.New("checked requires selection component")
		return n
	}
	n.checked = v
	n.hasChecked = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_checked(n.app.ptr, n.handle, C.uint32_t(btoi(v))))
		}
	}
	return n
}
func (n *Node) Value(v float64) *Node {
	if n.err != nil {
		return n
	}
	if (n.kind != 9 && n.kind != 10 && n.kind != 14 && n.kind != 42 && n.kind != 43 && n.kind != 52 && n.kind != 74) || math.IsNaN(v) || math.IsInf(v, 0) || (n.kind == 74 && (v < 0 || v > 1000000)) {
		n.err = errors.New("invalid value")
		return n
	}
	n.value = v
	n.hasValue = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_value(n.app.ptr, n.handle, C.double(v)))
		}
	}
	return n
}
func (n *Node) Range(min, max, step float64) *Node {
	if n.err != nil {
		return n
	}
	if (n.kind != 9 && n.kind != 14) || math.IsNaN(min) || math.IsNaN(max) || math.IsNaN(step) || math.IsInf(min, 0) || math.IsInf(max, 0) || math.IsInf(step, 0) || min >= max || step <= 0 {
		n.err = errors.New("invalid range")
		return n
	}
	n.min, n.max, n.step = min, max, step
	n.hasRange = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_range(n.app.ptr, n.handle, C.double(min), C.double(max), C.double(step)))
		}
	}
	return n
}
func (n *Node) OnChange(fn func(Event)) *Node {
	if n.err != nil {
		return n
	}
	if !((n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || n.kind == 17 || n.kind == 18 || (n.kind >= 20 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 46 || (n.kind >= 54 && n.kind <= 59) || n.kind == 61 || n.kind == 64 || n.kind == 65 || n.kind == 67 || n.kind == 68 || n.kind == 71 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 77 || n.kind == 78 || n.kind == 79 || n.kind == 80) {
		n.err = errors.New("change requires stateful component")
		return n
	}
	n.change = fn
	return n
}
func (a *App) Checked(n *Node) (bool, error) {
	if e := a.check(); e != nil {
		return false, e
	}
	if n == nil || n.app != a || n.removed {
		return false, errors.New("invalid node")
	}
	var v C.uint32_t
	if e := result(C.go_get_checked(a.ptr, n.handle, &v)); e != nil {
		return false, e
	}
	return v != 0, nil
}
func (a *App) Value(n *Node) (float64, error) {
	if e := a.check(); e != nil {
		return 0, e
	}
	if n == nil || n.app != a || n.removed {
		return 0, errors.New("invalid node")
	}
	var v C.double
	if e := result(C.go_get_value(a.ptr, n.handle, &v)); e != nil {
		return 0, e
	}
	return float64(v), nil
}
func Input(key string) *Node       { return &Node{kind: 12, key: key} }
func Textarea(key string) *Node    { return &Node{kind: 13, key: key} }
func NumberInput(key string) *Node { return &Node{kind: 14, key: key, min: 0, max: 100, step: 1} }
func (a *App) QueueText(v string) error {
	if err := a.check(); err != nil {
		return err
	}
	if err := valid(v); err != nil {
		return err
	}
	p := cs(v)
	defer C.free(unsafe.Pointer(p))
	return result(C.go_queue_text(a.ptr, p))
}
func (a *App) Text(n *Node) (string, error) {
	if err := a.check(); err != nil {
		return "", err
	}
	if n == nil || n.app != a || n.removed {
		return "", errors.New("invalid node")
	}
	p := (*C.char)(C.malloc(4097))
	defer C.free(unsafe.Pointer(p))
	var required C.uint32_t
	if err := result(C.go_get_text(a.ptr, n.handle, p, 4097, &required)); err != nil {
		return "", err
	}
	return C.GoString(p), nil
}
func Collapsible(key string) *Node { return &Node{kind: 15, key: key} }
func Alert(key, title, message string) *Node {
	return &Node{kind: 16, key: key, text: title, detail: message}
}
func RadioGroup(key string) *Node  { return &Node{kind: 17, key: key} }
func Tabs(key string) *Node        { return &Node{kind: 18, key: key} }
func Accordion(key string) *Node   { return &Node{kind: 19, key: key} }
func Item(key, label string) *Node { return &Node{kind: 20, key: key, text: label} }
func (n *Node) Detail(v string) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 12 && n.kind != 14 && n.kind != 16 && n.kind != 20 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 32 && n.kind != 51 && n.kind != 52 && n.kind != 61 && n.kind != 62 && n.kind != 68 && (n.kind < 24 || n.kind > 27) {
		n.err = errors.New("detail requires Alert, Select, or ComboBox")
		return n
	}
	if err := valid(v); err != nil {
		n.err = err
		return n
	}
	if err := n.validateAttachmentLabel(v); err != nil {
		n.err = err
		return n
	}
	n.detail = v
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			p := cs(v)
			n.err = result(C.go_set_detail_text(n.app.ptr, n.handle, p))
			C.free(unsafe.Pointer(p))
		}
	}
	return n
}
func (n *Node) Open(v bool) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 15 && n.kind != 20 && n.kind != 58 && n.kind != 62 && n.kind != 63 && n.kind != 75 && (n.kind < 24 || n.kind > 27) {
		n.err = errors.New("open requires Collapsible or Accordion Item")
		return n
	}
	n.opened = v
	n.hasOpen = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_open(n.app.ptr, n.handle, C.uint32_t(btoi(v))))
		}
	}
	return n
}
func (n *Node) Selected(item *Node) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 17 && n.kind != 18 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 40 && n.kind != 41 && n.kind != 44 && n.kind != 54 && n.kind != 65 {
		n.err = errors.New("selected requires a choice group")
		return n
	}
	if item == nil || item.kind != 20 {
		n.err = errors.New("selected must be Item")
		return n
	}
	n.selected = item
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else if item.app != n.app {
			n.err = errors.New("cross-app selection")
		} else {
			n.err = result(C.go_set_selected(n.app.ptr, n.handle, item.handle))
		}
	}
	return n
}
func (a *App) Selected(n *Node) (*Node, error) {
	if e := a.check(); e != nil {
		return nil, e
	}
	if n == nil || n.app != a || n.removed {
		return nil, errors.New("invalid group")
	}
	var h C.cui_bridge_node
	if e := result(C.go_get_selected(a.ptr, n.handle, &h)); e != nil {
		return nil, e
	}
	return a.nodes[keyID(h)], nil
}
func Select(key, name, placeholder string) *Node {
	return &Node{kind: 21, key: key, text: name, detail: placeholder}
}
func ComboBox(key, label string) *Node { return &Node{kind: 22, key: key, detail: label} }
func DropdownButton(key, label, menuLabel string) *Node {
	return &Node{kind: 23, key: key, text: label, detail: menuLabel}
}
func (a *App) DropdownPartBounds(n *Node, menuPart bool) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 23 {
		return Rect{}, errors.New("invalid dropdown")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_dropdown_part_bounds(a.ptr, n.handle, C.uint32_t(btoi(menuPart)), &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}

const (
	SurfaceAccepted  = 1
	SurfaceCancelled = 2
	SurfaceClosed    = 3
)

func Dialog(key, title, description string) *Node {
	return &Node{kind: 24, key: key, text: title, detail: description}
}
func AlertDialog(key, title, description string) *Node {
	return &Node{kind: 25, key: key, text: title, detail: description}
}
func Sheet(key, title, description string) *Node {
	return &Node{kind: 26, key: key, text: title, detail: description}
}
func Popover(key, title, description string) *Node {
	return &Node{kind: 27, key: key, text: title, detail: description}
}
func (n *Node) Anchor(anchor *Node) *Node {
	if n.err != nil {
		return n
	}
	if (n.kind != 27 && n.kind != 28 && n.kind != 29) || anchor == nil {
		n.err = errors.New("anchor requires Popover, Tooltip, or HoverCard and a node")
		return n
	}
	n.anchor = anchor
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else if anchor.app != n.app || anchor.removed {
			n.err = errors.New("cross-app or removed anchor")
		} else {
			n.err = result(C.go_set_anchor(n.app.ptr, n.handle, anchor.handle))
		}
	}
	return n
}
func (n *Node) OnClose(fn func(Event)) *Node {
	if n.err != nil {
		return n
	}
	if (n.kind < 24 || n.kind > 27) && n.kind != 62 {
		n.err = errors.New("close requires surface")
		return n
	}
	n.onClose = fn
	return n
}
func (a *App) Open(n *Node) (bool, error) {
	if e := a.check(); e != nil {
		return false, e
	}
	if n == nil || n.app != a || n.removed {
		return false, errors.New("invalid node")
	}
	var v C.uint32_t
	if e := result(C.go_get_open(a.ptr, n.handle, &v)); e != nil {
		return false, e
	}
	return v != 0, nil
}
func Tooltip(key, text string) *Node   { return &Node{kind: 28, key: key, text: text} }
func HoverCard(key, name string) *Node { return &Node{kind: 29, key: key, text: name} }
func Avatar(key, name string) *Node    { return &Node{kind: 30, key: key, text: name} }
func Bubble(key, message string) *Node { return &Node{kind: 31, key: key, text: message} }
func EmptyView(key, title, description string) *Node {
	return &Node{kind: 32, key: key, text: title, detail: description}
}
func GroupBox(key, title string) *Node { return &Node{kind: 33, key: key, text: title} }
func Kbd(key, text string) *Node       { return &Node{kind: 34, key: key, text: text} }
func Skeleton(key string) *Node        { return &Node{kind: 35, key: key} }
func Spinner(key string) *Node         { return &Node{kind: 36, key: key} }
func Tag(key, text string) *Node       { return &Node{kind: 37, key: key, text: text} }
func Marker(key, text string) *Node    { return &Node{kind: 38, key: key, text: text} }
func Shimmer(key, text string) *Node   { return &Node{kind: 39, key: key, text: text} }
func (n *Node) VisualSize(width, height float32) *Node {
	if n.err != nil {
		return n
	}
	if n.spinnerConfig != nil {
		n.err = errors.New("use Spinner configuration to size a configured Spinner")
		return n
	}
	if n.skeletonConfig != nil {
		n.err = errors.New("use Skeleton configuration to size a responsive Skeleton")
		return n
	}
	if n.iconConfig != nil {
		n.err = errors.New("use Icon configuration to size a configured Icon")
		return n
	}
	if !(n.kind == 30 || n.kind == 32 || n.kind == 33 || n.kind == 35 || n.kind == 36 || n.kind == 46 || n.kind == 50 || n.kind == 51 || n.kind == 52 || n.kind == 53 || n.kind == 54 || n.kind == 56 || n.kind == 58 || n.kind == 60 || n.kind == 61 || n.kind == 64 || n.kind == 65 || n.kind == 66 || n.kind == 67 || n.kind == 68 || n.kind == 69 || n.kind == 70 || n.kind == 71 || n.kind == 72 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 78 || n.kind == 79 || n.kind == 80) || !finite(width) || !finite(height) || width < 0 || height < 0 || width > 4096 || height > 4096 {
		n.err = errors.New("invalid visual size")
		return n
	}
	n.visualWidth, n.visualHeight = width, height
	n.hasVisualSize = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_visual_size(n.app.ptr, n.handle, C.float(width), C.float(height)))
		}
	}
	return n
}
func List(key string) *Node       { return &Node{kind: 40, key: key} }
func Menu(key string) *Node       { return &Node{kind: 41, key: key} }
func Pagination(key string) *Node { return &Node{kind: 42, key: key} }
func Rating(key string) *Node     { return &Node{kind: 43, key: key} }
func Stepper(key string) *Node    { return &Node{kind: 44, key: key} }
func (n *Node) Total(v uint32) *Node {
	if n.err != nil {
		return n
	}
	if (n.kind != 42 && n.kind != 43 && n.kind != 59) || v == 0 || (n.kind == 43 && v > 10) || (n.kind == 42 && v > 1000000) || (n.kind == 59 && v > 256) {
		n.err = errors.New("invalid total")
		return n
	}
	n.total = v
	n.hasTotal = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_total(n.app.ptr, n.handle, C.uint32_t(v)))
		}
	}
	return n
}
func (n *Node) StepStatus(v uint32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 20 || v > 3 {
		n.err = errors.New("invalid step status")
		return n
	}
	n.stepStatus = v
	n.hasStepStatus = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_step_status(n.app.ptr, n.handle, C.uint32_t(v)))
		}
	}
	return n
}

// Tree records are flat Item children. Depth is their preorder indentation, 0..63.
func Tree(key string) *Node { return &Node{kind: 54, key: key} }
func (n *Node) Depth(v uint32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 20 || v > 63 {
		n.err = errors.New("invalid tree depth")
		return n
	}
	n.stepStatus, n.hasStepStatus = v, true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_step_status(n.app.ptr, n.handle, C.uint32_t(v)))
		}
	}
	return n
}
func (a *App) NumericPartBounds(n *Node, part uint32) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if n == nil || n.app != a || n.removed {
		return Rect{}, errors.New("invalid node")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_numeric_part_bounds(a.ptr, n.handle, C.uint32_t(part), &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func DescriptionList(key string) *Node { return &Node{kind: 45, key: key} }
func Scrollable(key string) *Node      { return &Node{kind: 46, key: key} }
func (n *Node) ScrollExtent(width, height float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 46 || !finite(width) || !finite(height) || width < 0 || height < 0 || width > 32768 || height > 32768 {
		n.err = errors.New("invalid scroll extent")
		return n
	}
	n.scrollWidth, n.scrollHeight = width, height
	n.hasScrollExtent = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_scroll_extent(n.app.ptr, n.handle, C.float(width), C.float(height)))
		}
	}
	return n
}
func (a *App) QueueScroll(x, y, dx, dy float32) error {
	if e := a.check(); e != nil {
		return e
	}
	if !finite(x) || !finite(y) || !finite(dx) || !finite(dy) {
		return errors.New("invalid scroll input")
	}
	return result(C.go_queue_scroll(a.ptr, C.float(x), C.float(y), C.float(dx), C.float(dy)))
}
func StatusBar(key string) *Node  { return &Node{kind: 47, key: key} }
func Toolbar(key string) *Node    { return &Node{kind: 48, key: key} }
func TitleBar(key string) *Node   { return &Node{kind: 49, key: key} }
func Icon(key, name string) *Node { return &Node{kind: 50, key: key, text: name} }
func Message(key, header, body string) *Node {
	return &Node{kind: 51, key: key, text: body, detail: header}
}

type AttachmentState int

const (
	AttachmentPending AttachmentState = iota
	AttachmentUploading
	AttachmentProcessing
	AttachmentFailed
	AttachmentComplete
)

func Attachment(key, title, description string) *Node {
	return &Node{kind: 52, key: key, text: title, detail: description}
}
func (n *Node) AttachmentStatus(v AttachmentState) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 52 || v < AttachmentPending || v > AttachmentComplete {
		n.err = errors.New("invalid attachment status")
		return n
	}
	n.attachmentStatus = int(v)
	n.hasAttachmentStatus = true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_attachment_status(n.app.ptr, n.handle, C.cui_bridge_attachment_status(v)))
		}
	}
	return n
}

// SupportsAttachmentOptions reports whether the loaded bridge supports fitted metadata.
func (a *App) SupportsAttachmentOptions() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_attachment_configure_supported() != 0, nil
}

// SupportsAttachmentPreview reports whether the loaded bridge accepts RGBA previews.
func (a *App) SupportsAttachmentPreview() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_attachment_preview_supported() != 0, nil
}

// SupportsAttachmentSlots probes the optional factory independently of preview and action APIs.
func (a *App) SupportsAttachmentSlots() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_attachment_slots_supported() != 0, nil
}

// SupportsAttachmentActions reports whether the bridge has the optional producer and V1 queue view.
func (a *App) SupportsAttachmentActions() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_attachment_actions_supported() != 0, nil
}

func (n *Node) applyAttachmentActions(actions AttachmentActions) error {
	ok, err := n.app.SupportsAttachmentActions()
	if err != nil {
		return err
	}
	if !ok {
		return ErrAttachmentActionsUnavailable
	}
	return result(C.go_attachment_set_actions(n.app.ptr, n.handle, C.uint32_t(actions)))
}

// PollComponentNotificationV1 drains one event from the same queue as Dispatch and
// PollValueNotificationV1, adding the negotiated AttachmentAction view.
func (a *App) PollComponentNotificationV1() (ComponentNotificationV1, bool, error) {
	var notice ComponentNotificationV1
	ok, err := a.SupportsAttachmentActions()
	if err != nil {
		return notice, false, err
	}
	if !ok {
		return notice, false, ErrAttachmentActionsUnavailable
	}
	raw, text, ok, err := a.readValueNotificationV1(C.CUI_BRIDGE_VALUE_ATTACHMENT_ACTION_V1)
	if !ok || err != nil {
		return notice, false, err
	}
	if raw.payload_kind == C.CUI_BRIDGE_PAYLOAD_ATTACHMENT_ACTION {
		action, err := decodeAttachmentAction(raw, C.go_notification_attachment(&raw), a.nodes[keyID(raw.node)])
		if err != nil {
			return notice, false, err
		}
		notice.AttachmentAction = &action
		return notice, true, nil
	}
	value, ok, err := a.decodeValueNotificationV1(raw, text)
	if ok && err == nil {
		notice.Value = &value
	}
	return notice, ok, err
}

func (n *Node) applyAttachmentOptions(options *attachmentOptionsWire) error {
	ok, err := n.app.SupportsAttachmentOptions()
	if err != nil {
		return err
	}
	if !ok {
		return ErrAttachmentOptionsUnavailable
	}
	return result(C.go_attachment_configure(n.app.ptr, n.handle, options))
}

func (n *Node) applyAttachmentPreview(preview *attachmentPreview) error {
	ok, err := n.app.SupportsAttachmentPreview()
	if err != nil {
		return err
	}
	if !ok {
		return ErrAttachmentPreviewUnavailable
	}
	if preview == nil {
		return result(C.go_attachment_preview(n.app.ptr, n.handle, nil, 0, 0, 0, 0))
	}
	return result(C.go_attachment_preview(n.app.ptr, n.handle, (*C.uint8_t)(unsafe.Pointer(&preview.pixels[0])),
		C.uint32_t(preview.width), C.uint32_t(preview.height), C.uint32_t(preview.stride), C.uint32_t(len(preview.pixels))))
}

func Image(key, description string) *Node { return &Node{kind: 53, key: key, text: description} }
func TimeField(key string) *Node          { return &Node{kind: 55, key: key} }
func ColorPicker(key string) *Node        { return &Node{kind: 56, key: key} }
func Calendar(key string) *Node           { return &Node{kind: 57, key: key} }
func DatePicker(key string) *Node         { return &Node{kind: 58, key: key} }
func OTPInput(key string) *Node           { return &Node{kind: 59, key: key} }
func Form(key string) *Node               { return &Node{kind: 60, key: key} }
func InputGroup(key string) *Node         { return &Node{kind: 61, key: key} }
func Notification(key, title, message string) *Node {
	return &Node{kind: 62, key: key, text: title, detail: message}
}
func FocusTrap(key string) *Node       { return &Node{kind: 63, key: key} }
func MessageScroller(key string) *Node { return &Node{kind: 64, key: key} }
func Carousel(key string) *Node        { return &Node{kind: 65, key: key} }
func Resizable(key string) *Node       { return &Node{kind: 66, key: key} }

// CarouselAxis selects the primary direction of a Carousel.
type CarouselAxis uint32

const (
	CarouselHorizontal CarouselAxis = C.CUI_BRIDGE_CAROUSEL_HORIZONTAL
	CarouselVertical   CarouselAxis = C.CUI_BRIDGE_CAROUSEL_VERTICAL
)

// ErrCarouselAxisUnavailable means the loaded bridge only supports horizontal carousels.
var ErrCarouselAxisUnavailable = errors.New("Carousel axis requires cui_bridge_set_carousel_axis_v1")

// SupportsCarouselAxis reports whether the loaded bridge supports vertical Carousel layout.
func (a *App) SupportsCarouselAxis() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_axis_supported() != 0, nil
}

// CarouselAxis sets the direction for the next frame. The selected slide is preserved.
func (n *Node) CarouselAxis(axis CarouselAxis) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != int(C.CUI_BRIDGE_CAROUSEL) || n.removed || (axis != CarouselHorizontal && axis != CarouselVertical) {
		n.err = errors.New("CarouselAxis requires a live Carousel and a valid axis")
		return n
	}
	if n.app != nil && axis != n.carouselAxis {
		supported, err := n.app.SupportsCarouselAxis()
		if err != nil {
			n.err = err
		} else if !supported {
			n.err = ErrCarouselAxisUnavailable
		} else {
			n.err = result(C.go_carousel_set_axis(n.app.ptr, n.handle, C.uint32_t(axis)))
		}
		if n.err != nil {
			return n
		}
	}
	n.carouselAxis = axis
	return n
}

// ErrCarouselLoopingUnavailable means the loaded bridge lacks Carousel edge wrapping.
var ErrCarouselLoopingUnavailable = errors.New("Carousel looping requires cui_bridge_set_carousel_looping_v1")

// SupportsCarouselLooping reports whether the loaded bridge supports Carousel edge wrapping.
func (a *App) SupportsCarouselLooping() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_looping_supported() != 0, nil
}

// CarouselLooping enables edge wrapping on a track long enough to loop.
func (n *Node) CarouselLooping(enabled bool) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != int(C.CUI_BRIDGE_CAROUSEL) || n.removed {
		n.err = errors.New("CarouselLooping requires a live Carousel")
		return n
	}
	if n.app != nil && enabled != n.carouselLooping {
		supported, err := n.app.SupportsCarouselLooping()
		if err != nil {
			n.err = err
		} else if !supported {
			n.err = ErrCarouselLoopingUnavailable
		} else {
			n.err = result(C.go_carousel_set_looping(n.app.ptr, n.handle, C.uint32_t(btoi(enabled))))
		}
		if n.err != nil {
			return n
		}
	}
	n.carouselLooping = enabled
	return n
}

// CarouselControls selects which navigation controls appear below a Carousel.
type CarouselControls struct {
	Arrows     bool
	Pagination bool
}

// ErrCarouselControlsUnavailable means the loaded bridge lacks retained controls.
var ErrCarouselControlsUnavailable = errors.New("Carousel controls require cui_bridge_set_carousel_controls_v1")

// CarouselSlotRole identifies the control whose artwork a passive Row replaces.
type CarouselSlotRole uint32

const (
	CarouselSlotPrevious CarouselSlotRole = iota + 1
	CarouselSlotNext
	CarouselSlotMarker
)

// ErrCarouselSlotsUnavailable indicates that the provider lacks the slot factory.
var ErrCarouselSlotsUnavailable = errors.New("Carousel slots require cui_bridge_carousel_slot_create_v1")

// CarouselSlot creates a passive content row. Place Previous and Next directly
// in a Carousel and Marker directly in a slide Item. Text overrides the
// accessible name. The children supply the visible content.
func CarouselSlot(key string, role CarouselSlotRole) *Node {
	n := Row(key)
	if role < CarouselSlotPrevious || role > CarouselSlotMarker {
		n.err = errors.New("invalid Carousel slot role")
	}
	n.carouselSlot = role
	return n
}

// SupportsCarouselSlots reports whether the loaded bridge supports custom content.
func (a *App) SupportsCarouselSlots() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_slots_supported() != 0, nil
}

func (a *App) validateCarouselSlots(n, parent *Node) error {
	const carousel = int(C.CUI_BRIDGE_CAROUSEL)
	const item = int(C.CUI_BRIDGE_ITEM)
	if n.carouselSlot != 0 {
		if parent == nil ||
			(n.carouselSlot == CarouselSlotMarker && parent.kind != item) ||
			(n.carouselSlot != CarouselSlotMarker && parent.kind != carousel) {
			return errors.New("invalid Carousel slot owner")
		}
		if C.go_carousel_slots_supported() == 0 {
			return ErrCarouselSlotsUnavailable
		}
	}
	switch n.kind {
	case carousel:
		var roles [4]bool
		slides := 0
		for _, child := range n.children {
			if child == nil {
				return errors.New("invalid Carousel child")
			}
			switch child.carouselSlot {
			case 0:
				if child.kind != item {
					return errors.New("Carousel children must be Items or arrow slots")
				}
				slides++
			case CarouselSlotPrevious, CarouselSlotNext:
				if roles[child.carouselSlot] {
					return errors.New("duplicate Carousel arrow slot")
				}
				roles[child.carouselSlot] = true
			default:
				return errors.New("invalid Carousel arrow slot")
			}
		}
		if slides > 64 {
			return errors.New("Carousel exceeds 64 Items")
		}
	case item:
		marker := false
		for _, child := range n.children {
			if child == nil || child.carouselSlot == 0 {
				continue
			}
			if marker || child.carouselSlot != CarouselSlotMarker || parent == nil || parent.kind != carousel {
				return errors.New("invalid or duplicate Carousel marker slot")
			}
			marker = true
		}
	}
	return nil
}

// SupportsCarouselControls reports whether the loaded bridge supports retained navigation controls.
func (a *App) SupportsCarouselControls() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_controls_supported() != 0, nil
}

// CarouselControls configures the built-in arrows and slide markers.
func (n *Node) CarouselControls(options CarouselControls) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != int(C.CUI_BRIDGE_CAROUSEL) || n.removed {
		n.err = errors.New("CarouselControls requires a live Carousel")
		return n
	}
	var mask C.uint32_t
	if options.Arrows {
		mask |= C.CUI_BRIDGE_CAROUSEL_ARROWS_V1
	}
	if options.Pagination {
		mask |= C.CUI_BRIDGE_CAROUSEL_PAGINATION_V1
	}
	if n.app != nil && mask != n.carouselControls {
		supported, err := n.app.SupportsCarouselControls()
		if err != nil {
			n.err = err
		} else if !supported {
			n.err = ErrCarouselControlsUnavailable
		} else {
			n.err = result(C.go_carousel_set_controls(n.app.ptr, n.handle, mask))
		}
		if n.err != nil {
			return n
		}
	}
	n.carouselControls = mask
	return n
}

// CarouselControlSizes configures the named size of each Carousel control group.
// Inherit restores the original dimensions.
type CarouselControlSizes struct {
	Arrows, Pagination DesignSize
}

var ErrCarouselControlSizesUnavailable = errors.New("Carousel control sizes require cui_bridge_set_carousel_control_sizes_v1")

// SupportsCarouselControlSizes reports whether the loaded bridge accepts named control sizes.
func (a *App) SupportsCarouselControlSizes() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_control_sizes_supported() != 0, nil
}

func (n *Node) setCarouselControlSizes(sizes CarouselControlSizes) error {
	supported, err := n.app.SupportsCarouselControlSizes()
	if err != nil {
		return err
	}
	if !supported {
		return ErrCarouselControlSizesUnavailable
	}
	return result(C.go_carousel_set_control_sizes(n.app.ptr, n.handle, C.uint32_t(sizes.Arrows), C.uint32_t(sizes.Pagination)))
}

// CarouselControlSizes sets both sizes together, during authoring or after mount.
func (n *Node) CarouselControlSizes(sizes CarouselControlSizes) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != int(C.CUI_BRIDGE_CAROUSEL) || n.removed ||
		sizes.Arrows < Inherit || sizes.Arrows > Size9XL ||
		sizes.Pagination < Inherit || sizes.Pagination > Size9XL {
		n.err = errors.New("CarouselControlSizes requires a live Carousel and valid design sizes")
		return n
	}
	if n.app != nil && sizes != n.carouselControlSizes {
		if err := n.setCarouselControlSizes(sizes); err != nil {
			n.err = err
			return n
		}
	}
	n.carouselControlSizes = sizes
	return n
}

// ErrCarouselBasisUnavailable means the loaded bridge lacks fractional slide sizing.
var ErrCarouselBasisUnavailable = errors.New("Carousel basis requires cui_bridge_set_carousel_slide_basis_v1")

// SupportsCarouselBasis reports whether the loaded bridge supports fractional slides.
func (a *App) SupportsCarouselBasis() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_basis_supported() != 0, nil
}

// CarouselBasis sets the fraction of the viewport occupied by this Carousel
// Item and clears content sizing. Zero resets it to the full viewport. Layout
// changes emit no selection events.
func (n *Node) CarouselBasis(fraction float32) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != int(C.CUI_BRIDGE_ITEM) || n.removed || !finite(fraction) || fraction < 0 || fraction > 1 {
		n.err = errors.New("CarouselBasis requires a live Item and a finite fraction in [0,1]")
		return n
	}
	if n.app != nil && (fraction > 0 || n.carouselBasis > 0 || n.carouselFit) {
		supported, err := n.app.SupportsCarouselBasis()
		if err != nil {
			n.err = err
		} else if !supported {
			n.err = ErrCarouselBasisUnavailable
		} else {
			n.err = result(C.go_carousel_set_basis(n.app.ptr, n.handle, C.float(fraction)))
		}
		if n.err != nil {
			return n
		}
	}
	n.carouselBasis = fraction
	n.carouselFit = false
	return n
}

// ErrCarouselNavigationUnavailable means the loaded bridge lacks custom Carousel navigation.
var ErrCarouselNavigationUnavailable = errors.New("Carousel navigation requires the optional adjacent and step operations")

// SupportsCarouselNavigation reports if the loaded bridge offers both operations.
func (a *App) SupportsCarouselNavigation() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_navigation_supported() != 0, nil
}

// CarouselAdjacent reads the destination in the last laid out Carousel.
// next=false moves toward the previous snap. Nil means no destination.
func (a *App) CarouselAdjacent(carousel *Node, next bool) (*Node, error) {
	return a.carouselNavigate(carousel, next, false)
}

// CarouselStep selects a reachable slide and queues one ordinary Carousel change.
// It returns nil at a boundary or when the Carousel has no usable layout.
func (a *App) CarouselStep(carousel *Node, next bool) (*Node, error) {
	return a.carouselNavigate(carousel, next, true)
}

func (a *App) carouselNavigate(carousel *Node, next, commit bool) (*Node, error) {
	supported, err := a.SupportsCarouselNavigation()
	if err != nil {
		return nil, err
	}
	if carousel == nil || carousel.app != a || carousel.removed || carousel.kind != int(C.CUI_BRIDGE_CAROUSEL) {
		return nil, errors.New("Carousel navigation requires a live Carousel in this App")
	}
	if !supported {
		return nil, ErrCarouselNavigationUnavailable
	}
	var item C.cui_bridge_node
	if err := result(C.go_carousel_navigate(a.ptr, carousel.handle, C.uint32_t(btoi(next)),
		C.int(btoi(commit)), &item)); err != nil {
		return nil, err
	}
	return a.nodes[keyID(item)], nil
}

// ErrCarouselFitUnavailable means the loaded bridge lacks content-sized slides.
var ErrCarouselFitUnavailable = errors.New("Carousel fit requires cui_bridge_set_carousel_slide_fit_v1")

// SupportsCarouselFit reports whether the loaded bridge supports content-sized slides.
func (a *App) SupportsCarouselFit() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_carousel_fit_supported() != 0, nil
}

// CarouselFit measures the in-flow content of this direct Carousel Item.
// Enabling it replaces any previous basis. Disabling it restores the ordinary
// size if the Item was fitted. Layout changes emit no selection events.
func (n *Node) CarouselFit(enabled bool) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != int(C.CUI_BRIDGE_ITEM) || n.removed {
		n.err = errors.New("CarouselFit requires a live Item")
		return n
	}
	if n.app != nil {
		supported, err := n.app.SupportsCarouselFit()
		if err != nil {
			n.err = err
		} else if !supported && enabled {
			n.err = ErrCarouselFitUnavailable
		} else if supported {
			var value C.uint32_t
			if enabled {
				value = 1
			}
			n.err = result(C.go_carousel_set_fit(n.app.ptr, n.handle, value))
		}
		if n.err != nil {
			return n
		}
	}
	n.carouselFit = enabled
	if enabled {
		n.carouselBasis = 0
	}
	return n
}

type TextViewFormat uint32

const (
	Markdown TextViewFormat = iota
	HTML
	Plain
)

func TextView(key, source string) *Node { return &Node{kind: 67, key: key, text: source} }
func Editor(key, source, language string) *Node {
	return &Node{kind: 68, key: key, text: source, detail: language}
}

type ChartKind uint32

const (
	LineChart ChartKind = iota
	AreaChart
	BarChart
	PieChart
)

type ChartDatum struct {
	Label string
	Value float64
}

func Chart(key string) *Node     { return &Node{kind: 69, key: key} }
func Table(key string) *Node     { return &Node{kind: 70, key: key} }
func DataTable(key string) *Node { return &Node{kind: 71, key: key} }

type PlotPoint struct{ X, Y float64 }

func Plot(key string) *Node        { return &Node{kind: 72, key: key} }
func Command(key string) *Node     { return &Node{kind: 73, key: key} }
func VirtualList(key string) *Node { return &Node{kind: 74, key: key} }
func Sidebar(key string) *Node     { return &Node{kind: 75, key: key} }

// VirtualListAxis selects the main direction of a retained VirtualList.
type VirtualListAxis uint32

const (
	VirtualListVertical VirtualListAxis = iota
	VirtualListHorizontal
)

// ErrVirtualListAxisUnavailable means the bridge lacks the optional axis setter.
var ErrVirtualListAxisUnavailable = errors.New("VirtualList axis requires cui_bridge_virtual_list_set_axis_v1")

// SupportsVirtualListAxis reports support for horizontal retained lists.
func (a *App) SupportsVirtualListAxis() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_virtual_list_axis_supported() != 0, nil
}

// VirtualListAxis sets the direction for the next frame, preserving the scalar
// main offset subject to clamping. Row extents are widths when horizontal.
func (n *Node) VirtualListAxis(axis VirtualListAxis) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 74 || n.removed || (axis != VirtualListVertical && axis != VirtualListHorizontal) {
		n.err = errors.New("VirtualListAxis requires a live VirtualList and a valid axis")
		return n
	}
	if n.app != nil && axis != n.virtualListAxis {
		supported, err := n.app.SupportsVirtualListAxis()
		if err != nil {
			n.err = err
		} else if !supported {
			n.err = ErrVirtualListAxisUnavailable
		} else {
			n.err = result(C.go_virtual_list_set_axis(n.app.ptr, n.handle, C.uint32_t(axis)))
		}
		if n.err != nil {
			return n
		}
	}
	n.virtualListAxis = axis
	return n
}

// VirtualListRow places an existing direct Item in the next layout.
type VirtualListRow struct {
	Item *Node
	// Extent is the main-axis width or height in logical pixels.
	// Use zero for 32, or a value from 1 through 4096.
	Extent float32
}

// ErrVirtualListRowsUnavailable means the loaded bridge does not export the live row API.
var ErrVirtualListRowsUnavailable = errors.New("live VirtualList rows are unavailable in this bridge")

// SupportsVirtualListRows reports whether the bridge supports projections and anchor preservation.
func (a *App) SupportsVirtualListRows() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_virtual_list_rows_supported() != 0, nil
}

func (n *Node) mountedVirtualList() error {
	if n == nil || n.app == nil || n.removed || n.kind != int(C.CUI_BRIDGE_VIRTUAL_LIST) {
		return errors.New("mounted VirtualList required")
	}
	return n.app.check()
}

// AppendVirtualListItem attaches a text Item after the existing rows.
// Call SetVirtualListRows to move it or assign a custom extent.
func (n *Node) AppendVirtualListItem(key, label string) (*Node, error) {
	if err := n.mountedVirtualList(); err != nil {
		return nil, err
	}
	if err := valid(key); err != nil {
		return nil, err
	}
	if err := valid(label); err != nil {
		return nil, err
	}
	name := cs(key)
	defer C.free(unsafe.Pointer(name))
	a := n.app
	var handle C.cui_bridge_node
	if err := result(C.go_node_create(a.ptr, n.handle, C.CUI_BRIDGE_ITEM, name, &handle)); err != nil {
		return nil, err
	}
	item := Item(key, label)
	item.app, item.handle = a, handle
	if err := item.setNativeText(label); err != nil {
		_ = C.go_node_remove(a.ptr, handle)
		return nil, err
	}
	a.nodes[keyID(handle)] = item
	n.children = slices.DeleteFunc(n.children, func(child *Node) bool { return child.removed })
	n.children = append(n.children, item)
	return item, nil
}

// PreserveVirtualListAnchor opts into retaining the first visible Item and its
// offset during row edits. An explicit scroll value before the next step wins.
func (n *Node) PreserveVirtualListAnchor(enabled bool) error {
	if err := n.mountedVirtualList(); err != nil {
		return err
	}
	if C.go_virtual_list_rows_supported() == 0 {
		return ErrVirtualListRowsUnavailable
	}
	return result(C.go_virtual_list_preserve_anchor(n.app.ptr, n.handle, C.uint32_t(btoi(enabled))))
}

// SetVirtualListRows copies a complete order and set of extents for the existing
// direct Items. Include each live Item once. Errors preserve order and extents.
// Change membership through AppendVirtualListItem and Item.Remove before calling this.
func (n *Node) SetVirtualListRows(rows []VirtualListRow) error {
	if err := n.mountedVirtualList(); err != nil {
		return err
	}
	if C.go_virtual_list_rows_supported() == 0 {
		return ErrVirtualListRowsUnavailable
	}
	var data [64]C.cui_bridge_virtual_list_row_v1
	if len(rows) > len(data) {
		return errors.New("VirtualList supports up to 64 direct Items")
	}
	children := make([]*Node, len(rows))
	for i, row := range rows {
		if row.Item == nil || row.Item.app != n.app || row.Item.removed ||
			row.Item.kind != int(C.CUI_BRIDGE_ITEM) {
			return errors.New("live same-app VirtualList Item required")
		}
		if row.Extent != 0 && !(row.Extent >= 1 && row.Extent <= 4096) {
			return errors.New("invalid VirtualList Item extent")
		}
		data[i] = C.cui_bridge_virtual_list_row_v1{item: row.Item.handle, extent: C.float(row.Extent)}
		children[i] = row.Item
	}
	if err := result(C.go_virtual_list_set_rows(n.app.ptr, n.handle, &data[0], C.uint32_t(len(rows)))); err != nil {
		return err
	}
	n.children = children
	return nil
}

func (a *App) VirtualListItemBounds(list, item *Node) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if list == nil || item == nil || list.app != a || item.app != a || list.removed || item.removed || list.kind != 74 || item.kind != 20 {
		return Rect{}, errors.New("invalid virtual list item")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_virtual_list_item_bounds(a.ptr, list.handle, item.handle, &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (a *App) CommandItemBounds(command, item *Node) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if command == nil || item == nil || command.app != a || item.app != a || command.removed || item.removed || command.kind != 73 || item.kind != 20 {
		return Rect{}, errors.New("invalid command item")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_command_item_bounds(a.ptr, command.handle, item.handle, &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (n *Node) Points(v []PlotPoint) *Node {
	if n.err == nil {
		n.err = n.SetPoints(v)
	}
	return n
}
func (n *Node) SetPoints(v []PlotPoint) error {
	if n == nil || n.kind != 72 {
		return errors.New("points require Plot")
	}
	if n.removed {
		return errors.New("removed plot")
	}
	if len(v) > 256 {
		return errors.New("too many plot points")
	}
	for _, p := range v {
		if math.IsNaN(p.X) || math.IsNaN(p.Y) || math.IsInf(p.X, 0) || math.IsInf(p.Y, 0) || math.Abs(p.X) > 16384 || math.Abs(p.Y) > 16384 {
			return errors.New("invalid plot point")
		}
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		points := make([]C.cui_bridge_plot_point, len(v))
		for i, p := range v {
			points[i] = C.cui_bridge_plot_point{x: C.double(p.X), y: C.double(p.Y)}
		}
		var ptr *C.cui_bridge_plot_point
		if len(points) > 0 {
			ptr = &points[0]
		}
		if e := result(C.go_plot_set_points(n.app.ptr, n.handle, ptr, C.uint32_t(len(points)))); e != nil {
			return e
		}
		runtime.KeepAlive(points)
	}
	n.plotPoints = append([]PlotPoint(nil), v...)
	n.hasPlotPoints = true
	return nil
}
func (a *App) PlotPoint(n *Node, index uint32) (PlotPoint, error) {
	if e := a.check(); e != nil {
		return PlotPoint{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 72 {
		return PlotPoint{}, errors.New("invalid Plot")
	}
	var p C.cui_bridge_plot_point
	if e := result(C.go_plot_get_point(a.ptr, n.handle, C.uint32_t(index), &p)); e != nil {
		return PlotPoint{}, e
	}
	return PlotPoint{float64(p.x), float64(p.y)}, nil
}

// DataTableState reports the selected row key and one-based column positions.
// Zero means no row, column, or sort column is selected.
type DataTableState struct{ Row, Column, SortColumn, SortDirection uint32 }

const (
	dataTableLegacyRows = 32
	dataTableMaxRows    = 10_000
	dataTableTextBytes  = 128
)

// ErrKeyedDataTableUnavailable indicates that the loaded bridge lacks keyed replacement.
var ErrKeyedDataTableUnavailable = errors.New("keyed DataTable replacement is unavailable in this bridge")

// SupportsKeyedDataTable reports whether the active bridge exports keyed replacement.
func (a *App) SupportsKeyedDataTable() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_data_table_keyed_supported() != 0, nil
}

// ErrDataTableCapacityUnavailable indicates that the loaded bridge does not
// support the optional DataTable capacity API.
var ErrDataTableCapacityUnavailable = errors.New("DataTable capacity is unavailable in this bridge")

// DataTableMaxRowCapacity reports the active bridge's usable opt-in row limit.
// It returns zero when the bridge does not support the capacity API.
func (a *App) DataTableMaxRowCapacity() (uint32, error) {
	if err := a.check(); err != nil {
		return 0, err
	}
	maximum := uint32(C.go_data_table_max_row_capacity())
	if maximum > dataTableMaxRows {
		maximum = dataTableMaxRows
	}
	return maximum, nil
}

func (a *App) requireDataTableCapacity(capacity uint32) error {
	maximum, err := a.DataTableMaxRowCapacity()
	if err != nil {
		return err
	}
	if maximum == 0 {
		return ErrDataTableCapacityUnavailable
	}
	if capacity > maximum {
		return fmt.Errorf("DataTable capacity exceeds the provider's %d-row limit", maximum)
	}
	return nil
}

// DataTableLoadMore is a copied near-end request. The visible index range is
// half-open. LastRowKey is zero when the table has no rows.
type DataTableLoadMore struct {
	FrameSequence            uint64
	RowCount                 uint32
	VisibleBegin, VisibleEnd uint32
	LastRowKey, Capacity     uint32
}

// ErrDataTablePagingUnavailable indicates that the active bridge lacks the
// complete and compatible V1 paging API.
var ErrDataTablePagingUnavailable = errors.New("DataTable paging is unavailable in this bridge")

// SupportsDataTablePaging reports whether the active bridge supports V1 paging.
func (a *App) SupportsDataTablePaging() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_data_table_paging_supported() != 0, nil
}

// LoadMore opts into DataTable paging using the remaining-row threshold.
func (n *Node) LoadMore(threshold uint32) *Node {
	if n.err == nil {
		n.err = n.ConfigureLoadMore(true, threshold)
	}
	return n
}

// ConfigureLoadMore enables or disables paging. A mounted DataTable must have
// column headers, even when empty. The app owns any in-flight request.
func (n *Node) ConfigureLoadMore(enabled bool, threshold uint32) error {
	if n == nil || n.kind != 71 || n.removed {
		return errors.New("paging requires a live DataTable")
	}
	if threshold > 32 {
		return errors.New("DataTable paging threshold must be in 0..32")
	}
	if n.app != nil {
		supported, err := n.app.SupportsDataTablePaging()
		if err != nil {
			return err
		}
		if !supported {
			return ErrDataTablePagingUnavailable
		}
		if err := result(C.go_data_table_configure_load_more(n.app.ptr, n.handle,
			C.uint32_t(btoi(enabled)), C.uint32_t(threshold))); err != nil {
			return err
		}
	}
	n.tableLoadMore, n.tableLoadMoreThreshold = enabled, threshold
	return nil
}

// PollLoadMore consumes one demand raised by a successful frame. Its copy
// stays valid after removal or close. A false return means there is no demand.
// Polling leaves the regular callback queue untouched.
func (n *Node) PollLoadMore() (DataTableLoadMore, bool, error) {
	if n == nil || n.kind != 71 || n.app == nil || n.removed {
		return DataTableLoadMore{}, false, errors.New("polling requires a mounted DataTable")
	}
	supported, err := n.app.SupportsDataTablePaging()
	if err != nil {
		return DataTableLoadMore{}, false, err
	}
	if !supported {
		return DataTableLoadMore{}, false, ErrDataTablePagingUnavailable
	}
	var wire C.cui_bridge_data_table_load_more_v1
	status := C.go_data_table_poll_load_more(n.app.ptr, n.handle, &wire)
	if status == C.CUI_BRIDGE_EMPTY {
		return DataTableLoadMore{}, false, nil
	}
	if err := result(status); err != nil {
		return DataTableLoadMore{}, false, err
	}
	capacity := n.tableRowCapacity
	if capacity == 0 {
		capacity = dataTableLegacyRows
	}
	if wire.reserved != 0 || wire.frame == 0 || wire.visible_begin > wire.visible_end ||
		wire.visible_end > wire.row_count || wire.row_count >= wire.capacity ||
		uint32(wire.capacity) != capacity ||
		(wire.row_count == 0) != (wire.last_row_key == 0) {
		return DataTableLoadMore{}, false, errors.New("malformed DataTable load-more record")
	}
	return DataTableLoadMore{
		FrameSequence: uint64(wire.frame), RowCount: uint32(wire.row_count),
		VisibleBegin: uint32(wire.visible_begin), VisibleEnd: uint32(wire.visible_end),
		LastRowKey: uint32(wire.last_row_key), Capacity: uint32(wire.capacity),
	}, true, nil
}

type TableSelectionMode uint32

const (
	SelectRows TableSelectionMode = iota
	SelectColumns
	SelectCells
)

func (n *Node) SelectionMode(mode TableSelectionMode) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 71 || mode > SelectCells || n.removed {
		n.err = errors.New("invalid data table selection mode")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if e := result(C.go_data_table_set_selection_mode(n.app.ptr, n.handle, C.uint32_t(mode))); e != nil {
			n.err = e
			return n
		}
	}
	n.tableSelectionMode = uint32(mode)
	n.hasTableSelectionMode = true
	return n
}

func (n *Node) TableData(headers []string, rows [][]string) *Node {
	if n.err == nil {
		n.err = n.SetTable(headers, rows)
	}
	return n
}

// TableDataKeyed copies the matrix and stable row keys, retaining position on replacement.
func (n *Node) TableDataKeyed(headers []string, rows [][]string, rowKeys []uint32) *Node {
	if n.err == nil {
		n.err = n.SetTableKeyed(headers, rows, rowKeys)
	}
	return n
}

// TableDataKeyedCapacity copies a keyed snapshot with an explicit row capacity.
// The loaded bridge must support the capacity API to mount the table.
func (n *Node) TableDataKeyedCapacity(headers []string, rows [][]string, rowKeys []uint32, capacity uint32) *Node {
	if n.err == nil {
		n.err = n.SetTableKeyedCapacity(headers, rows, rowKeys, capacity)
	}
	return n
}

// SetTable copies the matrix, assigns one-based positional row keys, and resets table state.
func (n *Node) SetTable(headers []string, rows [][]string) error {
	return n.setTable(headers, rows, nil, 0)
}

// SetTableKeyed copies the matrix and keys. Keys are distinct, nonzero, and
// stable across replacements. The viewport follows the first visible row,
// and selection and navigation follow surviving keys. A failed update leaves
// the previous matrix and state intact.
func (n *Node) SetTableKeyed(headers []string, rows [][]string, rowKeys []uint32) error {
	if rowKeys == nil {
		rowKeys = []uint32{}
	}
	return n.setTable(headers, rows, rowKeys, 0)
}

// SetTableKeyedCapacity replaces the copied keyed matrix and its row capacity
// together, preserving the viewport and selection for surviving row keys.
// Supply one key per row and a capacity from 1 through DataTableMaxRowCapacity.
// A nil key slice is valid for an empty snapshot. A failed update leaves the
// previous matrix, capacity and state intact.
func (n *Node) SetTableKeyedCapacity(headers []string, rows [][]string, rowKeys []uint32, capacity uint32) error {
	if capacity == 0 || capacity > dataTableMaxRows {
		return fmt.Errorf("DataTable capacity must be in 1..%d", dataTableMaxRows)
	}
	if rowKeys == nil {
		rowKeys = []uint32{}
	}
	return n.setTable(headers, rows, rowKeys, capacity)
}

func (n *Node) setTable(headers []string, rows [][]string, rowKeys []uint32, capacity uint32) error {
	if n == nil || n.kind != 71 {
		return errors.New("table data requires DataTable")
	}
	if n.removed {
		return errors.New("removed data table")
	}
	limit := uint32(dataTableLegacyRows)
	if capacity != 0 {
		limit = capacity
		if n.app != nil {
			if err := n.app.requireDataTableCapacity(capacity); err != nil {
				return err
			}
		}
	}
	if len(headers) == 0 || len(headers) > 16 || len(rows) > int(limit) {
		return errors.New("invalid data table dimensions")
	}
	if rowKeys != nil {
		if len(rowKeys) != len(rows) {
			return errors.New("DataTable requires one row key per row")
		}
		seen := make(map[uint32]bool, len(rowKeys))
		for _, key := range rowKeys {
			if key == 0 || seen[key] {
				return errors.New("DataTable row keys must be distinct and nonzero")
			}
			seen[key] = true
		}
	}
	for _, s := range headers {
		if e := valid(s); e != nil {
			return e
		}
		if len(s) > dataTableTextBytes {
			return errors.New("data table text exceeds 128 bytes")
		}
	}
	for _, row := range rows {
		if len(row) != len(headers) {
			return errors.New("data table row width mismatch")
		}
		for _, s := range row {
			if e := valid(s); e != nil {
				return e
			}
			if len(s) > dataTableTextBytes {
				return errors.New("data table text exceeds 128 bytes")
			}
		}
	}
	if n.app != nil {
		if capacity == 0 {
			if err := n.app.check(); err != nil {
				return err
			}
			if rowKeys != nil && C.go_data_table_keyed_supported() == 0 {
				return ErrKeyedDataTableUnavailable
			}
		}
		if err := n.applyTable(headers, rows, rowKeys, capacity); err != nil {
			return err
		}
	}
	n.tableHeaders = copyTableRow(headers, capacity != 0)
	n.tableRows = make([][]string, len(rows))
	for i, row := range rows {
		n.tableRows[i] = copyTableRow(row, capacity != 0)
	}
	n.tableRowKeys = nil
	if rowKeys != nil {
		n.tableRowKeys = append([]uint32{}, rowKeys...)
	}
	n.tableRowCapacity = capacity
	n.hasTableData = true
	return nil
}

func copyTableRow(row []string, cloneText bool) []string {
	copied := append([]string(nil), row...)
	if cloneText {
		// Short substrings can otherwise retain arbitrarily large backing strings.
		for i, s := range copied {
			copied[i] = strings.Clone(s)
		}
	}
	return copied
}

// applyTable receives an already validated snapshot. C copies the contents
// synchronously. Every pointer in the Go vector addresses temporary C memory.
func (n *Node) applyTable(headers []string, rows [][]string, rowKeys []uint32, capacity uint32) error {
	columns := len(headers)
	count := (len(rows) + 1) * columns
	const stride = dataTableTextBytes + 1
	memory := C.malloc(C.size_t(count * stride))
	defer C.free(memory)
	text := unsafe.Slice((*byte)(memory), count*stride)
	pointers := make([]*C.char, count)
	put := func(i int, value string) {
		offset := i * stride
		copy(text[offset:offset+len(value)], value)
		text[offset+len(value)] = 0
		pointers[i] = (*C.char)(unsafe.Pointer(&text[offset]))
	}
	for i, value := range headers {
		put(i, value)
	}
	for r, row := range rows {
		for c, value := range row {
			put((r+1)*columns+c, value)
		}
	}
	var cells **C.char
	if len(rows) != 0 {
		cells = &pointers[columns]
	}
	if rowKeys == nil {
		return result(C.go_data_table_set(n.app.ptr, n.handle, &pointers[0], C.uint32_t(columns),
			cells, C.uint32_t(len(rows))))
	}
	var keys *C.uint32_t
	if len(rowKeys) != 0 {
		keys = (*C.uint32_t)(unsafe.Pointer(&rowKeys[0]))
	}
	if capacity != 0 {
		return result(C.go_data_table_set_keyed_capacity(n.app.ptr, n.handle, &pointers[0], C.uint32_t(columns),
			cells, keys, C.uint32_t(len(rows)), C.uint32_t(capacity)))
	}
	return result(C.go_data_table_set_keyed(n.app.ptr, n.handle, &pointers[0], C.uint32_t(columns),
		cells, keys, C.uint32_t(len(rows))))
}
func (a *App) DataTableCell(n *Node, row, col uint32) (string, error) {
	if e := a.check(); e != nil {
		return "", e
	}
	if n == nil || n.app != a || n.removed || n.kind != 71 {
		return "", errors.New("invalid DataTable")
	}
	p := (*C.char)(C.malloc(129))
	if p == nil {
		return "", errors.New("out of memory")
	}
	defer C.free(unsafe.Pointer(p))
	var required C.uint32_t
	if e := result(C.go_data_table_get_cell(a.ptr, n.handle, C.uint32_t(row), C.uint32_t(col), p, 129, &required)); e != nil {
		return "", e
	}
	return C.GoStringN(p, C.int(required-1)), nil
}
func (a *App) DataTableState(n *Node) (DataTableState, error) {
	if e := a.check(); e != nil {
		return DataTableState{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 71 {
		return DataTableState{}, errors.New("invalid DataTable")
	}
	var row, col, sortCol, sortDir C.uint32_t
	if e := result(C.go_data_table_get_state(a.ptr, n.handle, &row, &col, &sortCol, &sortDir)); e != nil {
		return DataTableState{}, e
	}
	return DataTableState{uint32(row), uint32(col), uint32(sortCol), uint32(sortDir)}, nil
}
func (a *App) DataTableCellBounds(n *Node, row, col uint32) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 71 {
		return Rect{}, errors.New("invalid DataTable")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_data_table_cell_bounds(a.ptr, n.handle, C.uint32_t(row), C.uint32_t(col), &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (a *App) DataTableHeaderBounds(n *Node, col uint32) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 71 {
		return Rect{}, errors.New("invalid DataTable")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_data_table_header_bounds(a.ptr, n.handle, C.uint32_t(col), &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (n *Node) ChartKind(v ChartKind) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 69 || v > PieChart {
		n.err = errors.New("invalid chart kind")
		return n
	}
	if v == PieChart {
		for _, d := range n.chartData {
			if d.Value < 0 {
				n.err = errors.New("pie values must be nonnegative")
				return n
			}
		}
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if e := result(C.go_chart_set_kind(n.app.ptr, n.handle, C.uint32_t(v))); e != nil {
			n.err = e
			return n
		}
	}
	n.chartKind, n.hasChartKind = v, true
	return n
}
func (n *Node) Data(v []ChartDatum) *Node {
	if n.err == nil {
		n.err = n.SetData(v)
	}
	return n
}
func (n *Node) SetData(v []ChartDatum) error {
	if n == nil || n.kind != 69 {
		return errors.New("data requires Chart")
	}
	if n.removed {
		return errors.New("removed chart")
	}
	if len(v) > 64 {
		return errors.New("too many chart data")
	}
	for _, d := range v {
		if e := valid(d.Label); e != nil {
			return e
		}
		if len(d.Label) > 128 || math.IsNaN(d.Value) || math.IsInf(d.Value, 0) || d.Value < -1e9 || d.Value > 1e9 || (n.chartKind == PieChart && d.Value < 0) {
			return errors.New("invalid chart datum")
		}
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		data := make([]C.cui_bridge_chart_datum, len(v))
		for i, d := range v {
			data[i].label_utf8 = cs(d.Label)
			data[i].value = C.double(d.Value)
		}
		defer func() {
			for _, d := range data {
				C.free(unsafe.Pointer(d.label_utf8))
			}
		}()
		var ptr *C.cui_bridge_chart_datum
		if len(data) > 0 {
			ptr = &data[0]
		}
		if e := result(C.go_chart_set_data(n.app.ptr, n.handle, ptr, C.uint32_t(len(data)))); e != nil {
			return e
		}
		runtime.KeepAlive(data)
	}
	n.chartData = append([]ChartDatum(nil), v...)
	n.hasChartData = true
	return nil
}
func (a *App) ChartDatum(n *Node, index uint32) (ChartDatum, error) {
	if e := a.check(); e != nil {
		return ChartDatum{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 69 {
		return ChartDatum{}, errors.New("invalid chart")
	}
	p := (*C.char)(C.malloc(129))
	if p == nil {
		return ChartDatum{}, errors.New("out of memory")
	}
	defer C.free(unsafe.Pointer(p))
	var required C.uint32_t
	var value C.double
	if e := result(C.go_chart_get_datum(a.ptr, n.handle, C.uint32_t(index), p, 129, &required, &value)); e != nil {
		return ChartDatum{}, e
	}
	return ChartDatum{Label: C.GoStringN(p, C.int(required-1)), Value: float64(value)}, nil
}
func (n *Node) Format(v TextViewFormat) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 67 || v > Plain {
		n.err = errors.New("invalid TextView format")
		return n
	}
	n.textViewFormat, n.hasTextViewFormat = uint32(v), true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_text_view_format(n.app.ptr, n.handle, C.uint32_t(v)))
		}
	}
	return n
}
func (a *App) SelectText(n *Node, anchor, cursor uint32) error {
	if e := a.check(); e != nil {
		return e
	}
	if n == nil || n.app != a || n.removed || n.kind != 67 {
		return errors.New("invalid TextView")
	}
	return result(C.go_text_view_select(a.ptr, n.handle, C.uint32_t(anchor), C.uint32_t(cursor)))
}
func (a *App) CopySelection(n *Node) (string, error) {
	if e := a.check(); e != nil {
		return "", e
	}
	if n == nil || n.app != a || n.removed || n.kind != 67 {
		return "", errors.New("invalid TextView")
	}
	p := (*C.char)(C.malloc(4097))
	if p == nil {
		return "", errors.New("out of memory")
	}
	defer C.free(unsafe.Pointer(p))
	var required C.uint32_t
	if e := result(C.go_text_view_copy_selection(a.ptr, n.handle, p, 4097, &required)); e != nil {
		return "", e
	}
	return C.GoStringN(p, C.int(required-1)), nil
}
func (n *Node) PanelSize(v float64) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 20 || math.IsNaN(v) || math.IsInf(v, 0) || v < 100 || v > 4096 {
		n.err = errors.New("invalid resizable panel size")
		return n
	}
	n.value, n.hasValue = v, true
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
		} else {
			n.err = result(C.go_set_value(n.app.ptr, n.handle, C.double(v)))
		}
	}
	return n
}
func (a *App) ResizableHandleBounds(group, before *Node) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if group == nil || before == nil || group.app != a || before.app != a || group.removed || before.removed || group.kind != 66 || before.kind != 20 {
		return Rect{}, errors.New("invalid resizable handle")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_resizable_handle_bounds(a.ptr, group.handle, before.handle, &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (n *Node) SetOffset(v float64) error {
	if n == nil || n.kind != 64 {
		return errors.New("offset requires MessageScroller")
	}
	if n.removed {
		return errors.New("removed message scroller")
	}
	if math.IsNaN(v) || math.IsInf(v, 0) || v < 0 {
		return errors.New("invalid scroll offset")
	}
	if n.app == nil {
		return errors.New("mount and step MessageScroller before setting offset")
	}
	if e := n.app.check(); e != nil {
		return e
	}
	return result(C.go_set_value(n.app.ptr, n.handle, C.double(v)))
}
func (n *Node) Date(v Date) *Node {
	if n.err != nil {
		return n
	}
	n.err = n.SetDate(v)
	return n
}
func validDate(v Date) bool {
	if v.Year < 1 || v.Year > 9999 || v.Month < 1 || v.Month > 12 || v.Day < 1 {
		return false
	}
	days := [...]int32{0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	limit := days[v.Month]
	if v.Month == 2 && (v.Year%400 == 0 || (v.Year%4 == 0 && v.Year%100 != 0)) {
		limit++
	}
	return v.Day <= limit
}
func cdate(v Date) C.cui_bridge_date {
	return C.cui_bridge_date{year: C.int32_t(v.Year), month: C.int32_t(v.Month), day: C.int32_t(v.Day)}
}
func (n *Node) SetDate(v Date) error {
	if n == nil || (n.kind != 57 && n.kind != 58) {
		return errors.New("date requires Calendar or DatePicker")
	}
	if n.removed {
		return errors.New("removed date control")
	}
	if !validDate(v) {
		return errors.New("invalid date")
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		if e := result(C.go_set_date(n.app.ptr, n.handle, cdate(v))); e != nil {
			return e
		}
	}
	n.dateValue, n.hasDate = v, true
	return nil
}
func (a *App) Date(n *Node) (Date, error) {
	if e := a.check(); e != nil {
		return Date{}, e
	}
	if n == nil || n.app != a || n.removed || (n.kind != 57 && n.kind != 58) {
		return Date{}, errors.New("invalid date control")
	}
	var v C.cui_bridge_date
	if e := result(C.go_get_date(a.ptr, n.handle, &v)); e != nil {
		return Date{}, e
	}
	return Date{int32(v.year), int32(v.month), int32(v.day)}, nil
}
func (a *App) CalendarDayBounds(n *Node, day Date) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 57 || !validDate(day) {
		return Rect{}, errors.New("invalid calendar day")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_calendar_day_bounds(a.ptr, n.handle, cdate(day), &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (n *Node) ColorRGBA(v uint32) *Node {
	if n.err != nil {
		return n
	}
	n.err = n.SetColorRGBA(v)
	return n
}
func (n *Node) SetColorRGBA(v uint32) error {
	if n == nil || n.kind != 56 {
		return errors.New("color requires ColorPicker")
	}
	if n.removed {
		return errors.New("removed color picker")
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		if e := result(C.go_set_color_rgba(n.app.ptr, n.handle, C.uint32_t(v))); e != nil {
			return e
		}
	}
	n.colorRGBA, n.hasColor = v, true
	return nil
}
func (a *App) ColorRGBA(n *Node) (uint32, error) {
	if e := a.check(); e != nil {
		return 0, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 56 {
		return 0, errors.New("invalid color picker")
	}
	var v C.uint32_t
	if e := result(C.go_get_color_rgba(a.ptr, n.handle, &v)); e != nil {
		return 0, e
	}
	return uint32(v), nil
}
func (a *App) ColorPartBounds(n *Node, part uint32) (Rect, error) {
	if e := a.check(); e != nil {
		return Rect{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 56 || part < 1 || part > 3 {
		return Rect{}, errors.New("invalid color part")
	}
	var r C.cui_bridge_rect
	if e := result(C.go_color_part_bounds(a.ptr, n.handle, C.uint32_t(part), &r)); e != nil {
		return Rect{}, e
	}
	return Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}, nil
}
func (n *Node) Time(v Time) *Node {
	if n.err != nil {
		return n
	}
	n.err = n.SetTime(v)
	return n
}
func (n *Node) SetTime(v Time) error {
	if n == nil || n.kind != 55 {
		return errors.New("time requires TimeField")
	}
	if n.removed {
		return errors.New("removed time field")
	}
	if v.Hour > 23 || v.Minute > 59 || v.Second > 59 {
		return errors.New("invalid 24-hour time")
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		if e := result(C.go_set_time(n.app.ptr, n.handle, C.cui_bridge_time{hour: C.uint32_t(v.Hour), minute: C.uint32_t(v.Minute), second: C.uint32_t(v.Second)})); e != nil {
			return e
		}
	}
	n.timeValue, n.hasTime = v, true
	return nil
}
func (a *App) Time(n *Node) (Time, error) {
	if e := a.check(); e != nil {
		return Time{}, e
	}
	if n == nil || n.app != a || n.removed || n.kind != 55 {
		return Time{}, errors.New("invalid time field")
	}
	var v C.cui_bridge_time
	if e := result(C.go_get_time(a.ptr, n.handle, &v)); e != nil {
		return Time{}, e
	}
	return Time{uint32(v.hour), uint32(v.minute), uint32(v.second)}, nil
}
func validRGBALength(length int, width, height, stride uint32) bool {
	count := uint64(stride) * uint64(height)
	return width > 0 && height > 0 && width <= 2048 && height <= 2048 &&
		stride >= width*4 && count <= 16*1024*1024 && uint64(length) == count
}

func (n *Node) RGBA(pixels []byte, width, height, stride uint32) *Node {
	if n.err != nil {
		return n
	}
	n.err = n.SetRGBA(pixels, width, height, stride)
	return n
}
func (n *Node) SetRGBA(pixels []byte, width, height, stride uint32) error {
	if n == nil || (n.kind != 53 && n.kind != 20) {
		return errors.New("RGBA requires Image or Stepper Item")
	}
	if n.removed {
		return errors.New("removed image")
	}
	if !validRGBALength(len(pixels), width, height, stride) {
		return errors.New("invalid RGBA dimensions or data length")
	}
	copyPixels := append([]byte(nil), pixels...)
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		e := result(C.go_set_image_rgba(n.app.ptr, n.handle, (*C.uint8_t)(unsafe.Pointer(&copyPixels[0])), C.uint32_t(width), C.uint32_t(height), C.uint32_t(stride), C.uint32_t(len(copyPixels))))
		runtime.KeepAlive(copyPixels)
		if e != nil {
			return e
		}
	}
	n.imagePixels = copyPixels
	n.imageWidth, n.imageHeight, n.imageStride = width, height, stride
	return nil
}

func shadowResult(status C.cui_bridge_status) error {
	if status == C.CUI_BRIDGE_BACKEND_UNAVAILABLE {
		return errors.New("shadow V1 unavailable; matching newer CUI bridge required")
	}
	return result(status)
}

// ShadowKind selects the root's DROP or INSET paint phase.
type ShadowKind uint32

const (
	ShadowDrop ShadowKind = iota
	ShadowInset
)

type ShadowPreset uint32

const (
	ShadowNone ShadowPreset = iota
	Shadow2XS
	ShadowXS
	ShadowSM
	ShadowMD
	ShadowLG
	ShadowXL
	Shadow2XL
)

type CornerRadii struct{ TopLeft, TopRight, BottomLeft, BottomRight float64 }
type BoxShadow struct {
	OffsetX, OffsetY, Sigma, Spread float64
	Kind                            ShadowKind
	RGBA                            uint32
}

// ShadowStyle is copied when authored. Empty layers suppress shadows; ResetBoxShadows restores defaults.
type ShadowStyle struct {
	Layers []BoxShadow
	Radii  CornerRadii
}

func shadowNumber(v, min, max float64) bool {
	return !math.IsNaN(v) && !math.IsInf(v, 0) && v >= min && v <= max && !math.IsInf(float64(float32(v)), 0)
}
func shadowRadii(r CornerRadii) bool {
	for _, v := range []float64{r.TopLeft, r.TopRight, r.BottomLeft, r.BottomRight} {
		if !shadowNumber(v, 0, 16384) {
			return false
		}
	}
	return true
}
func shadowKind(kind int) bool {
	return kind == 1 || kind == 2 || kind == 3 || kind == 4 || kind == 11 || kind == 12
}
func shadowWire(s ShadowStyle) (C.cui_bridge_box_shadows_v1, error) {
	v := C.cui_bridge_box_shadows_v1{struct_size: 224, schema_version: 1, count: C.uint32_t(len(s.Layers)), top_left: C.float(s.Radii.TopLeft), top_right: C.float(s.Radii.TopRight), bottom_left: C.float(s.Radii.BottomLeft), bottom_right: C.float(s.Radii.BottomRight)}
	if len(s.Layers) > 8 || !shadowRadii(s.Radii) {
		return v, errors.New("invalid shadow layer count/radii")
	}
	for i, l := range s.Layers {
		if l.Kind > ShadowInset || !shadowNumber(l.OffsetX, -4096, 4096) || !shadowNumber(l.OffsetY, -4096, 4096) || !shadowNumber(l.Sigma, 0, 256) || !shadowNumber(l.Spread, -4096, 4096) {
			return v, errors.New("invalid shadow layer")
		}
		v.layers[i] = C.cui_bridge_box_shadow_v1{offset_x: C.float(l.OffsetX), offset_y: C.float(l.OffsetY), sigma: C.float(l.Sigma), spread: C.float(l.Spread), kind: C.uint32_t(l.Kind), rgba: C.uint32_t(l.RGBA)}
	}
	return v, nil
}

// BoxShadows styles only row/column/label/button/input/badge roots, not parts or portals.
func (n *Node) BoxShadows(style ShadowStyle) *Node {
	if n.err != nil {
		return n
	}
	if !shadowKind(n.kind) {
		n.err = errors.New("unsupported shadow root")
		return n
	}
	wire, e := shadowWire(style)
	if e != nil {
		n.err = e
		return n
	}
	if n.app != nil {
		if e = n.app.check(); e == nil {
			e = shadowResult(C.go_shadow_set(n.app.ptr, n.handle, &wire))
		}
		if e != nil {
			n.err = e
			return n
		}
	}
	copied := style
	copied.Layers = append([]BoxShadow(nil), style.Layers...)
	delete(n.partShadows, ShadowRoot)
	n.shadowStyle = &copied
	return n
}
func (n *Node) ResetBoxShadows() *Node {
	if n.err != nil {
		return n
	}
	if !shadowKind(n.kind) {
		n.err = errors.New("unsupported shadow root")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if e := shadowResult(C.go_shadow_reset(n.app.ptr, n.handle)); e != nil {
			n.err = e
			return n
		}
	}
	delete(n.partShadows, ShadowRoot)
	n.shadowStyle = nil
	return n
}
func (a *App) ShadowCSS(x, y, radius, spread float64, rgba uint32, kind ShadowKind) (BoxShadow, error) {
	if e := a.check(); e != nil {
		return BoxShadow{}, e
	}
	if kind > ShadowInset || !shadowNumber(x, -4096, 4096) || !shadowNumber(y, -4096, 4096) || !shadowNumber(radius, 0, 512) || !shadowNumber(spread, -4096, 4096) {
		return BoxShadow{}, errors.New("invalid CSS shadow")
	}
	var v C.cui_bridge_box_shadow_v1
	if e := shadowResult(C.go_shadow_css(C.float(x), C.float(y), C.float(radius), C.float(spread), C.uint32_t(rgba), C.uint32_t(kind), &v)); e != nil {
		return BoxShadow{}, e
	}
	return BoxShadow{float64(v.offset_x), float64(v.offset_y), float64(v.sigma), float64(v.spread), ShadowKind(v.kind), uint32(v.rgba)}, nil
}
func (a *App) ShadowPreset(preset ShadowPreset, radii CornerRadii) (ShadowStyle, error) {
	if e := a.check(); e != nil {
		return ShadowStyle{}, e
	}
	if preset > Shadow2XL || !shadowRadii(radii) {
		return ShadowStyle{}, errors.New("invalid shadow preset/radii")
	}
	var v C.cui_bridge_box_shadows_v1
	if e := shadowResult(C.go_shadow_preset(C.uint32_t(preset), C.float(radii.TopLeft), C.float(radii.TopRight), C.float(radii.BottomLeft), C.float(radii.BottomRight), &v)); e != nil {
		return ShadowStyle{}, e
	}
	s := ShadowStyle{Radii: radii}
	for i := 0; i < int(v.count); i++ {
		l := v.layers[i]
		s.Layers = append(s.Layers, BoxShadow{float64(l.offset_x), float64(l.offset_y), float64(l.sigma), float64(l.spread), ShadowKind(l.kind), uint32(l.rgba)})
	}
	return s, nil
}

func (n *Node) setPartNative(part ShadowPart, style ShadowStyle) error {
	wire, e := shadowWire(style)
	if e != nil {
		return e
	}
	if n.app == nil {
		return nil
	}
	if e = n.app.check(); e != nil {
		return e
	}
	return shadowResult(C.go_part_set(n.app.ptr, n.handle, C.uint32_t(part), &wire))
}
func (n *Node) resetPartNative(part ShadowPart) error {
	if n.app == nil {
		return nil
	}
	if e := n.app.check(); e != nil {
		return e
	}
	return shadowResult(C.go_part_reset(n.app.ptr, n.handle, C.uint32_t(part)))
}
func (n *Node) ShadowTarget(part ShadowPart) (ShadowTargetInfo, error) {
	if n.app == nil {
		return ShadowTargetInfo{}, errors.New("capability query requires a mounted node")
	}
	if e := n.app.check(); e != nil {
		return ShadowTargetInfo{}, e
	}
	var info C.cui_bridge_shadow_target_info_v1
	if e := shadowResult(C.go_part_query(n.app.ptr, n.handle, C.uint32_t(part), &info)); e != nil {
		return ShadowTargetInfo{}, e
	}
	return ShadowTargetInfo{ShadowPart(info.canonical_role), uint32(info.flags)}, nil
}

func sliderWireValue(v SliderValue) C.cui_bridge_slider_value_v1 {
	return C.cui_bridge_slider_value_v1{kind: C.uint32_t(v.kind), start: C.double(v.start), end: C.double(v.end)}
}
func sliderWireConfig(c SliderConfig) C.cui_bridge_slider_config_v1 {
	flags := btoi(c.Requests) | btoi(c.Readonly)<<1 | btoi(c.ReverseFill)<<2 | btoi(c.ReducedMotion)<<3 | btoi(c.HideFocus)<<4
	return C.cui_bridge_slider_config_v1{struct_size: C.sizeof_cui_bridge_slider_config_v1, schema_version: 1, flags: C.uint32_t(flags), orientation: C.uint32_t(c.Orientation), source_generation: C.uint64_t(c.SourceGeneration), domain: C.cui_bridge_slider_domain_v1{minimum: C.double(c.Domain.Minimum), maximum: C.double(c.Domain.Maximum), step: C.double(c.Domain.Step), scale: C.uint32_t(c.Domain.Scale)}, rem: C.float(c.Rem), length: C.float(c.Length), layout: C.uint32_t(c.Layout)}
}
func sliderReadValue(v C.cui_bridge_slider_value_v1) (SliderValue, error) {
	if v.reserved != 0 {
		return SliderValue{}, errors.New("invalid Slider reserved value")
	}
	value := SliderValue{uint32(v.kind), float64(v.start), float64(v.end)}
	return value, value.validate()
}
func sliderReadDomain(d C.cui_bridge_slider_domain_v1) (SliderDomain, error) {
	if d.reserved != 0 {
		return SliderDomain{}, errors.New("invalid Slider reserved domain")
	}
	domain := SliderDomain{float64(d.minimum), float64(d.maximum), float64(d.step), SliderScale(d.scale)}
	return domain, domain.validate()
}
func (a *App) SupportsSliderModel() (bool, error) {
	if e := a.check(); e != nil {
		return false, e
	}
	return C.go_slider_supported() != 0, nil
}
func (a *App) EnableSliderRequests(capacity uint32) error {
	if e := a.check(); e != nil {
		return e
	}
	if capacity > 4096 {
		return errors.New("Slider capacity exceeds4096")
	}
	if e := result(C.go_slider_enable(a.ptr, C.uint32_t(capacity))); e != nil {
		return e
	}
	if a.sliderNamespace == nil {
		a.sliderNamespace = &sliderNamespace{nonzero: 1}
	}
	return nil
}
func (a *App) AcceptSliderRequest(receipt SliderReceipt, value SliderValue) error {
	if e := a.check(); e != nil {
		return e
	}
	if a.sliderNamespace == nil || receipt.namespace != a.sliderNamespace {
		return errors.New("foreign or unissued Slider receipt")
	}
	if e := value.validate(); e != nil {
		return e
	}
	i := receipt.info
	r := C.cui_bridge_slider_receipt_v1{struct_size: C.sizeof_cui_bridge_slider_receipt_v1, schema_version: 1, app_epoch: C.uint64_t(i.AppEpoch), node: C.cui_bridge_node{slot: C.uint32_t(i.Slot), generation: C.uint32_t(i.Generation)}, source_generation: C.uint64_t(i.SourceGeneration), configuration_revision: C.uint64_t(i.ConfigurationRevision), submission_frame: C.uint64_t(i.SubmissionFrame), request_sequence: C.uint64_t(i.RequestSequence)}
	v := sliderWireValue(value)
	return result(C.go_slider_accept(a.ptr, &r, &v))
}
func (a *App) PollSliderRequest() (SliderRequest, bool, error) {
	if e := a.check(); e != nil {
		return SliderRequest{}, false, e
	}
	if a.sliderNamespace == nil {
		return SliderRequest{}, false, errors.New("Slider queue not enabled")
	}
	var raw C.cui_bridge_slider_request_v1
	code := C.go_slider_poll(a.ptr, &raw)
	if code == C.CUI_BRIDGE_EMPTY {
		return SliderRequest{}, false, nil
	}
	if e := result(code); e != nil {
		return SliderRequest{}, false, e
	}
	r, e := a.sliderReadRequest(raw)
	return r, e == nil, e
}
func (a *App) sliderReadRequest(w C.cui_bridge_slider_request_v1) (SliderRequest, error) {
	bad := func() (SliderRequest, error) { return SliderRequest{}, errors.New("invalid Slider request copy") }
	if a.sliderNamespace == nil || w.struct_size != C.sizeof_cui_bridge_slider_request_v1 || w.schema_version != 1 || w.phase > 3 || w.reason > 9 || w.thumb > 1 || (w.event_kind != 1 && w.event_kind != 3 && w.event_kind != 5) || w.event_source > 4 || w.event_flags&^7 != 0 {
		return bad()
	}
	r := w.receipt
	if r.struct_size != C.sizeof_cui_bridge_slider_receipt_v1 || r.schema_version != 1 {
		return bad()
	}
	if w.event_flags&4 == 0 && w.event_source != 0 {
		return bad()
	}
	if w.event_flags&1 == 0 && w.event_frame != 0 || w.event_flags&2 == 0 && uint64(w.event_index) != ^uint64(0) {
		return bad()
	}
	current, e := sliderReadValue(w.current)
	if e != nil {
		return bad()
	}
	requested, e := sliderReadValue(w.requested)
	if e != nil {
		return bad()
	}
	domain, e := sliderReadDomain(w.domain)
	if e != nil {
		return bad()
	}
	gesture, e := sliderReadDomain(w.gesture_domain)
	if e != nil {
		return bad()
	}
	i := w.input
	if i.flags&^3 != 0 || i.modifiers&^15 != 0 {
		return bad()
	}
	if i.flags&1 == 0 && (i._type != 0 || i.flags != 0 || i.key != 0 || i.modifiers != 0 || i.x != 0 || i.y != 0 || i.repeat != 0 || i.pointer_type != 0 || i.pointer_button != 0 || i.accessibility_action != 0 || i.pointer_id != 0 || i.pointer_device != 0 || i.accessibility_target != 0 || i.accessibility_value != 0) {
		return bad()
	}
	if i.flags&1 != 0 {
		if !(i._type == 0 || i._type == 1 || i._type == 2 || i._type == 3 || i._type == 7 || i._type == 13) || !(i.key == 0 || i.key == 4 || i.key == 5 || i.key == 6 || i.key == 7 || i.key == 8 || i.key == 9 || i.key == 10 || i.key == 19 || i.key == 20) || i.repeat > 1 || i.pointer_type > 2 || i.pointer_button > 2 || !(i.accessibility_action == 0 || i.accessibility_action == 2 || i.accessibility_action == 3 || i.accessibility_action == 4) {
			return bad()
		}
	}
	if !finite(float32(i.x)) || !finite(float32(i.y)) || !sliderFinite(float64(i.accessibility_value)) || i.flags&2 != 0 && (i.flags&1 == 0 || i._type != 13 || i.accessibility_action != 4) {
		return bad()
	}
	if i.flags&2 == 0 && i.accessibility_value != 0 {
		return bad()
	}
	out := SliderRequest{Phase: SliderPhase(w.phase), Reason: uint32(w.reason), Thumb: uint32(w.thumb), EventKind: uint32(w.event_kind), EventSource: uint32(w.event_source), EventFlags: uint32(w.event_flags), Receipt: SliderReceipt{SliderReceiptInfo{uint64(r.app_epoch), uint32(r.node.slot), uint32(r.node.generation), uint64(r.source_generation), uint64(r.configuration_revision), uint64(r.submission_frame), uint64(r.request_sequence)}, a.sliderNamespace}, EventTarget: uint64(w.event_target), EventFrame: uint64(w.event_frame), EventIndex: uint64(w.event_index), Current: current, Requested: requested, Domain: domain, GestureDomain: gesture, Input: SliderInput{uint32(i._type), uint32(i.flags), uint32(i.key), uint32(i.modifiers), float32(i.x), float32(i.y), uint32(i.repeat), uint32(i.pointer_type), uint32(i.pointer_button), uint32(i.accessibility_action), uint64(i.pointer_id), uint64(i.pointer_device), uint64(i.accessibility_target), float64(i.accessibility_value)}}
	return out, nil
}
func (a *App) sliderConfigureNative(n *Node, c SliderConfig, value SliderValue) error {
	config, v := sliderWireConfig(c), sliderWireValue(value)
	return result(C.go_slider_configure(a.ptr, n.handle, &config, &v))
}
func (a *App) sliderSetNative(n *Node, value SliderValue) error {
	v := sliderWireValue(value)
	return result(C.go_slider_set(a.ptr, n.handle, &v))
}
func (a *App) sliderGetNative(n *Node) (SliderValue, error) {
	var v C.cui_bridge_slider_value_v1
	if e := result(C.go_slider_get(a.ptr, n.handle, &v)); e != nil {
		return SliderValue{}, e
	}
	return sliderReadValue(v)
}
func (a *App) sliderConfigNative(n *Node) (SliderConfig, error) {
	var w C.cui_bridge_slider_config_v1
	if e := result(C.go_slider_config(a.ptr, n.handle, &w)); e != nil {
		return SliderConfig{}, e
	}
	if w.struct_size != C.sizeof_cui_bridge_slider_config_v1 || w.schema_version != 1 || w.reserved != 0 || w.flags&^31 != 0 || (w.layout == 1 && w.length != 0) {
		return SliderConfig{}, errors.New("invalid Slider config copy")
	}
	d, e := sliderReadDomain(w.domain)
	if e != nil {
		return SliderConfig{}, e
	}
	return sliderConfig(SliderConfig{Domain: d, SourceGeneration: uint64(w.source_generation), Orientation: SliderOrientation(w.orientation), Layout: SliderLayout(w.layout), Rem: float32(w.rem), Length: float32(w.length), Requests: w.flags&1 != 0, Readonly: w.flags&2 != 0, ReverseFill: w.flags&4 != 0, ReducedMotion: w.flags&8 != 0, HideFocus: w.flags&16 != 0})
}
func (a *App) sliderBoundsNative(n *Node, part SliderPart) (Rect, bool, error) {
	var r C.cui_bridge_rect
	code := C.go_slider_bounds(a.ptr, n.handle, C.uint32_t(part), &r)
	if code == C.CUI_BRIDGE_EMPTY {
		return Rect{}, false, nil
	}
	if e := result(code); e != nil {
		return Rect{}, false, e
	}
	out := Rect{float32(r.x), float32(r.y), float32(r.width), float32(r.height)}
	if !finite(out.X) || !finite(out.Y) || !finite(out.Width) || !finite(out.Height) || out.Width < 0 || out.Height < 0 {
		return Rect{}, false, errors.New("invalid Slider bounds")
	}
	return out, true, nil
}
func (a *App) sliderPresentationNative(n *Node) (SliderPresentation, bool, error) {
	var p C.cui_bridge_slider_presentation_v1
	code := C.go_slider_presentation(a.ptr, n.handle, &p)
	if code == C.CUI_BRIDGE_EMPTY {
		return SliderPresentation{}, false, nil
	}
	if e := result(code); e != nil {
		return SliderPresentation{}, false, e
	}
	out := SliderPresentation{uint32(p.render_status), float64(p.core_time), float64(p.next_wake_seconds)}
	if p.struct_size != C.sizeof_cui_bridge_slider_presentation_v1 || p.schema_version != 1 || p.reserved != 0 || out.RenderStatus > 5 || !sliderFinite(out.CoreTime) || !sliderFinite(out.NextWakeSeconds) || out.CoreTime < 0 || out.NextWakeSeconds < 0 {
		return SliderPresentation{}, false, errors.New("invalid Slider presentation")
	}
	return out, true, nil
}

func (a *App) timeCall(n *Node, op uint32, capacity uint32, bytes []byte) (uint32, error) {
	if e := a.check(); e != nil {
		return 0, e
	}
	if op >= 2 && op <= 5 && n == nil {
		return 0, errors.New("nil localized TimeField")
	}
	var h C.cui_bridge_node
	if n != nil {
		if n.app != a || n.removed || n.kind != 55 {
			return 0, errors.New("invalid localized TimeField")
		}
		h = n.handle
	}
	var p *C.uchar
	if len(bytes) > 0 {
		p = (*C.uchar)(unsafe.Pointer(&bytes[0]))
	}
	code := C.go_time_call(a.ptr, h, C.uint32_t(op), C.uint32_t(capacity), p)
	runtime.KeepAlive(bytes)
	if code == C.CUI_BRIDGE_EMPTY {
		return uint32(code), nil
	}
	return uint32(code), result(code)
}
func (a *App) SupportsLocalizedTimeField() (bool, error) {
	if e := a.check(); e != nil {
		return false, e
	}
	return C.go_time_supported() != 0, nil
}

// SupportsSkeletonPresentation reports whether the active bridge exposes
// all of responsive Skeleton V1 with compatible record sizes.
func (a *App) SupportsSkeletonPresentation() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_skeleton_supported() != 0, nil
}

func (n *Node) configureMountedSkeleton(c SkeletonConfig) error {
	supported, err := n.app.SupportsSkeletonPresentation()
	if err != nil {
		return err
	}
	if !supported {
		return ErrSkeletonPresentationUnavailable
	}
	var flags C.uint32_t
	if c.Secondary {
		flags |= C.CUI_BRIDGE_SKELETON_SECONDARY
	}
	if c.CustomColor {
		flags |= C.CUI_BRIDGE_SKELETON_CUSTOM_COLOR
	}
	if c.ReducedMotion {
		flags |= C.CUI_BRIDGE_SKELETON_REDUCED_MOTION
	}
	r := C.cui_bridge_skeleton_config_v1{
		struct_size: C.sizeof_cui_bridge_skeleton_config_v1, schema_version: C.CUI_BRIDGE_SKELETON_V1,
		flags: flags, width_mode: C.uint32_t(c.WidthMode), width: C.float(c.Width),
		height: C.float(c.Height), max_width: C.float(c.MaxWidth), radius: C.float(c.Radius),
		duration: C.float(c.Duration), background_rgba: C.uint32_t(c.BackgroundRGBA),
	}
	return result(C.go_skeleton_configure(n.app.ptr, n.handle, &r))
}

// SkeletonConfig returns a copy of the authored responsive configuration.
func (n *Node) SkeletonConfig() (SkeletonConfig, error) {
	if n == nil || n.removed || n.skeletonConfig == nil {
		return SkeletonConfig{}, errors.New("node has no responsive Skeleton configuration")
	}
	if n.app == nil {
		return *n.skeletonConfig, nil
	}
	if err := n.app.check(); err != nil {
		return SkeletonConfig{}, err
	}
	r := C.cui_bridge_skeleton_config_v1{
		struct_size: C.sizeof_cui_bridge_skeleton_config_v1, schema_version: C.CUI_BRIDGE_SKELETON_V1,
	}
	if err := result(C.go_skeleton_get_config(n.app.ptr, n.handle, &r)); err != nil {
		return SkeletonConfig{}, err
	}
	c := SkeletonConfig{
		WidthMode: SkeletonWidthMode(r.width_mode), Width: float32(r.width), Height: float32(r.height),
		MaxWidth: float32(r.max_width), Radius: float32(r.radius), Duration: float32(r.duration),
		BackgroundRGBA: uint32(r.background_rgba), Secondary: r.flags&C.CUI_BRIDGE_SKELETON_SECONDARY != 0,
		CustomColor:   r.flags&C.CUI_BRIDGE_SKELETON_CUSTOM_COLOR != 0,
		ReducedMotion: r.flags&C.CUI_BRIDGE_SKELETON_REDUCED_MOTION != 0,
	}
	const knownFlags = C.CUI_BRIDGE_SKELETON_SECONDARY | C.CUI_BRIDGE_SKELETON_CUSTOM_COLOR | C.CUI_BRIDGE_SKELETON_REDUCED_MOTION
	if r.struct_size != C.sizeof_cui_bridge_skeleton_config_v1 ||
		r.schema_version != C.CUI_BRIDGE_SKELETON_V1 || r.flags&^knownFlags != 0 || c.validate() != nil {
		return SkeletonConfig{}, errors.New("invalid copied Skeleton configuration")
	}
	return c, nil
}

// SkeletonPresentation reads the most recent sample from a mounted responsive
// Skeleton. The bool is false when no current sample is available.
func (n *Node) SkeletonPresentation() (SkeletonPresentation, bool, error) {
	if n == nil || n.app == nil || n.removed || n.skeletonConfig == nil {
		return SkeletonPresentation{}, false, errors.New("presentation requires a mounted responsive Skeleton")
	}
	if err := n.app.check(); err != nil {
		return SkeletonPresentation{}, false, err
	}
	r := C.cui_bridge_skeleton_presentation_v1{
		struct_size: C.sizeof_cui_bridge_skeleton_presentation_v1, schema_version: C.CUI_BRIDGE_SKELETON_V1,
	}
	code := C.go_skeleton_get_presentation(n.app.ptr, n.handle, &r)
	if code == C.CUI_BRIDGE_EMPTY {
		return SkeletonPresentation{}, false, nil
	}
	if err := result(code); err != nil {
		return SkeletonPresentation{}, false, err
	}
	p := SkeletonPresentation{
		FrameSequence: uint64(r.frame_sequence), CoreTime: float64(r.core_time),
		NextWakeSeconds: float64(r.next_wake_seconds), Phase: float32(r.phase), Opacity: float32(r.opacity),
	}
	if r.struct_size != C.sizeof_cui_bridge_skeleton_presentation_v1 ||
		r.schema_version != C.CUI_BRIDGE_SKELETON_V1 || p.FrameSequence == 0 ||
		!(p.CoreTime >= 0) || math.IsInf(p.CoreTime, 0) ||
		(p.NextWakeSeconds != 0 && (!(p.NextWakeSeconds > p.CoreTime) || math.IsInf(p.NextWakeSeconds, 0))) ||
		!(p.Phase >= 0 && p.Phase <= 1 && p.Opacity >= .5 && p.Opacity <= 1) {
		return SkeletonPresentation{}, false, errors.New("invalid copied Skeleton presentation")
	}
	return p, true, nil
}

// SupportsDisclosurePresentation reports whether both Disclosure V1 setters
// are present in the selected provider.
func (a *App) SupportsDisclosurePresentation() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_disclosure_supported() != 0, nil
}

func (n *Node) setDisclosureConfig(config *disclosureConfigWire) error {
	supported, err := n.app.SupportsDisclosurePresentation()
	if err != nil {
		return err
	}
	if !supported {
		return ErrDisclosureOptionsUnavailable
	}
	return result(C.go_disclosure_configure(n.app.ptr, n.handle, config))
}

func (n *Node) setDisclosureStyle(style *disclosureStyleWire) error {
	supported, err := n.app.SupportsDisclosurePresentation()
	if err != nil {
		return err
	}
	if !supported {
		return ErrDisclosureOptionsUnavailable
	}
	return result(C.go_disclosure_style(n.app.ptr, n.handle, style))
}

// SupportsDisclosureHeading probes the optional retained heading setter.
func (a *App) SupportsDisclosureHeading() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_disclosure_heading_supported() != 0, nil
}

// SupportsDisclosureTitleState probes the retained title-state setter.
func (a *App) SupportsDisclosureTitleState() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_disclosure_title_supported() != 0, nil
}

// SupportsDisclosureDivider probes the optional joined divider setter.
func (a *App) SupportsDisclosureDivider() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_disclosure_divider_supported() != 0, nil
}

func (n *Node) setDisclosureHeading(layer DisclosureHeadingLayer, style *disclosureHeadingWire) error {
	supported, err := n.app.SupportsDisclosureHeading()
	if err != nil {
		return err
	}
	if !supported {
		return ErrDisclosureOptionsUnavailable
	}
	return result(C.go_disclosure_heading(n.app.ptr, n.handle, C.uint32_t(layer), style))
}

func (n *Node) setDisclosureTitleState(state DisclosureTitleState, style *disclosureHeadingWire) error {
	supported, err := n.app.SupportsDisclosureTitleState()
	if err != nil {
		return err
	}
	if !supported {
		return ErrDisclosureOptionsUnavailable
	}
	return result(C.go_disclosure_title(n.app.ptr, n.handle, C.uint32_t(state), style))
}

func (n *Node) setDisclosureDivider(style *disclosureDividerWire) error {
	supported, err := n.app.SupportsDisclosureDivider()
	if err != nil {
		return err
	}
	if !supported {
		return ErrDisclosureOptionsUnavailable
	}
	return result(C.go_disclosure_divider(n.app.ptr, n.handle, style))
}

// SupportsRatingOptions reports whether the selected provider has Rating V1.
func (a *App) SupportsRatingOptions() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_rating_supported() != 0, nil
}

func (n *Node) setRatingWire(wire *ratingWire) error {
	supported, err := n.app.SupportsRatingOptions()
	if err != nil {
		return err
	}
	if !supported {
		return ErrRatingOptionsUnavailable
	}
	return result(C.go_rating_configure(n.app.ptr, n.handle, wire))
}

func (n *Node) readRatingWire(wire *ratingWire) (bool, error) {
	supported, err := n.app.SupportsRatingOptions()
	if err != nil || !supported {
		return false, err
	}
	status := C.go_rating_get_config(n.app.ptr, n.handle, wire)
	if status == C.CUI_BRIDGE_EMPTY {
		return false, nil
	}
	err = result(status)
	return err == nil, err
}

// SupportsPaginationOptions reports whether the selected provider exposes all
// of Pagination V1 with a compatible layout.
func (a *App) SupportsPaginationOptions() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_pagination_supported() != 0, nil
}

func paginationWire() C.cui_bridge_pagination_config_v1 {
	return C.cui_bridge_pagination_config_v1{
		struct_size: C.sizeof_cui_bridge_pagination_config_v1, schema_version: C.CUI_BRIDGE_PAGINATION_V1,
	}
}

func (n *Node) setPaginationConfig(config *PaginationConfig) error {
	supported, err := n.app.SupportsPaginationOptions()
	if err != nil {
		return err
	}
	if !supported {
		return ErrPaginationOptionsUnavailable
	}
	if config == nil {
		return result(C.go_pagination_configure(n.app.ptr, n.handle, nil))
	}
	wire := paginationWire()
	if config.Counter {
		wire.flags |= C.CUI_BRIDGE_PAGINATION_COUNTER
	}
	if config.IconOnly {
		wire.flags |= C.CUI_BRIDGE_PAGINATION_ICON_ONLY
	}
	if config.Chevrons {
		wire.flags |= C.CUI_BRIDGE_PAGINATION_CHEVRONS
	}
	if config.JumpMenu {
		wire.flags |= C.CUI_BRIDGE_PAGINATION_JUMP_MENU
	}
	wire.visible_pages = C.uint32_t(config.VisiblePages)
	wire.available_width = C.float(config.AvailableWidth)
	for _, label := range []struct {
		target *C.char
		value  string
	}{
		{&wire.group[0], config.Group}, {&wire.first[0], config.First},
		{&wire.previous[0], config.Previous}, {&wire.next[0], config.Next},
		{&wire.last[0], config.Last}, {&wire.earlier_pages[0], config.EarlierPages},
		{&wire.later_pages[0], config.LaterPages},
	} {
		copy(paginationLabelBytes(label.target), label.value)
	}
	return result(C.go_pagination_configure(n.app.ptr, n.handle, &wire))
}

func paginationLabelBytes(source *C.char) []byte {
	return unsafe.Slice((*byte)(unsafe.Pointer(source)), C.CUI_BRIDGE_PAGINATION_LABEL_BYTES)
}

// PaginationConfig returns an owned copy and a boolean indicating whether an
// override is set. An ordinary pager has no override on an earlier provider.
func (n *Node) PaginationConfig() (PaginationConfig, bool, error) {
	if err := n.checkPagination(); err != nil {
		return PaginationConfig{}, false, err
	}
	if n.app == nil {
		if n.paginationConfig == nil {
			return PaginationConfig{}, false, nil
		}
		return *n.paginationConfig, true, nil
	}
	supported, err := n.app.SupportsPaginationOptions()
	if err != nil || !supported {
		return PaginationConfig{}, false, err
	}
	wire := paginationWire()
	code := C.go_pagination_get_config(n.app.ptr, n.handle, &wire)
	if code == C.CUI_BRIDGE_EMPTY {
		return PaginationConfig{}, false, nil
	}
	if err := result(code); err != nil {
		return PaginationConfig{}, false, err
	}
	const known = C.CUI_BRIDGE_PAGINATION_COUNTER | C.CUI_BRIDGE_PAGINATION_ICON_ONLY |
		C.CUI_BRIDGE_PAGINATION_CHEVRONS | C.CUI_BRIDGE_PAGINATION_JUMP_MENU
	if wire.struct_size != C.sizeof_cui_bridge_pagination_config_v1 ||
		wire.schema_version != C.CUI_BRIDGE_PAGINATION_V1 || wire.flags&^known != 0 {
		return PaginationConfig{}, false, errors.New("invalid copied Pagination options")
	}
	config := PaginationConfig{
		Counter:        wire.flags&C.CUI_BRIDGE_PAGINATION_COUNTER != 0,
		IconOnly:       wire.flags&C.CUI_BRIDGE_PAGINATION_ICON_ONLY != 0,
		Chevrons:       wire.flags&C.CUI_BRIDGE_PAGINATION_CHEVRONS != 0,
		JumpMenu:       wire.flags&C.CUI_BRIDGE_PAGINATION_JUMP_MENU != 0,
		VisiblePages:   uint32(wire.visible_pages),
		AvailableWidth: float32(wire.available_width),
	}
	for _, label := range []struct {
		source *C.char
		target *string
	}{
		{&wire.group[0], &config.Group}, {&wire.first[0], &config.First},
		{&wire.previous[0], &config.Previous}, {&wire.next[0], &config.Next},
		{&wire.last[0], &config.Last}, {&wire.earlier_pages[0], &config.EarlierPages},
		{&wire.later_pages[0], &config.LaterPages},
	} {
		buf := paginationLabelBytes(label.source)
		end := bytes.IndexByte(buf, 0)
		if end < 0 {
			return PaginationConfig{}, false, errors.New("invalid copied Pagination label")
		}
		*label.target = string(buf[:end])
	}
	if err := config.validate(); err != nil {
		return PaginationConfig{}, false, err
	}
	return config, true, nil
}

// SupportsPaginationCustomSize reports the independent custom-height capability.
func (a *App) SupportsPaginationCustomSize() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_pagination_custom_supported() != 0, nil
}
func (n *Node) setMountedPaginationCustomSize(size float32) error {
	supported, err := n.app.SupportsPaginationCustomSize()
	if err != nil {
		return err
	}
	if !supported {
		return ErrPaginationCustomSizeUnavailable
	}
	return result(C.go_pagination_set_custom_size(n.app.ptr, n.handle, C.float(size)))
}

// PaginationCustomSize returns zero when the named design size is active.
// Explicit mounted reads require the independent custom-height capability.
func (n *Node) PaginationCustomSize() (float32, error) {
	if err := n.checkPagination(); err != nil {
		return 0, err
	}
	if n.app == nil {
		return n.paginationCustomSize, nil
	}
	supported, err := n.app.SupportsPaginationCustomSize()
	if err != nil {
		return 0, err
	}
	if !supported {
		return 0, ErrPaginationCustomSizeUnavailable
	}
	var size C.float
	err = result(C.go_pagination_get_custom_size(n.app.ptr, n.handle, &size))
	return float32(size), err
}

// RadioPaintLayer selects a retained Radio state override. Base geometry applies in every state.
type RadioPaintLayer uint32

const (
	RadioPaintBase RadioPaintLayer = iota
	RadioPaintChecked
	RadioPaintHovered
	RadioPaintPressed
	RadioPaintFocused
	RadioPaintDisabled
)

// RadioPaintFields selects explicitly present values, including transparent colors and zero dimensions.
type RadioPaintFields uint32

const (
	RadioPaintRootBackground       RadioPaintFields = 1 << 0
	RadioPaintRootBorder           RadioPaintFields = 1 << 1
	RadioPaintRootBorderWidth      RadioPaintFields = 1 << 2
	RadioPaintRootRadius           RadioPaintFields = 1 << 3
	RadioPaintRootPadding          RadioPaintFields = 1 << 4
	RadioPaintRootGap              RadioPaintFields = 1 << 5
	RadioPaintIndicatorBackground  RadioPaintFields = 1 << 6
	RadioPaintIndicatorBorder      RadioPaintFields = 1 << 7
	RadioPaintIndicatorBorderWidth RadioPaintFields = 1 << 8
	RadioPaintIndicatorRadius      RadioPaintFields = 1 << 9
	RadioPaintIndicatorExtent      RadioPaintFields = 1 << 10
	RadioPaintMarkColor            RadioPaintFields = 1 << 11
	RadioPaintLabelColor           RadioPaintFields = 1 << 12
	RadioPaintDescriptionColor     RadioPaintFields = 1 << 13
	RadioPaintLabelSize            RadioPaintFields = 1 << 14
	RadioPaintDescriptionSize      RadioPaintFields = 1 << 15
	RadioPaintLabelWeight          RadioPaintFields = 1 << 16
	RadioPaintDescriptionWeight    RadioPaintFields = 1 << 17
)

// RadioPaint replaces one layer. Colors use numeric 0xRRGGBBAA; dimensions are logical pixels.
// Weights are normal=0, medium=1, semibold=2, bold=3. Geometry and text metrics require Base.
type RadioPaint struct {
	Fields               RadioPaintFields
	RootBackground       uint32
	RootBorder           uint32
	RootBorderWidth      float32
	RootRadius           float32
	RootPadding          float32
	RootGap              float32
	IndicatorBackground  uint32
	IndicatorBorder      uint32
	IndicatorBorderWidth float32
	IndicatorRadius      float32
	IndicatorExtent      float32
	MarkColor            uint32
	LabelColor           uint32
	DescriptionColor     uint32
	LabelSize            float32
	DescriptionSize      float32
	LabelWeight          uint32
	DescriptionWeight    uint32
}

var ErrRadioPaintUnavailable = errors.New("Radio paint is unavailable in this bridge")

// SupportsRadioPaint reports optional retained Radio paint support.
func (a *App) SupportsRadioPaint() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_radio_paint_supported() != 0, nil
}

// RadioPaint sets one layer before or after Mount. A zero field mask clears that layer.
func (n *Node) RadioPaint(layer RadioPaintLayer, paint RadioPaint) *Node {
	if n.err == nil {
		n.err = n.SetRadioPaint(layer, paint)
	}
	return n
}

// ClearRadioPaint restores defaults for one layer and preserves all other layers.
func (n *Node) ClearRadioPaint(layer RadioPaintLayer) error {
	return n.SetRadioPaint(layer, RadioPaint{})
}

// SetRadioPaint copies one layer without selecting or notifying. Only direct RadioGroup Items accept description fields.
// RadioGroup accepts root fields on Base and Disabled only. Unspecified fields retain theme defaults.
// Color and border width overrides can use any option state; radius, padding, gap, extent, size and weight require Base.
func (n *Node) SetRadioPaint(layer RadioPaintLayer, paint RadioPaint) error {
	if n == nil || n.removed ||
		(n.kind != int(C.CUI_BRIDGE_RADIO) && n.kind != int(C.CUI_BRIDGE_RADIO_GROUP) &&
			n.kind != int(C.CUI_BRIDGE_ITEM)) {
		return errors.New("paint requires a live Radio, RadioGroup or direct Item")
	}
	if layer > RadioPaintDisabled {
		return errors.New("invalid Radio paint layer")
	}
	f := paint.Fields
	const baseFields = RadioPaintRootRadius | RadioPaintRootPadding | RadioPaintRootGap | RadioPaintIndicatorRadius | RadioPaintIndicatorExtent | RadioPaintLabelSize | RadioPaintDescriptionSize | RadioPaintLabelWeight | RadioPaintDescriptionWeight
	const descriptionFields = RadioPaintDescriptionColor | RadioPaintDescriptionSize | RadioPaintDescriptionWeight
	if f & ^RadioPaintFields((1<<18)-1) != 0 || (layer != RadioPaintBase && f&baseFields != 0) ||
		(n.kind == int(C.CUI_BRIDGE_RADIO_GROUP) &&
			(f & ^(RadioPaintRootGap<<1-1) != 0 || (layer != RadioPaintBase && layer != RadioPaintDisabled))) ||
		(n.kind != int(C.CUI_BRIDGE_ITEM) && f&descriptionFields != 0) {
		return errors.New("invalid Radio paint fields for owner or layer")
	}
	validRange := func(bit RadioPaintFields, value, min, max float32) bool {
		return f&bit == 0 || (value >= min && value <= max)
	}
	if !validRange(RadioPaintRootBorderWidth, paint.RootBorderWidth, 0, 64) ||
		!validRange(RadioPaintRootRadius, paint.RootRadius, 0, 256) ||
		!validRange(RadioPaintRootPadding, paint.RootPadding, 0, 256) ||
		!validRange(RadioPaintRootGap, paint.RootGap, 0, 256) ||
		!validRange(RadioPaintIndicatorBorderWidth, paint.IndicatorBorderWidth, 0, 64) ||
		!validRange(RadioPaintIndicatorRadius, paint.IndicatorRadius, 0, 256) ||
		!validRange(RadioPaintIndicatorExtent, paint.IndicatorExtent, 4, 128) ||
		!validRange(RadioPaintLabelSize, paint.LabelSize, 1, 128) ||
		!validRange(RadioPaintDescriptionSize, paint.DescriptionSize, 1, 128) ||
		(f&RadioPaintLabelWeight != 0 && paint.LabelWeight > 3) || (f&RadioPaintDescriptionWeight != 0 && paint.DescriptionWeight > 3) {
		return errors.New("invalid Radio paint range")
	}

	if n.app != nil {
		if err := n.app.check(); err != nil {
			return err
		}
		if C.go_radio_paint_supported() == 0 {
			if f != 0 {
				return ErrRadioPaintUnavailable
			}
		} else {
			wire := C.cui_bridge_radio_paint_v1{struct_size: C.sizeof_cui_bridge_radio_paint_v1,
				schema_version: 1, fields: C.uint32_t(f)}
			wire.root_background_rgba = C.uint32_t(paint.RootBackground)
			wire.root_border_rgba = C.uint32_t(paint.RootBorder)
			wire.root_border_width = C.float(paint.RootBorderWidth)
			wire.root_radius = C.float(paint.RootRadius)
			wire.root_padding = C.float(paint.RootPadding)
			wire.root_gap = C.float(paint.RootGap)
			wire.indicator_background_rgba = C.uint32_t(paint.IndicatorBackground)
			wire.indicator_border_rgba = C.uint32_t(paint.IndicatorBorder)
			wire.indicator_border_width = C.float(paint.IndicatorBorderWidth)
			wire.indicator_radius = C.float(paint.IndicatorRadius)
			wire.indicator_extent = C.float(paint.IndicatorExtent)
			wire.mark_rgba = C.uint32_t(paint.MarkColor)
			wire.label_rgba = C.uint32_t(paint.LabelColor)
			wire.description_rgba = C.uint32_t(paint.DescriptionColor)
			wire.label_size = C.float(paint.LabelSize)
			wire.description_size = C.float(paint.DescriptionSize)
			wire.label_weight = C.uint32_t(paint.LabelWeight)
			wire.description_weight = C.uint32_t(paint.DescriptionWeight)
			if err := result(C.go_set_radio_paint(n.app.ptr, n.handle, C.uint32_t(layer), &wire)); err != nil {
				return err
			}
		}
	}
	if f == 0 {
		delete(n.radioPaints, layer)
	} else {
		if n.radioPaints == nil {
			n.radioPaints = make(map[RadioPaintLayer]RadioPaint)
		}
		n.radioPaints[layer] = paint
	}
	return nil
}

// SupportsKbdStroke reports whether typed shortcut presentation is available.
func (a *App) SupportsKbdStroke() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_kbd_stroke_supported() != 0, nil
}
func (n *Node) setKbdStroke(c kbdStrokeConfig) error {
	supported, err := n.app.SupportsKbdStroke()
	if err != nil {
		return err
	}
	if !supported {
		return ErrKbdStrokeUnavailable
	}
	key, name := cs(c.key), cs(c.options.AccessibleName)
	defer C.free(unsafe.Pointer(key))
	defer C.free(unsafe.Pointer(name))
	var flags C.uint32_t
	if c.options.Plain {
		flags |= C.CUI_BRIDGE_KBD_PLAIN_V1
	}
	if c.options.Outline {
		flags |= C.CUI_BRIDGE_KBD_OUTLINE_V1
	}
	if c.options.MacSymbols {
		flags |= C.CUI_BRIDGE_KBD_MAC_SYMBOLS_V1
	}
	return result(C.go_kbd_stroke_set(n.app.ptr, n.handle, C.uint32_t(c.options.Modifiers),
		flags, key, C.size_t(len(c.key)), name, C.size_t(len(c.options.AccessibleName))))
}

// SupportsSpinnerPresentation reports whether the active bridge exposes
// all of rotating Spinner V1 with compatible record sizes.
func (a *App) SupportsSpinnerPresentation() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_spinner_supported() != 0, nil
}

func (n *Node) configureMountedSpinner(c SpinnerConfig) error {
	supported, err := n.app.SupportsSpinnerPresentation()
	if err != nil {
		return err
	}
	if !supported {
		return ErrSpinnerPresentationUnavailable
	}
	var flags C.uint32_t
	if c.CustomColor {
		flags |= C.CUI_BRIDGE_SPINNER_CUSTOM_COLOR
	}
	if c.ReducedMotion {
		flags |= C.CUI_BRIDGE_SPINNER_REDUCED_MOTION
	}
	r := C.cui_bridge_spinner_config_v1{
		struct_size: C.sizeof_cui_bridge_spinner_config_v1, schema_version: C.CUI_BRIDGE_SPINNER_V1,
		flags: flags, size: C.uint32_t(c.Size), pixels: C.float(c.Pixels), rem_size: C.float(c.RemSize),
		duration: C.float(c.Duration), rgba: C.uint32_t(c.RGBA), easing: C.uint32_t(c.Easing),
	}
	for i, b := range []byte(c.Icon) {
		r.icon[i] = C.char(b)
	}
	return result(C.go_spinner_configure(n.app.ptr, n.handle, &r))
}

// SpinnerConfig returns a copy of the authored rotating configuration.
func (n *Node) SpinnerConfig() (SpinnerConfig, error) {
	if n == nil || n.removed || n.spinnerConfig == nil {
		return SpinnerConfig{}, errors.New("node has no rotating Spinner configuration")
	}
	if n.app == nil {
		return *n.spinnerConfig, nil
	}
	if err := n.app.check(); err != nil {
		return SpinnerConfig{}, err
	}
	r := C.cui_bridge_spinner_config_v1{
		struct_size: C.sizeof_cui_bridge_spinner_config_v1, schema_version: C.CUI_BRIDGE_SPINNER_V1,
	}
	if err := result(C.go_spinner_get_config(n.app.ptr, n.handle, &r)); err != nil {
		return SpinnerConfig{}, err
	}
	icon := make([]byte, 0, 64)
	for _, b := range r.icon {
		if b == 0 {
			break
		}
		icon = append(icon, byte(b))
	}
	if len(icon) == 64 {
		return SpinnerConfig{}, errors.New("unterminated Spinner icon")
	}
	c := SpinnerConfig{Size: SpinnerSize(r.size), Pixels: float32(r.pixels), RemSize: float32(r.rem_size), Duration: float32(r.duration), RGBA: uint32(r.rgba), Easing: SpinnerEasing(r.easing), Icon: string(icon), CustomColor: r.flags&C.CUI_BRIDGE_SPINNER_CUSTOM_COLOR != 0, ReducedMotion: r.flags&C.CUI_BRIDGE_SPINNER_REDUCED_MOTION != 0}
	const knownFlags = C.CUI_BRIDGE_SPINNER_CUSTOM_COLOR | C.CUI_BRIDGE_SPINNER_REDUCED_MOTION
	if r.struct_size != C.sizeof_cui_bridge_spinner_config_v1 ||
		r.schema_version != C.CUI_BRIDGE_SPINNER_V1 || r.flags&^knownFlags != 0 || c.validate() != nil {
		return SpinnerConfig{}, errors.New("invalid copied Spinner configuration")
	}
	return c, nil
}

// SpinnerPresentation reads the most recent sample from a mounted rotating
// Spinner. The bool is false when no current sample is available.
func (n *Node) SpinnerPresentation() (SpinnerPresentation, bool, error) {
	if n == nil || n.app == nil || n.removed || n.spinnerConfig == nil {
		return SpinnerPresentation{}, false, errors.New("presentation requires a mounted rotating Spinner")
	}
	if err := n.app.check(); err != nil {
		return SpinnerPresentation{}, false, err
	}
	r := C.cui_bridge_spinner_presentation_v1{
		struct_size: C.sizeof_cui_bridge_spinner_presentation_v1, schema_version: C.CUI_BRIDGE_SPINNER_V1,
	}
	code := C.go_spinner_get_presentation(n.app.ptr, n.handle, &r)
	if code == C.CUI_BRIDGE_EMPTY {
		return SpinnerPresentation{}, false, nil
	}
	if err := result(code); err != nil {
		return SpinnerPresentation{}, false, err
	}
	p := SpinnerPresentation{
		FrameSequence: uint64(r.frame_sequence), CoreTime: float64(r.core_time),
		NextWakeSeconds: float64(r.next_wake_seconds), Phase: float32(r.phase),
	}
	if r.struct_size != C.sizeof_cui_bridge_spinner_presentation_v1 ||
		r.schema_version != C.CUI_BRIDGE_SPINNER_V1 || r.reserved != 0 || p.FrameSequence == 0 ||
		!(p.CoreTime >= 0) || math.IsInf(p.CoreTime, 0) ||
		(p.NextWakeSeconds != 0 && (!(p.NextWakeSeconds > p.CoreTime) || math.IsInf(p.NextWakeSeconds, 0))) ||
		!(p.Phase >= 0 && p.Phase <= 1) {
		return SpinnerPresentation{}, false, errors.New("invalid copied Spinner presentation")
	}
	return p, true, nil
}

// SupportsSpinnerIconResources checks the optional shared artwork group and Icon resources.
func (a *App) SupportsSpinnerIconResources() (bool, error) {
	ok, err := a.SupportsIconResources()
	return ok && C.go_spinner_icon_supported() != 0, err
}

// SetSpinnerIconResource binds shared app-owned artwork on a mounted Spinner.
// nil restores its configured catalog icon. The resource remains caller-owned.
func (n *Node) SetSpinnerIconResource(resource *IconResource) error {
	if n == nil || n.app == nil || n.removed || n.kind != 36 {
		return errors.New("artwork requires a mounted Spinner")
	}
	supported, err := n.app.SupportsSpinnerIconResources()
	if err != nil {
		return err
	}
	if !supported {
		return ErrSpinnerIconResourcesUnavailable
	}
	var handle C.cui_bridge_icon_handle_v1
	if resource != nil {
		handle, err = resource.handleFor(n.app)
		if err != nil {
			return err
		}
	}
	if err = result(C.go_spinner_icon_set(n.app.ptr, n.handle, handle)); err != nil {
		return err
	}
	n.spinnerIconResource = resource.alias()
	return nil
}

// SpinnerIconResource returns an alias sharing the original resource ownership.
func (n *Node) SpinnerIconResource() (*IconResource, error) {
	if n == nil || n.app == nil || n.removed || n.kind != 36 {
		return nil, errors.New("artwork requires a mounted Spinner")
	}
	supported, err := n.app.SupportsSpinnerIconResources()
	if err != nil {
		return nil, err
	}
	if !supported {
		return nil, ErrSpinnerIconResourcesUnavailable
	}
	var handle C.cui_bridge_icon_handle_v1
	status := C.go_spinner_icon_get(n.app.ptr, n.handle, &handle)
	if status == C.CUI_BRIDGE_EMPTY {
		return nil, nil
	}
	if err = result(status); err != nil {
		return nil, err
	}
	saved, err := n.spinnerIconResource.handleFor(n.app)
	if err != nil {
		return nil, err
	}
	if saved != handle {
		return nil, errors.New("unknown Spinner resource ownership")
	}
	return n.spinnerIconResource.alias(), nil
}

// ErrResponsiveNotificationsUnavailable indicates the provider lacks responsive notices.
var ErrResponsiveNotificationsUnavailable = errors.New("responsive notifications are unavailable in this bridge")

// SupportsResponsiveNotifications probes the optional retained notification layout.
func (a *App) SupportsResponsiveNotifications() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_responsive_notifications_supported() != 0, nil
}

// SetResponsiveNotifications enables wrapping and independent Close for all retained
// notifications in this app. It defaults off, is reversible, and must run outside Step.
func (a *App) SetResponsiveNotifications(enabled bool) error {
	supported, err := a.SupportsResponsiveNotifications()
	if err != nil {
		return err
	}
	if !supported {
		return ErrResponsiveNotificationsUnavailable
	}
	return result(C.go_set_responsive_notifications(a.ptr, C.uint32_t(btoi(enabled))))
}

// SupportsKbdStyle reports optional retained Kbd style support.
func (a *App) SupportsKbdStyle() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_kbd_style_supported() != 0, nil
}
func (n *Node) setKbdStyle(style *C.cui_bridge_kbd_style_v1) error {
	supported, err := n.app.SupportsKbdStyle()
	if err != nil {
		return err
	}
	if !supported {
		return ErrKbdStyleUnavailable
	}
	return result(C.go_kbd_style_set(n.app.ptr, n.handle, style))
}

func makeKbdStyleWire(s KbdStyleOptions) *C.cui_bridge_kbd_style_v1 {
	var w C.cui_bridge_kbd_style_v1
	w.struct_size = C.sizeof_cui_bridge_kbd_style_v1
	w.schema_version = 1
	if s.MaxWidth != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_MAX_WIDTH
		w.max_width = C.float(*s.MaxWidth)
	}
	if s.Radius != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_RADIUS
		w.radius = C.float(*s.Radius)
	}
	if s.FontSize != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_FONT_SIZE
		w.font_size = C.float(*s.FontSize)
	}
	if s.LineHeight != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_LINE_HEIGHT
		w.line_height = C.float(*s.LineHeight)
	}
	if s.Fill {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_FILL
	}
	if s.Wrap {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_WRAP
	}
	if s.Italic {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_ITALIC
	}
	if s.Monospace {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_MONOSPACE
	}
	if s.Padding != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_PADDING
		w.padding_x = C.float(s.Padding.X)
		w.padding_y = C.float(s.Padding.Y)
	}
	if s.Background != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_BACKGROUND
		w.background_rgba = C.uint32_t(*s.Background)
	}
	if s.Border != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_BORDER
		w.border_width = C.float(s.Border.Width)
		w.border_rgba = C.uint32_t(s.Border.Color)
	}
	if s.Weight != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_WEIGHT
		w.weight = C.uint32_t(*s.Weight)
	}
	if s.TextColor != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_TEXT_COLOR
		w.text_rgba = C.uint32_t(*s.TextColor)
	}
	if g := s.TextGradient; g != nil {
		w.fields |= C.CUI_BRIDGE_KBD_STYLE_TEXT_GRADIENT
		w.x0 = C.float(g.X0)
		w.y0 = C.float(g.Y0)
		w.x1 = C.float(g.X1)
		w.y1 = C.float(g.Y1)
		w.stop_count = C.uint32_t(len(g.Stops))
		for i, v := range g.Stops {
			w.stops[i].position = C.float(v.Position)
			w.stops[i].color_rgba = C.uint32_t(v.Color)
		}
	}
	return &w
}

// SupportsNotificationAction reports whether the provider can retain a Notification action Button.
func (a *App) SupportsNotificationAction() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_notification_action_supported() != 0, nil
}

// SupportsNotificationAutoHide reports whether optional timed Notification dismissal is available.
func (a *App) SupportsNotificationAutoHide() (bool, error) {
	if err := a.check(); err != nil {
		return false, err
	}
	return C.go_notification_auto_hide_supported() != 0, nil
}

// AutoHide configures the next successful open. Enabled notifications dismiss
// after five seconds present, pausing while hovered or focused. The default is persistent.
func (n *Node) AutoHide(enabled bool) *Node {
	if n.err != nil {
		return n
	}
	if n.kind != 62 {
		n.err = errors.New("AutoHide requires Notification")
		return n
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			n.err = e
			return n
		}
		if C.go_notification_auto_hide_supported() == 0 {
			n.err = ErrNotificationAutoHideUnavailable
			return n
		}
		if e := result(C.go_notification_auto_hide(n.app.ptr, n.handle, C.uint32_t(btoi(enabled)))); e != nil {
			n.err = e
			return n
		}
	}
	n.notificationAutoHide = &enabled
	return n
}
