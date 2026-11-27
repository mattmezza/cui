package cui

import (
	"errors"
	"math"
	"reflect"
	"testing"
)

func TestDialogCompositionValidation(t *testing.T) {
	for _, node := range []*Node{
		Label("label").DialogOptions(&DialogOptions{}),
		Dialog("bad-width", "Dialog", "").DialogOptions(&DialogOptions{Width: float32(math.NaN())}),
		Dialog("bad-flag", "Dialog", "").DialogOptions(&DialogOptions{ShowClose: true}),
		DialogSlot("bad-slot", DialogFooter+1),
		Button("bad-action").DialogAction(DialogActionClose + 1),
	} {
		if node.Err() == nil {
			t.Fatalf("accepted invalid builder %#v", node)
		}
	}
	a := app(t)
	if e := a.Mount(Column("root").Children(DialogSlot("slot", DialogTitle))); e == nil {
		t.Fatal("orphan slot accepted")
	}
	if e := a.Mount(Column("root").Children(Button("orphan-action").DialogAction(DialogActionConfirm))); e == nil {
		t.Fatal("action accepted outside a Footer")
	}
}

func TestDialogCompositionProviderAndEvents(t *testing.T) {
	a := app(t)
	supported, e := a.SupportsDialogComposition()
	check(t, e)
	options := DialogOptions{Width: 300, Centered: true}
	accept := Button("remove").Text("Remove account").DialogAction(DialogActionConfirm)
	alert := AlertDialog("alert", "Remove account?", "Remove local preferences.").DialogOptions(&options)
	alert.Children(DialogSlot("body", DialogBody).Children(Label("Account stays available online.")),
		DialogSlot("footer", DialogFooter).Children(accept)).Open(true)
	options.Width = 121 // Mount uses the builder's copy.
	if !supported {
		if e := a.Mount(Column("root").Children(alert)); !errors.Is(e, ErrDialogCompositionUnavailable) {
			t.Fatal(e)
		}
		legacy := AlertDialog("legacy", "Legacy alert", "").Open(true)
		check(t, a.Mount(Column("legacy-root").Children(legacy)))
		check(t, a.Step(.016))
		return
	}
	dup := Dialog("dup", "Duplicate", "").DialogOptions(&DialogOptions{}).Children(
		DialogSlot("body-1", DialogBody), DialogSlot("body-2", DialogBody))
	if e := a.Mount(Column("bad-root").Children(dup)); e == nil {
		t.Fatal("duplicate Body was accepted")
	}
	var events []string
	var clickFrame uint64
	accept.OnClick(func(event Event) {
		clickFrame = event.Frame
		events = append(events, "click")
	})
	alert.OnClose(func(event Event) {
		if event.CloseResult != SurfaceAccepted || event.Frame != clickFrame {
			t.Errorf("unexpected close: %+v", event)
		}
		events = append(events, "close")
	})
	check(t, a.Mount(Column("root").Children(alert)))
	check(t, a.Step(.016))
	panel, e := a.Bounds(alert)
	check(t, e)
	if panel.Width != 300 {
		t.Fatalf("expected copied width, got %+v", panel)
	}
	box, e := a.Bounds(accept)
	check(t, e)
	check(t, a.QueuePointer(PointerDown, box.X+box.Width/2, box.Y+box.Height/2))
	check(t, a.QueuePointer(PointerUp, box.X+box.Width/2, box.Y+box.Height/2))
	check(t, a.Step(.016))
	_, e = a.Dispatch()
	check(t, e)
	if !reflect.DeepEqual(events, []string{"click", "close"}) {
		t.Fatalf("unexpected events: %v", events)
	}
	open, e := a.Open(alert)
	check(t, e)
	if open {
		t.Fatal("typed confirmation left the dialog open")
	}
}
