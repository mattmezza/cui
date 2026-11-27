package cui

import (
	"errors"
	"os"
	"strings"
	"testing"
)

func TestResponsiveNotifications(t *testing.T) {
	a, err := New(Options{Width: 220, Height: 320})
	check(t, err)
	t.Cleanup(func() { check(t, a.Close()) })
	supported, err := a.SupportsResponsiveNotifications()
	check(t, err)
	if os.Getenv("CUI_TEST_UNSUPPORTED_RESPONSIVE_NOTIFICATIONS") == "1" {
		if supported {
			t.Fatal("unexpected optional capability")
		}
		for _, enabled := range []bool{false, true} {
			if !errors.Is(a.SetResponsiveNotifications(enabled), ErrResponsiveNotificationsUnavailable) {
				t.Fatal("missing provider accepted setting")
			}
		}
		check(t, a.Mount(Column("root").Children(Notification("legacy", "Saved", "Body").Open(true))))
		check(t, a.Step(.4))
		return
	}
	if !supported {
		t.Fatal("missing responsive notifications")
	}
	note := Notification("note", "Saved", strings.Repeat("Your changes are safely stored. ", 8)).Open(true)
	clicks, closes := 0, 0
	note.OnClick(func(Event) { clicks++ }).OnClose(func(Event) { closes++ })
	check(t, a.Mount(Column("root").Children(note)))
	check(t, a.Step(.4))
	check(t, a.Step(.3))
	legacy, err := a.Bounds(note)
	check(t, err)
	check(t, a.SetResponsiveNotifications(true))
	check(t, a.Step(.4))
	check(t, a.Step(.3))
	responsive, err := a.Bounds(note)
	check(t, err)
	if responsive.Height <= legacy.Height {
		t.Fatalf("body did not wrap: legacy=%+v responsive=%+v", legacy, responsive)
	}
	x, y := responsive.X+responsive.Width-18, responsive.Y+18
	check(t, a.QueuePointer(PointerDown, x, y))
	check(t, a.QueuePointer(PointerUp, x, y))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	check(t, a.Step(.4))
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 0 || closes != 1 {
		t.Fatalf("Close without hover: clicks=%d closes=%d", clicks, closes)
	}
	check(t, a.SetResponsiveNotifications(false))
}
