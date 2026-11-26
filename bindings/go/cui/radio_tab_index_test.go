package cui

import (
	"errors"
	"os"
	"testing"
)

func TestRadioTabIndex(t *testing.T) {
	app, err := New(Options{Width: 360, Height: 240, MaxNodes: 5})
	check(t, err)
	defer app.Close()
	supported, err := app.SupportsRadioTabIndex()
	check(t, err)
	legacy := os.Getenv("CUI_TEST_RADIO_TAB_INDEX_LEGACY") == "1"
	if supported == legacy {
		t.Fatalf("support %t for legacy %t", supported, legacy)
	}
	for _, n := range []*Node{Checkbox("c"), Item("i", "Item"), Column("col")} {
		if n.SetRadioTabIndex(0) == nil || n.ClearRadioTabIndex() == nil {
			t.Fatal("accepted invalid owner")
		}
	}
	var events []Event
	record := func(e Event) { events = append(events, e) }
	radio := Radio("radio").Text("Updates").OnChange(record)
	first, second := Item("first", "Standard"), Item("second", "Priority")
	group := RadioGroup("group").Children(first, second).Selected(first).TabIndex(0).OnChange(record)
	root := Column("root").Children(radio, group)
	if legacy {
		if err := app.Mount(root); !errors.Is(err, ErrRadioTabIndexUnavailable) {
			t.Fatalf("preflight: %v", err)
		}
		if app.root != nil || len(app.nodes) != 0 || group.app != nil {
			t.Fatal("partial mount")
		}
		check(t, group.ClearRadioTabIndex())
	}
	check(t, app.Mount(root))
	if legacy {
		if err := radio.SetRadioTabIndex(-1); !errors.Is(err, ErrRadioTabIndexUnavailable) {
			t.Fatalf("legacy setter: %v", err)
		}
	} else {
		for _, index := range []int32{-2147483648, -1, 0, 1, 2147483647} {
			check(t, radio.SetRadioTabIndex(index))
			check(t, group.SetRadioTabIndex(index))
			if *group.radioTabIndex != index {
				t.Fatal("signed index lost")
			}
		}
	}
	check(t, radio.ClearRadioTabIndex())
	check(t, group.ClearRadioTabIndex())
	check(t, app.Step(.016))
	selected, err := app.Selected(group)
	check(t, err)
	if selected != first {
		t.Fatal("tab order changed selection")
	}
	for _, n := range []*Node{second, radio} {
		b, err := app.Bounds(n)
		check(t, err)
		x, y := b.X+b.Width/2, b.Y+b.Height/2
		check(t, app.QueuePointer(PointerDown, x, y))
		check(t, app.QueuePointer(PointerUp, x, y))
		check(t, app.Step(.016))
	}
	if len(events) != 0 {
		t.Fatal("callbacks not deferred")
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 2 || len(events) != 2 || events[0].Node != group || events[0].Selected != second || events[0].SelectedHandle.Generation == 0 || events[1].Node != radio || !events[1].Checked {
		t.Fatalf("selection payloads: %+v", events)
	}
	check(t, radio.Remove())
	if radio.ClearRadioTabIndex() == nil || radio.SetRadioTabIndex(0) == nil {
		t.Fatal("removed owner accepted")
	}
}

func TestRadioTabIndexKeyboard(t *testing.T) {
	if os.Getenv("CUI_TEST_RADIO_TAB_INDEX_LEGACY") == "1" {
		t.Skip("legacy provider has no signed Radio ordering")
	}
	app, err := New(Options{Width: 360, Height: 240})
	check(t, err)
	defer app.Close()
	standard := Item("standard", "Standard")
	priority := Item("priority", "Priority")
	var events []Event
	review := Button("review").Text("Review").OnClick(func(event Event) {
		events = append(events, event)
	})
	delivery := RadioGroup("delivery").Children(standard, priority).Selected(standard).
		TabIndex(-2).OnChange(func(event Event) { events = append(events, event) })
	check(t, app.Mount(Column("root").Children(review, delivery)))
	check(t, app.Step(.016))
	press := func(key Key, mods uint32) {
		t.Helper()
		check(t, app.QueueKey(KeyDown, key, mods, false))
		check(t, app.QueueKey(KeyUp, key, mods, false))
		check(t, app.Step(.016))
	}
	press(Tab, 0)
	press(Down, 0)
	press(Tab, 0)
	press(Enter, 0)
	press(Tab, ModShift)
	press(Up, 0)
	if len(events) != 0 {
		t.Fatal("tab traversal dispatched callbacks")
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 3 || len(events) != 3 || events[0].Node != delivery ||
		events[0].Selected != priority || events[1].Node != review ||
		events[2].Node != delivery || events[2].Selected != standard {
		t.Fatalf("unexpected keyboard events: %+v", events)
	}
}
