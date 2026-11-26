package cui

import (
	"errors"
	"os"
	"testing"
)

func TestDisclosurePresentation(t *testing.T) {
	app, err := New(Options{Width: 540, Height: 320})
	check(t, err)
	defer app.Close()
	supported, err := app.SupportsDisclosurePresentation()
	wantSupport := os.Getenv("CUI_TEST_LEGACY_DISCLOSURE") != "1"
	check(t, err)
	if supported != wantSupport {
		t.Fatalf("Disclosure V1 support = %t, want %t", supported, wantSupport)
	}

	if !supported {
		fold := Collapsible("advanced").Text("Advanced").Children(Label("Cache preferences"))
		check(t, fold.ConfigureDisclosure(&DisclosureConfig{}))
		root := Column("root").Children(fold)
		if err := app.Mount(root); !errors.Is(err, ErrDisclosureOptionsUnavailable) {
			t.Fatalf("missing provider returned %v", err)
		}
		check(t, fold.ConfigureDisclosure(nil))
		check(t, app.Mount(root))
		check(t, app.Step(.016))
		if err := fold.SetDisclosureTriggerStyle(&DisclosureTriggerStyle{}); !errors.Is(err, ErrDisclosureOptionsUnavailable) {
			t.Fatalf("missing style provider returned %v", err)
		}
		disclosureClick(t, app, fold)
		if checked, err := app.Checked(fold); err != nil || !checked {
			t.Fatalf("ordinary Collapsible did not open: %t, %v", checked, err)
		}
		return
	}

	var changes []Event
	record := func(event Event) { changes = append(changes, event) }
	transparent := uint32(0)
	cursor := DisclosureCursorPointer
	account := Item("account", "Account").Children(Label("Account preferences")).OnChange(record).Checked(true)
	editor := Item("editor", "Editor").Children(Label("Editor preferences")).OnChange(record).
		DisclosureStyle(DisclosureTriggerStyle{HoverRGBA: &transparent, Cursor: &cursor})
	card := DisclosureCard{Padding: 8, Radius: 10, Clip: true}
	group := Accordion("settings").DisclosureOptions(DisclosureConfig{
		Joined: true, Bordered: true, Single: true, AllowCollapse: true,
		DesignSize: LG, Width: 320, Card: &card, CollapsedIcon: "plus", ExpandedIcon: "minus",
	}).Children(account, editor)
	card.Padding = -1 // Draft setters already copied the record.
	cursor = DisclosureCursor(100)
	check(t, app.Mount(Column("root").Children(group)))
	check(t, app.Step(.016))
	_, err = app.Dispatch() // Drain the caller-supplied initial state.
	check(t, err)
	changes = nil
	groupBounds, err := app.Bounds(group)
	check(t, err)
	if groupBounds.Width != 320 {
		t.Fatalf("configured Accordion width = %g", groupBounds.Width)
	}
	if err := group.ConfigureDisclosure(&DisclosureConfig{CollapsedIcon: "absent-catalog-icon"}); err == nil {
		t.Fatal("accepted an unknown icon")
	}
	disclosureClick(t, app, editor)
	if len(changes) != 0 {
		t.Fatal("callback ran before dispatch")
	}
	check(t, group.ConfigureDisclosure(nil))
	check(t, editor.SetDisclosureTriggerStyle(nil))
	count, err := app.Dispatch()
	check(t, err)
	if count != 2 || len(changes) != 2 || changes[0].Node != account || changes[0].Checked ||
		changes[1].Node != editor || !changes[1].Checked {
		t.Fatalf("unexpected disclosure changes: %d, %+v", count, changes)
	}
	if checked, err := app.Checked(editor); err != nil || !checked {
		t.Fatalf("reset changed expansion: %t, %v", checked, err)
	}
	if count, err := app.Dispatch(); count != 0 || err != nil {
		t.Fatalf("reset queued or replayed events: %d, %v", count, err)
	}
	check(t, editor.Remove())
	if err := editor.SetDisclosureTriggerStyle(nil); err == nil {
		t.Fatal("accepted a stale node")
	}
}

func disclosureClick(t *testing.T, app *App, trigger *Node) {
	t.Helper()
	bounds, err := app.Bounds(trigger)
	check(t, err)
	if bounds.Width <= 0 || bounds.Height <= 0 {
		t.Fatalf("empty trigger bounds: %+v", bounds)
	}
	for _, phase := range []PointerPhase{PointerDown, PointerUp} {
		check(t, app.QueuePointer(phase, bounds.X+20, bounds.Y+12))
	}
	check(t, app.Step(.016))
}
