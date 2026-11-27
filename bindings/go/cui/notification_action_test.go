package cui

import (
	"errors"
	"os"
	"testing"
)

func TestNotificationActionPreflight(t *testing.T) {
	for _, n := range []*Node{
		Notification("n", "Title", "Body").Action(nil),
		Notification("n", "Title", "Body").Action(Label("bad")),
		Notification("n", "Title", "Body").Action(Button("a")).Action(Button("b")),
		Column("bad").Action(Button("a")),
	} {
		if n.Err() == nil {
			t.Fatal("invalid Action accepted")
		}
	}
	a := app(t)
	for _, n := range []*Node{
		Notification("n", "Title", "Body").Children(Label("bad")),
		Notification("n", "Title", "Body").Children(Button("a"), Button("b")),
	} {
		if a.Mount(Column("root").Children(n)) == nil {
			t.Fatal("invalid topology accepted")
		}
		if a.root != nil || len(a.nodes) != 0 {
			t.Fatal("preflight mounted nodes")
		}
	}
}

func TestNotificationAction(t *testing.T) {
	a := app(t)
	supported, err := a.SupportsNotificationAction()
	check(t, err)
	if expected, set := os.LookupEnv("CUI_EXPECT_NOTIFICATION_ACTION"); set && supported != (expected == "1") {
		t.Fatalf("provider reports capability %t, expected %s", supported, expected)
	}
	clicks := 0
	button := Button("action").Text("Undo").OnClick(func(Event) { clicks++ })
	note := Notification("note", "Saved", "Changes saved").Open(true).Action(button)
	root := Column("root").Children(note)
	if !supported {
		if err := a.Mount(root); !errors.Is(err, ErrNotificationActionUnavailable) {
			t.Fatalf("unsupported: %v", err)
		}
		if a.root != nil || len(a.nodes) != 0 || button.app != nil {
			t.Fatal("unsupported provider mounted nodes")
		}
		plain := Notification("plain", "Saved", "Changes saved").Open(true)
		check(t, a.Mount(Column("root").Children(plain)))
		check(t, a.Step(.016))
		opened, err := a.Open(plain)
		check(t, err)
		if !opened {
			t.Fatal("legacy Notification did not open")
		}
		return
	}
	check(t, a.Mount(root))
	check(t, a.Step(.016))
	b, err := a.Bounds(button)
	check(t, err)
	if b.Width <= 0 || b.Height <= 0 {
		t.Fatal("missing action bounds")
	}
	check(t, a.QueuePointer(PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, a.QueuePointer(PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, a.Step(.016))
	_, err = a.Dispatch()
	check(t, err)
	if clicks != 1 {
		t.Fatalf("clicks=%d", clicks)
	}
	check(t, a.Step(.016))
	opened, err := a.Open(note)
	check(t, err)
	if !opened {
		t.Fatal("action closed Notification")
	}
	b, err = a.Bounds(button)
	check(t, err)
	if b.Width <= 0 {
		t.Fatal("action closed Notification")
	}
	check(t, note.Remove())
	if !button.removed || button.click != nil {
		t.Fatal("parent removal retained action or callback")
	}
}
