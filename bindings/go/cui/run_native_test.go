package cui

import (
	"errors"
	"os"
	"strings"
	"testing"
)

func TestNativeRunLifecycle(t *testing.T) {
	if os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") != "1" {
		t.Skip("requires invisible-display wrapper")
	}
	if os.Getenv("DISPLAY") == "" || strings.Split(os.Getenv("DISPLAY"), ".")[0] == ":0" || os.Getenv("WAYLAND_DISPLAY") != "" {
		t.Fatal("unsafe display")
	}
	for _, mode := range []string{"close", "destroy", "panic", "nested"} {
		t.Run(mode, func(t *testing.T) {
			a, err := New(Options{Native: true, FontPath: os.Getenv("CUI_TEST_FONT")})
			if err != nil {
				t.Fatal(err)
			}
			defer func() {
				if !a.closed {
					_ = a.Close()
				}
			}()
			calls := 0
			b := Button("go").Text("Go").OnClick(func(Event) {
				calls++
				if mode == "panic" {
					panic("runner callback marker")
				}
				if mode == "nested" {
					if a.Run(.016) == nil || a.Step(.016) == nil {
						t.Error("nested run/step accepted")
					}
				}
				if mode == "destroy" {
					err = a.Close()
				} else {
					err = a.RequestClose()
				}
			})
			if err = a.Mount(Row("root").Children(b)); err != nil {
				t.Fatal(err)
			}
			if err = a.Step(.016); err != nil {
				t.Fatal(err)
			}
			r, e := a.Bounds(b)
			if e != nil {
				t.Fatal(e)
			}
			if e = a.QueuePointer(PointerDown, r.X+2, r.Y+2); e != nil {
				t.Fatal(e)
			}
			if e = a.QueuePointer(PointerUp, r.X+2, r.Y+2); e != nil {
				t.Fatal(e)
			}
			panicked := false
			func() {
				defer func() {
					if recover() != nil {
						panicked = true
					}
				}()
				err = a.Run(.016)
			}()
			if panicked != (mode == "panic") || calls != 1 || err != nil {
				t.Fatalf("calls=%d panic=%v err=%v", calls, panicked, err)
			}
			if !a.closed && !errors.Is(a.Step(.016), ErrClosed) {
				t.Fatal("runner did not terminate app")
			}
		})
	}
}

func TestPortableRunLeavesManualLoopAvailable(t *testing.T) {
	a, err := New(Options{})
	if err != nil {
		t.Fatal(err)
	}
	defer a.Close()
	if err = a.Mount(Row("root")); err != nil {
		t.Fatal(err)
	}
	if err = a.Run(0); err == nil {
		t.Fatal("zero timing accepted")
	}
	if err = a.Run(.016); err == nil {
		t.Fatal("portable runner accepted")
	}
	if err = a.Step(.016); err != nil {
		t.Fatal(err)
	}
}
