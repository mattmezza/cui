package cui

import (
	"errors"
	"math"
	"os"
	"testing"
)

func TestRadioPaint(t *testing.T) {
	app, err := New(Options{Width: 480, Height: 400, MaxNodes: 6})
	check(t, err)
	defer app.Close()
	supported, err := app.SupportsRadioPaint()
	check(t, err)
	legacy := os.Getenv("CUI_TEST_RADIO_PAINT_LEGACY") == "1"
	if supported == legacy {
		t.Fatalf("paint support %t, legacy %t", supported, legacy)
	}
	paint := RadioPaint{Fields: RadioPaintRootPadding | RadioPaintIndicatorExtent | RadioPaintRootBackground, RootPadding: 12, IndicatorExtent: 36}
	changes := 0
	radio := Radio("radio").Text("Updates").RadioPaint(RadioPaintBase, paint).OnChange(func(Event) { changes++ })
	first, second := Item("first", "First"), Item("second", "Second")
	second.RadioPaint(RadioPaintBase, RadioPaint{Fields: RadioPaintDescriptionSize, DescriptionSize: 15})
	group := RadioGroup("group").Children(first, second).Selected(first)
	group.RadioPaint(RadioPaintBase, RadioPaint{Fields: RadioPaintRootPadding, RootPadding: 3})
	root := Column("root").Children(radio, group)
	if legacy {
		if err := app.Mount(root); !errors.Is(err, ErrRadioPaintUnavailable) {
			t.Fatalf("legacy mount: %v", err)
		}
		if app.root != nil || len(app.nodes) != 0 || radio.app != nil {
			t.Fatal("partially mounted unsupported paint")
		}
		check(t, radio.ClearRadioPaint(RadioPaintBase))
		check(t, second.ClearRadioPaint(RadioPaintBase))
		check(t, group.ClearRadioPaint(RadioPaintBase))
	}
	check(t, app.Mount(root))
	check(t, app.Step(.016))
	before, err := app.Bounds(radio)
	check(t, err)
	if legacy {
		if err := radio.SetRadioPaint(RadioPaintBase, paint); !errors.Is(err, ErrRadioPaintUnavailable) {
			t.Fatalf("legacy setter: %v", err)
		}
		return
	}
	if before.Height < 60 {
		t.Fatalf("base geometry not applied: %+v", before)
	}
	check(t, radio.SetRadioPaint(RadioPaintChecked, RadioPaint{Fields: RadioPaintRootBackground | RadioPaintMarkColor, RootBackground: 0xff0000ff, MarkColor: 0}))
	x, y := before.X+before.Width/2, before.Y+before.Height/2
	check(t, app.QueuePointer(PointerDown, x, y))
	check(t, app.QueuePointer(PointerUp, x, y))
	check(t, app.Step(.016))
	checked, err := app.Checked(radio)
	check(t, err)
	if !checked || changes != 0 {
		t.Fatal("paint changed state or callback timing")
	}
	check(t, group.SetRadioPaint(RadioPaintDisabled, RadioPaint{Fields: RadioPaintRootBackground, RootBackground: 0}))
	check(t, radio.ClearRadioPaint(RadioPaintChecked))
	check(t, radio.ClearRadioPaint(RadioPaintBase))
	check(t, app.Step(.016))
	after, err := app.Bounds(radio)
	check(t, err)
	if after.Height >= before.Height {
		t.Fatalf("clear did not restore geometry: %v -> %v", before, after)
	}
	count, err := app.Dispatch()
	check(t, err)
	if count != 1 || changes != 1 {
		t.Fatalf("paint affected queued notification: %d/%d", count, changes)
	}
	check(t, radio.Remove())
	if radio.SetRadioPaint(RadioPaintBase, paint) == nil {
		t.Fatal("removed node accepted paint")
	}
}

func TestRadioPaintValidation(t *testing.T) {
	bad := []struct {
		node  *Node
		layer RadioPaintLayer
		paint RadioPaint
	}{
		{Label("label"), RadioPaintBase, RadioPaint{}},
		{Radio("r"), RadioPaintLayer(6), RadioPaint{}},
		{Radio("r"), RadioPaintChecked, RadioPaint{Fields: RadioPaintRootPadding}},
		{Radio("r"), RadioPaintBase, RadioPaint{Fields: RadioPaintDescriptionColor}},
		{RadioGroup("g"), RadioPaintHovered, RadioPaint{}},
		{RadioGroup("g"), RadioPaintBase, RadioPaint{Fields: RadioPaintMarkColor}},
		{Radio("r"), RadioPaintBase, RadioPaint{Fields: 1 << 18}},
		{Radio("r"), RadioPaintBase, RadioPaint{Fields: RadioPaintIndicatorExtent, IndicatorExtent: 3}},
		{Radio("r"), RadioPaintBase, RadioPaint{Fields: RadioPaintRootPadding, RootPadding: float32(math.NaN())}},
		{Radio("r"), RadioPaintBase, RadioPaint{Fields: RadioPaintLabelWeight, LabelWeight: 4}},
	}
	for i, c := range bad {
		if c.node.SetRadioPaint(c.layer, c.paint) == nil {
			t.Fatalf("accepted invalid case %d", i)
		}
	}
	radio := Radio("r")
	check(t, radio.SetRadioPaint(RadioPaintBase, RadioPaint{Fields: RadioPaintRootPadding, RootPadding: 0, IndicatorExtent: float32(math.NaN())}))
	if radio.radioPaints[RadioPaintBase].Fields != RadioPaintRootPadding {
		t.Fatal("lost explicit zero")
	}
	app, err := New(Options{MaxNodes: 3})
	check(t, err)
	defer app.Close()
	item := Item("item", "Item").RadioPaint(RadioPaintBase, RadioPaint{Fields: RadioPaintRootBackground})
	if app.Mount(Column("root").Children(item)) == nil || len(app.nodes) != 0 {
		t.Fatal("accepted non-RadioGroup Item paint")
	}
}
