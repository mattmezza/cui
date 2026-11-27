package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestPopoverOptionsValidation(t *testing.T) {
	for _, o := range []PopoverOptions{{Anchor: 8}, {Width: -1}, {Height: 4097}, {Offset: float32(math.NaN())}, {ReferenceRem: 257}} {
		if o.validate() == nil {
			t.Fatalf("accepted %+v", o)
		}
	}
}
func TestPopoverOptionsProviderAndEscape(t *testing.T) {
	a := app(t)
	supported, e := a.SupportsPopoverOptions()
	check(t, e)
	if want := os.Getenv("CUI_POPOVER_UNSUPPORTED") != "1"; supported != want {
		t.Fatalf("Popover support %v, want %v", supported, want)
	}
	trigger := Button("account").Text("Account")
	close := Button("close").Text("Close")
	popup := Popover("menu", "Account", "").Anchor(trigger).Children(close)
	check(t, a.Mount(Column("root").Children(trigger, popup)))
	popup.PopoverOptions(&PopoverOptions{Anchor: PopoverTopRight, Width: 180, Height: 120, Margin: 8, HideClose: true, FocusContent: true})
	if !supported {
		if !errors.Is(popup.Err(), ErrPopoverOptionsUnavailable) {
			t.Fatal(popup.Err())
		}
		popup.err = nil
		popup.Open(true)
		check(t, popup.Err())
		check(t, a.Step(.016))
		return
	}
	check(t, popup.Err())
	popup.Open(true)
	check(t, popup.Err())
	check(t, a.Step(.016))
	check(t, a.Step(.016))
	b, e := a.Bounds(close)
	check(t, e)
	if b.Width <= 0 || b.X < 8 || b.Y < 8 || b.X+b.Width > 472 {
		t.Fatalf("configured bounds: %+v", b)
	}
	activated := false
	close.OnClick(func(Event) {
		activated = true
		popup.Open(false)
	})
	check(t, a.QueueKey(KeyDown, Enter, 0, false))
	check(t, a.QueueKey(KeyUp, Enter, 0, false))
	check(t, a.Step(.016))
	_, e = a.Dispatch()
	check(t, e)
	if !activated {
		t.Fatal("focus_content did not activate body button")
	}
	check(t, popup.Err())
	check(t, a.Step(.016))
	open, e := a.Open(popup)
	check(t, e)
	if open {
		t.Fatal("body callback did not close Popover")
	}
	popup.Open(true)
	check(t, a.Step(.016))
	check(t, a.QueueKey(KeyDown, Escape, 0, false))
	check(t, a.Step(.016))
	open, e = a.Open(popup)
	check(t, e)
	if open {
		t.Fatal("Escape did not dismiss")
	}
	popup.PopoverOptions(nil)
	check(t, popup.Err())
}

func TestPopoverSecondaryTrigger(t *testing.T) {
	a := app(t)
	secondary, err := a.SupportsPopoverSecondaryTrigger()
	check(t, err)
	expected := os.Getenv("CUI_POPOVER_UNSUPPORTED") != "1" &&
		os.Getenv("CUI_POPOVER_SECONDARY_UNSUPPORTED") != "1"
	if secondary != expected {
		t.Fatalf("secondary support = %v, want %v", secondary, expected)
	}
	clicks, closed := 0, 0
	trigger := Button("trigger").OnClick(func(Event) { clicks++ })
	menu := Popover("menu", "Account", "").Anchor(trigger).OnClose(func(Event) { closed++ })
	check(t, a.Mount(Column("root").Children(menu, trigger)))
	menu.PopoverOptions(&PopoverOptions{SecondaryTrigger: true})
	if !secondary {
		err := menu.Err()
		if !errors.Is(err, ErrPopoverSecondaryTriggerUnavailable) && !errors.Is(err, ErrPopoverOptionsUnavailable) {
			t.Fatalf("unsupported options: %v", err)
		}
		return
	}
	check(t, menu.Err())
	check(t, a.Step(.016))
	b, err := a.Bounds(trigger)
	check(t, err)
	for _, open := range []bool{true, false} {
		for _, phase := range []PointerPhase{PointerDown, PointerUp} {
			check(t, a.QueuePointerButton(phase, PointerSecondary, b.X+b.Width/2, b.Y+b.Height/2))
		}
		check(t, a.Step(.016))
		check(t, a.Step(.016))
		_, err = a.Dispatch()
		check(t, err)
		actual, err := a.Open(menu)
		check(t, err)
		if actual != open {
			t.Fatalf("open = %v, want %v", actual, open)
		}
	}
	if clicks != 0 || closed != 1 {
		t.Fatalf("clicks = %d, closes = %d", clicks, closed)
	}
}
