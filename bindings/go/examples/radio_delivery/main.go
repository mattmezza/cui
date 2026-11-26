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
	standard := cui.Item("standard", "Standard")
	decoration := cui.Tag("recommended", "Recommended")
	priority := cui.Item("priority", "Priority").Detail("Tracked delivery. Recommended.").
		DecorateContent(content).Children(decoration)
	summary := cui.Label("Estimated arrival: Friday")
	delivery := cui.RadioGroup("delivery").Children(standard, priority).Selected(standard).
		AnimateSelection(motion).OnChange(func(event cui.Event) {
		arrival := "Friday"
		if event.Selected == priority {
			arrival = "Wednesday"
		}
		must(summary.SetText("Estimated arrival: " + arrival))
	})
	must(app.Mount(cui.Column("checkout").GapUnits(3).
		Children(cui.Label("Delivery service"), delivery, summary)))

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
}

func must(err error) {
	if err != nil {
		panic(err)
	}
}
