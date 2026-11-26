package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestDisclosureHeadingAndDivider(t *testing.T) {
	app, err := New(Options{Width: 430, Height: 420})
	check(t, err)
	defer app.Close()
	headings, err := app.SupportsDisclosureHeading()
	check(t, err)
	dividers, err := app.SupportsDisclosureDivider()
	check(t, err)
	want := os.Getenv("CUI_TEST_LEGACY_DISCLOSURE") != "1"
	if headings != want || dividers != want {
		t.Fatalf("heading=%t divider=%t, want both %t", headings, dividers, want)
	}

	fold := Collapsible("advanced").Text("Advanced").Children(Label("Details"))
	first := Item("account", "Workspace and access preferences")
	second := Item("delivery", "Delivery")
	group := Accordion("settings").Children(first, second)
	root := Column("root").Children(group, fold)
	if !headings {
		check(t, fold.SetDisclosureHeading(DisclosureHeadingNormal, &DisclosureHeading{
			Typography: &DisclosureTypography{FontSize: 18},
		}))
		if err := app.Mount(root); !errors.Is(err, ErrDisclosureOptionsUnavailable) {
			t.Fatalf("missing draft heading returned %v", err)
		}
		check(t, fold.SetDisclosureHeading(DisclosureHeadingNormal, nil))
		check(t, group.SetDisclosureDivider(&DisclosureDivider{Thickness: 4}))
		if err := app.Mount(root); !errors.Is(err, ErrDisclosureOptionsUnavailable) {
			t.Fatalf("missing draft divider returned %v", err)
		}
		check(t, group.SetDisclosureDivider(nil))
		legacyGroup := Accordion("legacy-accordion")
		check(t, app.Mount(Column("legacy-root").Children(legacyGroup, fold)))
		check(t, app.Step(.016))
		if err := fold.SetDisclosureHeading(DisclosureHeadingNormal, nil); !errors.Is(err, ErrDisclosureOptionsUnavailable) {
			t.Fatalf("missing mounted heading returned %v", err)
		}
		if err := legacyGroup.SetDisclosureDivider(nil); !errors.Is(err, ErrDisclosureOptionsUnavailable) {
			t.Fatalf("missing mounted divider returned %v", err)
		}
		disclosureClick(t, app, fold)
		checked, err := app.Checked(fold)
		check(t, err)
		if !checked {
			t.Fatal("ordinary Collapsible still needs to open")
		}
		return
	}

	var changes []Event
	first.OnChange(func(e Event) { changes = append(changes, e) })
	typography := DisclosureTypography{
		FontSize: 20, LineHeight: 28, Wrap: true, Weight: DisclosureWeightMedium,
		Align: DisclosureAlignRight,
	}
	expanded := DisclosureTypography{FontSize: 24, LineHeight: 32, Wrap: true, Weight: DisclosureWeightBold}
	box := DisclosureHeadingBox{Width: -1, Height: 58, Padding: 8}
	transparent := uint32(0)
	group.DisclosureOptions(DisclosureConfig{Joined: true, Width: 220, AllowCollapse: true}).
		DisclosureHeading(DisclosureHeadingNormal, DisclosureHeading{Typography: &typography}).
		DisclosureHeading(DisclosureHeadingExpanded, DisclosureHeading{
			Box:        &DisclosureHeadingBox{Width: -1, Height: 124, Padding: 8},
			Typography: &expanded,
		}).
		DisclosureDivider(DisclosureDivider{Thickness: 4, InsetLeft: 10, InsetRight: 14, ColorRGBA: 0x347a97ff})
	second.DisclosureHeading(DisclosureHeadingNormal, DisclosureHeading{Box: &box, TitleRGBA: &transparent})
	fold.DisclosureHeading(DisclosureHeadingNormal, DisclosureHeading{Box: &DisclosureHeadingBox{Width: 220, Height: 54}})
	typography.FontSize, box.Height = -1, -1
	transparent = 0xffffffff
	check(t, app.Mount(root))
	check(t, app.Step(.016))

	firstBounds, err := app.Bounds(first)
	check(t, err)
	secondBounds, err := app.Bounds(second)
	check(t, err)
	foldBounds, err := app.Bounds(fold)
	check(t, err)
	if firstBounds.Width != 220 || firstBounds.Height <= 45 || secondBounds.Height != 58 ||
		secondBounds.Y != firstBounds.Y+firstBounds.Height+4 || foldBounds.Height != 54 {
		t.Fatalf("heading and divider geometry: first=%+v, second=%+v, fold=%+v", firstBounds, secondBounds, foldBounds)
	}
	bad := &DisclosureHeading{Typography: &DisclosureTypography{FontSize: float32(math.NaN())}}
	if err := group.SetDisclosureHeading(DisclosureHeadingNormal, bad); err == nil {
		t.Fatal("accepted nonfinite typography")
	}
	if err := group.SetDisclosureDivider(&DisclosureDivider{InsetLeft: -1}); err == nil {
		t.Fatal("accepted a negative divider inset")
	}
	if err := first.SetDisclosureDivider(nil); err == nil {
		t.Fatal("accepted a divider on an Item")
	}
	disclosureClick(t, app, first)
	if len(changes) != 0 {
		t.Fatal("expansion callback ran before dispatch")
	}
	firstBounds, err = app.Bounds(first)
	check(t, err)
	secondBounds, err = app.Bounds(second)
	check(t, err)
	if firstBounds.Width != 220 || firstBounds.Height != 124 {
		t.Fatalf("expanded box geometry: %+v", firstBounds)
	}
	check(t, group.SetDisclosureDivider(&DisclosureDivider{}))
	check(t, app.Step(.016))
	removed, err := app.Bounds(second)
	check(t, err)
	if removed.Y != secondBounds.Y-4 {
		t.Fatalf("expected zero-thickness divider to remove 4px, got %g", removed.Y-secondBounds.Y)
	}
	check(t, group.SetDisclosureDivider(nil))
	check(t, second.SetDisclosureHeading(DisclosureHeadingNormal, &DisclosureHeading{}))
	check(t, app.Step(.016))
	restored, err := app.Bounds(second)
	check(t, err)
	if restored.Y != removed.Y+1 || restored.Height == secondBounds.Height {
		t.Fatalf("expected the theme divider and inherited heading, got %+v", restored)
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 1 || len(changes) != 1 || !changes[0].Checked || changes[0].Node != first {
		t.Fatalf("style updates added or replayed change events: %d, %+v", count, changes)
	}
}

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
