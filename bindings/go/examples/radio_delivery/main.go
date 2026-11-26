// Choose a delivery service to update the estimated arrival.
package main

import (
	"flag"
	"os"

	"github.com/mattmezza/cui/bindings/go/cui"
)

func main() {
	font := flag.String("font", "", "font path for a native SDL window")
	smoke := flag.Bool("smoke", false, "step a hidden window on the private test display")
	flag.Parse()
	if *smoke && (*font == "" || os.Getenv("CUI_TEST_VIRTUAL_DISPLAY") != "1") {
		panic("--smoke requires a font and the private display from scripts/test_local.py")
	}
	app, err := cui.New(cui.Options{
		Width: 320, Height: 200, Title: "Delivery options",
		Native: *font != "", FontPath: *font, InitiallyHidden: *smoke,
	})
	must(err)
	defer app.Close()

	content, err := app.SupportsRadioItemContent()
	must(err)
	motion, err := app.SupportsRadioMotion()
	must(err)
	tabOrder, err := app.SupportsRadioTabIndex()
	must(err)
	tooltip, err := app.SupportsRadioTooltip()
	must(err)
	paint, err := app.SupportsRadioPaint()
	must(err)
	standard := cui.Item("standard", "Standard")
	decoration := cui.Tag("recommended", "Recommended")
	priority := cui.Item("priority", "Priority").Detail("Tracked delivery. Recommended.").
		DecorateContent(content).Children(decoration)
	if tooltip {
		standard.RadioTooltip("Arrives Friday with standard shipping")
		priority.RadioTooltip("Arrives Wednesday with tracking", 350)
	}
	summary := cui.Label("Estimated arrival: Friday")
	delivery := cui.RadioGroup("delivery").Children(standard, priority).Selected(standard).
		AnimateSelection(motion).OnChange(func(event cui.Event) {
		arrival := "Friday"
		if event.Selected == priority {
			arrival = "Wednesday"
		}
		must(summary.SetText("Estimated arrival: " + arrival))
	})
	if paint {
		delivery.RadioPaint(cui.RadioPaintBase, cui.RadioPaint{Fields: cui.RadioPaintRootPadding | cui.RadioPaintRootGap, RootPadding: 8, RootGap: 6})
		priority.RadioPaint(cui.RadioPaintChecked, cui.RadioPaint{Fields: cui.RadioPaintRootBackground | cui.RadioPaintMarkColor, RootBackground: 0xe8f3ffff, MarkColor: 0x1467cfff})
	}
	reset := cui.Button("reset").Text("Reset selection").OnClick(func(cui.Event) {
		delivery.Selected(standard)
		must(summary.SetText("Estimated arrival: Friday"))
	})
	if tabOrder {
		delivery.TabIndex(-2)
	}
	must(app.Mount(cui.Column("checkout").GapUnits(3).
		Children(cui.Label("Delivery service"), reset, delivery, summary)))

	if *font != "" && !*smoke {
		must(app.Run(.016))
		return
	}
	must(app.Step(.016))
	target := priority
	if content {
		target = decoration
	}
	bounds, err := app.Bounds(target)
	must(err)
	x, y := bounds.X+bounds.Width/2, bounds.Y+bounds.Height/2
	must(app.QueuePointer(cui.PointerDown, x, y))
	must(app.QueuePointer(cui.PointerUp, x, y))
	must(app.Step(.016))
	events, err := app.Dispatch()
	must(err)
	text, err := app.Text(summary)
	must(err)
	if events != 1 || text != "Estimated arrival: Wednesday" {
		panic("expected one selection callback and an updated estimate")
	}
	if tabOrder {
		for _, key := range []cui.Key{cui.Tab, cui.Enter} {
			must(app.QueueKey(cui.KeyDown, key, 0, false))
			must(app.QueueKey(cui.KeyUp, key, 0, false))
			must(app.Step(.016))
		}
		events, err = app.Dispatch()
		must(err)
		selected, err := app.Selected(delivery)
		must(err)
		text, err = app.Text(summary)
		must(err)
		if events != 1 || selected != standard || text != "Estimated arrival: Friday" {
			panic("expected Reset selection to be the next Tab stop")
		}
	}
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
