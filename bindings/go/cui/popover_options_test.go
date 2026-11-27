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
