package cui

import (
	"errors"
	"os"
	"testing"
)

func TestRadioItemContent(t *testing.T) {
	app, err := New(Options{Width: 420, Height: 320, MaxNodes: 16})
	check(t, err)
	defer app.Close()
	legacy := os.Getenv("CUI_TEST_RADIO_CONTENT_LEGACY") == "1"
	supported, err := app.SupportsRadioItemContent()
	check(t, err)
	if supported == legacy {
		t.Fatalf("support %t for legacy %t", supported, legacy)
	}
	if Label("bad").SetRadioItemContent(true) == nil {
		t.Fatal("accepted non-Item")
	}
	decoration := Label("Tracked delivery").Key("decoration")
	first := Item("standard", "Standard")
	second := Item("priority", "Priority").DecorateContent(true).Children(Row("details").Children(decoration, Badge("Fast")))
	var changes []Event
	group := RadioGroup("delivery").Children(first, second).Selected(first).OnChange(func(e Event) { changes = append(changes, e) })
	root := Column("root").Children(group)
	if legacy {
		if err := app.Mount(root); !errors.Is(err, ErrRadioItemContentUnavailable) {
			t.Fatalf("legacy mount: %v", err)
		}
		if app.root != nil || len(app.nodes) != 0 || second.app != nil {
			t.Fatal("unsupported mount mutated tree")
		}
		check(t, second.SetRadioItemContent(false))
	}
	check(t, app.Mount(root))
	check(t, app.Step(.016))
	press := func(n *Node) {
		b, err := app.Bounds(n)
		check(t, err)
		if b.Width <= 0 || b.Height <= 0 {
			t.Fatal("missing bounds")
		}
		check(t, app.QueuePointer(PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
		check(t, app.QueuePointer(PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
		check(t, app.Step(.016))
	}
	if legacy {
		if err := second.SetRadioItemContent(true); !errors.Is(err, ErrRadioItemContentUnavailable) {
			t.Fatalf("legacy update: %v", err)
		}
		press(second)
	} else {
		press(decoration)
	}
	if len(changes) != 0 {
		t.Fatal("callback dispatched during Step")
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 1 || len(changes) != 1 || changes[0].Node != group || changes[0].Selected != second || changes[0].SelectedHandle.Generation == 0 {
		t.Fatalf("events: %+v", changes)
	}
	check(t, second.SetRadioItemContent(false))
	check(t, app.Step(.016))
	if !legacy {
		if _, err := app.Bounds(decoration); err == nil {
			t.Fatal("hidden decoration exposed bounds")
		}
		check(t, second.SetRadioItemContent(true))
		check(t, app.Step(.016))
		b, err := app.Bounds(decoration)
		check(t, err)
		if b.Width <= 0 || b.Height <= 0 {
			t.Fatal("decoration did not reappear")
		}
	}
	selected, err := app.Selected(group)
	check(t, err)
	count, err = app.Dispatch()
	check(t, err)
	if selected != second || count != 0 {
		t.Fatal("toggle changed selection or emitted events")
	}
	press(first)
	count, err = app.Dispatch()
	check(t, err)
	selected, err = app.Selected(group)
	check(t, err)
	if selected != first || count != 1 {
		t.Fatal("ordinary selection failed")
	}
	check(t, second.Remove())
	if second.SetRadioItemContent(true) == nil {
		t.Fatal("removed Item accepted setting")
	}
}
func TestRadioItemContentPreflight(t *testing.T) {
	for _, tree := range []*Node{
		Column("root").Children(Item("orphan", "Orphan").DecorateContent(true)),
		Column("root").Children(Tabs("tabs").Children(Item("tab", "Tab").DecorateContent(true))),
		Column("root").Children(RadioGroup("group").Children(Item("item", "Item").DecorateContent(true).Children(Row("row").Children(Button("action"))))),
	} {
		app, err := New(Options{})
		check(t, err)
		if err := app.Mount(tree); err == nil {
			t.Fatal("accepted invalid decoration tree")
		}
		if app.root != nil || len(app.nodes) != 0 {
			t.Fatal("invalid tree partially mounted")
		}
		check(t, app.Close())
	}
}

func TestRadioItemContentMountedRejectsInteractiveChildren(t *testing.T) {
	app, err := New(Options{})
	check(t, err)
	defer app.Close()
	hidden := Button("action")
	item := Item("option", "Priority").Children(Column("details").Children(hidden))
	group := RadioGroup("delivery").Children(item)
	check(t, app.Mount(Column("root").Children(group)))
	check(t, app.Step(.016))
	if err := item.SetRadioItemContent(true); err == nil {
		t.Fatal("accepted an interactive decoration")
	}
	if item.radioContent {
		t.Fatal("failed update changed retained setting")
	}
	check(t, app.Step(.016))
	if _, err := app.Bounds(hidden); err == nil {
		t.Fatal("failed update revealed hidden child")
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 0 {
		t.Fatal("failed update emitted an event")
	}
}
