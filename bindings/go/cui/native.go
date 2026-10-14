package cui

/*
#cgo CFLAGS: -std=c11 -Wall -Wextra -Werror -I${SRCDIR}/../../../include
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
#define API(X) X(abi_version, uint32_t, (void), ()) \
 X(options_size, uint32_t, (void), ()) \
 X(status_message, const char *, (cui_bridge_status s), (s)) \
 X(create, cui_bridge_app *, (const cui_bridge_options *o, cui_bridge_status *s), (o,s)) \
 X(destroy, void, (cui_bridge_app *a), (a)) \
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
 X(step, cui_bridge_status, (cui_bridge_app *a, float dt), (a,dt)) \
 X(dropdown_part_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t menu, cui_bridge_rect *r), (a,n,menu,r)) \
 X(node_bounds, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_rect *r), (a,n,r)) \
 X(request_close, cui_bridge_status, (cui_bridge_app *a), (a))
#define DECL(name, ret, params, args) static ret (*fp_##name) params; static ret go_##name params { return fp_##name args; }
#define DECL_VOID(name, ret, params, args) DECL(name, ret, params, args)
API(DECL)
#undef DECL
static int bridge_open(const char *path) {
 if (bridge_lib) return 0;
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
 API(LOAD)
#undef LOAD
 bridge_lib = lib;
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
	"strings"
	"sync"
	"time"
	"unicode/utf8"
	"unsafe"
)

type App struct {
	ptr    *C.cui_bridge_app
	thread C.bridge_thread_token
	root   *Node
	nodes  map[uint64]*Node
	closed bool
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
		return nil, errors.New("CUI_BRIDGE_LIBRARY required")
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
	if !filepath.IsAbs(o.Library) {
		return nil, errors.New("bridge library path must be absolute")
	}
	loaderMu.Lock()
	if loadedLibrary != "" && loadedLibrary != o.Library {
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
	loadedLibrary = o.Library
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
func (a *App) Close() error {
	if err := a.check(); err != nil {
		return err
	}
	C.go_destroy(a.ptr)
	a.ptr = nil
	a.closed = true
	runtime.UnlockOSThread()
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
func Label(text string) *Node          { return &Node{kind: 3, text: text, size: int(Base)} }
func Button(key string) *Node          { return &Node{kind: 4, key: key, size: int(Base)} }
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
	if !(n.kind == 4 || (n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || (n.kind >= 17 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 48 || (n.kind >= 54 && n.kind <= 61)) {
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
		if n.kind >= 3 && n.kind != 15 && n.kind != 17 && n.kind != 18 && n.kind != 19 && n.kind != 20 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 24 && n.kind != 26 && n.kind != 27 && n.kind != 29 && n.kind != 33 && n.kind != 40 && n.kind != 41 && n.kind != 44 && n.kind != 45 && n.kind != 46 && n.kind != 47 && n.kind != 48 && n.kind != 49 && n.kind != 54 && n.kind != 60 && n.kind != 61 && n.kind != 63 && len(n.children) > 0 {
			return errors.New("leaf has children")
		}
		if (n.kind == 54 || n.kind == 60) && len(n.children) > 64 {
			return errors.New("tree exceeds 64 Items")
		}
		if n.kind == 17 || n.kind == 18 || n.kind == 19 || n.kind == 21 || n.kind == 22 || n.kind == 23 || n.kind == 40 || n.kind == 41 || n.kind == 44 || n.kind == 45 || n.kind == 47 || n.kind == 48 || n.kind == 49 || n.kind == 54 || n.kind == 60 || n.kind == 61 {
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
		if n.hasOpen && (n.kind < 24 || n.kind == 58 || n.kind == 62 || n.kind == 63) {
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
		if ((n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || ((n.kind >= 17 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 48 || (n.kind >= 54 && n.kind <= 61))) && n.disabled {
			if err := result(C.go_set_disabled(a.ptr, h, 1)); err != nil {
				return err
			}
		}
		if (n.kind == 9 || n.kind == 14) && n.hasRange {
			if err := result(C.go_set_range(a.ptr, h, C.double(n.min), C.double(n.max), C.double(n.step))); err != nil {
				return err
			}
		}
		if (n.kind == 9 || n.kind == 10 || n.kind == 14 || n.kind == 42 || n.kind == 43 || n.kind == 52) && n.hasValue {
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
func (a *App) Run(dt float32) error {
	if a == nil || a.root == nil {
		return errors.New("mount a tree first")
	}
	if !finite(dt) || dt <= 0 {
		return errors.New("Run requires positive finite dt")
	}
	frame := time.Duration(float64(dt) * float64(time.Second))
	if frame <= 0 {
		return errors.New("dt too small")
	}
	for {
		start := time.Now()
		err := a.Step(dt)
		if err != nil {
			if errors.Is(err, ErrClosed) {
				return nil
			}
			return err
		}
		if _, err := a.Dispatch(); err != nil {
			return err
		}
		if a.closed {
			return nil
		}
		if rest := frame - time.Since(start); rest > 0 {
			time.Sleep(rest)
		}
	}
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
	if (n.kind != 9 && n.kind != 10 && n.kind != 14 && n.kind != 42 && n.kind != 43 && n.kind != 52) || math.IsNaN(v) || math.IsInf(v, 0) {
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
	if !((n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || n.kind == 17 || n.kind == 18 || (n.kind >= 20 && n.kind <= 23) || n.kind == 28 || n.kind == 29 || (n.kind >= 40 && n.kind <= 44) || n.kind == 46 || (n.kind >= 54 && n.kind <= 59) || n.kind == 61) {
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
	if n.kind != 16 && n.kind != 20 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 32 && n.kind != 51 && n.kind != 52 && n.kind != 61 && n.kind != 62 && (n.kind < 24 || n.kind > 27) {
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
	if n.kind != 15 && n.kind != 20 && n.kind != 58 && n.kind != 62 && n.kind != 63 && (n.kind < 24 || n.kind > 27) {
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
	if n.kind != 17 && n.kind != 18 && n.kind != 21 && n.kind != 22 && n.kind != 23 && n.kind != 40 && n.kind != 41 && n.kind != 44 && n.kind != 54 {
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
	if !(n.kind == 30 || n.kind == 32 || n.kind == 33 || n.kind == 35 || n.kind == 36 || n.kind == 46 || n.kind == 50 || n.kind == 51 || n.kind == 52 || n.kind == 53 || n.kind == 54 || n.kind == 56 || n.kind == 58 || n.kind == 60 || n.kind == 61) || !finite(width) || !finite(height) || width < 0 || height < 0 || width > 4096 || height > 4096 {
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
func FocusTrap(key string) *Node { return &Node{kind: 63, key: key} }
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
