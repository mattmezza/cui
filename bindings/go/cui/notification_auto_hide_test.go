package cui

import (
	"errors"
	"os"
	"testing"
)

func TestNotificationAutoHide(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsNotificationAutoHide()
	check(t, err)
	if expected, set := os.LookupEnv("CUI_EXPECT_NOTIFICATION_AUTO_HIDE"); set && supported != (expected == "1") {
		t.Fatalf("capability=%t expected=%s", supported, expected)
	}
	if Column("invalid").AutoHide(true).Err() == nil {
		t.Fatal("invalid kind accepted")
	}
	closes, clicks := 0, 0
	note := Notification("note", "Saved", "Changes saved").AutoHide(true).Open(true).OnClose(func(Event) { closes++ }).OnClick(func(Event) { clicks++ })
	if !supported {
		for _, enabled := range []bool{true, false} {
			n := Notification("unsupported", "Saved", "Body").AutoHide(enabled)
			if err := a.Mount(Column("root").Children(n)); !errors.Is(err, ErrNotificationAutoHideUnavailable) {
				t.Fatalf("unsupported=%v", err)
			}
			if a.root != nil || len(a.nodes) != 0 || n.app != nil {
				t.Fatal("unsupported tree partially mounted")
			}
		}
		note = Notification("plain", "Saved", "Body").Open(true)
	}
	check(t, a.Mount(Column("root").Children(note)))
	// Stay outside the Notification so elapsed time exercises actual expiration.
	check(t, a.QueuePointer(PointerMove, -100, -100))
	for i := 0; i < 90; i++ {
		check(t, a.Step(.1))
		_, err := a.Dispatch()
		check(t, err)
	}
	opened, err := a.Open(note)
	check(t, err)
	if opened == supported {
		t.Fatalf("open=%t supported=%t", opened, supported)
	}
	if !supported {
		return
	}
	if closes != 1 || clicks != 0 {
		t.Fatalf("closes=%d clicks=%d", closes, clicks)
	}
	// A post-mount policy update takes effect on the next open.
	check(t, note.AutoHide(false).Open(true).Err())
	for i := 0; i < 90; i++ {
		check(t, a.Step(.1))
		_, err := a.Dispatch()
		check(t, err)
	}
	opened, err = a.Open(note)
	check(t, err)
	if !opened || closes != 1 || clicks != 0 {
		t.Fatalf("persistent open=%t closes=%d clicks=%d", opened, closes, clicks)
	}
}
