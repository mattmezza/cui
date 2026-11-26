package cui

import (
	"errors"
	"os"
	"testing"
)

func TestRadioMotion(t *testing.T) {
	app, err := New(Options{Width: 360, Height: 240, MaxNodes: 5})
	check(t, err)
	defer app.Close()

	legacy := os.Getenv("CUI_TEST_RADIO_MOTION_LEGACY") == "1"
	supported, err := app.SupportsRadioMotion()
	check(t, err)
	if supported == legacy {
		t.Fatalf("motion support %t for legacy provider %t", supported, legacy)
	}
	if Checkbox("check").AnimateSelection(true).Err() == nil || Tabs("tabs").SetRadioMotion(false) == nil {
		t.Fatal("accepted selection motion outside the Radio family")
	}

	var changes []Event
	record := func(event Event) { changes = append(changes, event) }
	standalone := Radio("updates").Text("Receive updates").OnChange(record)
	standard := Item("standard", "Standard")
	priority := Item("priority", "Priority")
	delivery := RadioGroup("delivery").Children(standard, priority).Selected(standard).
		AnimateSelection(true).OnChange(record)
	root := Column("root").Children(standalone, delivery)

	if legacy {
		if err := app.Mount(root); !errors.Is(err, ErrRadioMotionUnavailable) {
			t.Fatalf("mount on legacy provider: %v", err)
		}
		if app.root != nil || len(app.nodes) != 0 || delivery.app != nil {
			t.Fatal("unsupported option partially mounted the tree")
		}
		check(t, delivery.SetRadioMotion(false))
	}
	check(t, app.Mount(root))
	check(t, app.Step(.016))
	selected, err := app.Selected(delivery)
	check(t, err)
	if selected != standard {
		t.Fatal("enabling motion changed the initial selection")
	}
	if legacy {
		if err := standalone.SetRadioMotion(true); !errors.Is(err, ErrRadioMotionUnavailable) {
			t.Fatalf("mounted legacy opt-in: %v", err)
		}
	} else {
		check(t, standalone.SetRadioMotion(true))
	}

	press := func(node *Node) {
		b, err := app.Bounds(node)
		check(t, err)
		if b.Width <= 0 || b.Height <= 0 {
			t.Fatal("missing Radio hit target")
		}
		x, y := b.X+b.Width/2, b.Y+b.Height/2
		check(t, app.QueuePointer(PointerDown, x, y))
		check(t, app.QueuePointer(PointerUp, x, y))
		check(t, app.Step(.016))
	}
	press(priority)
	press(standalone)
	selected, err = app.Selected(delivery)
	check(t, err)
	checked, err := app.Checked(standalone)
	check(t, err)
	if selected != priority || !checked || len(changes) != 0 {
		t.Fatal("Radio did not preserve immediate state and deferred callback timing")
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 2 || len(changes) != 2 ||
		changes[0].Node != delivery || changes[0].Selected != priority || changes[0].SelectedHandle.Generation == 0 ||
		changes[1].Node != standalone || !changes[1].Checked {
		t.Fatalf("unexpected legacy changes: %+v", changes)
	}

	check(t, delivery.SetRadioMotion(false))
	check(t, standalone.SetRadioMotion(false))
	check(t, app.Step(.25))
	count, err = app.Dispatch()
	check(t, err)
	if count != 0 || len(changes) != 2 {
		t.Fatal("disabling motion replayed selection")
	}
	check(t, standalone.Remove())
	if standalone.SetRadioMotion(true) == nil {
		t.Fatal("removed Radio accepted a motion setting")
	}
}
