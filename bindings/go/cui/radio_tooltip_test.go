package cui

import (
	"errors"
	"os"
	"strings"
	"testing"
)

func TestRadioTooltip(t *testing.T) {
	app, err := New(Options{Width: 360, Height: 240, MaxNodes: 8})
	check(t, err)
	defer app.Close()
	legacy := os.Getenv("CUI_TEST_RADIO_TOOLTIP_LEGACY") == "1"
	supported, err := app.SupportsRadioTooltip()
	check(t, err)
	if supported == legacy {
		t.Fatalf("support %t legacy %t", supported, legacy)
	}
	for _, n := range []*Node{Label("bad"), RadioGroup("bad")} {
		if n.SetRadioTooltip("hint") == nil {
			t.Fatal("accepted wrong kind")
		}
	}
	radio := Radio("updates").Text("Updates").RadioTooltip("Original")
	if radio.radioTooltipDelay != 350 {
		t.Fatal("unexpected default tooltip delay")
	}
	for _, text := range []string{strings.Repeat("é", 129), "bad\x00text", "\xff"} {
		if radio.SetRadioTooltip(text) == nil || radio.radioTooltipText != "Original" {
			t.Fatal("invalid text changed hint")
		}
	}
	for _, delay := range []int{-1, 10001} {
		if radio.SetRadioTooltip("new", delay) == nil || radio.radioTooltipText != "Original" {
			t.Fatal("invalid delay changed hint")
		}
	}
	if radio.SetRadioTooltip("new", 0, 1) == nil {
		t.Fatal("accepted multiple delays")
	}
	first := Item("standard", "Standard").RadioTooltip(strings.Repeat("é", 128), 0)
	second := Item("priority", "Priority").RadioTooltip("Tracked", 10000)
	changes := 0
	group := RadioGroup("delivery").Children(first, second).Selected(first).OnChange(func(Event) { changes++ })
	root := Column("root").Children(radio, group)
	if legacy {
		if err := app.Mount(root); !errors.Is(err, ErrRadioTooltipUnavailable) {
			t.Fatalf("legacy mount: %v", err)
		}
		if app.root != nil || len(app.nodes) != 0 {
			t.Fatal("partial mount")
		}
		for _, n := range []*Node{radio, first, second} {
			check(t, n.SetRadioTooltip(""))
		}
	}
	check(t, app.Mount(root))
	check(t, app.Step(.016))
	if legacy {
		if err := radio.SetRadioTooltip("new"); !errors.Is(err, ErrRadioTooltipUnavailable) {
			t.Fatalf("legacy update: %v", err)
		}
	} else {
		check(t, radio.SetRadioTooltip("Updated", 0))
		check(t, second.SetRadioTooltip("Updated group hint"))
	}
	b, err := app.Bounds(second)
	check(t, err)
	check(t, app.QueuePointer(PointerDown, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, app.QueuePointer(PointerUp, b.X+b.Width/2, b.Y+b.Height/2))
	check(t, app.Step(.016))
	selected, err := app.Selected(group)
	check(t, err)
	if selected != second || changes != 0 {
		t.Fatal("changed selection or callback timing")
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 1 || changes != 1 {
		t.Fatal("expected selection event")
	}
	check(t, radio.SetRadioTooltip("", -1))
	check(t, second.SetRadioTooltip("", 10001))
	check(t, app.Step(.5))
	count, err = app.Dispatch()
	check(t, err)
	if count != 0 {
		t.Fatal("hint emitted event")
	}
	check(t, radio.Remove())
	if radio.SetRadioTooltip("hint") == nil {
		t.Fatal("removed node accepted hint")
	}
}
func TestRadioTooltipItemOwnership(t *testing.T) {
	app, err := New(Options{})
	check(t, err)
	defer app.Close()
	item := Item("item", "Item").RadioTooltip("hint")
	if app.Mount(Column("root").Children(Tabs("tabs").Children(item).Selected(item))) == nil {
		t.Fatal("accepted Tabs Item")
	}
	if app.root != nil || len(app.nodes) != 0 {
		t.Fatal("partial invalid mount")
	}
	check(t, item.SetRadioTooltip(""))
	check(t, app.Mount(Column("root").Children(Tabs("tabs").Children(item).Selected(item))))
	if supported, _ := app.SupportsRadioTooltip(); supported && item.SetRadioTooltip("hint") == nil {
		t.Fatal("mounted Tabs Item accepted hint")
	}
	if item.radioTooltipText != "" {
		t.Fatal("failed update changed hint")
	}
}
