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
typedef cui_bridge_status (*run_after_fn)(void *);
typedef cui_bridge_status (*run_fn)(cui_bridge_app *, run_after_fn, void *);
extern int goBridgeAfterFrame(uintptr_t user);
#if defined(CUI_GO_STATIC)
extern uint32_t cui_bridge_notification_v1_size(void) __attribute__((weak));
extern uint32_t cui_bridge_notification_versions(void) __attribute__((weak));
extern cui_bridge_status cui_bridge_poll_notification_v1(cui_bridge_app *, uint32_t, cui_bridge_notification_v1 *, char *, uint32_t, uint32_t *) __attribute__((weak));
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
static uint32_t go_notification_boolean(const cui_bridge_notification_v1 *n) { return n->payload.boolean_value; }
static cui_bridge_table_notification_v1 go_notification_table(const cui_bridge_notification_v1 *n) { return n->payload.table; }
static uint32_t go_notification_surface(const cui_bridge_notification_v1 *n) { return n->payload.surface_result; }
static int bridge_initialized;
static int bridge_open(const char *path) {
 if (bridge_initialized) return 0;
#if defined(CUI_GO_STATIC)
 if (!path[0]) {
#define STATIC_LOAD(name, ret, params, args) fp_##name = cui_bridge_##name;
 API(STATIC_LOAD, STATIC_LOAD)
#undef STATIC_LOAD
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
*/
import "C"

import (
	"errors"
	"fmt"
	"math"
	"os"
	"path/filepath"
	"runtime"
	"runtime/cgo"
	"strings"
	"sync"
	"unicode/utf8"
	"unsafe"
)

type App struct {
	ptr     *C.cui_bridge_app
	thread  C.bridge_thread_token
	root    *Node
	nodes   map[uint64]*Node
	closed  bool
	running bool
}
type Options struct {
	Native                   bool
	Width, Height            float32
	Title, FontPath, Library string
	FontFallbacks            []string
	MaxNodes, MaxEvents      uint32
}
type Node struct {
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
	checked                              bool
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
	hasTableData                         bool
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
	ptr := C.go_create(&options, &code)
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
	return result(C.go_request_close(a.ptr))
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
	if n.app != nil {
		if err := n.app.check(); err != nil {
			return err
		}
		s := cs(v)
		defer C.free(unsafe.Pointer(s))
		if err := result(C.go_set_text(n.app.ptr, n.handle, s)); err != nil {
			return err
		}
	}
	n.text = v
	return nil
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
	if n.err != nil {
		return n
	}
	if !(n.kind == 4 || (n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || (n.kind >= 17 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 48 || (n.kind >= 54 && n.kind <= 61) || (n.kind >= 65 && n.kind <= 69) || n.kind == 71 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 77 || n.kind == 78 || n.kind == 79) {
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
	var visit func(*Node, int) error
	visit = func(n *Node, d int) error {
		if n == nil || seen[n] || n.app != nil || n.removed {
			return errors.New("invalid or reused node")
		}
		seen[n] = true
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
		if n.kind >= 3 && n.kind != 15 && n.kind != 17 && n.kind != 18 && n.kind != 19 && n.kind != 20 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 24 && n.kind != 26 && n.kind != 27 && n.kind != 29 && n.kind != 33 && n.kind != 40 && n.kind != 41 && n.kind != 44 && n.kind != 45 && n.kind != 46 && n.kind != 47 && n.kind != 48 && n.kind != 49 && n.kind != 54 && n.kind != 60 && n.kind != 61 && n.kind != 63 && n.kind != 64 && n.kind != 65 && n.kind != 66 && n.kind != 70 && n.kind != 73 && n.kind != 74 && n.kind != 75 && n.kind != 76 && n.kind != 78 && n.kind != 79 && n.kind != 80 && len(n.children) > 0 {
			return errors.New("leaf has children")
		}
		if (n.kind == 54 || n.kind == 60 || n.kind == 64 || n.kind == 65 || n.kind == 66 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 79) && len(n.children) > 64 {
			return errors.New("tree exceeds 64 Items")
		}
		if n.kind == 17 || n.kind == 18 || n.kind == 19 || n.kind == 21 || n.kind == 22 || n.kind == 23 || n.kind == 40 || n.kind == 41 || n.kind == 44 || n.kind == 45 || n.kind == 47 || n.kind == 48 || n.kind == 49 || n.kind == 54 || n.kind == 60 || n.kind == 61 || n.kind == 64 || n.kind == 65 || n.kind == 66 || n.kind == 70 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 79 {
			for _, ch := range n.children {
				if ch == nil || ch.kind != 20 {
					return errors.New("group children must be Item")
				}
			}
		}
		for _, ch := range n.children {
			if err := visit(ch, d+1); err != nil {
				return err
			}
		}
		return nil
	}
	if err := visit(root, 1); err != nil {
		return err
	}
	var add func(*Node, C.cui_bridge_node) error
	add = func(n *Node, p C.cui_bridge_node) error {
		key := cs(n.key)
		defer C.free(unsafe.Pointer(key))
		var h C.cui_bridge_node
		if err := result(C.go_node_create(a.ptr, p, C.cui_bridge_kind(n.kind), key, &h)); err != nil {
			return err
		}
		n.app = a
		n.handle = h
		a.nodes[keyID(h)] = n
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
			if err := n.SetText(n.text); err != nil {
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
			if e := n.SetTable(n.tableHeaders, n.tableRows); e != nil {
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
		if n.hasVisualSize {
			if e := result(C.go_set_visual_size(a.ptr, h, C.float(n.visualWidth), C.float(n.visualHeight))); e != nil {
				return e
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
		if ((n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || ((n.kind >= 17 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 48 || (n.kind >= 54 && n.kind <= 61) || (n.kind >= 65 && n.kind <= 69) || n.kind == 71 || n.kind == 73 || n.kind == 74 || n.kind == 75 || n.kind == 77 || n.kind == 78 || n.kind == 79)) && n.disabled {
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
	a.root = root
	return nil
}
func (a *App) Step(dt float32) error {
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

func (a *App) Dispatch() (int, error) {
	count := 0
	for {
		e, ok, err := a.PollEvent()
		if err != nil {
			return count, err
		}
		if !ok {
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
	state := &runState{app: a}
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
	if state.err != nil && !errors.Is(state.err, ErrClosed) {
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

type DataTableState struct{ Row, Column, SortColumn, SortDirection uint32 }

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
func (n *Node) SetTable(headers []string, rows [][]string) error {
	if n == nil || n.kind != 71 {
		return errors.New("table data requires DataTable")
	}
	if n.removed {
		return errors.New("removed data table")
	}
	if len(headers) == 0 || len(headers) > 16 || len(rows) > 32 {
		return errors.New("invalid data table dimensions")
	}
	for _, s := range headers {
		if e := valid(s); e != nil {
			return e
		}
		if len(s) > 128 {
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
			if len(s) > 128 {
				return errors.New("data table text exceeds 128 bytes")
			}
		}
	}
	if n.app != nil {
		if e := n.app.check(); e != nil {
			return e
		}
		ch := make([]*C.char, len(headers))
		cc := make([]*C.char, len(headers)*len(rows))
		defer func() {
			for _, p := range ch {
				if p != nil {
					C.free(unsafe.Pointer(p))
				}
			}
			for _, p := range cc {
				if p != nil {
					C.free(unsafe.Pointer(p))
				}
			}
		}()
		for i, s := range headers {
			ch[i] = cs(s)
		}
		for r, row := range rows {
			for c, s := range row {
				cc[r*len(headers)+c] = cs(s)
			}
		}
		var cells **C.char
		if len(cc) > 0 {
			cells = &cc[0]
		}
		if e := result(C.go_data_table_set(n.app.ptr, n.handle, (**C.char)(unsafe.Pointer(&ch[0])), C.uint32_t(len(ch)), cells, C.uint32_t(len(rows)))); e != nil {
			return e
		}
		runtime.KeepAlive(ch)
		runtime.KeepAlive(cc)
	}
	n.tableHeaders = append([]string(nil), headers...)
	n.tableRows = make([][]string, len(rows))
	for i, row := range rows {
		n.tableRows[i] = append([]string(nil), row...)
	}
	n.hasTableData = true
	return nil
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
func (n *Node) RGBA(pixels []byte, width, height, stride uint32) *Node {
	if n.err != nil {
		return n
	}
	n.err = n.SetRGBA(pixels, width, height, stride)
	return n
}
func (n *Node) SetRGBA(pixels []byte, width, height, stride uint32) error {
	if n == nil || n.kind != 53 {
		return errors.New("RGBA requires Image")
	}
	if n.removed {
		return errors.New("removed image")
	}
	count := uint64(stride) * uint64(height)
	if width == 0 || height == 0 || width > 2048 || height > 2048 || stride < uint32(uint64(width)*4) || count > 16777216 || uint64(len(pixels)) != count {
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
