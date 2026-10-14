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

static void *bridge_lib;
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
 X(set_open, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_selected, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node group, cui_bridge_node item), (a,group,item)) \
 X(get_selected, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node group, cui_bridge_node *out), (a,group,out)) \
 X(set_gap, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, float v), (a,n,v)) \
 X(set_button_variant, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_variant v), (a,n,v)) \
 X(set_button_size, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_size v), (a,n,v)) \
 X(set_button_rounding, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, cui_bridge_button_rounding v, float radius), (a,n,v,radius)) \
 X(set_button_disabled, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint32_t v), (a,n,v)) \
 X(set_button_hover_rgba, cui_bridge_status, (cui_bridge_app *a, cui_bridge_node n, uint8_t r, uint8_t g, uint8_t b, uint8_t alpha), (a,n,r,g,b,alpha)) \
 X(queue_pointer, cui_bridge_status, (cui_bridge_app *a, cui_bridge_pointer_phase p, float x, float y), (a,p,x,y)) \
 X(queue_key, cui_bridge_status, (cui_bridge_app *a, cui_bridge_key_phase p, cui_bridge_key k, uint32_t m, uint32_t r), (a,p,k,m,r)) \
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
 HMODULE lib = LoadLibraryA(path);
 if (!lib) return -1;
#define LOAD(name, ret, params, args) do { FARPROC sym = GetProcAddress(lib, "cui_bridge_" #name); if (!sym) { FreeLibrary(lib); return -2; } memcpy(&fp_##name, &sym, sizeof sym); } while (0);
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
 return "LoadLibrary or GetProcAddress failed";
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
	"runtime"
	"strings"
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
	MaxNodes, MaxEvents      uint32
}
type Node struct {
	kind                           int
	key, text                      string
	children                       []*Node
	app                            *App
	handle                         C.cui_bridge_node
	gap                            float32
	size, variant, rounding        int
	radius                         float32
	disabled                       bool
	hover                          [4]uint8
	hasHover                       bool
	click                          func(Event)
	change                         func(Event)
	checked                        bool
	value, min, max, step          float64
	hasRange, hasValue, hasChecked bool
	detail                         string
	opened, hasOpen                bool
	selected                       *Node
	err                            error
	removed                        bool
}
type Rect struct{ X, Y, Width, Height float32 }
type Handle struct{ Slot, Generation uint32 }
type Event struct {
	Node           *Node
	Frame          uint64
	Type           int
	Checked        bool
	Value          float64
	Text           string
	Selected       *Node
	SelectedHandle Handle
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
	path := cs(o.Library)
	defer C.free(unsafe.Pointer(path))
	if C.bridge_open(path) != 0 {
		return nil, fmt.Errorf("bridge load failed: %s", C.GoString(C.bridge_dlerror()))
	}
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
	return &App{ptr: ptr, thread: C.bridge_thread(), nodes: map[uint64]*Node{}}, nil
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
	if n.kind < 4 || (n.kind > 9 && n.kind < 12) || n.kind == 16 || n.kind > 23 {
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
	if n.kind != 4 && n.kind != 23 {
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
		if n.kind >= 3 && n.kind != 15 && n.kind != 17 && n.kind != 18 && n.kind != 19 && n.kind != 20 && n.kind != 21 && n.kind != 22 && n.kind != 23 && len(n.children) > 0 {
			return errors.New("leaf has children")
		}
		if n.kind == 17 || n.kind == 18 || n.kind == 19 || n.kind == 21 || n.kind == 22 || n.kind == 23 {
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
		if n.text != "" {
			if err := n.SetText(n.text); err != nil {
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
		if n.hasOpen {
			if e := result(C.go_set_open(a.ptr, h, C.uint32_t(btoi(n.opened)))); e != nil {
				return e
			}
		}
		if n.gap != 0 {
			if err := result(C.go_set_gap(a.ptr, h, C.float(n.gap))); err != nil {
				return err
			}
		}
		if n.kind >= 5 && n.kind <= 8 && n.hasChecked {
			if err := result(C.go_set_checked(a.ptr, h, C.uint32_t(btoi(n.checked)))); err != nil {
				return err
			}
		}
		if ((n.kind >= 5 && n.kind <= 9) || (n.kind >= 12 && n.kind <= 15) || (n.kind >= 17 && n.kind <= 23)) && n.disabled {
			if err := result(C.go_set_disabled(a.ptr, h, 1)); err != nil {
				return err
			}
		}
		if (n.kind == 9 || n.kind == 14) && n.hasRange {
			if err := result(C.go_set_range(a.ptr, h, C.double(n.min), C.double(n.max), C.double(n.step))); err != nil {
				return err
			}
		}
		if (n.kind == 9 || n.kind == 10 || n.kind == 14) && n.hasValue {
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
	if err := add(root, C.cui_bridge_node{}); err != nil {
		return err
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
	return Event{Node: n, Frame: uint64(e.frame_sequence), Type: int(e._type), Checked: e.checked != 0, Value: float64(e.value), Text: C.GoStringN(dst, C.int(e.text_length)), Selected: selected, SelectedHandle: Handle{uint32(e.selected.slot), uint32(e.selected.generation)}}, true, nil
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
	if n.kind < 5 || n.kind > 8 {
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
	if (n.kind != 9 && n.kind != 10 && n.kind != 14) || math.IsNaN(v) || math.IsInf(v, 0) {
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
	if n.kind < 5 || n.kind > 23 || n.kind == 10 || n.kind == 11 || n.kind == 16 || n.kind == 19 {
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
	if n.kind != 16 && n.kind != 21 && n.kind != 22 && n.kind != 23 {
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
	if n.kind != 15 && n.kind != 20 {
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
	if n.kind != 17 && n.kind != 18 && n.kind != 21 && n.kind != 22 && n.kind != 23 {
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
