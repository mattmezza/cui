package cui

import (
	"os"
	"strings"
	"testing"
)

func TestWindowVisibilityNative(t *testing.T) {
	if os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") != "1" {
		t.Skip("private Xvfb required")
	}
	if os.Getenv("DISPLAY") == "" || strings.Split(os.Getenv("DISPLAY"), ".")[0] == ":0" || os.Getenv("WAYLAND_DISPLAY") != "" {
		t.Fatal("unsafe display")
	}
	if app, err := New(Options{InitiallyHidden: true}); err == nil {
		app.Close()
		t.Fatal("headless hidden accepted")
	}
	for _, hidden := range []bool{true, false} {
		a, err := New(Options{Native: true, InitiallyHidden: hidden, FontPath: os.Getenv("CUI_TEST_FONT")})
		if err != nil {
			t.Fatal(err)
		}
		check := func(want bool) {
			t.Helper()
			got, e := a.WindowVisible()
			if e != nil || got != want {
				t.Fatalf("visible %v %v want %v", got, e, want)
			}
		}
		check(!hidden)
		calls := 0
		b := Button("show").Text("Show").OnClick(func(Event) {
			check(!hidden)
			if e := a.SetWindowVisible(true); e != nil {
				t.Fatal(e)
			}
			check(true)
			if e := a.SetWindowVisible(false); e != nil {
				t.Fatal(e)
			}
			check(false)
			calls++
			if e := a.RequestClose(); e != nil {
				t.Fatal(e)
			}
		})
		if err = a.Mount(Row("root").Children(b)); err != nil {
			t.Fatal(err)
		}
		if err = a.Step(.016); err != nil {
			t.Fatal(err)
		}
		check(!hidden)
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
		if e = a.Run(.016); e != nil {
			t.Fatal(e)
		}
		if calls != 1 {
			t.Fatal(calls)
		}
		if _, e = a.WindowVisible(); e == nil {
			t.Fatal("closed visibility accepted")
		}
		if e = a.Close(); e != nil {
			t.Fatal(e)
		}
	}
}

func TestWindowVisibilityLegacyLibrary(t *testing.T) {
	if os.Getenv("CUI_WINDOW_LEGACY_LIBRARY") != "1" {
		t.Skip("older-symbol fixture required")
	}
	a, err := New(Options{})
	if err != nil {
		t.Fatal(err)
	}
	defer a.Close()
	if err = a.Mount(Column("root")); err != nil {
		t.Fatal(err)
	}
	if err = a.Step(.016); err != nil {
		t.Fatal(err)
	}
	if _, err = a.WindowVisible(); err == nil {
		t.Fatal("missing query accepted")
	}
	if err = a.SetWindowVisible(true); err == nil {
		t.Fatal("missing setter accepted")
	}
	if other, e := New(Options{InitiallyHidden: true}); e == nil {
		other.Close()
		t.Fatal("missing constructor accepted")
	}
}
